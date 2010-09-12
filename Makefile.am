SUBDIRS = src

SLACK_FILES = slack-desc slack-required
EXTRA_DIST = $(SLACK_FILES)

PKGDIR=`pwd`/pkg
pkg: all
	$(MAKE) install DESTDIR=$(PKGDIR)
	$(mkinstalldirs) $(PKGDIR)/install
	install slack-desc $(PKGDIR)/install/
	install slack-required $(PKGDIR)/install/
	-chown -R $$(stat --format "%u:%g" $(bindir)) $(PKGDIR)/$(bindir)
	-chown -R $$(stat --format "%u:%g" $(sbindir)) $(PKGDIR)/$(sbindir)
	-find $(PKGDIR)/$(bindir) $(PKGDIR)/$(sbindir)/ -type f -exec strip --strip-unneeded {} \;
	( cd $(PKGDIR) && /sbin/makepkg -l y -c n ../@PACKAGE@-@PACKAGE_VERSION@-@host_cpu@-1.tgz )

