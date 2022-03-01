ifneq (exec,$(notdir $(patsubst %/,%,$(dir $(CURDIR)))))
include $(CURDIR)/../mk/target.mk
else

include $(ROOTDIR)/mk/variables.mk

INCDIR := $(ROOTDIR)/src

$(warning ROOTDIR=$(ROOTDIR))
$(warning CURDIR=$(CURDIR))
$(warning SRCDIR=$(SRCDIR))
$(warning OBJDIR=$(OBJDIR))
$(warning INCDIR=$(INCDIR))
$(warning MAKECMDGOALS=$(MAKECMDGOALS))

VPATH = $(SRCDIR)

CDEFS = -DVERSION="$(VERSION)" -DPKGNAME="$(PKGNAME)" -DPKGMAINT="$(PKGMAINT)" -DARCH="$(TARGET_ARCH_TYPE)"
CXXFLAGS = $(CDEFS) -D_GNU_SOURCE -D__STDC_LIMIT_MACROS -std=gnu++17 -I$(INCDIR) -Wall -Werror -g
CFLAGS = $(CDEFS) -D_GNU_SOURCE -D__STDC_LIMIT_MACROS -std=gnu17 -I$(INCDIR) -Wall -Werror -g
LDFLAGS =
LDLIBS =

OBJDEPS =
ifndef NOLIBDISCID
LDLIBS += -ldiscid
OBJDEPS += mbid.o
else
CFLAGS += -DNOLIBDISCID
endif
ifndef NOLIBDVDREAD
LDLIBS += -ldvdread
OBJDEPS += dvdid.o
else
CFLAGS += -DNOLIBDVDREAD
endif


.PHONY: exec
exec: $(PKGPROGS)

discinfo: discinfo.c drive.o fdid.o $(OBJDEPS)
	$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -o $@ -DEXECNAME=$@

endif
