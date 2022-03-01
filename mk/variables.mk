PKGNAME = discinfo
PKGPROGS = discinfo
PKGMAINT = <jlkaus@gmail.com>

EMPTY :=
SPACE := $(EMPTY) $(EMPTY)
COMMA := ,
PKGPROGS_EXP = {$(subst $(SPACE),$(COMMA),$(PKGPROGS))}

ifndef ROOTDIR
ROOTDIR := $(CURDIR)
export ROOTDIR
endif

ifndef TARGET_ARCH_TYPE
TARGET_ARCH_TYPE := $(shell $(ROOTDIR)/mk/get-arch)
export TARGET_ARCH_TYPE
endif
ifndef VERSION
VERSION := $(shell cat $(ROOTDIR)/VERSION)
export VERSION
endif

TAR = /bin/tar
MKDIR = /bin/mkdir
INSTALL = /usr/bin/install
WGET=/usr/bin/wget
WGET_OPT= --quiet --compression=auto
CONVERT=/usr/bin/convert
FIND=/usr/bin/find
CP=/bin/cp
CHMOD=/bin/chmod
CC = /usr/bin/gcc
CXX = /usr/bin/g++

DESTDIR =
prefix = /usr/local
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin
sbindir = $(exec_prefix)/sbin
libexecdir = $(exec_prefix)/libexec
datarootdir = $(prefix)/share
datadir = $(datarootdir)
sysconfdir = $(prefix)/etc
sharedstatedir = $(prefix)/com
localstatedir = $(prefix)/var
runstatedir = $(localstatedir)/run
includedir = $(prefix)/include
docdir = $(datarootdir)/doc/$(PKGNAME)
infodir = $(datarootdir)/info
libdir = $(exec_prefix)/lib
mandir = $(datarootdir)/man


$(warning PKGNAME=$(PKGNAME))
$(warning PKGPROGS=$(PKGPROGS))
$(warning PKGMAINT=$(PKGMAINT))
$(warning VERSION=$(VERSION))
$(warning TARGET_ARCH_TYPE=$(TARGET_ARCH_TYPE))

$(warning DESTDIR=$(DESTDIR))
$(warning prefix=$(prefix))
