EXTRA_DIST = hmm_smooth.perl train-relax.perl make-probs-file.perl TRAIN unk-tags constr_gram.manual nec/README nec/TRAIN.sh nec/lexicon.cc nec/train.cc
bin_PROGRAMS = indexdict convertdict compile_ukb

indexdict_SOURCES = indexdict.cc
indexdict_LDADD = -ldb_cxx

convertdict_SOURCES = convertdict.cc
convertdict_LDADD = -lfries -lpcre

compile_ukb_SOURCES = compile_ukb.cc $(topsrcdir)/libmorfo/disambiguator/ukb/globalVars.cc $(topsrcdir)/libmorfo/disambiguator/ukb/kbGraph.cc $(topsrcdir)/libmorfo/disambiguator/ukb/wdict.cc $(topsrcdir)/libmorfo/disambiguator/ukb/common.cc
compile_ukb_LDADD = -lboost_filesystem


