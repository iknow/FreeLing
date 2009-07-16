EXTRA_DIST = hmm_smooth.perl train-relax.perl make-probs-file.perl TRAIN unk-tags constr_gram.manual nec/README nec/TRAIN.sh nec/lexicon.cc nec/train.cc
bin_PROGRAMS = indexdict convertdict compile_kb

indexdict_SOURCES = indexdict.cc
indexdict_LDADD = -ldb_cxx

convertdict_SOURCES = convertdict.cc
convertdict_LDADD = -lfries -lpcre

compile_kb_SOURCES = $(top_srcdir)/src/libmorfo/disambiguator/ukb/compile_kb.cc
compile_kb_LDADD = -ldb_cxx -lmorfo -lfries -ldb_cxx -lpcre -lomlet -lboost_program_options -lboost_filesystem
compile_kb_LDFLAGS = -L$(top_srcdir)/src/libmorfo


