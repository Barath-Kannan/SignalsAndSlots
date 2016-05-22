#>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
#||BUILD FLAGS||#
#>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

#Generates moc files from headers for QT applications
BUILD_MOCS = 0

#Generates dynamic (shared object) and static (archive) from non-main objects
BUILD_LIB = 1

#Generates binary from all objects
BUILD_BIN = 0

#Generates test application
BUILD_TESTS = 0

#Runs pre-build scripts
RUN_PREBUILD = 0

#Runs post-build scripts
RUN_POSTBUILD = 0

#>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
#||EXTERNALS||#
#>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

#>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
#||BUILD CONFIG||#
#>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

######################
## Compiler Options ##
######################
#Compiler
CC = g++

#Compiler options
OPTS = -Wall -fPIC
RELEASE_OPTS = -O2 -O3
DEBUG_OPTS = -g

#Static library archiver
LG = ar

#Static archiver options
LGOPTS = -r

#Project source file extensions
SRCTYPES = cpp cc c

#Project header file extensions
HTYPES = h hpp

###########################
## Project Configuration ##
###########################

PROJECT = gtest

#Define Flags
DEFINE =  LINUX

#Pre-Build Scripts
PREBUILD = 

#Post-Build Scripts
POSTBUILD = 

############################
# Directory Configuration ##
############################

# Project Directories
SRCDIR = src
INCDIR = inc
TESTDIR = 

#Generated Directories
GENDIR = gen

####################################
# External Dependencies and Paths ##
####################################

# External Include Directories 
EXTINC = src

# External/System Libraries
LIBDIRS = 
LIBS   = pthread

#Additional Source files to compile
ADDSRC = 

# External Links
LINKS = 

#Additional files/folders to clean
CLEANFILES = 

###########################
# Unit Test Dependencies ##
###########################

#Unit testing library
TESTLIB = 

#Unit testing library include path
TESTINC = 

#>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
#||ADDITIONAL OPTIONS||#
#>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

# debug = builds with debug options by default (can be manually specified by invoking the debug target)
# release = builds with release options by default (can be manually specified by invoking the release target)
BUILD_TYPE = debug

# 0 = Include only INCDIR
# 1 = Recursively include all directories in INCDIR
RECURSIVELY_INCLUDE_LOCAL = 0

# 0 = Include only directories listed in EXTINC
# 1 = Recursively include all directories for each directory specified in extinc
RECURSIVELY_INCLUDE_EXTERNAL = 0

# 0 = Don't include main object file in generated libraries
# 1 = Include main object file in generated libraries
MAIN_OBJECT_IN_LIBRARIES = 1

# 0 = The project contains no main method (i.e. exclusively a library)
# 1 = The project contains at least one main method
PROJECT_CONTAINS_MAIN = 1

# If they exist, sources with main methods can be explicitly specified here.
# This saves compilation time, as the makefile will no longer need to 
# search the object files for the main symbol
EXPLICIT_MAIN_SOURCE = CustomTestPrinter.cpp

#>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
#||BUILD SCRIPT||# 
#>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
include core.mk
