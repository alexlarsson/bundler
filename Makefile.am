NULL =

AM_CPPFLAGS =					\
	-I$(top_srcdir)				\
	-I$(top_builddir)			\
	-DLIBEXECDIR=\"$(libexecdir)\"		\
	-DBINDIR=\"$(bindir)\"			\
	-DLIBDIR=\"$(libdir)\"			\
	$(NULL)

libexec_PROGRAMS =				\
	bundler-helper				\
	$(NULL)

noinst_PROGRAMS =				\
	trampoline				\
	test-bundle				\
	$(NULL)

noinst_DATA = test.squashfs

test.squashfs:
	rm -f test.squashfs
	mksquashfs $(srcdir)/test test.squashfs

trampoline_SOURCES = trampoline.c

bundler_helper_SOURCES = helper.c

test-bundle: trampoline test.squashfs
	cp trampoline test-bundle
	objcopy --add-section .bundle=test.squashfs test-bundle

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(BUNDLE_PREFIX)
	$(mkinstalldirs) $(DESTDIR)$(SESSION_PREFIX)

if SETUID_INSTALL
install-exec-hook:
	chown root $(DESTDIR)$(libexecdir)/bundler-helper
	chmod 4755 $(DESTDIR)$(libexecdir)/bundler-helper
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
	test/start \
	$(NULL)

DISTCHECK_CONFIGURE_FLAGS=--disable-setuid-install

CLEANFILES=glick2.conf glick.desktop
