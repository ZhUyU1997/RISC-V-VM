sinclude ../scripts/env.mk

CROSS_COMPILE	:=riscv64-linux-gnu-

# Make variables (CC, etc...)
AS			:=	$(CROSS_COMPILE)gcc -x assembler-with-cpp
CC			:=	$(CROSS_COMPILE)gcc
CPP			:=	$(CROSS_COMPILE)gcc -E
CXX			:=	$(CROSS_COMPILE)g++
LD			:=	$(CROSS_COMPILE)ld
AR			:=	$(CROSS_COMPILE)ar
OC			:=	$(CROSS_COMPILE)objcopy
OD			:=	$(CROSS_COMPILE)objdump
NM			:=	$(CROSS_COMPILE)nm
STRIP		:=	$(CROSS_COMPILE)strip

export AS AR CC LD CPP CXX OC OD NM HOSTCC

X_CFLAGS	+= -std=c11 -march=rv32imafd -mabi=ilp32d -mcmodel=medany -ffreestanding -O0
X_ASFLAGS	+= -march=rv32imafd -mabi=ilp32d -mcmodel=medany -ffreestanding 

SRC			+= start.S test.c
NAME		:= test.bin

define CUSTOM_TARGET_CMD
echo [CUSTOM TARGET] $@; \
$(LD) $(X_OBJS) -o $@.o -T $(srctree)/$(src)/test.ld -nostdlib; \
$(OC) -v -O binary $@.o $@
endef