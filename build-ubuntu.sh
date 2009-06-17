debuild -S -sa -i\.git/ -I.git && {
	echo "haven't you forgotten to add the current version to debian/changelog?"
	echo
	echo "now you can run:"
	echo "dput qtheorafrontend ../qtheorafrontend_VERSION_source.changes"
}
