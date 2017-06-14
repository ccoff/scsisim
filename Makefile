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

LIB_SRC = usb.c scsi.c sim.c gsm.c utils.c
LIB_OBJS = $(LIB_SRC:%.c=%.o)

BASE_LIB_NAME = scsisim
SHARED_LIB_NAME = lib$(BASE_LIB_NAME).so
STATIC_LIB_NAME = lib$(BASE_LIB_NAME).a
SHARED_OBJS_DIR = shared
STATIC_OBJS_DIR = static
SHARED_OBJS = $(addprefix $(SHARED_OBJS_DIR)/, $(LIB_OBJS))
STATIC_OBJS = $(addprefix $(STATIC_OBJS_DIR)/, $(LIB_OBJS))

$(SHARED_OBJS_DIR)/%.o: CFLAGS += -fpic
$(SHARED_OBJS_DIR)/%.o: ./$(LIB_SRC) | $(SHARED_OBJS_DIR)
	$(CC) $(CFLAGS) -c $*.c -o $@

$(STATIC_OBJS_DIR)/%.o: ./$(LIB_SRC) | $(STATIC_OBJS_DIR)
	$(CC) $(CFLAGS) -c $*.c -o $@

DEMO_NAME = demo
DEMO_SRC = demo.c
DEMO_OBJS = $(DEMO_SRC:%.c=%.o)

.PHONY: all clean .FORCE

all: shared_lib static_lib demo

$(SHARED_OBJS_DIR) $(STATIC_OBJS_DIR):
	@mkdir $@

shared_lib: $(SHARED_OBJS)
	$(CC) $(LDFLAGS) -shared -o $(SHARED_LIB_NAME) $(SHARED_OBJS) 
	@echo "*******************************"
	@echo "*   Shared library complete   *"
	@echo "*******************************"

static_lib: $(STATIC_OBJS)
	$(AR) rcs $(STATIC_LIB_NAME) $(STATIC_OBJS) 
	@echo "*******************************"
	@echo "*   Static library complete   *"
	@echo "*******************************"

demo: demo.o .FORCE
	$(CC) $(LDFLAGS) -o $(DEMO_NAME) $(DEMO_OBJS) $(STATIC_LIB_NAME)
# To link the demo with the shared library instead:
#	$(CC) $(LDFLAGS) $(DEMO_OBJS) -o $(DEMO_NAME) -L. -l$(BASE_LIB_NAME)
	@echo "*******************************"
	@echo "*        Demo complete        *"
	@echo "*******************************"

clean:
	$(RM) $(SHARED_OBJS) $(STATIC_OBJS) $(SHARED_LIB_NAME) $(STATIC_LIB_NAME) $(DEMO_OBJS) $(DEMO_NAME)
	@echo "*******************************"
	@echo "*      Cleanup complete       *"
	@echo "*******************************"

# End makefile

