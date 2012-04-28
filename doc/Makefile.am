EXTRA_DIST = postrr.txt
CLEANFILES = postrr.7

man_MANS = postrr.7

postrr.7: postrr.txt

.txt.7:
	@(cd ..; ./version-gen.sh > /dev/null)
	@A2X@ -d manpage -f manpage \
		-apackage_version=$(PACKAGE_VERSION) \
		-abuild_date="$$( date --utc '+%F' )" \
		$<

