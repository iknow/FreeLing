
FREELINGDIR = /usr/local
LIBSDIR = /usr/local
PYTHONDIR = /usr/include/python2.6

_libmorfo_python.so: libmorfo_wrap.cxx
	g++ -shared -o _libmorfo_python.so libmorfo_wrap.cxx -lmorfo -ldb_cxx -lpcre -lomlet -lfries -lboost_filesystem -I $(FREELINGDIR)/include -L $(FREELINGDIR)/lib  -I $(LIBSDIR)/include -L $(LIBSDIR)/lib -I $(PYTHONDIR)

libmorfo_wrap.cxx: libmorfo_python.i
	swig -python -c++ -o libmorfo_wrap.cxx libmorfo_python.i

