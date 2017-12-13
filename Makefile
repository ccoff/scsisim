#
#  Makefile for SCSI-generic SIM card driver libraries
#  and demonstration executable.
#
#  Copyright (c) 2017, Chris Coffey <kpuc@sdf.org>
#
#  Permission to use, copy, modify, and/or distribute this software
#  for any purpose with or without fee is hereby granted, provided
#  that the above copyright notice and this permission notice appear
#  in all copies.
#
#  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL 
#  WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
#  WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
#  AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
#  DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
#  OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
#  TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
#  PERFORMANCE OF THIS SOFTWARE.
#


CC = gcc
CFLAGS = -g -Wall -std=gnu99
LDFLAGS = -g

SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build

COMPILE_OBJS = $(CC) $(CFLAGS) -I $(INCLUDE_DIR) -c $(addprefix $(SRC_DIR)/, $*.c) -o $@

# Libraries:
LIB_SRC = usb.c scsi.c sim.c gsm.c utils.c
LIB_OBJS = $(LIB_SRC:%.c=%.o)
BASE_LIB_NAME = scsisim

SHARED_LIB_NAME = lib$(BASE_LIB_NAME).so
SHARED_OBJS_DIR = $(BUILD_DIR)/shared-objs
SHARED_OBJS = $(addprefix $(SHARED_OBJS_DIR)/, $(LIB_OBJS))

STATIC_LIB_NAME = lib$(BASE_LIB_NAME).a
STATIC_OBJS_DIR = $(BUILD_DIR)/static-objs
STATIC_OBJS = $(addprefix $(STATIC_OBJS_DIR)/, $(LIB_OBJS))

$(SHARED_OBJS_DIR)/%.o: CFLAGS += -fpic
$(SHARED_OBJS_DIR)/%.o: $(addprefix $(SRC_DIR)/, $(LIB_SRC)) | $(SHARED_OBJS_DIR)
	$(COMPILE_OBJS)

$(STATIC_OBJS_DIR)/%.o: $(addprefix $(SRC_DIR)/, $(LIB_SRC)) | $(STATIC_OBJS_DIR)
	$(COMPILE_OBJS)

# Demonstration executable:
DEMO_NAME = demo
DEMO_SRC = demo.c
DEMO_OBJS_DIR = $(BUILD_DIR)/demo-objs
DEMO_OBJS = $(addprefix $(DEMO_OBJS_DIR)/, $(DEMO_SRC:%.c=%.o))

$(DEMO_OBJS_DIR)/%.o: $(addprefix $(SRC_DIR)/, $(DEMO_SRC)) | $(DEMO_OBJS_DIR)
	$(COMPILE_OBJS)

# Targets:
.PHONY: all clean .FORCE

all: shared_lib static_lib demo

$(SHARED_OBJS_DIR) $(STATIC_OBJS_DIR) $(DEMO_OBJS_DIR):
	@mkdir -p $@

shared_lib: $(SHARED_OBJS)
	$(CC) $(LDFLAGS) -shared -o $(BUILD_DIR)/$(SHARED_LIB_NAME) $(SHARED_OBJS) 
	@echo "*******************************"
	@echo "*   Shared library complete   *"
	@echo "*******************************"

static_lib: $(STATIC_OBJS)
	$(AR) rcs $(BUILD_DIR)/$(STATIC_LIB_NAME) $(STATIC_OBJS) 
	@echo "*******************************"
	@echo "*   Static library complete   *"
	@echo "*******************************"

demo: $(DEMO_OBJS) .FORCE
	$(CC) $(LDFLAGS) -o $(BUILD_DIR)/$(DEMO_NAME) $(DEMO_OBJS) $(BUILD_DIR)/$(STATIC_LIB_NAME)
# To link the demo with the shared library instead, comment out the previous line,
# uncomment the following line, and then run 'cd build && LD_LIBRARY_PATH=$(pwd) ./demo'
#	$(CC) $(LDFLAGS) $(DEMO_OBJS) -o $(BUILD_DIR)/$(DEMO_NAME) -L./$(BUILD_DIR) -l$(BASE_LIB_NAME)
	@echo "*******************************"
	@echo "*        Demo complete        *"
	@echo "*******************************"

clean:
	$(RM) -r $(SHARED_OBJS_DIR) $(STATIC_OBJS_DIR) $(DEMO_OBJS_DIR)
	$(RM) $(BUILD_DIR)/$(SHARED_LIB_NAME) $(BUILD_DIR)/$(STATIC_LIB_NAME) $(BUILD_DIR)/$(DEMO_NAME)
	@echo "*******************************"
	@echo "*      Cleanup complete       *"
	@echo "*******************************"

# End makefile

