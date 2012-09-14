NULL =

AM_CPPFLAGS =					\
	-I$(top_srcdir)				\
	-I$(top_builddir)			\
	-DLIBEXECDIR=\"$(libexecdir)\"		\
	-DBINDIR=\"$(bindir)\"			\
	-DLIBDIR=\"$(libdir)\"			\
	$(NULL)

bin_PROGRAMS =					\
	bundler-builder				\
	$(NULL)

bin_SCRIPTS =					\
	bundler-create				\
	$(NULL)

libexec_PROGRAMS =				\
	bundler-helper				\
	bundler-trampoline			\
	$(NULL)

noinst_SCRIPTS =				\
	test-bundle				\
	$(NULL)

noinst_DATA = test.squashfs bundler.spec

bundlerdir = $(datadir)/bundler

test.squashfs:
	rm -f test.squashfs
	mksquashfs $(srcdir)/test test.squashfs

bundler_trampoline_SOURCES = trampoline.c

bundler_helper_SOURCES = helper.c

bundler_builder_SOURCES = builder.c

test-bundle: bundler-trampoline test.squashfs
	cp bundler-trampoline test-bundle
	objcopy --add-section .bundle=test.squashfs test-bundle

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(BUNDLE_PREFIX)

edit = sed \
	-e 's|@bindir[@]|$(bindir)|g' \
	-e 's|@libexecdir[@]|$(libexecdir)|g' \
	-e 's|@pkgdatadir[@]|$(pkgdatadir)|g' \
	-e 's|@prefix[@]|$(prefix)|g' \
	-e 's|@version[@]|$(PACKAGE_VERSION)|g'

bundler-create: Makefile bundler-create.in
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	mv $@.tmp $@
	chmod a+x $@

bundler.spec: Makefile bundler.spec.in
	rm -f $@ $@.tmp
	srcdir=''; \
	  test -f ./$@.in || srcdir=$(srcdir)/; \
	  $(edit) $${srcdir}$@.in >$@.tmp
	mv $@.tmp $@

if SETUID_INSTALL
install-exec-hook:
	chown root $(DESTDIR)$(libexecdir)/bundler-helper
	chmod 4755 $(DESTDIR)$(libexecdir)/bundler-helper
	chown root $(DESTDIR)$(bindir)/bundler-builder
	chmod 4755 $(DESTDIR)$(bindir)/bundler-builder
endif

ChangeLog:
	$(AM_V_GEN) if test -d "$(srcdir)/.git"; then \
	  (GIT_DIR=$(top_srcdir)/.git ./missing --run git log --stat) | fmt --split-only > $@.tmp \
	  && mv -f $@.tmp $@ \
	  || ($(RM) $@.tmp; \
	      echo Failed to generate ChangeLog, your ChangeLog may be outdated >&2; \
	      (test -f $@ || echo git-log is required to generate this file >> $@)); \
	else \
	  test -f $@ || \
	  (echo A git checkout and git-log is required to generate ChangeLog >&2 && \
	  echo A git checkout and git-log is required to generate this file >> $@); \
	fi


EXTRA_DIST= \
	bundler-create.in \
	bundler.spec.in \
	bundler.spec \
	test/start \
	$(NULL)

DISTCHECK_CONFIGURE_FLAGS=--disable-setuid-install

CLEANFILES=bundler-create test.squashfs test-bundle bundler.spec
