EXTRA_DIST = hmm_smooth.perl train-relax.perl make-probs-file.perl TRAIN unk-tags unk-tags.parole constr_gram.manual nec/README nec/TRAIN.sh nec/lexicon.cc nec/train.cc ner/README ner/TRAIN.sh ner/lexicon.cc ner/train.cc

if USE_LIBDB
  bin_PROGRAMS = indexdict convertdict dicc2phon compile_kb
else 
  bin_PROGRAMS = convertdict dicc2phon compile_kb
endif 

if BOOST_MT
  MT="-mt"
endif

if BOOST_GCC
 MT="-gcc-mt"
endif

if USE_LIBDB
 indexdict_SOURCES = indexdict.cc
 indexdict_LDADD = -ldb_cxx
endif

convertdict_SOURCES = convertdict.cc
convertdict_LDADD = -lfries -lpcre

dicc2phon_SOURCES = corrector/dicc2phon.cc $(top_srcdir)/src/libmorfo/corrector/phonetics.cc $(top_srcdir)/src/libmorfo/corrector/soundChange.cc $(top_srcdir)/src/libmorfo/traces.cc
dicc2phon_LDADD = -lfries -lpcre
dicc2phon_CXXFLAGS = -I$(top_srcdir)/src/include/

compile_kb_SOURCES = compile_kb.cc $(top_srcdir)/src/libmorfo/disambiguator/ukb/globalVars.cc $(top_srcdir)/src/libmorfo/disambiguator/ukb/kbGraph.cc $(top_srcdir)/src/libmorfo/disambiguator/ukb/wdict.cc $(top_srcdir)/src/libmorfo/disambiguator/ukb/common.cc 
compile_kb_CXXFLAGS = -I$(top_srcdir)/src/libmorfo/disambiguator/ukb
compile_kb_LDADD = -lboost_filesystem$(MT) -lboost_program_options$(MT)

