# .bash_profile

# Get the aliases and functions
if [ -f ~/.bashrc ]; then
   . ~/.bashrc
fi

# User specific environment and startup programs
export PATH=$PATH:$HOME/.local/bin:$HOME/bin

# Enable Intel tools
export INTEL_LICENSE_FILE=/usr/local/licenseserver/psxe.lic
export PATH=/glob/intel-python/python3/bin/:/glob/intel-python/python2/bin/:${PATH}
source /glob/development-tools/parallel-studio/bin/compilervars.sh intel64
export PATH=$PATH:/bin
if [ -d /opt/intel/inteloneapi ]; then source /opt/intel/inteloneapi/setvars.sh > /dev/null 2>&1; fi

# Make sure that most programs (in particular, pip) leave temp files locally
if [ ! -d ${HOME}/tmp ]; then
  mkdir ${HOME}/tmp
fi
export TMPDIR=${HOME}/tmp
#export PBS_DEFAULT=v-qsvr-nda