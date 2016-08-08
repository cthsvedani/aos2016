#
# Copyright 2014, NICTA
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(NICTA_BSD)
#

# Targets
TARGETS := libclock.a

# Source files required to build the target
CFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard ${SOURCE_DIR}/src/plat/$(PLAT)/*.c))
CFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard ${SOURCE_DIR}/src/plat/$(PLAT)/uboot/*.c))
CFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard ${SOURCE_DIR}/src/*.c))
CFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard ${SOURCE_DIR}/src/plat/$(PLAT)/resource/*.c))

HDRFILES := \
    $(wildcard ${SOURCE_DIR}/include/*) \
    $(wildcard ${SOURCE_DIR}/include/clock) \
    $(wildcard ${SOURCE_DIR}/plat_include/$(PLAT)/*) \
    $(wildcard ${SOURCE_DIR}/arch_include/${ARCH}/*)

CFLAGS := -g -O0
include $(SEL4_COMMON)/common.mk

