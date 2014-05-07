#
# A simple makefile for building project composed of C source files.
#
# Julie Zelenski, for CS107, Sept 2012
#

# It is likely that default C compiler is already gcc, but be explicit anyway
CC = gcc

# The CFLAGS variable sets the flags for the compiler.  CS107 uses these flags:
#  -m32        emit code for IA32 architecture
#  -g          compile with debug information
#  -O0         do not optimize generated code
#  -std=gnu99  use the C99 standard language definition with GNU extensions
#  -W<various> configures which diagnostic warnings are given
CFLAGS = -m32 -g -O0 -std=gnu99 -Wall -Wfloat-equal -Wtype-limits -Wpointer-arith -Wlogical-op -fno-diagnostics-show-option

# The LDFLAGS variable sets the flags for the linker. CS107 uses these flags:
#	-m32	emit code for IA32 architecture
LDFLAGS = -m32

# The line below defines the variable 'programs' to name all of the programs
# to be built by this makefile.
programs = reassemble

# The line below defines a target named 'all', configured to trigger the 
# build of everything named in the 'programs' variable. The first target 
# defined in the makefile becomes the default target. When make is invoked 
# without any arguments, it builds the default target.
all : $(programs)

# The programs are executable built from a single source file and don't
# require any customization of the build process. The default make rules build
# the program named 'binky' by compiling the single file 'binky.c', which is all
# we need here.

# The line below defines the clean target to remove any previous build results
clean:
	rm -f $(programs) core *.o

# PHONY is used to mark targets that don't represent actual files/build products
.PHONY: clean all


