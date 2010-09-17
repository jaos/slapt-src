SUBDIRS = src po

SLACK_FILES = slack-desc slack-required slapt-srcrc
EXTRA_DIST = $(SLACK_FILES) slapt-src.8

PKGDIR=`pwd`/pkg

clean-local: clean-pkg
.PHONY: clean-pkg
clean-pkg:
	-@rm -rf pkg
	-@rm -rf @PACKAGE@-@PACKAGE_VERSION@-@host_cpu@-1.tgz

install-data-local:
	$(mkinstalldirs) $(DESTDIR)/etc/slapt-get
	install --mode=0644 slapt-srcrc $(DESTDIR)/etc/slapt-get/slapt-srcrc.new
	if [ ! -f $(DESTDIR)/etc/slapt-get/slapt-srcrc ]; then mv $(DESTDIR)/etc/slapt-get/slapt-srcrc.new $(DESTDIR)/etc/slapt-get/slapt-srcrc; fi
	$(mkinstalldirs) $(DESTDIR)$(mandir)/man8
	install slapt-src.8 $(DESTDIR)$(mandir)/man8/
	gzip -f $(DESTDIR)$(mandir)/man8/slapt-src.8
	-chown -R $$(stat --format "%u:%g" $(bindir)) $(DESTDIR)/$(bindir)
	-find $(DESTDIR)/$(bindir) -type f -exec strip --strip-unneeded {} \;

pkg: all
	$(MAKE) install DESTDIR=$(PKGDIR)
	$(mkinstalldirs) $(PKGDIR)/install
	install slack-desc $(PKGDIR)/install/
	install slack-required $(PKGDIR)/install/
	@echo "if [ ! -d etc/slapt-get ]; then mkdir -p etc/slapt-get; fi; if [ ! -f etc/slapt-get/slapt-srcrc ]; then mv -f etc/slapt-get/slapt-srcrc.new etc/slapt-get/slapt-srcrc; diff -q etc/slapt-get/slapt-srcrc etc/slapt-get/slapt-srcrc.new >/dev/null 2>&1 && rm etc/slapt-get/slapt-srcrc.new; fi;" > $(PKGDIR)/install/doinst.sh
	$(mkinstalldirs) $(PKGDIR)/etc/slapt-get
	install --mode=0644 slapt-srcrc $(PKGDIR)/etc/slapt-get/slapt-srcrc.new
	$(mkinstalldirs) $(PKGDIR)/$(mandir)/man8
	install slapt-src.8 $(PKGDIR)$(mandir)/man8/
	gzip -f $(PKGDIR)$(mandir)/man8/slapt-src.8
	-chown -R $$(stat --format "%u:%g" $(bindir)) $(PKGDIR)/$(bindir)
	-find $(PKGDIR)/$(bindir) -type f -exec strip --strip-unneeded {} \;
	$(mkinstalldirs) $(PKGDIR)/$(datadir)/doc/@PACKAGE@
	install COPYING ChangeLog README TODO $(PKGDIR)/$(datadir)/doc/@PACKAGE@/
	( cd $(PKGDIR) && /sbin/makepkg -l y -c n ../@PACKAGE@-@PACKAGE_VERSION@-@host_cpu@-1.tgz )

