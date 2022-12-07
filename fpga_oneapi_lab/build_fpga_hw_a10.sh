#!/bin/bash
export PATH=/glob/intel-python/python3/bin/:/glob/intel-python/python2/bin/:${PATH}
source /opt/intel/oneapi/setvars.sh > /dev/null 2>&1
rm -Rf bin
cp -pR bin-original bin
echo "Got here"
dpcpp -fintelfpga ./lab/hough_transform_local_mem.cpp -Xshardware -Xsboard=/opt/intel/oneapi/intel_a10gx_pac:pac_a10 -Xsprofile -o ./bin/hough_transform_local_mem.fpga
echo "Got here2"