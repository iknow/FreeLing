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


//------------------------------------------------------------------//
//
//                    IMPORTANT NOTICE
//
//  This file contains a simple main program to illustrate 
//  usage of FreeLing analyzers library.
//
//  This sample main program may be used straightforwardly as 
//  a basic front-end for the analyzers (e.g. to analyze corpora)
//
//  Neverthless, if you want embed the FreeLing libraries inside
//  a larger application, or you want to deal with other 
//  input/output formats (e.g. XML), the efficient and elegant 
//  way to do so is consider this file as a mere example, and call 
//  the library from your your own main code.
//
//------------------------------------------------------------------//



using namespace std;

#include <sstream>
#include <iostream> 

#include <map>
#include <list>

/// headers to call freeling library
#include "fries/util.h"
#include "freeling.h"

/// config file/options handler for this particular sample application
#include "config.h"

/// utf <-> latin1 conversion
#include "iso2utf.h"

// we use pointers to the analyzers, so we
// can create only those strictly necessary.
tokenizer *tk;
splitter *sp;
maco *morfo;
nec *neclass;
senses *sens;
disambiguator *dsb;
POS_tagger *tagger;
chart_parser *parser;
dependency_parser *dep;
coref *corfc;
// read configuration file and command-line options
config *cfg;
// performance statistics
#ifdef SERVER
  clock_t start;
  double cpuTime_total=0.0;
  int sentences=0;
  int words=0;
#endif


#ifdef SERVER 
  #define ReadLine(text)            sock->read_message(text)
  #define WriteResults(ls,b)        SendResults(ls,b)
  #define WriteResultsDoc(ls,b,doc) SendResults(ls,b,doc)
  #include "signal.h"
  #include "socket.h"
  socket_CS *sock;
#else 
  #define ReadLine(text)            getline(cin,text)
  #define WriteResults(ls,b)        PrintResults(cout,ls,b)
  #define WriteResultsDoc(ls,b,doc) PrintResults(cout,ls,b,doc)
#endif

#define encode(s)  (cfg->UTF8?Latin1toUTF8(s.c_str()):s)

//---------------------------------------------
// Apply analyzer cascade to sentences in given list
//---------------------------------------------

void AnalyzeSentences(list<sentence> &ls) {
  if (cfg->InputFormat < MORFO && cfg->OutputFormat >= MORFO) 
    morfo->analyze (ls);
  if (cfg->OutputFormat >= MORFO and 
      (cfg->SENSE_SenseAnnotation == MFS or cfg->SENSE_SenseAnnotation == ALL)) 
    sens->analyze (ls);
  if (cfg->InputFormat < TAGGED && cfg->OutputFormat >= TAGGED) 
    tagger->analyze (ls);
  if (cfg->OutputFormat >= TAGGED and (cfg->SENSE_SenseAnnotation == UKB)) 
    dsb->analyze (ls);
  if (cfg->OutputFormat >= TAGGED and cfg->NEC_NEClassification) 
    neclass->analyze (ls);
  if (cfg->OutputFormat >= SHALLOW)
    parser->analyze (ls);
  if (cfg->OutputFormat >= PARSED)
    dep->analyze (ls);
}


//---------------------------------------------
// Print senses informaion for an analysis
//---------------------------------------------

string OutputSenses (const analysis & a) {

  string res;
  list<pair<string,double> > ls = a.get_senses ();
  if (ls.size () > 0) {
    if (cfg->SENSE_SenseAnnotation == MFS)
      res = " " + ls.begin()->first;
    else  // ALL or UKB specified
      res = " " + util::pairlist2string (ls, ":", "/");
  }
  else
    res = " -";

  return res;
}


//---------------------------------------------
// print parse tree
//--------------------------------------------

void PrintTree (ostream &sout, parse_tree::iterator n, int depth, const document &doc) {

  parse_tree::sibling_iterator d;

  sout << string (depth * 2, ' ');  
  if (n->num_children () == 0) {
    if (n->info.is_head ()) sout << "+";
    word w = n->info.get_word ();
    sout << "(" << encode(w.get_form()) << " " << encode(w.get_lemma()) << " " << w.get_parole ();
    sout << OutputSenses ((*w.selected_begin ()));
    sout << ")" << endl;
  }
  else {
    if (n->info.is_head ()) sout << "+";

    sout<<n->info.get_label();
    if (cfg->COREF_CoreferenceResolution) {
      // Print coreference group, if needed.
      int ref = doc.get_coref_group(n->info.get_node_id());
      if (ref != -1 and n->info.get_label() == "sn") sout<<"(REF:" << ref <<")";
    }
    sout << "_[" << endl;

    for (d = n->sibling_begin (); d != n->sibling_end (); ++d) 
      PrintTree (sout, d, depth + 1, doc);
    sout << string (depth * 2, ' ') << "]" << endl;
  }
}


//---------------------------------------------
// print dependency tree
//---------------------------------------------

void PrintDepTree (ostream &sout, dep_tree::iterator n, int depth, const document &doc) {
  dep_tree::sibling_iterator d, dm;
  int last, min, ref;
  bool trob;

  sout << string (depth*2, ' ');

  parse_tree::iterator pn = n->info.get_link();
  sout<<pn->info.get_label(); 
  ref = (cfg->COREF_CoreferenceResolution ? doc.get_coref_group(pn->info.get_node_id()) : -1);
  if (ref != -1 and pn->info.get_label() == "sn") {
    sout<<"(REF:" << ref <<")";
  }
  sout<<"/" << n->info.get_label() << "/";

  word w = n->info.get_word();
  sout << "(" << encode(w.get_form()) << " " << encode(w.get_lemma()) << " " << w.get_parole ();
  sout << OutputSenses ((*w.selected_begin()));
  sout << ")";
  
  if (n->num_children () > 0) {
    sout << " [" << endl;
    
    // Print Nodes
    for (d = n->sibling_begin (); d != n->sibling_end (); ++d)
      if (!d->info.is_chunk ())
	PrintDepTree (sout, d, depth + 1, doc);
    
    // print CHUNKS (in order)
    last = 0;
    trob = true;
    while (trob) {
      // while an unprinted chunk is found look, for the one with lower chunk_ord value
      trob = false;
      min = 9999;
      for (d = n->sibling_begin (); d != n->sibling_end (); ++d) {
	if (d->info.is_chunk ()) {
	  if (d->info.get_chunk_ord () > last
	      and d->info.get_chunk_ord () < min) {
	    min = d->info.get_chunk_ord ();
	    dm = d;
	    trob = true;
	  }
	}
      }
      if (trob)
	PrintDepTree (sout, dm, depth + 1, doc);
      last = min;
    }
    
    sout << string (depth * 2, ' ') << "]";
  }
  sout << endl;
}


//---------------------------------------------
// print retokenization combinations for a word
//---------------------------------------------

list<analysis> printRetokenizable(ostream &sout, const list<word> &rtk, list<word>::const_iterator w, const string &lem, const string &tag) {
  
  list<analysis> s;
  if (w==rtk.end()) 
    s.push_back(analysis(lem.substr(1),tag.substr(1)));
		
  else {
    list<analysis> s1;
    list<word>::const_iterator w1=w; w1++;
    for (word::const_iterator a=w->begin(); a!=w->end(); a++) {
      s1=printRetokenizable(sout, rtk, w1, lem+"+"+a->get_lemma(), tag+"+"+a->get_parole());
      s.splice(s.end(),s1);
    }
  }
  return s;
}  

//---------------------------------------------
// print analysis for a word
//---------------------------------------------

void PrintWord (ostream &sout, const word &w, bool only_sel, bool probs) {
  word::const_iterator ait;

  word::const_iterator a_beg,a_end;
  if (only_sel) {
    a_beg = w.selected_begin();
    a_end = w.selected_end();
  }
  else {
    a_beg = w.analysis_begin();
    a_end = w.analysis_end();
  }

  for (ait = a_beg; ait != a_end; ait++) {
    if (ait->is_retokenizable ()) {
      list <word> rtk = ait->get_retokenizable ();
      list <analysis> la=printRetokenizable(sout, rtk, rtk.begin(), "", "");
      for (list<analysis>::iterator x=la.begin(); x!=la.end(); x++) {
	sout << " " << encode(x->get_lemma()) << " " << x->get_parole();
	if (probs) sout << " " << ait->get_prob()/la.size();
      }
    }
    else {
      sout << " " << encode(ait->get_lemma()) << " " << ait->get_parole ();
      if (probs) sout << " " << ait->get_prob ();
    }

    if (cfg->SENSE_SenseAnnotation != NONE)
      sout << OutputSenses (*ait);
  }
}

//---------------------------------------------
// print obtained analysis.
//---------------------------------------------

void PrintResults (ostream &sout, list<sentence > &ls, bool separate, const document &doc=document()) {
  sentence::const_iterator w;
  list<sentence>::iterator is;
    
  for (is = ls.begin (); is != ls.end (); is++) {
    if (cfg->OutputFormat >= SHALLOW) {
      /// obtain parse tree and draw it at will
      switch (cfg->OutputFormat) {

        case SHALLOW:
        case PARSED: {
	  parse_tree & tr = is->get_parse_tree ();
	  PrintTree (sout, tr.begin (), 0, doc);
	  sout << endl;
	  }
	  break;
	
        case DEP: {
	  dep_tree & dep = is->get_dep_tree ();
	  PrintDepTree (sout, dep.begin (), 0, doc);
          }
	  break;
	
        default:   // should never happen
	  break;
      }
    }
    else {
      for (w = is->begin (); w != is->end (); w++) {
	sout << encode(w->get_form());
	
	if (cfg->OutputFormat == MORFO or cfg->OutputFormat == TAGGED) {
	  if (cfg->TrainingOutput) {
	    /// Trainig output: selected analysis (no prob) + all analysis (with probs)
	    PrintWord(sout,*w,true,false);
	    sout<<" #";
	    PrintWord(sout,*w,false,true);
	  }
	  else {
	    /// Normal output: selected analysis (with probs)
	    PrintWord(sout,*w,true,true);  
	  }
	}

	sout << endl;	
      }
    }
    // sentence separator: blank line.
    if (separate) sout << endl;
  }
}


//---------------------------------------------
// Destroy analyzers 
//---------------------------------------------

void cleanup() {
  // clean up. Note that deleting a null pointer is a safe (yet useless) operation
  delete tk;
  delete sp;
  delete morfo;
  delete tagger;
  delete neclass;
  delete sens;
  delete dsb;
  delete parser;
  delete dep;
  delete corfc;

  delete cfg;
}


//---------------------------------------------
// Capture signal to shut server down
//---------------------------------------------

#ifdef SERVER
void terminate (int param)
{
  cerr<<"SERVER: Signal received. Stopping"<<endl;
  cleanup();
  exit(1);
}
#endif


//---------------------------------------------
// Send performance stats to the client
//---------------------------------------------

#ifdef SERVER
void PrintStats() {
  ostringstream sout;
  sout << "Words: "<<words<<", sentences: "<<sentences<<", cpuTime_total: "<<cpuTime_total<<endl;
  sout << "Words/sentence: "<<(sentences>0?words/sentences:0)<<", words/second: "<<(cpuTime_total>0?words/cpuTime_total:0)<<", sentences/second: "<<(cpuTime_total>0?sentences/cpuTime_total:0)<<endl;
  sock->write_message(sout.str());
}
#endif


//---------------------------------------------
// Reset performance stats.
//---------------------------------------------

#ifdef SERVER
void ResetStats() {
  words=0;
  sentences=0;
  cpuTime_total=0.0; 
  sock->write_message("FL-SERVER-READY");  
}
#endif


//---------------------------------------------
// Update performance stats.
//---------------------------------------------

#ifdef SERVER
void UpdateStats(const list<sentence> &ls) {

  for (list<sentence>::const_iterator is=ls.begin(); is!=ls.end(); is++) {
    words+=is->size();
    sentences++;
  }  

  clock_t end = clock();
  cpuTime_total += (end-(double)start)/(CLOCKS_PER_SEC);
}
#endif


//---------------------------------------------
// Send ACK to client. Let him know we expect more data.
//---------------------------------------------

#ifdef SERVER
void SendACK () {  
  sock->write_message("FL-SERVER-READY");  
}
#endif


//---------------------------------------------
// Send results via socket
//---------------------------------------------

#ifdef SERVER
void SendResults (list <sentence> &ls, bool sep, const document &doc=document() ) {
  word::const_iterator ait;
  sentence::const_iterator w;
  list<sentence>::iterator is;

  if (ls.empty()) {
    SendACK();
    return;
  }

  ostringstream sout;
  PrintResults(sout,ls,sep,doc);
  sock->write_message(sout.str());

  UpdateStats(ls);
}
#endif


//---------------------------------------------
// read configuration file and command-line options, 
// and create appropriate analyzers
//---------------------------------------------

void CreateAnalyzers(char **argv) {

  cfg = new config(argv);

  if (!((cfg->InputFormat < cfg->OutputFormat) or
	(cfg->InputFormat == cfg->OutputFormat and cfg->InputFormat == TAGGED
	 and cfg->NEC_NEClassification)))
    {
      cerr <<"Error - Input format cannot be more complex than desired output."<<endl;
      exit (1);
    }

  if (cfg->COREF_CoreferenceResolution and cfg->OutputFormat<=TAGGED) {
    cerr <<"Error - Requested coreference resolution is only compatible with output format 'parsed' or 'dep'." <<endl;
    exit (1);
  }

  if (cfg->OutputFormat < TAGGED and (cfg->SENSE_SenseAnnotation == UKB))   {
    cerr <<"Error - UKB word sense disambiguation requires PoS tagging. Specify 'tagged', 'parsed' or 'dep' output format." <<endl;
    exit (1);
  }

  if (cfg->OutputFormat != TAGGED and cfg->TrainingOutput) {
    cerr <<"Warning - OutputFormat changed to 'tagged' since option --train was specified." <<endl;
    cfg->OutputFormat = TAGGED;
  }
  
  //--- create needed analyzers, depending on given options ---//

  // tokenizer requested
  if (cfg->InputFormat < TOKEN and cfg->OutputFormat >= TOKEN)
    tk = new tokenizer (cfg->TOK_TokenizerFile);
  // splitter requested
  if (cfg->InputFormat < SPLITTED and cfg->OutputFormat >= SPLITTED)
    sp = new splitter (cfg->SPLIT_SplitterFile);

  // morfological analysis requested
  if (cfg->InputFormat < MORFO and cfg->OutputFormat >= MORFO) {
      // the morfo class requires several options at creation time.
      // they are passed packed in a maco_options object.
      maco_options opt (cfg->Lang);
      // boolean options to activate/desactivate modules
      // default: all modules deactivated (options set to "false")
      opt.set_active_modules (cfg->MACO_AffixAnalysis,
			      cfg->MACO_MultiwordsDetection,
			      cfg->MACO_NumbersDetection,
			      cfg->MACO_PunctuationDetection,
			      cfg->MACO_DatesDetection,
			      cfg->MACO_QuantitiesDetection,
			      cfg->MACO_DictionarySearch,
			      cfg->MACO_ProbabilityAssignment,
			      cfg->MACO_NER_which,
			      cfg->MACO_OrthographicCorrection);

      // decimal/thousand separators used by number detection
      opt.set_nummerical_points (cfg->MACO_Decimal, cfg->MACO_Thousand);
      // Minimum probability for a tag for an unkown word
      opt.set_threshold (cfg->MACO_ProbabilityThreshold);
      // Data files for morphological submodules. by default set to ""
      // Only files for active modules have to be specified 
      opt.set_data_files (cfg->MACO_LocutionsFile, cfg->MACO_QuantitiesFile,
			  cfg->MACO_AffixFile, cfg->MACO_ProbabilityFile,
			  cfg->MACO_DictionaryFile, cfg->MACO_NPdataFile,
			  cfg->MACO_PunctuationFile,cfg->MACO_CorrectorFile);

      // create analyzer with desired options
      morfo = new maco (opt);
  }

  // sense annotation requested
  if (cfg->InputFormat < SENSE and cfg->OutputFormat >= MORFO
      and (cfg->SENSE_SenseAnnotation == MFS or cfg->SENSE_SenseAnnotation == ALL))
    sens = new senses (cfg->SENSE_SenseFile, cfg->SENSE_DuplicateAnalysis);
  else if (cfg->InputFormat < SENSE and cfg->OutputFormat >= TAGGED
      and (cfg->SENSE_SenseAnnotation == UKB))      
    dsb = new disambiguator (cfg->UKB_BinFile, cfg->UKB_DictFile, cfg->UKB_Epsilon, cfg->UKB_MaxIter);

  // tagger requested, see which method
  if (cfg->InputFormat < TAGGED and cfg->OutputFormat >= TAGGED) {
      if (cfg->TAGGER_which == HMM)
	tagger =
	  new hmm_tagger (cfg->Lang, cfg->TAGGER_HMMFile, cfg->TAGGER_Retokenize,
			  cfg->TAGGER_ForceSelect);
      else if (cfg->TAGGER_which == RELAX)
	tagger =
	  new relax_tagger (cfg->TAGGER_RelaxFile, cfg->TAGGER_RelaxMaxIter,
			    cfg->TAGGER_RelaxScaleFactor,
			    cfg->TAGGER_RelaxEpsilon, cfg->TAGGER_Retokenize,
			    cfg->TAGGER_ForceSelect);
  }

  // NEC requested
  if (cfg->InputFormat <= TAGGED and cfg->OutputFormat >= TAGGED and 
          (cfg->NEC_NEClassification or cfg->COREF_CoreferenceResolution)) {
      neclass = new nec ("NP", cfg->NEC_FilePrefix);
  }
  
  // Chunking requested
  if (cfg->InputFormat < SHALLOW and (cfg->OutputFormat >= SHALLOW or cfg->COREF_CoreferenceResolution)) {
      parser = new chart_parser (cfg->PARSER_GrammarFile);
  }

  // Dependency parsing requested
  if (cfg->InputFormat < SHALLOW and cfg->OutputFormat >= PARSED) 
    dep = new dep_txala (cfg->DEP_TxalaFile, parser->get_start_symbol ());

  if (cfg->COREF_CoreferenceResolution) {
    int vectors = COREFEX_DIST | COREFEX_IPRON | COREFEX_JPRON | COREFEX_IPRONM | COREFEX_JPRONM
				| COREFEX_STRMATCH | COREFEX_DEFNP | COREFEX_DEMNP | COREFEX_GENDER | COREFEX_NUMBER
				| COREFEX_SEMCLASS | COREFEX_PROPNAME | COREFEX_ALIAS | COREFEX_APPOS;
    corfc = new coref(cfg->COREF_CorefFile, vectors);
  } 
}



//---------------------------------------------
void ProcessLineCoref(const string &text, list<word> &av,
                      list<sentence> &ls,
                      paragraph &par, document &doc) {
  if (text=="") { // new paragraph.
    // flush buffer
    sp->split(av, true, ls);
    // add sentece to paragraph
    par.insert(par.end(), ls.begin(), ls.end());  
    // Add paragraph to document
    if (not par.empty()) doc.push_back(par);  
    // prepare for next paragraph
    par.clear(); 
  }
  else {
    // tokenize input line into a list of words
    tk->tokenize(text, av);
    // accumulate list of words in splitter buffer, returning a list of sentences.
    sp->split(av, false, ls);
    // add sentece to paragraph
    par.insert(par.end(), ls.begin(), ls.end());
    
    // clear temporary lists;
    av.clear(); ls.clear();
  }
}


//---------------------------------------------
void PostProcessCoref(const string &text, list<word> &av,
                      list<sentence> &ls,
                      paragraph &par, document &doc) {
  // flush splitter buffer  
  sp->split(av, true, ls);
  // add sentece to paragraph
  par.insert(par.end(), ls.begin(), ls.end());
  // add paragraph to document.
  doc.push_back(par);
  
  // Analyze each document paragraph with all required analyzers
  for (document::iterator p=doc.begin(); p!=doc.end(); p++) {
    morfo->analyze(*p);
    tagger->analyze(*p);
    neclass->analyze(*p);
    parser->analyze(*p);
  }
  
  // solve coreference
  corfc->analyze(doc);
  
  // if dependence analysis was requested, do it now (coref solver
  // only works on chunker output, not over complete trees)
  if (dep)
    for (document::iterator p=doc.begin(); p!=doc.end(); p++)
      dep->analyze(*p);
  
  // output results in requested format 
  for (document::iterator par=doc.begin(); par!=doc.end(); par++) 
    WriteResultsDoc(*par, true, doc); 
}

//---------------------------------------------
void ProcessLinePlain(const string &text, unsigned long &offs) {
  list<word> av;
  list<sentence> ls;

  tk->tokenize (text, offs, av);

  if (cfg->OutputFormat == TOKEN) {
    ls.push_back(sentence(av));
    WriteResults(ls,false);
  }
  else {  // OutputFormat >= SPLITTED    
    sp->split (av, cfg->AlwaysFlush, ls);
    AnalyzeSentences(ls);         
    WriteResults (ls,true);
 }
}


//---------------------------------------------
void ProcessLineToken(const string &text, unsigned long &totlen, list<word> &av) {

  // get next word
  word w (text);
  w.set_span (totlen, totlen + text.size ());
  totlen += text.size () + 1;
  av.push_back (w);

  // check for splitting after some words have been accumulated, 
  if (av.size () > 10) {  
    list<sentence> ls;
    sp->split (av, false, ls);
    AnalyzeSentences(ls);    
    WriteResults(ls,true);
    
    av.clear ();		// clear list of words for next use
  }
  else {
    #ifdef SERVER
      SendACK();
    #endif
  }
}

//---------------------------------------------
void ProcessLineSplitted(const string &text, unsigned long &totlen, sentence &av) {
  string form, lemma, tag, sn, spr;
  double prob;

  if (text != "") {  // got a word line
     istringstream sin;
    sin.str (text);
    // get word form
    sin >> form;
    
    // build new word
    word w (form);
    w.set_span (totlen, totlen + form.size ());
    totlen += text.size () + 1;
    
    // process word line, according to input format.
    // add all analysis in line to the word.
    w.clear ();
    if (cfg->InputFormat == MORFO) {
      while (sin >> lemma >> tag >> spr) {
	analysis an (lemma, tag);
	prob = util::string2double (spr);	  
	an.set_prob (prob);
	w.add_analysis (an);
      }
    }
    else if (cfg->InputFormat == SENSES) {
      while (sin >> lemma >> tag >> spr >> sn) {
	analysis an (lemma, tag);
	prob = util::string2double (spr);
	an.set_prob (prob);
	list<string> lpair=util::string2list (sn,"/");
	list<pair<string,double> > lsen;
	for (list<string>::iterator i=lpair.begin(); i!=lpair.end(); i++) {
	  size_t p=i->find(":");
	  lsen.push_back(make_pair(i->substr(0,p),util::string2double(i->substr(p))));
	}
	an.set_senses(lsen);
	w.add_analysis (an);
      }
    }
    else if (cfg->InputFormat == TAGGED) {
      sin >> lemma >> tag;
      analysis an (lemma, tag);
      an.set_prob (1.0);
      w.add_analysis (an);
    }
    
    // append new word to sentence
    av.push_back (w);

    #ifdef SERVER
      SendACK();
    #endif
  }
  else { // blank line, sentence end.
    list<sentence> ls;

    totlen += 2;
    ls.push_back (av);
    
    AnalyzeSentences(ls);      
    WriteResults (ls,true);
    
    av.clear ();   // clear list of words for next use
  }
}  


//---------------------------------------------
// Sample main program
//---------------------------------------------
int main (int argc, char **argv) {
  string text;
  list<word> av;
  sentence sent;
  list<sentence> ls;
  paragraph par;
  document doc;
  unsigned long offs=0;
  
  #ifdef SERVER
    // read server port number
    string sport(argv[1]); 
    istringstream ssa1(sport);
    int server_port; 
    if (!(ssa1>>server_port)) { 
      cerr<<"Wrong port number "<<sport<<endl;
      exit(0);
    }
    // remove port number from argv
    argv++; argc--;  
  #endif

  // read configuration file and command-line options, 
  // and create appropriate analyzers
  CreateAnalyzers(argv);

  #ifdef SERVER
    // open sockets to listen for clients
    cerr<<"SERVER: Analyzers loaded."<<endl;
    sock = new socket_CS(server_port);  // Open socket.
    signal (SIGTERM,terminate);     // Set ending signals capture
    signal (SIGQUIT,terminate);     // to allow clean exits.
  #endif

  bool stop=false;    /// The server version will never stop. 
  while (not stop) {  /// The standalone version will stop after one iteration.

    try {
      #ifdef SERVER
        cerr<<"SERVER: opening channels. Waiting connections"<<endl;
        sock->wait_client();
      #endif

      // --- Main loop: read an process all input lines up to EOF ---
      while (ReadLine(text)) {

        #ifdef SERVER
          start = clock();
	  if (text=="RESET_STATS") { 
	    ResetStats();
	    continue;
	  }
	  else if (text=="PRINT_STATS") {
	    PrintStats();
	    continue;
	  }
        #endif

        if (cfg->UTF8) // input is utf, convert to latin-1
	  text=utf8toLatin(text.c_str());      
	
	if (cfg->COREF_CoreferenceResolution)  // coreference requested, plain text input 
	  ProcessLineCoref(text,av,ls,par,doc);
	
	else if (cfg->InputFormat == PLAIN)    // Input is plain text.
	  ProcessLinePlain(text,offs);
	
	else if (cfg->InputFormat == TOKEN)    // Input is tokenized.
	  ProcessLineToken(text,offs,av);
	
	else if (cfg->InputFormat >= SPLITTED) // Input is (at least) tokenized and splitted.
	  ProcessLineSplitted(text,offs,sent);   
	
      } // --- end while(readline)
      
      // Document has been processed. Perform required post-processing
      // (or just make sure to empty splitter buffers).
      
      #ifdef SERVER
        start = clock();
      #endif
      
      if (cfg->COREF_CoreferenceResolution)   // All document read, solve correferences.
	PostProcessCoref(text,av,ls,par,doc);
    
      else {  // no coreferences, just flush buffers.
	
	if (cfg->InputFormat == PLAIN or cfg->InputFormat == TOKEN) {
	  // flush splitter buffer
	  if (cfg->OutputFormat >= SPLITTED) { 
	    sp->split (av, true, ls);	
	    av.clear();
	  }
	}
	else { // cfg->InputFormat >= SPLITTED.
	  // add last sentence in case it was missing a blank line after it
	  if (!sent.empty()) ls.push_back(sent);
	}
	
	// process last sentence in buffer (if any)
	AnalyzeSentences(ls);
	WriteResults(ls,true);
      }
    }   
    catch( exception& e ) {
      cerr << "analyzer. Caught exception: "<<e.what()<< endl;
    }
    catch (...) { 
      cerr << "analyzer. Caught unknown exception."<<endl;
    }
    
    #ifdef SERVER
      cerr<<"SERVER: client ended. Closing connection."<<endl;
      sock->close_connection();
    #else 
      stop=true;   // if not server version, stop when document is processed
    #endif
  }
  
  // clean up and exit
  cleanup();
}

