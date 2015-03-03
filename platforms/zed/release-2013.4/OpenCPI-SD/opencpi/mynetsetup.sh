# This script should be customized to do what you want.
# It is used in two contexts:
# 1. The core setup has not been run, so run it with your specific parameters
#    (mount point on development host, etc.), and supply the IP address as arg
# 2. The core setup HAS been run and you are just setting up a shell or ssh session

trap "trap - ERR; break" ERR; for i in 1; do
if test "$OCPI_BASE_DIR" = ""; then
  if test "$1" = ""; then
     echo It appears that the environment is not set up yet.
     echo You must supply the IP address of the OpenCPI server machine as an argument to this script.
     break
  fi
  # CUSTOMIZE THIS LINE FOR YOUR ENVIRONENT
  # Second arg is shared file system mount point on development system
  # Third argument is opencpi dir relative to mount point
  # Fourth argument is time server for the (old) time protocol used by the rdate command
  # Fifth arg is timezone spec - see "man timezone" for the format.
  source /mnt/card/opencpi/zednetsetup.sh $1 /Users/jek/Business ocpi/main/opencpi time.nist.gov EST5EDT,M3.2.0,M11.1.0
  break # this script will be rerun recursively by setup.sh
fi
# Tell the ocpihdl utility to always assume the FPGA device is the zynq PL.
export OCPI_DEFAULT_HDL_DEVICE=pl:0
# Set my OCPI path to some bitstream directories I am working on.
export OCPI_LIBRARY_PATH=$OCPI_LIBRARY_PATH:$OCPI_BASE_DIR/hdl/assemblies/biascapture:$OCPI_BASE_DIR/hdl/assemblies/testbias:$OCPI_BASE_DIR/hdl/assemblies/patternbias
export OCPI_SMB_SIZE=100000
export OCPI_SUPPRESS_HDL_NETWORK_DISCOVERY=1
# Get ready to run some test xml-based applications
cd $OCPI_BASE_DIR/tools/cdk/examples/xml
# Shorten the default shell prompt
PS1='% '
# Print the available containers as a sanity check
echo Discovering available containers...
ocpirun -C
# Since we are sourcing this script we can't use "exit", do "done" is for "break"
done
