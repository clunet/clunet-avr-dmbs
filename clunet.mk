#
#            DMBS Build System
#     Released into the public domain.
#
#  dean [at] fourwalledcubicle [dot] com
#        www.fourwalledcubicle.com
#

# Include Guard
ifeq ($(filter CLUNET, $(DMBS_BUILD_MODULES)),)

# Sanity check user supplied DMBS path
ifndef DMBS_PATH
$(error Makefile DMBS_PATH option cannot be blank)
endif

# Location of the current module
CLUNET_MODULE_PATH := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))

# Import the CORE module of DMBS
include $(DMBS_PATH)/core.mk

# This module needs to be included before gcc.mk
ifneq ($(findstring GCC, $(DMBS_BUILD_MODULES)),)
$(error Include this module before gcc.mk)
endif

# Help settings
DMBS_BUILD_MODULES         += CLUNET
DMBS_BUILD_TARGETS         += 
DMBS_BUILD_MANDATORY_VARS  += DMBS_PATH CLUNET_DEVICE_ADDRESS CLUNET_BOOTLOADER_SUPPORT CLUNET_BOOTLOADER_ADDRESS
DMBS_BUILD_OPTIONAL_VARS   += 
DMBS_BUILD_PROVIDED_VARS   += CLUNET_SRC
DMBS_BUILD_PROVIDED_MACROS += 

# Sanity check user supplied values
$(foreach MANDATORY_VAR, $(DMBS_BUILD_MANDATORY_VARS), $(call ERROR_IF_UNSET, $(MANDATORY_VAR)))

# CLUNET Library

CC_FLAGS   += -DCLUNET_BOOTLOADER_SUPPORT=$(CLUNET_BOOTLOADER_SUPPORT) -DCLUNET_DEVICE_ADDRESS=$(CLUNET_DEVICE_ADDRESS)
ifneq ($(CLUNET_BOOTLOADER_SUPPORT), 0)
 # bootloader support enabled
 CC_FLAGS  += -DCLUNET_BOOTLOADER_ADDRESS=$(CLUNET_BOOTLOADER_ADDRESS)
 LD_FLAGS  += -ubtldr_main -Wl,--section-start=.bootloader=$(CLUNET_BOOTLOADER_ADDRESS)
 LD_FLAGS  += -Wl,--section-start=.clunetreset=$(shell printf "0x%X\n" $$(($(CLUNET_BOOTLOADER_ADDRESS) - 2)))
endif

ifeq ($(CLUNET_BOOTLOADER_SUPPORT), 3)
 SRC           := 
 TARGET        := bootloader
 CLUNET_SRC    := $(CLUNET_MODULE_PATH)/src/boot.S
 LD_FLAGS      += -nostartfiles
else
 CLUNET_SRC    := $(CLUNET_MODULE_PATH)/src/clunet.S
 ifeq ($(CLUNET_BOOTLOADER_SUPPORT), 2)
  CLUNET_SRC   += $(CLUNET_MODULE_PATH)/src/boot.S
 endif
endif

# Compiler flags and sources
SRC         += $(CLUNET_SRC)
CC_FLAGS    += -I$(CLUNET_MODULE_PATH)/include/

# Phony build targets for this module
.PHONY: $(DMBS_BUILD_TARGETS)

endif
