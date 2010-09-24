SUBDIRS = m4m po libjpeg exif test

man_MANS = exif.1

# newer versions of automake/autoconf require adding exif.1 here
EXTRA_DIST = exif.1 @PACKAGE_TARNAME@.spec

ACLOCAL_AMFLAGS = -I auto-m4 -I m4m

#######################################################################
# Help for the maintainer
#

# Simulate something like
#   EXTRA_DIST_IF_EXIST = ChangeLog.cvs
# If present, ship ChangeLog.cvs in source tarball.
# If not present, don't ship it.
dist-hook:
	if test -f $(srcdir)/ChangeLog.cvs; then \
		cp -p $(srcdir)/ChangeLog.cvs $(distdir)/ChangeLog.cvs; \
	fi

.PHONY: cvs-changelog
cvs-changelog: $(srcdir)/ChangeLog.cvs

.PHONY: $(srcdir)/ChangeLog.cvs
$(srcdir)/ChangeLog.cvs:
	if test -f "$(srcdir)/cvs2cl.usermap"; then \
		usermap="--usermap ./cvs2cl.usermap"; \
	else \
		usermap=""; \
	fi; \
	(cd "$(srcdir)" && cvs2cl $${usermap} --utc --domain users.sourceforge.net -f "ChangeLog.cvs")

.PHONY: cvs-tag-release
cvs-tag-release:
	@tag="$$(echo "$(PACKAGE_TARNAME)-$(PACKAGE_VERSION)-release" | sed 's|\.|_|g')"; \
	echo "Are you sure you want to CVS tag \`$${tag}' your source files?"; \
	echo "Press Ctrl-C to abort, Enter to continue."; \
	read; \
	cd "$(srcdir)" && cvs tag "$${tag}"

.PHONY: upload-release
upload-release:
	echo ncftpput upload.sourceforge.net incoming/ $(PACKAGE_TARNAME)-$(PACKAGE_VERSION).*

# Create an HTML version of the man page for the web site
.1.html:
	man2html -r -H libexif.sourceforge.net -M / "$^" | sed '1,/^$$/d' >"$@"

# End of Makefile.am.
