EXTRA_DIST = hmm_smooth.perl train-relax.perl make-probs-file.perl TRAIN unk-tags constr_gram.manual nec/README nec/TRAIN.sh nec/lexicon.cc nec/train.cc
bin_PROGRAMS = indexdict convertdict createIndex compile_ukb

indexdict_SOURCES = indexdict.cc
indexdict_LDADD = -ldb_cxx

convertdict_SOURCES = convertdict.cc
convertdict_LDADD = -lfries -lpcre

createIndex_SOURCES = corrector/createIndex.cc $(top_srcdir)/src/libmorfo/corrector/phonetics.cc $(top_srcdir)/src/libmorfo/corrector/soundChange.cc
createIndex_LDADD = -lfries -lpcre
createIndex_CXXFLAGS = -I$(top_srcdir)/src/include/

compile_ukb_SOURCES = compile_ukb.cc $(top_srcdir)/src/libmorfo/disambiguator/ukb/globalVars.cc $(top_srcdir)/src/libmorfo/disambiguator/ukb/kbGraph.cc $(top_srcdir)/src/libmorfo/disambiguator/ukb/wdict.cc $(top_srcdir)/src/libmorfo/disambiguator/ukb/common.cc 
compile_ukb_CXXFLAGS = -I$(top_srcdir)/src/libmorfo/disambiguator/ukb
compile_ukb_LDADD = -lboost_filesystem

