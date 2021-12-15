//==============================================================
// Copyright © 2021 Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================

#include <vector>
#include <CL/sycl.hpp>
#include <sycl/ext/intel/fpga_extensions.hpp>
#include <chrono>
#include <fstream>

// This file defines the sin and cos values for each degree up to 180
#include "sin_cos_values.h"

#define WIDTH 180
#define HEIGHT 120
#define IMAGE_SIZE WIDTH*HEIGHT
#define THETAS 180
#define RHOS 217 //Size of the image diagonally: (sqrt(180^2+120^2))
#define NS (1000000000.0) // number of nanoseconds in a second

using namespace std;
using namespace sycl;

// This function reads in a bitmap and outputs an array of pixels
void read_image(char *image_array);

class Hough_Transform_kernel;

int main() {

  //Declare arrays
  char pixels[IMAGE_SIZE];
  short accumulators[THETAS*RHOS*2];

  //Initialize the accumulators
  fill(accumulators, accumulators + THETAS*RHOS*2, 0);

  //Read bitmap
  //Read in the bitmap file and get a vector of pixels
  read_image(pixels);

  //Block off this code
  //Putting all SYCL work within here ensures it concludes before this block
  //  goes out of scope. Destruction of the buffers is blocking until the
  //  host retrieves data from the buffer.
  {
    //Profiling setup
    //Set things up for profiling at the host
    chrono::high_resolution_clock::time_point t1_host, t2_host;
    event queue_event;
    cl_ulong t1_kernel, t2_kernel;
    double time_kernel;
    auto property_list = sycl::property_list{sycl::property::queue::enable_profiling()};

    //Buffer setup
    //Define the sizes of the buffers
    //The sycl buffer creation expects a type of sycl:: range for the size
    range<1> num_pixels{IMAGE_SIZE};
    range<1> num_accumulators{THETAS*RHOS*2};
    range<1> num_table_values{180};

    //Create the buffers which will pass data between the host and FPGA
    sycl::buffer<char, 1> pixels_buf(pixels, num_pixels);
    sycl::buffer<short, 1> accumulators_buf(accumulators,num_accumulators);
    sycl::buffer<float, 1> sin_table_buf(sinvals,num_table_values);
    sycl::buffer<float, 1> cos_table_buf(cosvals,num_table_values);
  
    //Device selection
    //We will explicitly compile for the FPGA_EMULATOR, CPU_HOST, or FPGA
    #if defined(FPGA_EMULATOR)
      ext::intel::fpga_emulator_selector device_selector;
    #else
      ext::intel::fpga_selector device_selector;
    #endif

    //Create queue
    sycl::queue device_queue(device_selector,NULL,property_list);
  
    //Query platform and device
    sycl::platform platform = device_queue.get_context().get_platform();
    sycl::device device = device_queue.get_device();
    std::cout << "Platform name: " <<  platform.get_info<sycl::info::platform::name>().c_str() << std::endl;
    std::cout << "Device name: " <<  device.get_info<sycl::info::device::name>().c_str() << std::endl;

    //Device queue submit
    queue_event = device_queue.submit([&](sycl::handler &cgh) {
      //Uncomment if you need to output to the screen within your kernel
      //sycl::stream os(1024,128,cgh);
      //Example of how to output to the screen
      //os<<"Hello world "<<8+5<<sycl::endl;
    
      //Create accessors
      auto _pixels = pixels_buf.get_access<sycl::access::mode::read>(cgh);
      auto _sin_table = sin_table_buf.get_access<sycl::access::mode::read>(cgh);
      auto _cos_table = cos_table_buf.get_access<sycl::access::mode::read>(cgh);
      auto _accumulators = accumulators_buf.get_access<sycl::access::mode::read_write>(cgh);

      //Call the kernel
      cgh.single_task<class Hough_Transform_kernel>([=]() [[intel::kernel_args_restrict]] {

      short accum_local[RHOS*2*THETAS];

      for (int i = 0; i < RHOS*2*THETAS; i++) {
          accum_local[i] = 0;
      }

      for (uint y=0; y<HEIGHT; y++) {
        for (uint x=0; x<WIDTH; x++){
          unsigned short int increment = 0;
        if (_pixels[(WIDTH*y)+x] != 0) {
          increment = 1;
        } else {
          increment = 0;
        }

            for (int theta=0; theta<THETAS; theta++){
              int rho = x*_cos_table[theta] + y*_sin_table[theta];
              accum_local[(THETAS*(rho+RHOS))+theta] += increment;
            }
          }
        }

        for (int i = 0; i < RHOS*2*THETAS; i++) {
         _accumulators[i] = accum_local[i];
        }
   
      });
  
    });

    //Wait for the kernel to get finished before reporting the profiling
    device_queue.wait();

    // Report kernel execution time and throughput
    t1_kernel = queue_event.get_profiling_info<sycl::info::event_profiling::command_start>();
    t2_kernel = queue_event.get_profiling_info<sycl::info::event_profiling::command_end>();
    time_kernel = (t2_kernel - t1_kernel) / NS;
    std::cout << "Kernel execution time: " << time_kernel << " seconds" << std::endl;
  }

  //Test the results against the golden results
  ifstream myFile;
  myFile.open("golden_check_file.txt",ifstream::in);
  ofstream checkFile;
  checkFile.open("compare_results.txt",ofstream::out);
  vector<int> myList;

  int number;
  while (myFile >> number) {
    myList.push_back(number);
  }

  bool failed = false;
  for (int i=0; i<THETAS*RHOS*2; i++) {
    if ((myList[i]>accumulators[i]+1) || (myList[i]<accumulators[i]-1)) {
      failed = true;
      checkFile << "Failed at " << i << ". Expected: " << myList[i] << ", Actual: "
        << accumulators[i] << std::endl;
    }
  }

  myFile.close();
  checkFile.close();

  if (failed) {printf("FAILED\n");}
  else {printf("VERIFICATION PASSED!!\n");}

  return 1;


}

/* This function reads in a bitmap file and puts it into a vector for processing */

//Struct of 3 bytes for R,G,B components
typedef struct __attribute__((__packed__)) {
  unsigned char  b;
  unsigned char  g;
  unsigned char  r;
} PIXEL;

void read_image(char *image_array) {
  //Declare a vector to hold the pixels read from the image
  //The image is 720x480 so the CPU runtimes are not too long for emulation
  PIXEL im[WIDTH*HEIGHT];

  //Open the image file for reading
  ifstream img;
  img.open("pic.bmp",ios::in);

  //The next part reads the image file into memory
  
  //Bitmap files have a 54-byte header. Skip these bits
  img.seekg(54,ios::beg);
    
  //Loop through the img stream and store pixels in an array
  for (uint i = 0; i < WIDTH*HEIGHT; i++) {
    img.read(reinterpret_cast<char*>(&im[i]),sizeof(PIXEL));

    //The image is black and white (passed through a Sobel filter already)
    //Store 1 in the array for a white pixel, 0 for a black pixel
    if (im[i].r==0 && im[i].g==0 && im[i].b==0) {
      image_array[i] = 0;
    } else {
      image_array[i] = 1;
    }

  }

}
