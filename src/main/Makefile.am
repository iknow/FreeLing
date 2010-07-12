EXTRA_DIST = LKB sample_analyzer simple_examples README
INCLUDES = -I$(top_srcdir)/src/include -I$(top_srcdir)/libcfg+

bin_PROGRAMS = analyzer analyzer_server analyzer_client

if BOOST_MT
  MT="-mt"
endif

if BOOST_GCC
 MT="-gcc-mt"
endif

if USE_LIBDB
 LIBDB= -ldb_cxx
endif

analyzer_SOURCES = sample_analyzer/analyzer.cc sample_analyzer/config.h

analyzer_LDADD = -lmorfo -lfries -lomlet -lcfg+ -lpcre $(LIBDB) -lboost_filesystem$(MT)
analyzer_LDFLAGS = -L$(top_srcdir)/src/libmorfo -L$(top_srcdir)/libcfg+

analyzer_server_SOURCES = sample_analyzer/analyzer.cc sample_analyzer/config.h sample_analyzer/socket.h
analyzer_server_LDADD = -lmorfo -lfries -lomlet -lcfg+ -lpcre $(LIBDB) -lboost_filesystem$(MT)
analyzer_server_LDFLAGS = -L$(top_srcdir)/src/libmorfo -L$(top_srcdir)/libcfg+
analyzer_server_CPPFLAGS = -DSERVER

analyzer_client_SOURCES = sample_analyzer/analyzer_client.cc sample_analyzer/socket.h

install-exec-hook:
	cp sample_analyzer/analyze sample_analyzer/initialize $(bindir)
	chmod +x $(bindir)/analyze

uninstall-hook:
	@rm -rf $(bindir)/analyze $(bindir)/initialize
