SUBDIRS = m4m po libjpeg exif test

man_MANS = exif.1

# newer versions of automake/autoconf require adding exif.1 here
EXTRA_DIST = exif.1 @PACKAGE_TARNAME@.spec

ACLOCAL_AMFLAGS = -I auto-m4 -I m4m

#######################################################################
# Help for the maintainer
#

# Simulate something like
#   EXTRA_DIST_IF_EXIST = ChangeLog.git
# If present, ship ChangeLog.git in source tarball.
# If not present, don't ship it.
dist-hook:
	if test -f $(srcdir)/ChangeLog.git; then \
		cp -p $(srcdir)/ChangeLog.git $(distdir)/ChangeLog.git; \
	fi

.PHONY: git-changelog
git-changelog: $(srcdir)/ChangeLog.git

.PHONY: $(srcdir)/ChangeLog.git
$(srcdir)/ChangeLog.git:
	(cd "$(srcdir)" && git2cl > $@)

.PHONY: git-tag-release
git-tag-release:
	@tag="$$(echo "$(PACKAGE_TARNAME)-$(PACKAGE_VERSION)-release" | sed 's|\.|_|g')"; \
	echo "Are you sure you want to git tag \`$${tag}' your source files?"; \
	echo "Press Ctrl-C to abort, Enter to continue."; \
	read; \
	cd "$(srcdir)" && git tag "$${tag}"

# Create an HTML version of the man page for the web site
.1.html:
	man2html -r -H libexif.github.io -M / "$^" | sed '1,/^$$/d' >"$@"

# End of Makefile.am.
