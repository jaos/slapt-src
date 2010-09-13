SUBDIRS = src

SLACK_FILES = slack-desc slack-required slapt-srcrc
EXTRA_DIST = $(SLACK_FILES) slapt-src.8

PKGDIR=`pwd`/pkg

clean-local: rmpkg
rmpkg:
	-@rm -rf pkg
	-@rm -rf @PACKAGE@-@PACKAGE_VERSION@-@host_cpu@-1.tgz

pkg: all
	$(MAKE) install DESTDIR=$(PKGDIR)
	$(mkinstalldirs) $(PKGDIR)/install
	install slack-desc $(PKGDIR)/install/
	install slack-required $(PKGDIR)/install/
	@echo "if [ ! -d etc/slapt-get ]; then mkdir -p etc/slapt-get; fi; if [ ! -f etc/slapt-get/slapt-srcrc ]; then mv -f etc/slapt-get/slapt-srcrc.new etc/slapt-get/slapt-srcrc; diff -q etc/slapt-get/slapt-srcrc etc/slapt-get/slapt-srcrc.new >/dev/null 2>&1 && rm etc/slapt-get/slapt-srcrc.new; fi;" > $(PKGDIR)/install/doinst.sh
	$(mkinstalldirs) $(PKGDIR)/etc/slapt-get
	install --mode=0644 slapt-srcrc $(PKGDIR)/etc/slapt-get/slapt-srcrc.new
	$(mkinstalldirs) $(PKGDIR)/usr/man/man8
	install slapt-src.8 $(PKGDIR)/usr/man/man8/
	gzip -f $(PKGDIR)/usr/man/man8/slapt-src.8
	-chown -R $$(stat --format "%u:%g" $(bindir)) $(PKGDIR)/$(bindir)
	-find $(PKGDIR)/$(bindir) -type f -exec strip --strip-unneeded {} \;
	( cd $(PKGDIR) && /sbin/makepkg -l y -c n ../@PACKAGE@-@PACKAGE_VERSION@-@host_cpu@-1.tgz )

