ifndef ROOTDIR
ROOTDIR := $(CURDIR)
export ROOTDIR
endif

include $(ROOTDIR)/mk/variables.mk

INSTALLDIR := $(ROOTDIR)/gen/install

.PHONY: all
all: exec

.PHONY: exec
exec:
	$(MAKE) -C $(ROOTDIR)/src exec

.PHONY: install-dir
install-dir: clean-install
	$(MKDIR) -p $(INSTALLDIR)
	$(MAKE) -C $(CURDIR) DESTDIR=$(INSTALLDIR) install

.PHONY: package
package: install-dir
	cd $(INSTALLDIR) && $(TAR) czf $(ROOTDIR)/gen/$(PKGNAME)-$(VERSION)-$(TARGET_ARCH_TYPE).tar.gz *

.PHONY: install
install: exec
	$(INSTALL) -D -m 755 -t $(DESTDIR)$(bindir)/ $(ROOTDIR)/gen/exec/$(TARGET_ARCH_TYPE)/$(PKGPROGS_EXP)
	$(INSTALL) -d $(DESTDIR)$(datadir)/$(PKGNAME)
	$(CHMOD) -R a+Xr,g-w,o-w $(DESTDIR)$(datadir)/$(PKGNAME)

.PHONY: uninstall
uninstall:
	$(RM) -f $(DESTDIR)$(bindir)/$(PKGPROGS_EXP)
	-$(RM) -r $(DESTDIR)$(datadir)/$(PKGNAME)

.PHONY: clean
clean: clean-exec clean-install

.PHONY: distclean
distclean: distclean-exec distclean-install
	-$(RM) -r $(ROOTDIR)/gen

.PHONY: mostlyclean
mostlyclean: mostlyclean-exec mostlyclean-install

.PHONY: clean-exec
clean-exec:
	$(MAKE) -C $(ROOTDIR)/src clean

.PHONY: mostlyclean-exec
mostlyclean-exec:
	$(MAKE) -C $(ROOTDIR)/src mostlyclean

.PHONY: distclean-exec
distclean-exec:
	$(MAKE) -C $(ROOTDIR)/src distclean

.PHONY: clean-install
clean-install: distclean-install

.PHONY: mostlyclean-install
mostlyclean-install: distclean-install

.PHONY: distclean-install
distclean-install:
	-$(RM) -r $(INSTALLDIR)

.DEFAULT:
	$(MAKE) -C $(ROOTDIR)/src $(MAKECMDGOALS)
