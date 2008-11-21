languages = ca es en it gl

if OS_SOLARIS
ZCAT=gzcat
else
ZCAT=zcat
endif

pkgdata_DATA =

endatadir = $(pkgdatadir)/en
endata_DATA = en/*
ennecdatadir = $(pkgdatadir)/en/nec
ennecdata_DATA = en/nec/*
enchunkdir = $(pkgdatadir)/en/chunker
enchunk_DATA = en/chunker/*
endepdatadir = $(pkgdatadir)/en/dep
endepdata_DATA = en/dep/*

esdatadir = $(pkgdatadir)/es
esdata_DATA = es/*
esnecdatadir = $(pkgdatadir)/es/nec
esnecdata_DATA = es/nec/*
esdepdatadir = $(pkgdatadir)/es/dep
esdepdata_DATA = es/dep/*

cadatadir = $(pkgdatadir)/ca
cadata_DATA = ca/*
canecdatadir = $(pkgdatadir)/ca/nec
canecdata_DATA = ca/nec/*
cadepdatadir = $(pkgdatadir)/ca/dep
cadepdata_DATA = ca/dep/*

gldatadir = $(pkgdatadir)/gl
gldata_DATA = gl/*
glnecdatadir = $(pkgdatadir)/gl/nec
glnecdata_DATA = gl/nec/*
gldepdatadir = $(pkgdatadir)/gl/dep
gldepdata_DATA = gl/dep/*

itdatadir = $(pkgdatadir)/it
itdata_DATA = it/*
itnecdatadir = $(pkgdatadir)/it/nec
itnecdata_DATA = it/nec/*

commondatadir = $(pkgdatadir)/common
commondata_DATA = common/*
commonnecdatadir = $(pkgdatadir)/common/nec
commonnecdata_DATA = common/nec/*

configdatadir = $(pkgdatadir)/config
configdata_DATA = config/*

install-data-hook:
	@echo "Indexing language dictionaries. This will take some minutes."
	@for lg in $(languages); do \
	   echo "...Indexing "$$lg" dictionary, please wait."; \
	   rm -f $(pkgdatadir)/$$lg/maco.db; \
	   $(ZCAT) -f $(pkgdatadir)/$$lg/dicc.src | $(bindir)/indexdict $(pkgdatadir)/$$lg/maco.db; \
	   chmod +r $(pkgdatadir)/$$lg/maco.db; \
	   echo "...Indexing "$$lg" sense dictionary, please wait."; \
	   for v in $(pkgdatadir)/$$lg/senses*.src.gz $(pkgdatadir)/$$lg/senses*.src; do \
             src=`echo $$v | sed 's/.gz//' `; \
             name=`echo $$v | sed 's/.gz//' | sed 's/.src//' `; \
             rm -f $$name.db; \
	     $(ZCAT) -f $$src | $(bindir)/indexdict $$name.db; \
	     chmod +r $$name.db; \
	   done \
	done
	@echo "Indexing WN-based semantic database"
	@rm -f $(pkgdatadir)/common/wn16.db;
	@$(ZCAT) -f $(pkgdatadir)/common/wn16.src | $(bindir)/indexdict $(pkgdatadir)/common/wn16.db;
	@chmod +r $(pkgdatadir)/common/wn16.db;
