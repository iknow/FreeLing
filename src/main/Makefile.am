EXTRA_DIST = LKB sample_analyzer sample_server_analyzer simple_examples README
INCLUDES = -I$(top_srcdir)/src/include
bin_PROGRAMS = analyzer analyzer_server analyzer_client

analyzer_SOURCES = sample_analyzer/analyzer.cc sample_analyzer/config.h
analyzer_server_SOURCES = sample_analyzer/analyzer_server.cc sample_analyzer/config.h
analyzer_client_SOURCES = sample_analyzer/analyzer_client.cc

analyzer_LDADD = -lmorfo -lfries -lomlet -lcfg+ -lpcre -ldb_cxx -lboost_filesystem
analyzer_LDFLAGS = -L$(top_srcdir)/src/libmorfo

analyzer_server_LDADD = -lmorfo -lfries -lomlet -lcfg+ -lpcre -ldb_cxx -lboost_filesystem
analyzer_server_LDFLAGS = -L$(top_srcdir)/src/libmorfo

analyzer_client_LDADD = -lpthread

install-exec-hook:
	cp sample_analyzer/analyze sample_analyzer/analyze.utf sample_analyzer/initialize $(bindir)
	chmod +x $(bindir)/analyze $(bindir)/analyze.utf
