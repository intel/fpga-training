#!/bin/bash
export PATH=/glob/intel-python/python3/bin/:/glob/intel-python/python2/bin/:${PATH}
source /opt/intel/oneapi/setvars.sh > /dev/null 2>&1
rm -Rf bin_s10
cp -pR bin-original bin_s10
echo "Got here"
dpcpp -fintelfpga ./lab_s10/hough_transform_local_mem.cpp -Xshardware -Xsboard=/opt/intel/oneapi/intel_s10sx_pac:pac_s10 -Xsprofile -o ./bin_s10/hough_transform_local_mem_stratix10.fpga
echo "Got here2"