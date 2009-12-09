languages = as ca cy en es gl it pt

ZCAT=gunzip -c

pkgdata_DATA =

esdatadir = $(pkgdatadir)/es
esdata_DATA = es/*.*
esnecdatadir = $(pkgdatadir)/es/nec
esnecdata_DATA = es/nec/*
esnerdatadir = $(pkgdatadir)/es/ner
esnerdata_DATA = es/ner/*
esdepdatadir = $(pkgdatadir)/es/dep
esdepdata_DATA = es/dep/*
escorefdatadir = $(pkgdatadir)/es/coref
escorefdata_DATA = es/coref/*
escorrecdatadir = $(pkgdatadir)/es/corrector
escorrecdata_DATA = es/corrector/*


cadatadir = $(pkgdatadir)/ca
cadata_DATA = ca/*.*
canecdatadir = $(pkgdatadir)/ca/nec
canecdata_DATA = ca/nec/*
cadepdatadir = $(pkgdatadir)/ca/dep
cadepdata_DATA = ca/dep/*

gldatadir = $(pkgdatadir)/gl
gldata_DATA = gl/*.*
glnecdatadir = $(pkgdatadir)/gl/nec
glnecdata_DATA = gl/nec/*
gldepdatadir = $(pkgdatadir)/gl/dep
gldepdata_DATA = gl/dep/*

asdatadir = $(pkgdatadir)/as
asdata_DATA = as/*.*
asnecdatadir = $(pkgdatadir)/as/nec
asnecdata_DATA = as/nec/*
asdepdatadir = $(pkgdatadir)/as/dep
asdepdata_DATA = as/dep/*

itdatadir = $(pkgdatadir)/it
itdata_DATA = it/*.*
itnecdatadir = $(pkgdatadir)/it/nec
itnecdata_DATA = it/nec/*

ptdatadir = $(pkgdatadir)/pt
ptdata_DATA = pt/*.*
ptnecdatadir = $(pkgdatadir)/pt/nec
ptnecdata_DATA = pt/nec/*

endatadir = $(pkgdatadir)/en
endata_DATA = en/*.*
ennecdatadir = $(pkgdatadir)/en/nec
ennecdata_DATA = en/nec/*
enchunkdir = $(pkgdatadir)/en/chunker
enchunk_DATA = en/chunker/*
endepdatadir = $(pkgdatadir)/en/dep
endepdata_DATA = en/dep/*

cydatadir = $(pkgdatadir)/cy
cydata_DATA = cy/*.*
cynecdatadir = $(pkgdatadir)/cy/nec
cynecdata_DATA = cy/nec/*
cychunkdir = $(pkgdatadir)/cy/chunker
cychunk_DATA = cy/chunker/*
cydepdatadir = $(pkgdatadir)/cy/dep
cydepdata_DATA = cy/dep/*

commondatadir = $(pkgdatadir)/common
commondata_DATA = common/*.*
commonnecdatadir = $(pkgdatadir)/common/nec
commonnecdata_DATA = common/nec/*
commoncorrecdatadir = $(pkgdatadir)/common/corrector
commoncorrecdata_DATA = common/corrector/*

configdatadir = $(pkgdatadir)/config
configdata_DATA = config/*


install-data-hook:
	@echo "Indexing language dictionaries. This will take some minutes."
	@for lg in $(languages); do \
	   if [ -f "$(pkgdatadir)/$$lg/corrector/corrector.db" ]; \
	   	then rm -f $(pkgdatadir)/$$lg/corrector/corrector.db; \
	   fi; \
	   if [ -f "$(pkgdatadir)/$$lg/corrector/corrector.rules" ]; \
		then echo "...Creating sound database for "$$lg" dictionary, please wait."; \
		     $(ZCAT) -f $(pkgdatadir)/$$lg/dicc.src | $(bindir)/dicc2phon $(pkgdatadir)/$$lg/corrector/corrector.rules $(pkgdatadir)/common/corrector/corrector.soundDicFile $(pkgdatadir)/common/corrector/corrector.sampa  |$(bindir)/indexdict $(pkgdatadir)/$$lg/corrector/corrector.db; \
           fi; \
	   echo "...Indexing "$$lg" dictionary, please wait."; \
	   rm -f $(pkgdatadir)/$$lg/maco.db; \
	   $(ZCAT) -f $(pkgdatadir)/$$lg/dicc.src | $(bindir)/indexdict $(pkgdatadir)/$$lg/maco.db; \
	   echo "...Indexing "$$lg" sense dictionary, please wait."; \
	   for src in $(pkgdatadir)/$$lg/senses*.src; do \
             name=`echo $$src | sed 's/.src//' `; \
             rm -f $$name.db; \
	     $(ZCAT) -f $$src | $(bindir)/indexdict $$name.db; \
	     $(ZCAT) -f $$src | $(bindir)/convertdict > $$name.ukb; \
	   done \
	done 
	@echo "Indexing WN-based semantic database"
	@rm -f $(pkgdatadir)/common/wn16.db;
	@$(ZCAT) -f $(pkgdatadir)/common/wn16.src | $(bindir)/indexdict $(pkgdatadir)/common/wn16.db;
	@$(bindir)/compile_ukb $(pkgdatadir)/common/wn16-ukb.src $(pkgdatadir)/common/wn16-ukb.bin;

uninstall-hook:
	@rm -rf $(pkgdatadir)















