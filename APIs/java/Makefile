FREELINGDIR = /usr/local
SWIGDIR = /usr/share/swig1.3
JAVADIR = /usr/lib/jvm/java-6-sun
LIBSDIR = /usr/local
GCC=g++

libmorfo_java.so: libmorfo_wrap.cxx
	$(GCC) -shared -o libmorfo_java.so libmorfo_wrap.cxx -lmorfo -ldb_cxx -lpcre -lomlet -lfries  -lboost_filesystem-mt -I /usr/include/db4 -L $(FREELINGDIR)/lib  -I $(FREELINGDIR)/include -I $(LIBSDIR)/include -L $(LIBSDIR)/lib -I $(JAVADIR)/include -I $(JAVADIR)/include/linux -fPIC
	javac morfo/*.java
	jar -cf libmorfo_java.jar morfo

## There used to be problems when deleting "maco" objects from java.
## Maybe swig has fixed this... If you experience such problems,
## overwrite class morfo/maco.java with classes_aux/maco.java before
## packing the .jar

libmorfo_wrap.cxx: libmorfo_java.i
	rm -rf morfo; mkdir morfo
	swig -java -c++ -package morfo -outdir morfo -o libmorfo_wrap.cxx -I$(SWIGDIR)/java -I$(SWIGDIR)/std  -I$(SWIGDIR) -I$(SWIGDIR)/python/ -I$(FREELINGDIR)/APIs/morfo libmorfo_java.i

clean:
	rm -rf *.jar *.so *.cxx *.class morfo
