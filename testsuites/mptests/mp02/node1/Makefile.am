##
## $Id$
##

rtems_tests_PROGRAMS = mp02-node1
mp02_node1_SOURCES = ../init.c ../task1.c ../system.h

dist_rtems_tests_DATA = mp02-node1.scn
dist_rtems_tests_DATA += mp02-node1.doc

include $(RTEMS_ROOT)/make/custom/@RTEMS_BSP@.cfg
include $(top_srcdir)/../automake/compile.am
include $(top_srcdir)/../automake/leaf.am

AM_CPPFLAGS += -DNODE_NUMBER=1 -I$(srcdir)/..

AM_CPPFLAGS += -I$(top_srcdir)/../support/include

LINK_OBJS = 
LINK_LIBS = $(mp02_node1_LDLIBS)

mp02-node1$(EXEEXT): $(mp02_node1_OBJECTS) $(mp02_node1_DEPENDENCIES)
	@rm -f mp02-node1$(EXEEXT)
	$(make-exe)

include $(top_srcdir)/../automake/local.am
