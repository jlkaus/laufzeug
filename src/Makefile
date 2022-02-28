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

.PHONY: exec
exec: $(PKGPROGS)

discid: private CFLAGS += -DNOLIBDVDREAD -DEXECNAME=$@
discid: discinfo.c  drive.o mbid.o fdid.o
	$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -ldiscid -o $@

dvdinfo: private CFLAGS += -DNOLIBDISCID -DEXECNAME=$@
dvdinfo: discinfo.c drive.o dvdid.o fdid.o
	$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -ldvdread -o $@

discinfo: private CFLAGS += -DEXECNAME=$@
discinfo: discinfo.c drive.o dvdid.o mbid.o fdid.o
	$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -ldiscid -ldvdread -o $@

ldiscid: private CFLAGS += -DNOLIBDVDREAD -DNOLIBDISCID -DEXECNAME=$@
ldiscid: discinfo.c drive.o fdid.o
	$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -o $@


endif
