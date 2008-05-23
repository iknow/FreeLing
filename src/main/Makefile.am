EXTRA_DIST = config.h sample.cc LKB analyze.utf analyze.server analyze README
INCLUDES = -I$(top_srcdir)/src/include
bin_PROGRAMS = analyzer

analyzer_SOURCES = analyzer.cc
analyzer_LDADD = -lmorfo -lomlet -lfries -lcfg+ -lpcre -ldb_cxx
analyzer_LDFLAGS = -L$(top_srcdir)/src/libmorfo

install-exec-hook:
	cp analyze analyze.utf analyze.server $(bindir)
	chmod +x $(bindir)/analyze $(bindir)/analyze.utf $(bindir)/analyze.server
