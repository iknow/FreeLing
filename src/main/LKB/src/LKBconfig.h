//////////////////////////////////////////////////////////////////
//
//    FreeLing - Open Source Language Analyzers
//
//    Copyright (C) 2004   TALP Research Center
//                         Universitat Politecnica de Catalunya
//
//    This library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU General Public
//    License as published by the Free Software Foundation; either
//    version 2.1 of the License, or (at your option) any later version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    General Public License for more details.
//
//    You should have received a copy of the GNU General Public
//    License along with this library; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
//    contact: Lluis Padro (padro@lsi.upc.es)
//             TALP Research Center
//             despatx C6.212 - Campus Nord UPC
//             08034 Barcelona.  SPAIN
//
////////////////////////////////////////////////////////////////

#ifndef _CONFIG
#define _CONFIG

#include <cfg+.h>
#include "freeling/traces.h"

#define MOD_TRACENAME "CONFIG_OPTIONS"
#define MOD_TRACECODE OPTIONS_TRACE

#define DefaultConfigFile "analyzer.cfg" // default ConfigFile

// codes for input-output formats
#define PLAIN    0
#define TOKEN    1
#define SPLITTED 2
#define MORFO    3
#define TAGGED   4
#define SENSES   5
#define SHALLOW  6
#define PARSED   7
#define DEP      8

// codes for tagging algorithms
#define HMM   0
#define RELAX 1

// codes for dependency parsers
#define TXALA	0

// codes for sense annotation
#define NONE  0
#define ALL   1
#define MFS   2
#define UKB   3

// codes for ForceSelect
#define FORCE_NONE   0
#define FORCE_TAGGER 1
#define FORCE_RETOK  2

using namespace std;

////////////////////////////////////////////////////////////////
///  Class config implements a set of specific options
/// for the NLP analyzer, providing a C++ wrapper to 
/// libcfg+ library.
////////////////////////////////////////////////////////////////

class config {

 public:
    char * ConfigFile;

    /// General options
    char * Lang;
    /// General options
    int InputFormat, OutputFormat;
    /// General options
    int AlwaysFlush;
    /// General options
    int TrainingOutput;
    /// General options
    int UTF8;

    /// Tokenizer options
    char * TOK_TokenizerFile;

    /// Splitter options
    char * SPLIT_SplitterFile;

    /// Morphological analyzer options
    int MACO_AffixAnalysis, MACO_MultiwordsDetection, 
        MACO_NumbersDetection, MACO_PunctuationDetection, 
        MACO_DatesDetection, MACO_QuantitiesDetection, 
        MACO_DictionarySearch, MACO_ProbabilityAssignment, MACO_OrthographicCorrection;
    int MACO_NER_which;
    /// Morphological analyzer options
    char *MACO_Decimal, *MACO_Thousand;

    /// Morphological analyzer options
    char *MACO_LocutionsFile, *MACO_QuantitiesFile, *MACO_AffixFile, 
         *MACO_ProbabilityFile, *MACO_DictionaryFile, 
         *MACO_NPdataFile, *MACO_PunctuationFile;
	
    /// Orthographic correction options
    char *MACO_CorrectorFile;
	 
    double MACO_ProbabilityThreshold;

    // NEC options
    int NEC_NEClassification;
    char *NEC_FilePrefix;

    // Sense annotator options
    int SENSE_SenseAnnotation;
    char *SENSE_SenseFile;
    int SENSE_DuplicateAnalysis;
    char *UKB_BinFile;
    char *UKB_DictFile;
    int UKB_MaxIter;
    double UKB_Epsilon;

    /// Tagger options
    char * TAGGER_HMMFile;
    char * TAGGER_RelaxFile;
    int TAGGER_which;
    int TAGGER_RelaxMaxIter;
    double TAGGER_RelaxScaleFactor;
    double TAGGER_RelaxEpsilon;
    int TAGGER_Retokenize;
    int TAGGER_ForceSelect;

    /// Parser options
    char * PARSER_GrammarFile;

    /// Dependency options
    char * DEP_TxalaFile;    

    int COREF_CoreferenceResolution;
    char * COREF_CorefFile;

    /// constructor
    config(char **argv) {
      CFG_CONTEXT con;
      register int ret;
      int help;
      // Auxiliary for string translation
      char *InputF, *OutputF, *Ner, *Tagger, *SenseAnot, *Force;
      // Auxiliary for boolean handling
      int train, utf, flush,noflush, afx,noafx,   loc,noloc,   numb,nonumb,
          punt,nopunt,   date,nodate,   quant,noquant,  dict,nodict,   prob,noprob,
  	  nec,nonec,     dup,nodup,      retok,noretok,  coref,nocoref, orto, noorto;
      char *cf_flush, *cf_afx, *cf_loc,   *cf_numb,
           *cf_punt,  *cf_date, *cf_quant, *cf_dict, *cf_prob,
	   *cf_nec,  *cf_dup,   *cf_retok,  *cf_coref, *cf_orto;

 
      // Options structure
      struct cfg_option OptionList[] = {  // initialization
	{"help",    'h',  NULL,                      CFG_BOOL, (void *) &help, 0},
	{NULL,      'f',  NULL,                      CFG_STR,  (void *) &ConfigFile, 0},
	// general options
	{"lang",    '\0', "Lang",                    CFG_STR,  (void *) &Lang, 0},
	{"tlevel",   'l', "TraceLevel",              CFG_INT,  (void *) &traces::TraceLevel, 0},
	{"tmod",     'm', "TraceModule",             CFG_INT,  (void *) &traces::TraceModule, 0},
	{"inpf",    '\0', "InputFormat",             CFG_STR,  (void *) &InputF, 0},
	{"outf",    '\0', "OutputFormat",            CFG_STR,  (void *) &OutputF, 0},
	{"train",   '\0', NULL,                      CFG_BOOL, (void *) &train, 0},
	{"utf",     '\0', "UseUTF",                  CFG_BOOL, (void *) &utf, 0},
	{"flush",   '\0', NULL,                      CFG_BOOL, (void *) &flush, 0},
	{"noflush", '\0', NULL,                      CFG_BOOL, (void *) &noflush, 0},
	{NULL,      '\0', "AlwaysFlush",             CFG_STR,  (void *) &cf_flush, 0},
	// tokenizer options
	{"ftok",    '\0', "TokenizerFile",           CFG_STR, (void *) &TOK_TokenizerFile, 0},
	// splitter options
	{"fsplit",  '\0', "SplitterFile",            CFG_STR,  (void *) &SPLIT_SplitterFile, 0},
	// morfo options
	{"afx",    '\0', NULL,                      CFG_BOOL, (void *) &afx, 0},
	{"noafx",  '\0', NULL,                      CFG_BOOL, (void *) &noafx, 0},
	{NULL,      '\0', "AffixAnalysis",          CFG_STR,  (void *) &cf_afx, 0},
	{"loc",     '\0', NULL,                      CFG_BOOL, (void *) &loc, 0},
	{"noloc",   '\0', NULL,                      CFG_BOOL, (void *) &noloc, 0},
	{NULL,      '\0', "MultiwordsDetection",     CFG_STR,  (void *) &cf_loc, 0},
	{"numb",    '\0', NULL,                      CFG_BOOL, (void *) &numb, 0},
	{"nonumb",  '\0', NULL,                      CFG_BOOL, (void *) &nonumb, 0},
	{NULL,      '\0', "NumbersDetection",        CFG_STR,  (void *) &cf_numb, 0},
	{"punt",    '\0', NULL,                      CFG_BOOL, (void *) &punt, 0},
	{"nopunt",  '\0', NULL,                      CFG_BOOL, (void *) &nopunt, 0},
	{NULL,      '\0', "PunctuationDetection",    CFG_STR,  (void *) &cf_punt, 0},
	{"date",    '\0', NULL,                      CFG_BOOL, (void *) &date, 0},
	{"nodate",  '\0', NULL,                      CFG_BOOL, (void *) &nodate, 0},
	{NULL,      '\0', "DatesDetection",          CFG_STR,  (void *) &cf_date, 0},
	{"quant",   '\0', NULL,                      CFG_BOOL, (void *) &quant, 0},
	{"noquant", '\0', NULL,                      CFG_BOOL, (void *) &noquant, 0},
	{NULL,      '\0', "QuantitiesDetection",     CFG_STR,  (void *) &cf_quant, 0},
	{"dict",    '\0', NULL,                      CFG_BOOL, (void *) &dict, 0},
	{"nodict",  '\0', NULL,                      CFG_BOOL, (void *) &nodict, 0},
	{"orto",    '\0', NULL,                      CFG_BOOL, (void *) &orto, 0},
	{"noorto",  '\0', NULL,                      CFG_BOOL, (void *) &noorto, 0},
	{NULL,      '\0', "OrthographicCorrection",  CFG_STR,  (void *) &cf_orto, 0},
	{NULL,      '\0', "DictionarySearch",        CFG_STR,  (void *) &cf_dict, 0},
	{"prob",    '\0', NULL,                      CFG_BOOL, (void *) &prob, 0},
	{"noprob",  '\0', NULL,                      CFG_BOOL, (void *) &noprob, 0},
	{NULL,      '\0', "ProbabilityAssignment",   CFG_STR,  (void *) &cf_prob, 0},
	{"ner",     '\0', "NERecognition",           CFG_STR,  (void *) &Ner, 0},
	{"dec",     '\0', "DecimalPoint",            CFG_STR,  (void *) &MACO_Decimal, 0},
	{"thou",    '\0', "ThousandPoint",           CFG_STR,  (void *) &MACO_Thousand, 0},
	{"floc",    'L',  "LocutionsFile",           CFG_STR,  (void *) &MACO_LocutionsFile, 0},
	{"fqty",    'Q',  "QuantitiesFile",          CFG_STR,  (void *) &MACO_QuantitiesFile, 0},
	{"fafx",    'S',  "AffixFile",               CFG_STR,  (void *) &MACO_AffixFile, 0},
	{"fprob",   'P',  "ProbabilityFile",         CFG_STR,  (void *) &MACO_ProbabilityFile, 0},
	{"thres",   'e',  "ProbabilityThreshold",    CFG_DOUBLE, (void *) &MACO_ProbabilityThreshold, 0},
	{"fdict",   'D',  "DictionaryFile",          CFG_STR,  (void *) &MACO_DictionaryFile, 0},
	{"fcorr",   'K',  "CorrectorFile",           CFG_STR,  (void *) &MACO_CorrectorFile, 0},
	{"fnp",     'N',  "NPDataFile",              CFG_STR,  (void *) &MACO_NPdataFile, 0},
	{"fpunct",  'F',  "PunctuationFile",         CFG_STR,  (void *) &MACO_PunctuationFile, 0},
	// NEC options
	{"nec",     '\0', NULL,                      CFG_BOOL, (void *) &nec, 0},
	{"nonec",   '\0', NULL,                      CFG_BOOL, (void *) &nonec, 0},
	{NULL,      '\0', "NEClassification",        CFG_STR,  (void *) &cf_nec, 0},
	{"fnec",    '\0', "NECFilePrefix",           CFG_STR,  (void *) &NEC_FilePrefix, 0},
	// Sense options
	{"sense",   's',  "SenseAnnotation",         CFG_STR,  (void *) &SenseAnot, 0},
	{"fsense",  'W',  "SenseFile",               CFG_STR,  (void *) &SENSE_SenseFile, 0},
	{"fukbrel", 'U',  "UKBRelations",            CFG_STR,  (void *) &UKB_BinFile, 0},
	{"fukbdic", 'V',  "UKBDictionary",           CFG_STR,  (void *) &UKB_DictFile, 0},
	{"ukbeps",  '\0', "UKBEpsilon",              CFG_DOUBLE,(void *) &UKB_Epsilon, 0},
	{"ukbiter", '\0', "UKBMaxIter",              CFG_INT,  (void *) &UKB_MaxIter, 0},
	{"dup",     '\0', NULL,                      CFG_BOOL, (void *) &dup, 0},
	{"nodup",   '\0', NULL,                      CFG_BOOL, (void *) &nodup, 0},
	{NULL,      '\0', "DuplicateAnalysis",       CFG_STR,  (void *) &cf_dup, 0},
	// tagger options
	{"hmm",  'H',  "TaggerHMMFile",              CFG_STR,  (void *) &TAGGER_HMMFile, 0},
	{"rlx",  'R',  "TaggerRelaxFile",            CFG_STR,  (void *) &TAGGER_RelaxFile, 0},
	{"tag",  't',  "Tagger",                     CFG_STR,  (void *) &Tagger, 0},
	{"iter", 'i', "TaggerRelaxMaxIter",          CFG_INT,  (void *) &TAGGER_RelaxMaxIter, 0},
	{"sf",   'r', "TaggerRelaxScaleFactor",      CFG_DOUBLE, (void *) &TAGGER_RelaxScaleFactor, 0},
	{"eps",  '\0', "TaggerRelaxEpsilon",         CFG_DOUBLE, (void *) &TAGGER_RelaxEpsilon, 0},
	{"rtk",   '\0', NULL,                        CFG_BOOL, (void *) &retok, 0},
	{"nortk", '\0', NULL,                        CFG_BOOL, (void *) &noretok, 0},
	{NULL,    '\0', "TaggerRetokenize",          CFG_STR,  (void *) &cf_retok, 0},
	{"force", '\0', "TaggerForceSelect",         CFG_STR,  (void *) &Force, 0},
	// parser options
	{"grammar", 'G',  "GrammarFile",             CFG_STR, (void *) &PARSER_GrammarFile, 0},
        // dep options
	{"txala", 'T', "DepTxalaFile",               CFG_STR, (void *) &DEP_TxalaFile, 0},
	// Coreference options
	{"coref",   '\0', NULL,                        CFG_BOOL, (void *) &coref, 0},
	{"nocoref", '\0', NULL,                        CFG_BOOL, (void *) &nocoref, 0},
	{NULL,      '\0', "CoreferenceResolution",     CFG_STR,  (void *) &cf_coref, 0},
	{"fcorf",   'C', "CorefFile",                 CFG_STR,  (void *) &COREF_CorefFile, 0},
	CFG_END_OF_LIST
      };
      
      // Creating context 
      con = cfg_get_context(OptionList);
      if (con == NULL) {
	ERROR_CRASH("Error creating context. Not enough memory?");
      }
      
      // init auxiliary variables
      InputF=NULL; OutputF=NULL;  Ner=NULL; Tagger=NULL; SenseAnot=NULL; Force=NULL;
      train=false; utf=false;
      flush=false; noflush=false; afx=false;   noafx=false; 
      loc=false;   noloc=false;   numb=false;   nonumb=false;   punt=false; nopunt=false;
      date=false;  nodate=false;  quant=false;  noquant=false;  dict=false; nodict=false; 
      prob=false;  noprob=false;  nec=false;  nonec=false; 
      dup=false;   nodup=false;   retok=false; noretok=false; coref=false; nocoref=false;
      orto=false; noorto=false; 
      cf_flush=NULL; cf_afx=NULL;  cf_loc=NULL;   cf_numb=NULL; 
      cf_punt=NULL;  cf_date=NULL;  cf_quant=NULL; cf_dict=NULL;  cf_prob=NULL;
      cf_nec=NULL;   cf_dup=NULL;   cf_retok=NULL;  cf_coref=NULL; cf_orto=NULL;
      
      // Set built-in default values.
      ConfigFile=NULL; help=false;
      Lang=NULL; traces::TraceLevel=0; traces::TraceModule=0;
      AlwaysFlush=false;
      TrainingOutput=false;
      UTF8=false;
      TOK_TokenizerFile=NULL;
      SPLIT_SplitterFile=NULL;
      MACO_AffixAnalysis=false;   MACO_MultiwordsDetection=false; 
      MACO_NumbersDetection=false; MACO_PunctuationDetection=false; 
      MACO_DatesDetection=false;   MACO_QuantitiesDetection=false; 
      MACO_DictionarySearch=false; MACO_ProbabilityAssignment=false; 
      MACO_Decimal=NULL; MACO_Thousand=NULL;
      MACO_LocutionsFile=NULL; MACO_QuantitiesFile=NULL; MACO_AffixFile=NULL; 
      MACO_ProbabilityFile=NULL; MACO_DictionaryFile=NULL; 
      MACO_NPdataFile=NULL; MACO_PunctuationFile=NULL;
      MACO_CorrectorFile=NULL; 
      MACO_ProbabilityThreshold=0.0; 
      MACO_NER_which=0;
      NEC_NEClassification=false; NEC_FilePrefix=NULL; 
      SENSE_SenseAnnotation=NONE; SENSE_SenseFile=NULL; 
      SENSE_DuplicateAnalysis=false; 
      UKB_BinFile=NULL; UKB_DictFile=NULL;
      UKB_MaxIter=0; UKB_Epsilon=0;
      TAGGER_which=0; TAGGER_HMMFile=NULL; TAGGER_RelaxFile=NULL; 
      TAGGER_RelaxMaxIter=0; TAGGER_RelaxScaleFactor=0.0; TAGGER_RelaxEpsilon=0.0;
      TAGGER_Retokenize=0; TAGGER_ForceSelect=0;
      PARSER_GrammarFile=NULL;
      COREF_CoreferenceResolution=false; COREF_CorefFile=NULL;
      DEP_TxalaFile=NULL; 

       // parse comand line
      cfg_set_cmdline_context(con, 1, -1, argv);
      ret = cfg_parse(con);
      if (ret != CFG_OK) {
	ERROR_CRASH("Error "+util::int2string(ret)+" parsing command line. "+string(cfg_get_error_str(con)));
      }
      
      // Check options
      
      // Help screen required
      if (help) {
	PrintHelpScreen();
	exit(0); // return to system
      }
      
      // if no config file given, use default
      if (ConfigFile == NULL) {
	WARNING("No config file specified (option -f). Trying to open default file '"+string(DefaultConfigFile)+"'");
	ConfigFile = (char *)malloc(sizeof(char)*(1+strlen(DefaultConfigFile)));
 	strcpy(ConfigFile,DefaultConfigFile);
      }
      
      // parse and load options from ConfigFile
      cfg_set_cfgfile_context(con, 0, -1, ConfigFile);
      ret = cfg_parse(con);
      if (ret != CFG_OK) {
	ERROR_CRASH("Error "+util::int2string(ret)+" parsing config file '"+string(ConfigFile)+"'. "+string(cfg_get_error_str(con)));
      }

      // Handle boolean options expressed as strings in config file
      SetBooleanOptionCF(cf_flush,AlwaysFlush,"AlwaysFlush");
      SetBooleanOptionCF(cf_afx,MACO_AffixAnalysis,"AffixAnalysis");
      SetBooleanOptionCF(cf_loc, MACO_MultiwordsDetection,"MultiwordsDetection");
      SetBooleanOptionCF(cf_numb,MACO_NumbersDetection,"NumbersDetection");
      SetBooleanOptionCF(cf_punt,MACO_PunctuationDetection,"PunctuationDetection");
      SetBooleanOptionCF(cf_date,MACO_DatesDetection,"DatesDetection");
      SetBooleanOptionCF(cf_quant,MACO_QuantitiesDetection,"QuantitiesDetection");
      SetBooleanOptionCF(cf_dict,MACO_DictionarySearch,"DictionarySearch");
      SetBooleanOptionCF(cf_prob,MACO_ProbabilityAssignment,"ProbabilityAssignment");
      SetBooleanOptionCF(cf_orto,MACO_OrthographicCorrection,"OrthographicCorrection");
      SetBooleanOptionCF(cf_nec,NEC_NEClassification,"NEClassification");
      SetBooleanOptionCF(cf_dup,SENSE_DuplicateAnalysis,"DuplicateAnalysis");
      SetBooleanOptionCF(cf_retok,TAGGER_Retokenize,"TaggerRetokenize");
      SetBooleanOptionCF(cf_coref,COREF_CoreferenceResolution,"CoreferenceResolution");
      
      // Reload command line options to override ConfigFile options
      cfg_set_cmdline_context(con, 1, -1, argv);
      ret = cfg_parse(con);
      if (ret != CFG_OK) {
	ERROR_CRASH("Error "+util::int2string(ret)+" parsing command line. "+string(cfg_get_error_str(con)));
      }

      // check options involving Filenames for environment vars expansion.
      ExpandFileName(TOK_TokenizerFile);
      ExpandFileName(SPLIT_SplitterFile);
      ExpandFileName(MACO_LocutionsFile);
      ExpandFileName(MACO_QuantitiesFile);
      ExpandFileName(MACO_AffixFile);
      ExpandFileName(MACO_ProbabilityFile);
      ExpandFileName(MACO_DictionaryFile); 
      ExpandFileName(MACO_NPdataFile);
      ExpandFileName(MACO_PunctuationFile);
      ExpandFileName(NEC_FilePrefix); 
      ExpandFileName(SENSE_SenseFile); 
      ExpandFileName(UKB_BinFile); 
      ExpandFileName(UKB_DictFile);
      ExpandFileName(TAGGER_HMMFile);
      ExpandFileName(TAGGER_RelaxFile); 
      ExpandFileName(PARSER_GrammarFile); 
      ExpandFileName(DEP_TxalaFile);
      ExpandFileName(COREF_CorefFile); 
      ExpandFileName(MACO_CorrectorFile);
	     
      // Handle boolean options expressed with --myopt or --nomyopt in command line
      SetBooleanOptionCL(train,!train,TrainingOutput,"train");
      SetBooleanOptionCL(utf,!utf,UTF8,"utf");
      SetBooleanOptionCL(flush,noflush,AlwaysFlush,"flush");
      SetBooleanOptionCL(afx,noafx,MACO_AffixAnalysis,"afx");
      SetBooleanOptionCL(loc,noloc, MACO_MultiwordsDetection,"loc");
      SetBooleanOptionCL(numb,nonumb,MACO_NumbersDetection,"numb");
      SetBooleanOptionCL(punt,nopunt,MACO_PunctuationDetection,"punt");
      SetBooleanOptionCL(date,nodate,MACO_DatesDetection,"date");
      SetBooleanOptionCL(quant,noquant,MACO_QuantitiesDetection,"quant");
      SetBooleanOptionCL(dict,nodict,MACO_DictionarySearch,"dict");
      SetBooleanOptionCL(prob,noprob,MACO_ProbabilityAssignment,"prob");
      SetBooleanOptionCL(orto,noorto,MACO_OrthographicCorrection,"orto");
      SetBooleanOptionCL(nec,nonec,NEC_NEClassification,"nec");
      SetBooleanOptionCL(dup,nodup,SENSE_DuplicateAnalysis,"dup");
      SetBooleanOptionCL(retok,noretok,TAGGER_Retokenize,"retk");
      SetBooleanOptionCL(coref,nocoref,COREF_CoreferenceResolution,"coref");


      string s;
      // translate InputF and OutputF strings to more useful integer values.
      s = (InputF==NULL ? "plain" : string(InputF));
      if (s=="plain") InputFormat = PLAIN;
      else if (s=="token") InputFormat = TOKEN;
      else if (s=="splitted") InputFormat = SPLITTED;
      else if (s=="morfo") InputFormat = MORFO;
      else if (s=="tagged") InputFormat = TAGGED;
      else if (s=="sense") InputFormat = SENSES;
      else { ERROR_CRASH("Unknown or invalid input format: "+s);}
      
      s = (OutputF==NULL ? "tagged" : string(OutputF));
      if (s=="token") OutputFormat = TOKEN;
      else if (s=="splitted") OutputFormat = SPLITTED;
      else if (s=="morfo") OutputFormat = MORFO;
      else if (s=="tagged") OutputFormat = TAGGED;
      else if (s=="shallow") OutputFormat = SHALLOW;
      else if (s=="parsed") OutputFormat = PARSED;
      else if (s=="dep") OutputFormat = DEP;
      else { ERROR_CRASH("Unknown or invalid output format: "+s);}

      // translate Ner string to more useful integer values.
      s = (Ner==NULL ? "basic" : string(Ner));
      if (s=="basic") MACO_NER_which = NER_BASIC;
      else if (s=="bio") MACO_NER_which = NER_BIO;
      else if (s=="none" || s=="no") MACO_NER_which = NER_NONE;
      else WARNING("Invalid NER algorithm '"+s+"'. Using default.");

      // translate Tagger string to more useful integer values.
      s = (Tagger==NULL ? "hmm" : string(Tagger));
      if (s=="hmm") TAGGER_which = HMM;
      else if (s=="relax") TAGGER_which = RELAX;
      else WARNING("Invalid tagger algorithm '"+s+"'. Using default.");

      // Translate ForceSelect string to more useful integer values.
      s = (Force==NULL ? "retok" : string(Force));
      if (s=="none" || s=="no") TAGGER_ForceSelect = FORCE_NONE;
      else if (s=="tagger") TAGGER_ForceSelect = FORCE_TAGGER;
      else if (s=="retok") TAGGER_ForceSelect = FORCE_RETOK;
      else WARNING("Invalid ForceSelect value '"+s+"'. Using default.");

      // translate SenseAnot string to more useful integer values.
      s = (SenseAnot==NULL ? "none" : string(SenseAnot));
      if (s=="none" || s=="no") SENSE_SenseAnnotation = NONE;
      else if (s=="all") SENSE_SenseAnnotation = ALL;
      else if (s=="mfs") SENSE_SenseAnnotation = MFS;
      else if (s=="ukb") SENSE_SenseAnnotation = UKB;
      else WARNING("Invalid sense annotation option '"+s+"'. Using default.");
    }

 private:

    void ExpandFileName(char* &s) {
      if (s==NULL) return;
      
      string name(s);
      size_t n=name.find_first_of("$"); 
      if (n!=string::npos) {
	size_t i=name.find_first_of("/\\",n+1);
	if (i==string::npos) i=name.size();
	char* exp=getenv(name.substr(n+1,i-n-1).c_str());
	if (exp==NULL){
	  WARNING("Undefined variable "+name.substr(n+1,i-n-1)+" in configuration file "+string(ConfigFile)+" expanded as empty string.");
	  name = name.substr(0,n) + name.substr(i);
	}
	else {
	  name = name.substr(0,n) + string(exp) + name.substr(i);
	}
	
	free(s);
	s = (char *) calloc(name.size()+1,sizeof(char));
	strcpy (s,name.c_str());
      }
    }
    
    void SetBooleanOptionCF (const char *os, int &opt, const string &name) {
      if (os==NULL) return;
      string s(os);
      if (s=="yes" || s=="y" || s=="on" || s=="true")
        opt=true;
      else if (s=="no" || s=="n" || s=="off" || s=="false")
        opt=false;
      else if (!s.empty())
	WARNING("Invalid boolean value '"+s+"' for option "+name+" in configuration file. Using default value.");
      //else: s is empty, so the option hasn't been specified. Use default.
    } 

    void SetBooleanOptionCL (const int pos, const int neg, int &opt, const string &name) {
      if (pos && neg)  
	WARNING("Ambiguous specification for option --"+name+" in command line. Using default value.");
      else if (pos)
	opt=true;
      else if (neg)
        opt=false;
      //else: nothing specified, leave things as they are.
    }

    void PrintHelpScreen(){
      cout<<"Available options: "<<endl<<endl;
      cout<<"-h, --help             This screen "<<endl;
      cout<<"-f filename            Use alternative configuration file (default: analyzer.cfg)"<<endl;
      cout<<"--lang language        Language (sp: Spanish, ca: Catalan, en: English)"<<endl;
      cout<<"--flush, --noflush     Consider each newline as a sentence end"<<endl;
      cout<<"--inpf string          Input format (plain,token,splitted,morfo,sense,tagged)"<< endl;
      cout<<"--outf string          Output format (token,splitted,morfo,tagged,shallow,parsed,dep)"<< endl;
      cout<<"--train                Produce output format suitable for train scripts (default: disabled)"<<endl;
      cout<<"--utf                  Input is UTF8 (default: disabled)"<<endl;
      cout<<"--ftok filename        Tokenizer rules file "<<endl;
      cout<<"--fsplit filename      Splitter options file "<<endl;
      cout<<"--afx, --noafx         Whether to perform affix analysis"<<endl;
      cout<<"--loc, --noloc         Whether to perform multiword detection"<<endl;
      cout<<"--numb, --nonumb       Whether to perform number detection"<<endl;
      cout<<"--punt, --nopunt       Whether to perform punctuation detection"<<endl;
      cout<<"--date, --nodate       Whether to perform date and time expressions detection"<<endl;
      cout<<"--quant, --noquant     Whether to perform quantities detection"<<endl;
      cout<<"--dict, --nodict       Whether to perform dictionary search"<<endl;
      cout<<"--prob, --noprob       Whether to perform probability assignment"<<endl;
      cout<<"--orto, --noorto       Whether to perform orthographic correction"<<endl;
      cout<<"--ner string           Which kind of NE recognition is to be performed (basic, bio, none)"<<endl;
      cout<<"--dec string           Decimal point character"<<endl;
      cout<<"--thou string          Thousand point character"<<endl;
      cout<<"--floc,-L filename     Multiwords file"<<endl;
      cout<<"--fqty,-Q filename     Quantities file"<<endl;
      cout<<"--fafx,-S filename     Affix rules file"<<endl;
      cout<<"--fprob,-P filename    Probabilities file"<<endl;
      cout<<"--thres,-e float       Probability threshold for unknown word tags"<<endl;
      cout<<"--fdict,-D filename    Dictionary database"<<endl;
      cout<<"--fnp,-N filename      NP recognizer data file"<<endl;
      cout<<"--nec, --nonec         Whether to perform NE classification"<<endl;
      cout<<"--fnec filename        Filename prefix for NEC data XX.rgf, XX.lex, XX.abm"<<endl;
      cout<<"--sense,-s string      Type of sense annotation (no|none,all,mfs,ukb)"<<endl;
      cout<<"--fsense,-W filename   Sense dictionary file"<<endl;
      cout<<"--fukbdic,-V filename  Sense dictionary file for UKB"<<endl;
      cout<<"--fukbrel,-U filename  Compiled relation file for UKB"<<endl;
      cout<<"--ukbeps float         Convergence epsilon for UKB"<<endl;
      cout<<"--ukbiter iter         Maximum iterations for UKB"<<endl;
      cout<<"--dup, --nodup         Whether to duplicate analysis for each different sense"<<endl;
      cout<<"--fpunct,-F filename   Punctuation symbols file"<<endl;
      cout<<"--tag,-t string        Tagging alogrithm to use (hmm, relax)"<<endl;
      cout<<"--hmm,-H filename      Data file for HMM tagger"<<endl;
      cout<<"--rlx,-R filename      Data file for RELAX tagger"<<endl;
      cout<<"--rtk, --nortk         Whether to perform retokenization after PoS tagging"<<endl;
      cout<<"--force string         Whether/when the tagger must be forced to select only one tag per word (none,tagger,retok)"<<endl;
      cout<<"--iter,-i int          Maximum number of iterations allowed for RELAX tagger."<<endl;
      cout<<"--sf,-r float          Support scale factor for RELAX tagger (affects step size)"<<endl;
      cout<<"--eps float            Epsilon value to decide when RELAX tagger achieves no more changes"<<endl;
      cout<<"--grammar,-G filename  Grammar file for chart parser"<<endl;
      cout<<"--txala,-T filename    Rule file for Txala dependency parser"<<endl;
      cout<<"--coref, --nocoref     Whether to perform coreference resolution"<<endl;
      cout<<"--fcorf,-C filename    Coreference solver data file"<<endl;
      cout<<"--fcorr,-K filename    Spell corrector configuration file"<<endl;
      cout<<endl;
    }

};


#endif

