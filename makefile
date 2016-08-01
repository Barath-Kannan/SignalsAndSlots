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
BUILD_TESTS = 1

#Runs pre-build scripts
RUN_PREBUILD = 1

#Runs post-build scripts
RUN_POSTBUILD = 0

#>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
#||EXTERNALS||#
#>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
GOOGLETEST_INC = 3rdparty/googletest/inc
GOOGLETEST_LIB = 3rdparty/googletest/gen/debug/lib/static/libgtest.a

#>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
#||BUILD CONFIG||#
#>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

######################
## Compiler Options ##
######################

#Compiler
CC = g++

#Compiler options
OPTS = -Wall -std=c++14 -fPIC
RELEASE_OPTS = -O3
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

PROJECT = BSignals
VERSION = 1.1

#Define Flags
DEFINE =  LINUX

#Pre-Build Scripts
PREBUILD = +$(MAKE) -C 3rdparty/googletest

#Post-Build Scripts
POSTBUILD = 

############################
# Directory Configuration ##
############################

# Project Directories
SRCDIR = src
INCDIR = inc
TESTDIR = test

#Generated Directories
GENDIR = gen

####################################
# External Dependencies and Paths ##
####################################

# External Include Directories 
EXTINC = 

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
TESTLIB = $(GOOGLETEST_LIB)

#Unit testing library include path
TESTINC = $(GOOGLETEST_INC)

#>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
#||ADDITIONAL OPTIONS||#
#>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

# debug - builds with debug options by default (can be manually specified by invoking the debug target)
# release - builds with release options by default (can be manually specified by invoking the release target)
BUILD_TYPE = release

# 0 = Include only INCDIR
# 1 = Recursively include all directories in INCDIR
RECURSIVELY_INCLUDE_LOCAL = 0

# 0 = Include only directories listed in EXTINC
# 1 = Recursively include all directories for each directory specified in extinc
RECURSIVELY_INCLUDE_EXTERNAL = 0

# 0 = Don't include main object file in generated libraries
# 1 = Include main object file in generated libraries
MAIN_OBJECT_IN_LIBRARIES = 0

# 0 = The project contains no main method (i.e. exclusively a library)
# 1 = The project contains at least one main method
PROJECT_CONTAINS_MAIN = 0

# If they exist, sources with main methods can be explicitly specified here.
# This saves compilation time, as the makefile will no longer need to
# search the object files for the main symbol
EXPLICIT_MAIN_SOURCE =

# 0 = Binary names are generated from the project name and the main source file name(s)
# 1 = Binary names are generated from the project name only (only valid when there is 1 main source)
# 2 = Binary source names are generated from the main source file name(s) only
BINARY_NAMING_CONVENTION = 0

#>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
#||BUILD SCRIPT||# 
#>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
include core.mk
