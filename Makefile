#*****************************************************
# Generic make rules for all files in this repo
#*****************************************************

#************************
# Author: Zee Lv
# Data:   Jan 22, 2019
#************************

#******************************************************************************
# Modified by Juan Pablo Pino
# May, 2019
#******************************************************************************

# clean up all build product in all sub folders
clean:
	find . -name "*.o" -type f -delete
	find . -name "*.d" -type f -delete
	rm -r build 2> /dev/null
