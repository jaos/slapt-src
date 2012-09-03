SUBDIRS = src po

SLACK_FILES = slack-desc slack-required slapt-srcrc
EXTRA_DIST = $(SLACK_FILES) slapt-src.8 slapt-src.ru.8

PKGDIR=`pwd`/pkg

clean-local: clean-pkg
.PHONY: clean-pkg
clean-pkg:
	-@rm -rf pkg
	-@rm -rf @PACKAGE@-@PACKAGE_VERSION@-@host_cpu@-1.tgz

install-data-local:
	$(mkinstalldirs) $(DESTDIR)/etc/slapt-get
	install --mode=0644 slapt-srcrc $(DESTDIR)/etc/slapt-get/slapt-srcrc.new
	if [ "$(DESTDIR)" != "$(PKGDIR)" ]; then if [ ! -f $(DESTDIR)/etc/slapt-get/slapt-srcrc ]; then mv $(DESTDIR)/etc/slapt-get/slapt-srcrc.new $(DESTDIR)/etc/slapt-get/slapt-srcrc; fi; fi
	$(mkinstalldirs) $(DESTDIR)$(mandir)/man8
	install slapt-src.8 $(DESTDIR)$(mandir)/man8/
	gzip -f $(DESTDIR)$(mandir)/man8/slapt-src.8
	$(mkinstalldirs) $(DESTDIR)$(mandir)/ru/man8
	install slapt-src.ru.8 $(DESTDIR)$(mandir)/ru/man8/slapt-src.8
	gzip -f $(DESTDIR)$(mandir)/ru/man8/slapt-src.8
	strip --strip-unneeded $(DESTDIR)/$(bindir)/@PACKAGE@
	$(mkinstalldirs) $(DESTDIR)/usr/src/slapt-src
	-chown -R root:users $(DESTDIR)/usr/src/slapt-src
	-chmod g+s $(DESTDIR)/usr/src/slapt-src
	$(mkinstalldirs) $(DESTDIR)/$(datadir)/doc/@PACKAGE@
	install COPYING ChangeLog README TODO $(DESTDIR)/$(datadir)/doc/@PACKAGE@/
	-chown $$(stat --format "%u:%g" $(bindir)) $(DESTDIR)/$(bindir)
	-chown $$(stat --format "%u:%g" $(bindir)) $(DESTDIR)/$(bindir)/@PACKAGE@

pkg: all
	$(MAKE) install DESTDIR=$(PKGDIR)
	$(mkinstalldirs) $(PKGDIR)/install
	install slack-desc $(PKGDIR)/install/
	install slack-required $(PKGDIR)/install/
	@echo "if [ ! -d etc/slapt-get ]; then mkdir -p etc/slapt-get; fi; if [ ! -f etc/slapt-get/slapt-srcrc ]; then mv -f etc/slapt-get/slapt-srcrc.new etc/slapt-get/slapt-srcrc; diff -q etc/slapt-get/slapt-srcrc etc/slapt-get/slapt-srcrc.new >/dev/null 2>&1 && rm etc/slapt-get/slapt-srcrc.new; fi;" > $(PKGDIR)/install/doinst.sh
	( cd $(PKGDIR) && /sbin/makepkg -l y -c n ../@PACKAGE@-@PACKAGE_VERSION@-@host_cpu@-1.tgz )

