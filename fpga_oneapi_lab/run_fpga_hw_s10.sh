#!/bin/bash
export PATH=/glob/intel-python/python3/bin/:/glob/intel-python/python2/bin/:${PATH}
source /opt/intel/oneapi/setvars.sh > /dev/null 2>&1
cd bin
aocl initialize acl0 pac_s10
./hough_transform_local_mem.fpga
