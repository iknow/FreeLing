
FREELINGDIR = /home/padro/test
LIBSDIR = /usr/local/nfssoft/LIBS
PERLDIR = /usr/lib/perl5/5.8.7/i386-linux

libmorfo_perl.so: libmorfo_wrap.cxx
	g++ -shared -o libmorfo_perl.so libmorfo_wrap.cxx -lmorfo -ldb_cxx -lpcre -I $(FREELINGDIR)/include -L $(FREELINGDIR)/lib  -I $(LIBSDIR)/include -L $(LIBSDIR)/lib -I $(PERLDIR)/CORE

libmorfo_wrap.cxx: libmorfo_perl.i
	swig -perl5 -c++ -o libmorfo_wrap.cxx libmorfo_perl.i

