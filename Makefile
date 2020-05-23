sinclude scripts/env.mk

X_CFLAGS	+= -std=c11 -g
X_ASFLAGS	+= -g
X_LIBS		+= m

SRC			+= main.c
NAME		:= vm