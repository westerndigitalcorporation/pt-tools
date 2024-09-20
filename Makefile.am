# SPDX-License-Identifier: CC0-1.0
#
# Copyright (C) 2024 Western Digital Corporation or its affiliates.

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src man

EXTRA_DIST = autogen.sh README.md COPYING.GPL CONTRIBUTING

if BUILD_RPM

rpmdir = $(abs_top_builddir)/rpmbuild

EXTRA_DIST += ptio.spec
RPMARCH=`$(RPM) --eval %_target_cpu`

rpm: dist
	@echo "Building RPM package..."
	@mkdir -p $(rpmdir)
	$(RPMBUILD) -ta --clean \
		-D "_topdir $(rpmdir)" \
		-D "pkg_name ptio" \
		-D "pkg_version $(PACKAGE_VERSION)" \
		ptio-$(PACKAGE_VERSION).tar.gz
	@mv -f $(rpmdir)/RPMS/$(RPMARCH)/*.rpm $(abs_top_builddir)
	@mv -f $(rpmdir)/RPMS/noarch/*.rpm $(abs_top_builddir)
	@mv -f $(rpmdir)/SRPMS/*.rpm $(abs_top_builddir)
	@rm -rf $(rpmdir)
	@rm -f ptio-$(PACKAGE_VERSION).tar.gz
else
rpm:
	@echo "Building RPM packages requires rpmbuild and rpm utilities"
	exit 1
endif
