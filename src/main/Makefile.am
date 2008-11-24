EXTRA_DIST = config.h sample.cc LKB analyze.utf analyze.server analyze initialize README
INCLUDES = -I$(top_srcdir)/src/include -I$(top_srcdir)/libfries/src/include -I$(top_srcdir)/libomlet/src/include
bin_PROGRAMS = analyzer

analyzer_SOURCES = analyzer.cc

if ENABLE_MALT
  LJVM = -ljvm
else
  LJVM =
endif
analyzer_LDADD = -lmorfo -lfries -lomlet -lcfg+ -lpcre -ldb_cxx $(LJVM)
analyzer_LDFLAGS = -L$(top_srcdir)/src/libmorfo -L$(top_srcdir)/libomlet/src/libomlet -L$(top_srcdir)/libfries/src/libfries

install-exec-hook:
	cp analyze analyze.utf analyze.server initialize $(bindir)
	chmod +x $(bindir)/analyze $(bindir)/analyze.utf $(bindir)/analyze.server
