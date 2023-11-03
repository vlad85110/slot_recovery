# contrib/slot_recovery/Makefile

MODULE_big = slot_recovery
OBJS = \
	$(WIN32RES) \
	slot_recovery.o \
	callbacks.o \
	functions.o

EXTENSION = slot_recovery
DATA = slot_recovery--1.0.sql
#PGFILEDESC = "slot_recovery - preload relation data into system buffer cache"

TAP_TESTS = 1

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/slot_recovery
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
