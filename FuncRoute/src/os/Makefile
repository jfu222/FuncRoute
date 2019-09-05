# Makefile for VideoImageEncoderDecoder project
#
# fuj@1000video.com.cn & 2017.06.16
#


GCC = gcc
CC = g++

is_debug = 0
DEBUG =

ifeq ($(is_debug), 1)
	DEBUG = -g
else
	DEBUG = -O3
endif

CFLAGS = -c -Wall $(DEBUG) -fPIC -I. -fstack-protector-all

#----------------------------------
lib_a = libos.a

SUBDIRS = \
	linux \
	.

#----------------------------------
all:
	$(MAKE) $(AM_MAKEFLAGS) all-recursive

#----------------------------------
all-am: Makefile $(lib_a)

$(lib_a):
#	rm -f $(lib_a)
#	ln -sf linux/liblinux.a $(lib_a)

#----------------------------------
recursive_targets = all-recursive clean-recursive

$(recursive_targets):
	dot_seen=no; \
	target=`echo $@ | sed s/-recursive//`; \
	list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		echo "Making $$target in $$subdir"; \
		if test "$$subdir" = "."; then \
			dot_seen=yes; \
			local_target="$$target-am"; \
		else \
			local_target="$$target"; \
		fi; \
		(cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) $$local_target is_debug=$(is_debug)); \
	done;

#----------------------------------
clean: clean-recursive

clean-am:
#	rm -f $(lib_a)

#----------------------------------
.PHONY: all $(recursive_targets) clean

