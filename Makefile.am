# SPDX-License-Identifier: CC0-1.0
#
# SPDX-FileCopyrightText: 2024 Western Digital Corporation or its affiliates.

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = lib tools

EXTRA_DIST = autogen.sh \
             README.md \
             LICENSES/GPL-2.0-or-later.txt

if BUILD_RPM

rpmdir = $(abs_top_builddir)/rpmbuild

EXTRA_DIST += pt-tools.spec
RPMARCH=`$(RPM) --eval %_target_cpu`

rpm: dist
	@echo "Building RPM package..."
	@mkdir -p $(rpmdir)
	$(RPMBUILD) -ta --clean \
		-D "_topdir $(rpmdir)" \
		-D "pkg_name pt-tools" \
		-D "pkg_version $(PACKAGE_VERSION)" \
		pt-tools-$(PACKAGE_VERSION).tar.gz
	@mv -f $(rpmdir)/RPMS/$(RPMARCH)/*.rpm $(abs_top_builddir)
	@mv -f $(rpmdir)/SRPMS/*.rpm $(abs_top_builddir)
	@rm -rf $(rpmdir)
	@rm -f pt-tools-$(PACKAGE_VERSION).tar.gz
else
rpm:
	@echo "Building RPM packages requires rpmbuild and rpm utilities"
	exit 1
endif

CLEANFILES = *.rpm *.tar.gz
DISTCLEANFILES = *.rpm *.tar.gz configure
