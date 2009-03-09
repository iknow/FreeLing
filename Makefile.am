EXTRA_DIST = data doc APIs LICENSES
SUBDIRS = src data doc 

dist-hook:
	rm -rf `find $(distdir)/data -name .svn`
	rm -rf `find $(distdir)/doc -name .svn`
	rm -rf `find $(distdir)/APIs -name .svn`
	rm -rf `find $(distdir)/LICENSES -name .svn`
