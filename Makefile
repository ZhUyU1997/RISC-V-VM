sinclude scripts/env.mk

X_CFLAGS	+= -std=c11 -g
X_ASFLAGS	+= -g
X_LIBS		+= m pthread
X_LDFLAGS	+= `pkg-config sdl2 --libs --cflags`

SRC			+= main.c util.c fb.c
NAME		:= vm

MODULE		+= test