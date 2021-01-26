SUBDIRS = src po

SLACK_FILES = slack-desc slack-required slack-suggests slapt-srcrc
EXTRA_DIST = $(SLACK_FILES) slapt-src.8 slapt-src.ru.8

PKGDIR=`pwd`/pkg
PKGREL = 1

clean-local: clean-pkg
.PHONY: clean-pkg
clean-pkg:
	-@rm -rf pkg
	-@rm -rf @PACKAGE@-@PACKAGE_VERSION@-@host_cpu@-1.txz

install-data-local: install-exec
	$(mkinstalldirs) $(DESTDIR)$(sysconfdir)/slapt-get
	$(INSTALL_DATA) $(top_srcdir)/slapt-srcrc $(DESTDIR)$(sysconfdir)/slapt-get/slapt-srcrc.new
	if [ "$(DESTDIR)" != "$(PKGDIR)" ]; then if [ ! -f $(DESTDIR)$(sysconfdir)/slapt-get/slapt-srcrc ]; then mv $(DESTDIR)$(sysconfdir)/slapt-get/slapt-srcrc.new $(DESTDIR)$(sysconfdir)/slapt-get/slapt-srcrc; fi; fi
	$(mkinstalldirs) $(DESTDIR)$(mandir)/man8
	$(INSTALL_DATA) $(top_srcdir)/slapt-src.8 $(DESTDIR)$(mandir)/man8/
	gzip -f $(DESTDIR)$(mandir)/man8/slapt-src.8
	$(mkinstalldirs) $(DESTDIR)$(mandir)/ru/man8
	$(INSTALL_DATA) $(top_srcdir)/slapt-src.ru.8 $(DESTDIR)$(mandir)/ru/man8/slapt-src.8
	gzip -f $(DESTDIR)$(mandir)/ru/man8/slapt-src.8
	strip --strip-unneeded $(DESTDIR)/$(bindir)/@PACKAGE@
	$(mkinstalldirs) $(DESTDIR)$(exec_prefix)/src/slapt-src
	-chown -R root:users $(DESTDIR)$(exec_prefix)/src/slapt-src
	-chmod g+s $(DESTDIR)$(exec_prefix)/src/slapt-src
	$(mkinstalldirs) $(DESTDIR)/$(datadir)/doc/@PACKAGE@
	$(INSTALL_DATA) $(top_srcdir)/COPYING $(top_srcdir)/ChangeLog $(top_srcdir)/README $(top_srcdir)/TODO $(DESTDIR)/$(datadir)/doc/@PACKAGE@/
	-chown $$(stat --format "%u:%g" $(bindir)) $(DESTDIR)/$(bindir)
	-chown $$(stat --format "%u:%g" $(bindir)) $(DESTDIR)/$(bindir)/@PACKAGE@

uninstall-local:
	-rm -rf $(DESTDIR)/$(datadir)/doc/@PACKAGE@/
	-rm -f $(DESTDIR)$(mandir)/ru/man8/slapt-src.8*
	-rm -f $(DESTDIR)$(mandir)/man8/slapt-src.8*
	-rm -f $(DESTDIR)$(sysconfdir)/slapt-get/slapt-srcrc*

pkg: all
	$(MAKE) install DESTDIR=$(PKGDIR)
	$(mkinstalldirs) $(PKGDIR)/install
	install slack-desc $(PKGDIR)/install/
	install slack-required $(PKGDIR)/install/
	install slack-suggests $(PKGDIR)/install/
	@echo "if [ ! -d etc/slapt-get ]; then mkdir -p etc/slapt-get; fi; if [ ! -f etc/slapt-get/slapt-srcrc ]; then mv -f etc/slapt-get/slapt-srcrc.new etc/slapt-get/slapt-srcrc; else ! diff -q etc/slapt-get/slapt-srcrc etc/slapt-get/slapt-srcrc.new >/dev/null 2>&1 || rm etc/slapt-get/slapt-srcrc.new; fi;" > $(PKGDIR)/install/doinst.sh
	( cd $(PKGDIR) && /sbin/makepkg -l y -c n ../@PACKAGE@-@PACKAGE_VERSION@-@host_cpu@-$(PKGREL).txz )

