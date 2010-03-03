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

#include "errno.h"
#include "signal.h"
#include "sys/stat.h"
#include <fstream>
#include <sstream>
#include <iostream>

#include <map>
#include <vector>

/// headers to call freeling library
#include "freeling.h"

/// config file/options handler for this particular sample application
#include "config.h"
#include "socket.h"


// we use global variables to be able to clean on TERM signal.

// we use pointers to the analyzers, so we
// can create only those strictly necessary.
tokenizer *tk = NULL;
splitter *sp = NULL;
maco *morfo = NULL;
nec *neclass = NULL;
senses *sens = NULL;
disambiguator *dsb = NULL;
POS_tagger *tagger = NULL;
chart_parser *parser = NULL;
dependency_parser *dep = NULL;
coref *corfc = NULL;
// read configuration file and command-line options
config *cfg;
// communicate with clients;
socket_CS *sock;


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
// print obtained analysis.
//---------------------------------------------

void PrintTree (ostringstream &sout, parse_tree::iterator n, int depth, const document &doc=document()) {

  parse_tree::sibling_iterator d;

  sout << string (depth * 2, ' ');
  if (n->num_children () == 0) {
    if (n->info.is_head ()) sout << "+";
    word w = n->info.get_word ();
    sout << "(" << w.get_form () << " " << w.get_lemma () << " " << w.get_parole ();
    sout << OutputSenses ((*w.selected_begin ()));
    sout << ")" << endl;
  }
  else {
    if (n->info.is_head ()) sout << "+";

    sout<<n->info.get_label();
    if (cfg->COREF_CoreferenceResolution) {
      // Print coreference group, if needed.
      int ref = doc.get_coref_group(n->info.get_node_id());
      if (ref != -1 && n->info.get_label() == "sn") sout<<"(REF:" << ref <<")";
    }
    sout << "_[" << endl;

    for (d = n->sibling_begin (); d != n->sibling_end (); ++d) 
      PrintTree (sout, d, depth + 1, doc);
    sout << string (depth * 2, ' ') << "]" << endl;
  }
}



//---------------------------------------------
// print obtained dependence analysis.
//---------------------------------------------
// void PrintDepTree(const dep_tree &tr, dep_tree::iterator n, int depth, const config &cfg) {
//   dep_tree::sibling_iterator d;

//     cout<<string(depth*2,' '); 

//     if(n->is_chunk()) { cout<<n->get_link()->get_label()<<"_";}

//     word w=n->get_word();
//     cout<<"("<<w.get_form()<<" "<<w.get_lemma()<<" "<<w.get_parole();
//     OutputSenses(w,cfg);
//     cout<<")";  

//     if (n.number_of_children()>0) { 
//        cout<<" ["<<endl; 
//        for (d=tr.begin(n); d!=tr.end(n); d++)
//       PrintDepTree(tr, d, depth+1, cfg);
//        cout<<string(depth*2,' ')<<"]"; 
//     }
//     cout<<endl;
// }


void PrintDepTree (ostringstream &sout, dep_tree::iterator n, int depth, const document &doc=document()) {
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
  sout << "(" << w.get_form() << " " << w.get_lemma() << " " << w.get_parole ();
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
	      && d->info.get_chunk_ord () < min) {
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
// print obtained analysis.
//---------------------------------------------

void PrintResults (list <sentence> &ls, const document &doc=document() ) {
  word::const_iterator ait;
  sentence::const_iterator w;
  list < sentence >::iterator is;
  int nsentence = 1;

  if (ls.empty()) {
    sock->write_message("FL-SERVER-READY");  
    return;
  }

  ostringstream sout;
  for (is = ls.begin (); is != ls.end (); is++, ++nsentence) {

    if (cfg->OutputFormat >= PARSED) {
      /// obtain parse tree and draw it at will
      switch (cfg->OutputFormat) {
	
        case PARSED: {
	   parse_tree & tr = is->get_parse_tree ();
	   PrintTree (sout, tr.begin (), 0, doc);
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
	sout << w->get_form ();
	
	if (cfg->OutputFormat == MORFO || cfg->OutputFormat == TAGGED) {
	  for (ait = w->selected_begin (); ait != w->selected_end (); ait++) {
	    if (ait->is_retokenizable ()) {
	      list < word > rtk = ait->get_retokenizable ();
	      list < word >::iterator r;
	      string lem, par;
	      for (r = rtk.begin (); r != rtk.end (); r++) {
		lem = lem + "+" + r->get_lemma ();
		par = par + "+" + r->get_parole ();
	      }
	      sout << " " << lem.substr (1) << " " 
		   << par.substr (1) << " " << ait->get_prob ();
	    }
	    else {
	      sout << " " << ait->get_lemma () << " " << ait->
		get_parole () << " " << ait->get_prob ();
	    }
	    if (cfg->SENSE_SenseAnnotation != NONE)
	      sout << OutputSenses (*ait);
	  }
	}
	sout << endl;	
      }
    } 
    // sentence separator: blank line.
    sout << endl;	
  }

  //cout<<"RESPONSE:"<<sout.str()<<endl;
  sock->write_message(sout.str());
}


//---------------------------------------------
// Apply analyzer cascade to sentences in given list
//---------------------------------------------

void AnalyzeSentences(list<sentence> &ls) {
  if (cfg->OutputFormat >= MORFO)
    morfo->analyze (ls);
  if (cfg->OutputFormat >= MORFO and 
      (cfg->SENSE_SenseAnnotation == MFS or cfg->SENSE_SenseAnnotation == ALL))
    sens->analyze (ls);
  if (cfg->OutputFormat >= TAGGED)
    tagger->analyze (ls);
  if (cfg->OutputFormat >= TAGGED and (cfg->SENSE_SenseAnnotation == UKB))  
    dsb->analyze (ls);
  if (cfg->OutputFormat >= TAGGED and cfg->NEC_NEClassification)
    neclass->analyze (ls);
  if (cfg->OutputFormat >= PARSED)
    parser->analyze (ls);
  if (cfg->OutputFormat >= DEP)
    dep->analyze (ls);
}


//---------------------------------------------
// Coreference resolution. Input is plain text, whole document.
//---------------------------------------------
void ProcessCoreference () {

  string text;
  vector < word > av;
  list < sentence > ls;
  paragraph par;
  document doc;

  // get plain text input lines while not EOF.
  while (sock->read_message(text)) {
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
      tk->tokenize(text,av);
      // accumulate list of words in splitter buffer, returning a list of sentences.
      sp->split(av, false, ls);
      // add sentece to paragraph
      par.insert(par.end(), ls.begin(), ls.end());

      // clear temporary lists;
      av.clear(); ls.clear();
    }
  }
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
    PrintResults(*par, doc);

}


//---------------------------------------------
// Plain text input, incremental processing
//---------------------------------------------
void ProcessPlain (double &cpuTime_total, int &sentences, int &words){

  string text;
  unsigned long offs = 0;
  vector < word > av;
  list < sentence > ls;

  while (sock->read_message(text)>0) {
    //cout<<"processing "<<text<<endl;
    clock_t start = clock();
    //cout << "start" << start << endl;
    if (text=="RESET_STATS") {  // resetting command
      cpuTime_total=0.0;
      sentences=0;
      words=0;
      sock->write_message("FL-SERVER-READY");  
      continue;
    }
    else if (text=="PRINT_STATS") { // print_stats command
      ostringstream sout;
      sout << "Words: "<<words<<", sentences: "<<sentences<<", cpuTime_total: "<<cpuTime_total<<endl;
      sout << "Words/sentence: "<<(sentences>0?words/sentences:0)<<", words/second: "<<(cpuTime_total>0?words/cpuTime_total:0)<<", sentences/second: "<<(cpuTime_total>0?sentences/cpuTime_total:0)<<endl;
      sock->write_message(sout.str());
      continue;
    }
    
    if (cfg->OutputFormat >= TOKEN)
      tk->tokenize (text, offs, av);
    if (cfg->OutputFormat >= SPLITTED)
      sp->split (av, cfg->AlwaysFlush, ls);
    
    AnalyzeSentences(ls);
    
    // update stats
    sentences+=ls.size();          
    list<sentence>::iterator is;
    for (is=ls.begin(); is!=ls.end(); is++) {
      sentence se=*is;
      words+=se.size();
    }

    PrintResults (ls);

    clock_t end = clock();
    cpuTime_total += (end-(double)start)/(CLOCKS_PER_SEC);            
    
    av.clear ();		// clear list of words for next use
    ls.clear ();		// clear list of sentences for next use
  }
  
  clock_t start = clock();

  //flush splitter buffer
  if (cfg->OutputFormat >= SPLITTED) 
    sp->split (av, true, ls);

  // process last sentence in buffer (if any)
  AnalyzeSentences(ls);

  // update stats
  sentences+=ls.size();          
  list<sentence>::iterator is;
  for (is=ls.begin(); is!=ls.end(); is++) {
    sentence se=*is;
    words+=se.size();
  }

  // Output results
  PrintResults (ls);

  clock_t end = clock();
  cpuTime_total += (end-(double)start)/(CLOCKS_PER_SEC);            

}


//----------------------------------------------
// Process already tokenized text.
// This sample program expects one token per line.
//----------------------------------------------
void ProcessToken () {
  string text;
  vector < word > av;
  list < sentence > ls;
  unsigned long totlen = 0;

  while (sock->read_message(text)) {
    // get next word
    word w (text);
    w.set_span (totlen, totlen + text.size ());
    totlen += text.size () + 1;
    av.push_back (w);
    
    // check for splitting after some words have been accumulated, 
    if (av.size () > 10)	{
      if (cfg->OutputFormat >= SPLITTED) sp->split (av, false, ls);

      AnalyzeSentences(ls);
      
      PrintResults (ls);
      
      av.clear ();		// clear list of words for next use
      ls.clear ();		// clear list of sentences for next use
    }
  }
  
  //flush splitter buffer
  if (cfg->OutputFormat >= SPLITTED) sp->split (av, true, ls);	
  // process last sentence in buffer (if any)
  AnalyzeSentences(ls);

  PrintResults (ls);
}


//-------------------------------------------------------
// Process already tokenized and splitted (and maybe more things)
// This sample program expects one blank line between 
// sentences and one token per line with format:
//     word lemma1 tag1 lemma2 tag2 ...
// The list of (lemma,tag) pairs may be:
//   - empty (just splitted input)
//   - of variable length (morfological analysis)
//   - of length 1 (tagged)
//---------------------------------------------
void ProcessSplitted () {
  string text, form, lemma, tag, sn, spr;
  sentence av;
  double prob;
  list < sentence > ls;
  unsigned long totlen = 0;

  while (sock->read_message(text))
    {
      if (text != "")
	{			// got a word line
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
	  if (cfg->InputFormat == MORFO)
	    {
	      while (sin >> lemma >> tag >> spr)
		{
		  analysis an (lemma, tag);
		  prob = util::string2double (spr);
		  an.set_prob (prob);
		  w.add_analysis (an);
		}
	    }
	  else if (cfg->InputFormat == SENSES)
	    {
	      while (sin >> lemma >> tag >> spr >> sn)
		{
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
	  else if (cfg->InputFormat == TAGGED)
	    {
	      sin >> lemma >> tag;
	      analysis an (lemma, tag);
	      an.set_prob (1.0);
	      w.add_analysis (an);
	    }

	  // append new word to sentence
	  av.push_back (w);
	}
      else
	{			// blank line, sentence end.
	  totlen += 2;

	  ls.push_back (av);

	  AnalyzeSentences(ls);

	  PrintResults (ls);

	  av.clear ();		// clear list of words for next use
	  ls.clear ();		// clear list of sentences for next use
	}
    }

  // process last sentence in buffer (if any)
  ls.push_back (av);		// last sentence (may not have blank line after it)

  AnalyzeSentences(ls);

  PrintResults (ls);
}



//---------------------------------------------
// Capture signal to terminate process
//---------------------------------------------
void terminate (int param)
{

  // --- shutting down server
  cerr<<"SERVER: Signal received. Stopping"<<endl;

  // clean up. Note that deleting a null pointer is a safe (yet useless) operation
  delete tk;
  delete sp;
  delete morfo;
  delete tagger;
  delete neclass;
  delete sens;
  delete parser;
  delete dep;
  delete corfc;

  delete cfg;

  // terminate
  exit(1);
}


//---------------------------------------------
// Sample main program
//---------------------------------------------
int main (int argc, char **argv) {

  // read server port number
  string sport(argv[1]); 
  istringstream ssa1(sport);
  int server_port; 
  if (!(ssa1>>server_port)) { 
    cerr<<"Wrong port number "<<sport<<endl;
    exit(0);
  }

  // read config options
  argv++; argc--;
  cfg = new config(argv);

  if (!((cfg->InputFormat < cfg->OutputFormat) ||
	(cfg->InputFormat == cfg->OutputFormat && cfg->InputFormat == TAGGED
	 && cfg->NEC_NEClassification)))
    {
      cerr <<"Error - Input format cannot be more complex than desired output."<<endl;
      exit (1);
    }

  if (cfg->COREF_CoreferenceResolution && cfg->OutputFormat<=TAGGED) {
    cerr <<"Error - Requested coreference resolution is only compatible with output format 'parsed' or 'dep'." <<endl;
    exit (1);
  }

  if (cfg->OutputFormat < TAGGED and (cfg->SENSE_SenseAnnotation == UKB))   {
    cerr <<"Error - UKB word sense disambiguation requires PoS tagging. Specify 'tagged', 'parsed' or 'dep' output format." <<endl;
    exit (1);
  }
  //--- create needed analyzers, depending on given options ---//

  // tokenizer requested
  if (cfg->InputFormat < TOKEN && cfg->OutputFormat >= TOKEN)
    tk = new tokenizer (cfg->TOK_TokenizerFile);
  // splitter requested
  if (cfg->InputFormat < SPLITTED && cfg->OutputFormat >= SPLITTED)
    sp = new splitter (cfg->SPLIT_SplitterFile);

  // morfological analysis requested
  if (cfg->InputFormat < MORFO && cfg->OutputFormat >= MORFO) {
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
			  cfg->MACO_PunctuationFile, cfg->MACO_CorrectorFile);

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
  if (cfg->InputFormat < TAGGED && cfg->OutputFormat >= TAGGED) {
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
  if (cfg->InputFormat <= TAGGED && cfg->OutputFormat >= TAGGED && 
          (cfg->NEC_NEClassification || cfg->COREF_CoreferenceResolution)) {
      neclass = new nec ("NP", cfg->NEC_FilePrefix);
  }

  // Chunking requested
  if (cfg->InputFormat < PARSED && (cfg->OutputFormat >= PARSED || cfg->COREF_CoreferenceResolution)) {
      parser = new chart_parser (cfg->PARSER_GrammarFile);
  }

  // Dependency parsing requested
  if (cfg->InputFormat < PARSED && cfg->OutputFormat >= DEP) 
    dep = new dep_txala (cfg->DEP_TxalaFile, parser->get_start_symbol ());

  cerr<<"SERVER: Analyzers loaded."<<endl;

  // crear socket
  sock = new socket_CS(server_port);

  double cpuTime_total=0.0;
  int sentences=0;
  int words=0;

  // serve client requests until server is killed
  while (true) {
          
    //cerr<<"SERVER: opening channels. Waiting for new connection"<<endl;
    sock->wait_client();
    //cerr<<"New client connected"<<endl;
    
    //--- get and process client input up to EOF ---

    // coreference requested, plain text input
    if (cfg->COREF_CoreferenceResolution) {
      int vectors = COREFEX_DIST | COREFEX_IPRON | COREFEX_JPRON | COREFEX_IPRONM | COREFEX_JPRONM
	| COREFEX_STRMATCH | COREFEX_DEFNP | COREFEX_DEMNP | COREFEX_GENDER | COREFEX_NUMBER
	| COREFEX_SEMCLASS | COREFEX_PROPNAME | COREFEX_ALIAS | COREFEX_APPOS;
      corfc = new coref(cfg->COREF_CorefFile, vectors);
      ProcessCoreference ();
    } 
    // Input is plain text.
    else if (cfg->InputFormat == PLAIN)
      ProcessPlain (cpuTime_total, sentences, words);
    // Input is tokenized. 
    else if (cfg->InputFormat == TOKEN)
      ProcessToken ();
    // Input is (at least) tokenized and splitted.
    else if (cfg->InputFormat >= SPLITTED)
      ProcessSplitted ();
    
    //cerr<<"SERVER: client ended. Closing connection."<<endl;
    sock->close_connection();

  }

}
