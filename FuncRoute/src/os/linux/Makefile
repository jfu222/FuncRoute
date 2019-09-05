# Makefile for VideoImageEncoderDecoder project
#
# fuj@1000video.com.cn & 2017.06.16
#


GCC = gcc
CC = g++
AR = ar
ARFLAGS = crs

is_debug = 0
DEBUG =

ifeq ($(is_debug), 1)
	DEBUG = -g
else
	DEBUG = -O3
endif

CFLAGS = -c -Wall $(DEBUG) -fPIC -I. -fstack-protector-all

#----------------------------------
lib_a = liblinux.a

src_files = $(wildcard *.cpp)

objects = $(patsubst %.cpp, %.o, $(src_files))

#----------------------------------
all: $(lib_a)

$(lib_a): $(objects)
	-rm -f $(lib_a)
	$(AR) $(ARFLAGS) $(lib_a) $(objects)

$(objects): %.o: %.cpp
	$(CC) $(CFLAGS) $< -o $@

#----------------------------------
clean:
	rm -f *.o $(lib_a)

#----------------------------------
.PHONY: all clean

