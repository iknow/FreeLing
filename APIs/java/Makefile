FREELINGDIR = /usr/local
SWIGDIR = /usr/share/swig1.3
JAVADIR = /usr/lib/jvm/java-6-sun
LIBSDIR = /usr/local
GCC=g++-4.2

libmorfo_java.so: libmorfo_wrap.cxx
	$(GCC) -shared -o libmorfo_java.so libmorfo_wrap.cxx -lmorfo -ldb_cxx -lpcre -lomlet -lfries  -lboost_filesystem-mt -I /usr/include/db4 -L $(FREELINGDIR)/lib  -I $(FREELINGDIR)/include -I $(LIBSDIR)/include -L $(LIBSDIR)/lib -I $(JAVADIR)/include -I $(JAVADIR)/include/linux
	cp classes_aux/* morfo/
	javac morfo/*.java
	jar -cf libmorfo_java.jar morfo

libmorfo_wrap.cxx: libmorfo_java.i
	rm -rf morfo; mkdir morfo
	swig -java -c++ -package morfo -outdir morfo -o libmorfo_wrap.cxx -I$(SWIGDIR)/java -I$(SWIGDIR)/std  -I$(SWIGDIR) -I$(SWIGDIR)/python/ -I$(FREELINGDIR)/APIs/morfo libmorfo_java.i

