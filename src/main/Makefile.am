EXTRA_DIST = LKB sample_analyzer sample_server_analyzer simple_examples README
INCLUDES = -I$(top_srcdir)/src/include
bin_PROGRAMS = analyzer

analyzer_SOURCES = sample_analyzer/analyzer.cc sample_analyzer/config.h

if ENABLE_MALT
  LJVM = -ljvm
else
  LJVM =
endif
analyzer_LDADD = -lmorfo -lfries -lomlet -lcfg+ -lpcre -ldb_cxx $(LJVM)
analyzer_LDFLAGS = -L$(top_srcdir)/src/libmorfo

install-exec-hook:
	cp sample_analyzer/analyze sample_analyzer/analyze.utf sample_analyzer/initialize sample_server_analyzer/analyze.server $(bindir)
	chmod +x $(bindir)/analyze $(bindir)/analyze.utf $(bindir)/analyze.server
