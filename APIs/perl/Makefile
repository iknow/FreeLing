
FREELINGDIR = /usr/local/
LIBSDIR = /usr/local/
PERLDIR = /usr/lib/perl/5.10

libmorfo_perl.so: libmorfo_wrap.cxx
	g++ -shared -o libmorfo_perl.so libmorfo_wrap.cxx -lmorfo -ldb_cxx -lpcre -lfries -lomlet -lboost_filesystem -I $(FREELINGDIR)/include -L $(FREELINGDIR)/lib  -I $(LIBSDIR)/include -L $(LIBSDIR)/lib -I $(PERLDIR)/CORE -fPIC

libmorfo_wrap.cxx: libmorfo_perl.i
	swig -perl5 -c++ -o libmorfo_wrap.cxx libmorfo_perl.i

