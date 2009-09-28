INCLUDES = -I$(top_srcdir)/src/include
EXTRA_DIST = chart_parser/readgram.l relax_tagger/readCG.l

lib_LTLIBRARIES = libmorfo.la

libmorfo_la_SOURCES = accents.cc accents_modules.cc automat.cc dates.cc dates_modules.cc dictionary.cc tagger.cc hmm_tagger.cc locutions.cc maco.cc np.cc bioner.cc nec.cc numbers.cc numbers_modules.cc maco_options.cc probabilities.cc punts.cc quantities.cc quantities_modules.cc splitter.cc suffixes.cc tokenizer.cc senses.cc semdb.cc traces.cc dependencies.cc dep_rules.cc chart_parser/chart_parser.cc chart_parser/chart.cc chart_parser/grammar.cc chart_parser/readgram.cc relax_tagger/constraint_grammar.cc relax_tagger/readCG.cc relax_tagger/relax_tagger.cc relax_tagger/relax.cc coref/coref.cc coref/coref_fex.cc disambiguator/disambiguator.cc disambiguator/ukb/common.cc disambiguator/ukb/configFile.cc disambiguator/ukb/disambGraph.cc disambiguator/ukb/globalVars.cc disambiguator/ukb/wdict.cc disambiguator/ukb/csentence.cc disambiguator/ukb/fileElem.cc disambiguator/ukb/kbGraph.cc disambiguator/ukb/*.h

libmorfo_la_LDFLAGS = -version-info 3:0:1
