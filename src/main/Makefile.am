EXTRA_DIST = LKB sample_analyzer simple_examples README
INCLUDES = -I$(top_srcdir)/src/include -I$(top_srcdir)/libcfg+
bin_PROGRAMS = analyzer analyzer_server_npipe analyzer_server_socket analyzer_serverSTD analyzer_client_npipe analyzer_client_socket analyzer_clientUTF

if BOOST_GCC
 B1="-gcc"
else 
 B1=""
endif

if BOOST_MT
 MT=$(B1)"-mt"
else
 MT=$(B1)
endif

analyzer_SOURCES = sample_analyzer/analyzer.cc sample_analyzer/config.h
analyzer_server_npipe_SOURCES = sample_analyzer/analyzer_server_npipe.cc sample_analyzer/config.h
analyzer_client_npipe_SOURCES = sample_analyzer/analyzer_client_npipe.cc
analyzer_serverSTD_SOURCES = sample_analyzer/analyzer_serverSTD.cc sample_analyzer/config.h
analyzer_clientUTF_SOURCES = sample_analyzer/analyzer_clientUTF.cc

analyzer_server_socket_SOURCES = sample_analyzer/analyzer_server_socket.cc sample_analyzer/config.h  sample_analyzer/socket.h
analyzer_client_socket_SOURCES = sample_analyzer/analyzer_client_socket.cc sample_analyzer/socket.h

analyzer_LDADD = -lmorfo -lfries -lomlet -lcfg+ -lpcre -ldb_cxx -lboost_filesystem$(MT)
analyzer_LDFLAGS = -L$(top_srcdir)/src/libmorfo -L$(top_srcdir)/libcfg+

analyzer_server_npipe_LDADD = -lmorfo -lfries -lomlet -lcfg+ -lpcre -ldb_cxx -lboost_filesystem$(MT)
analyzer_server_npipe_LDFLAGS = -L$(top_srcdir)/src/libmorfo -L$(top_srcdir)/libcfg+

analyzer_server_socket_LDADD = -lmorfo -lfries -lomlet -lcfg+ -lpcre -ldb_cxx -lboost_filesystem$(MT)
analyzer_server_socket_LDFLAGS = -L$(top_srcdir)/src/libmorfo -L$(top_srcdir)/libcfg+

analyzer_serverSTD_LDADD = -lmorfo -lfries -lomlet -lcfg+ -lpcre -ldb_cxx -lboost_filesystem$(MT)
analyzer_serverSTD_LDFLAGS = -L$(top_srcdir)/src/libmorfo -L$(top_srcdir)/libcfg+

analyzer_client_npipe_LDADD = -lpthread
analyzer_clientUTF_LDADD = -lpthread

install-exec-hook:
	cp sample_analyzer/analyze sample_analyzer/analyze.utf sample_analyzer/initialize $(bindir)
	chmod +x $(bindir)/analyze $(bindir)/analyze.utf
