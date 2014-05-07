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
CFLAGS = -m32 -g -O0 -std=gnu99 -Wall -Wfloat-equal -Wtype-limits -Wpointer-arith -Wlogical-op -fno-diagnostics-show-option -I.

# The LDFLAGS variable sets the flags for the linker. CS107 uses these flags:
#	-m32	emit code for IA32 architecture
LDFLAGS = -m32

# The line below defines the variable 'programs' to name all of the client programs
# to be built by this makefile. If you write additional client programs,
# add them to the list below so they can be built using make. The programs
# named in this list will be compiled from a similarly-named .c file (i.e.
# the program vectest is built from client program vectest.c which 
# uses the CVector/CMap library). 
programs = vectest maptest synonyms

# The line below defines a target named 'all', configured to trigger the 
# build of everything named in the 'programs' variable. The first target 
# defined in the makefile becomes the default target. When make is invoked 
# without any arguments, it builds the default target.
all : $(programs)


# Specific per-target customizations and prerequisites are listed here

# The test programs use the ADTs from cvecmap lib so must include it
# as a prerequisite. The line below extends the implicit rule for building 
# an executable to include the additional prerequisite for all programs
# named above
$(programs): %:%.c libcvecmap.a

# The soln target makes solution versions of the program.
# For each program 'binky' in $(programs) the rule specifies how
# to build 'binky_soln' by linking the binky.c code to the
# sample library versions of CVector/CMap, producing a binky_soln
# you can compare to binky
soln_programs = $(programs:%=%_soln)
soln: $(soln_programs)

$(soln_programs): %_soln: %.c /afs/ir/class/cs107/samples/assign3/libcvecmap_soln.a
		$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -o $@

# Custom rule needed to build library (no implicit rule), marking the object files
# as intermediate will discard them after folding into library
libcvecmap.a: cmap.o cvector.o
	$(AR) $(ARFLAGS) $@ $?
.INTERMEDIATE: cmap.o cvector.o

# The line below defines the clean target to remove any previous build results
clean:
	rm -f $(programs) $(soln_programs) libcvecmap.a core *.o

# PHONY is used to mark targets that don't represent actual files/build products
.PHONY: clean all soln
