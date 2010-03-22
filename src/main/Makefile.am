EXTRA_DIST = LKB sample_analyzer simple_examples README
INCLUDES = -I$(top_srcdir)/src/include -I$(top_srcdir)/libcfg+
bin_PROGRAMS = analyzer analyzer_server_npipe analyzer_server_socket analyzer_client_npipe analyzer_client_socket analyzernou analyzernou_server

if BOOST_MT
  MT="-mt"
endif

if BOOST_GCC
 MT="-gcc-mt"
endif


analyzer_SOURCES = sample_analyzer/analyzer.cc sample_analyzer/analyzer_common.cc sample_analyzer/config.h sample_analyzer/analyzer_common.h
analyzer_server_npipe_SOURCES = sample_analyzer/analyzer_server_npipe.cc sample_analyzer/config.h
analyzer_client_npipe_SOURCES = sample_analyzer/analyzer_client_npipe.cc


analyzernou_SOURCES = sample_analyzer/analyzer_nou.cc sample_analyzer/config.h 
analyzernou_server_SOURCES = sample_analyzer/analyzer_nou.cc sample_analyzer/config.h sample_analyzer/socket.h
analyzernou_LDADD = -lmorfo -lfries -lomlet -lcfg+ -lpcre -ldb_cxx -lboost_filesystem$(MT)
analyzernou_LDFLAGS = -L$(top_srcdir)/src/libmorfo -L$(top_srcdir)/libcfg+
analyzernou_server_LDADD = -lmorfo -lfries -lomlet -lcfg+ -lpcre -ldb_cxx -lboost_filesystem$(MT)
analyzernou_server_LDFLAGS = -L$(top_srcdir)/src/libmorfo -L$(top_srcdir)/libcfg+
analyzernou_server_CPPFLAGS = -DSERVER

analyzer_server_socket_SOURCES = sample_analyzer/analyzer_server_socket.cc sample_analyzer/analyzer_common.cc sample_analyzer/config.h sample_analyzer/analyzer_common.h sample_analyzer/socket.h
analyzer_client_socket_SOURCES = sample_analyzer/analyzer_client_socket.cc sample_analyzer/socket.h

analyzer_LDADD = -lmorfo -lfries -lomlet -lcfg+ -lpcre -ldb_cxx -lboost_filesystem$(MT)
analyzer_LDFLAGS = -L$(top_srcdir)/src/libmorfo -L$(top_srcdir)/libcfg+

analyzer_server_npipe_LDADD = -lmorfo -lfries -lomlet -lcfg+ -lpcre -ldb_cxx -lboost_filesystem$(MT)
analyzer_server_npipe_LDFLAGS = -L$(top_srcdir)/src/libmorfo -L$(top_srcdir)/libcfg+

analyzer_server_socket_LDADD = -lmorfo -lfries -lomlet -lcfg+ -lpcre -ldb_cxx -lboost_filesystem$(MT)
analyzer_server_socket_LDFLAGS = -L$(top_srcdir)/src/libmorfo -L$(top_srcdir)/libcfg+

analyzer_client_npipe_LDADD = -lpthread


install-exec-hook:
	cp sample_analyzer/analyze sample_analyzer/analyze.utf sample_analyzer/initialize $(bindir)
	chmod +x $(bindir)/analyze $(bindir)/analyze.utf
