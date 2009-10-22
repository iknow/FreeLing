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

void OutputSenses (const analysis & a, const config &cfg) {
  list<pair<string,double> > ls = a.get_senses ();
  if (ls.size () > 0) {
    if (cfg.SENSE_SenseAnnotation == MFS)
      cout << " " << ls.begin()->first;
    else  // ALL or UKB specified
      cout << " " << util::pairlist2string (ls, ":", "/");
  }
  else
    cout << " -";
}

//---------------------------------------------
// print obtained analysis.
//---------------------------------------------
void PrintTree (parse_tree::iterator n, int depth, const config &cfg, const document &doc=document()) {
  parse_tree::sibling_iterator d;

  cout << string (depth * 2, ' ');
  if (n->num_children () == 0) {
    if (n->info.is_head ()) cout << "+";
    word w = n->info.get_word ();
    cout << "(" << w.get_form () << " " << w.get_lemma () << " " << w.get_parole ();
    OutputSenses ((*w.selected_begin ()), cfg);
    cout << ")" << endl;
  }
  else {
    if (n->info.is_head ()) cout << "+";

    cout<<n->info.get_label();
    if (cfg.COREF_CoreferenceResolution) {
      // Print coreference group, if needed.
      int ref = doc.get_coref_group(n->info.get_node_id());
      if (ref != -1 && n->info.get_label() == "sn") cout<<"(REF:" << ref <<")";
    }
    cout << "_[" << endl;

    for (d = n->sibling_begin (); d != n->sibling_end (); ++d) 
      PrintTree (d, depth + 1, cfg, doc);
    cout << string (depth * 2, ' ') << "]" << endl;
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


void PrintDepTree (dep_tree::iterator n, int depth, const config & cfg, const document &doc=document()) {
  dep_tree::sibling_iterator d, dm;
  int last, min, ref;
  bool trob;

  cout << string (depth*2, ' ');

  parse_tree::iterator pn = n->info.get_link();
  cout<<pn->info.get_label(); 
  ref = (cfg.COREF_CoreferenceResolution ? doc.get_coref_group(pn->info.get_node_id()) : -1);
  if (ref != -1 and pn->info.get_label() == "sn") {
    cout<<"(REF:" << ref <<")";
  }
  cout<<"/" << n->info.get_label() << "/";

  word w = n->info.get_word();
  cout << "(" << w.get_form() << " " << w.get_lemma() << " " << w.get_parole ();
  OutputSenses ((*w.selected_begin()), cfg);
  cout << ")";
  
  if (n->num_children () > 0) {
    cout << " [" << endl;
    
    // Print Nodes
    for (d = n->sibling_begin (); d != n->sibling_end (); ++d)
      if (!d->info.is_chunk ())
	PrintDepTree (d, depth + 1, cfg, doc);
    
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
	PrintDepTree (dm, depth + 1, cfg, doc);
      last = min;
    }
    
    cout << string (depth * 2, ' ') << "]";
  }
  cout << endl;
}


//---------------------------------------------
// print obtained analysis.
//---------------------------------------------

void PrintResults (list < sentence > &ls, const config & cfg, const document &doc=document() ) {
  word::const_iterator ait;
  sentence::const_iterator w;
  list < sentence >::iterator is;
  int nsentence = 1;
  
  
  for (is = ls.begin (); is != ls.end (); is++, ++nsentence) {

    if (cfg.OutputFormat >= PARSED) {
      /// obtain parse tree and draw it at will
      switch (cfg.OutputFormat) {
	
        case PARSED: {
	  parse_tree & tr = is->get_parse_tree ();
	  PrintTree (tr.begin (), 0, cfg, doc);
	  cout << endl;
	  }
	  break;
	
        case DEP: {
	  dep_tree & dep = is->get_dep_tree ();
	  PrintDepTree (dep.begin (), 0, cfg, doc);
          }
	  break;
	
        default:   // should never happen
	  break;
      }
    }
    else {
      for (w = is->begin (); w != is->end (); w++) {
	cout << w->get_form ();
	
	if (cfg.OutputFormat == MORFO || cfg.OutputFormat == TAGGED) {
	  //      for(ait=w->analysis_begin(); ait!=w->analysis_end(); ait++){
	  for (ait = w->selected_begin (); ait != w->selected_end (); ait++) {
	    if (ait->is_retokenizable ()) {
	      list < word > rtk = ait->get_retokenizable ();
	      list < word >::iterator r;
	      string lem, par;
	      for (r = rtk.begin (); r != rtk.end (); r++) {
		lem = lem + "+" + r->get_lemma ();
		par = par + "+" + r->get_parole ();
	      }
	      cout << " " << lem.substr (1) << " " 
		   << par.substr (1) << " " << ait->get_prob ();
	    }
	    else {
	      cout << " " << ait->get_lemma () << " " << ait->
		get_parole () << " " << ait->get_prob ();
	    }
	    if (cfg.SENSE_SenseAnnotation != NONE)
	      OutputSenses (*ait, cfg);
	  }
	}
	cout << endl;	
      }
    }
    // sentence separator: blank line.
    cout << endl;
  }
}


//---------------------------------------------
// Coreference resolution. Input is plain text, whole document.
//---------------------------------------------
void ProcessCoreference (const config & cfg, tokenizer * tk, splitter * sp, maco * morfo,
	      POS_tagger * tagger, nec * neclass, senses * sens,
	      chart_parser * parser, dependency_parser * dep, coref *corfc)
{
  string text;
  list < word > av;
  list < word >::const_iterator i;
  list < sentence > ls;
  paragraph par;
  document doc;

  // get plain text input lines while not EOF.
  while (getline(cin,text)) {
    if (text=="") { // new paragraph.
      // flush buffer
      ls=sp->split(av, true);
      // add sentece to paragraph
      par.insert(par.end(), ls.begin(), ls.end());  
      // Add paragraph to document
      if (not par.empty()) doc.push_back(par);  
      // prepare for next paragraph
      par.clear(); 
    }
    else {
      // tokenize input line into a list of words
      av=tk->tokenize(text);
      // accumulate list of words in splitter buffer, returning a list of sentences.
      ls=sp->split(av, false);
      // add sentece to paragraph
      par.insert(par.end(), ls.begin(), ls.end());

      // clear temporary lists;
      av.clear(); ls.clear();
    }
  }
  // flush splitter buffer  
  ls=sp->split(av, true);
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
    PrintResults(*par, cfg, doc);

}


//---------------------------------------------
// Plain text input, incremental processing
//---------------------------------------------
void ProcessPlain (const config & cfg, tokenizer * tk, splitter * sp, maco * morfo,
	      POS_tagger * tagger, nec * neclass, senses * sens, disambiguator * dsb,
	      chart_parser * parser, dependency_parser * dep)
{
  string text;
  unsigned long offs = 0;
  list < word > av;
  list < word >::const_iterator i;
  list < sentence > ls;

  while (std::getline (std::cin, text))
    {
      if (cfg.OutputFormat >= TOKEN)
	av = tk->tokenize (text, offs);
      if (cfg.OutputFormat >= SPLITTED)
	ls = sp->split (av, cfg.AlwaysFlush);
      if (cfg.OutputFormat >= MORFO)
	morfo->analyze (ls);
      if (cfg.OutputFormat >= MORFO and 
	  (cfg.SENSE_SenseAnnotation == MFS or cfg.SENSE_SenseAnnotation == ALL))
	sens->analyze (ls);
      if (cfg.OutputFormat >= TAGGED)
	tagger->analyze (ls);
      if (cfg.OutputFormat >= TAGGED and (cfg.SENSE_SenseAnnotation == UKB))  
	dsb->analyze (ls);
      
      if (cfg.OutputFormat >= TAGGED and cfg.NEC_NEClassification)
	neclass->analyze (ls);
      if (cfg.OutputFormat >= PARSED)
	parser->analyze (ls);
      if (cfg.OutputFormat >= DEP) 
	dep->analyze (ls);

      if (cfg.OutputFormat == TOKEN) {
	// if only tokenizing, just print one token per line
	for (i = av.begin (); i != av.end (); i++)
	  cout << i->get_form () << endl;
      }
      else {
	// if higher processing performed, print sentences separed by blank lines.
	PrintResults (ls, cfg);
      }
      
      av.clear ();		// clear list of words for next use
      ls.clear ();		// clear list of sentences for next use
    }

  // process last sentence in buffer (if any)
  if (cfg.OutputFormat >= SPLITTED)
    ls = sp->split (av, true);	//flush splitter buffer
  if (cfg.OutputFormat >= MORFO)
    morfo->analyze (ls);
  if (cfg.OutputFormat >= MORFO and 
      (cfg.SENSE_SenseAnnotation == MFS or cfg.SENSE_SenseAnnotation == ALL))
    sens->analyze (ls);
  if (cfg.OutputFormat >= TAGGED)
    tagger->analyze (ls);
  if (cfg.OutputFormat >= TAGGED and (cfg.SENSE_SenseAnnotation == UKB))  
    dsb->analyze (ls);
  if (cfg.OutputFormat >= TAGGED and cfg.NEC_NEClassification)
    neclass->analyze (ls);
  if (cfg.OutputFormat >= PARSED)
    parser->analyze (ls);
  if (cfg.OutputFormat >= DEP)
    dep->analyze (ls);

  // if higher processing performed, print sentences separed by blank lines.
  PrintResults (ls, cfg);
}


//----------------------------------------------
// Process already tokenized text.
// This sample program expects one token per line.
//----------------------------------------------
void
ProcessToken (const config & cfg, splitter * sp, maco * morfo,
	      POS_tagger * tagger, nec * neclass, senses * sens,
	      disambiguator * dsb, chart_parser * parser, dependency_parser * dep)
{
  string text;
  list < word > av;
  list < sentence > ls;
  unsigned long totlen = 0;

  while (std::getline (std::cin, text))
    {
      // get next word
      word w (text);
      w.set_span (totlen, totlen + text.size ());
      totlen += text.size () + 1;
      av.push_back (w);

      // check for splitting after some words have been accumulated, 
      if (av.size () > 10)
	{

	  if (cfg.OutputFormat >= SPLITTED)
	    ls = sp->split (av, false);
	  if (cfg.OutputFormat >= MORFO)
	    morfo->analyze (ls);
	  if (cfg.OutputFormat >= MORFO and 
	      (cfg.SENSE_SenseAnnotation == MFS or cfg.SENSE_SenseAnnotation == ALL))
	    sens->analyze (ls);
	  if (cfg.OutputFormat >= TAGGED)
	    tagger->analyze (ls);
	  if (cfg.OutputFormat >= TAGGED and (cfg.SENSE_SenseAnnotation == UKB))  
	    dsb->analyze (ls);
	  if (cfg.OutputFormat >= TAGGED and cfg.NEC_NEClassification)
	    neclass->analyze (ls);
	  if (cfg.OutputFormat >= PARSED)
	    parser->analyze (ls);
	  if (cfg.OutputFormat >= DEP) 
	    dep->analyze (ls);

	  PrintResults (ls, cfg);

	  av.clear ();		// clear list of words for next use
	  ls.clear ();		// clear list of sentences for next use
	}
    }

  // process last sentence in buffer (if any)
  if (cfg.OutputFormat >= SPLITTED)
    ls = sp->split (av, true);	//flush splitter buffer
  if (cfg.OutputFormat >= MORFO)
    morfo->analyze (ls);
  if (cfg.OutputFormat >= MORFO and 
      (cfg.SENSE_SenseAnnotation == MFS or cfg.SENSE_SenseAnnotation == ALL))
    sens->analyze (ls);
  if (cfg.OutputFormat >= TAGGED)
    tagger->analyze (ls);
  if (cfg.OutputFormat >= TAGGED and (cfg.SENSE_SenseAnnotation == UKB))  
    dsb->analyze (ls);
  if (cfg.OutputFormat >= TAGGED and cfg.NEC_NEClassification)
    neclass->analyze (ls);
  if (cfg.OutputFormat >= PARSED)
    parser->analyze (ls);
  if (cfg.OutputFormat >= DEP) 
    dep->analyze (ls);

  PrintResults (ls, cfg);
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
void
ProcessSplitted (const config & cfg, maco * morfo, POS_tagger * tagger,
		 nec * neclass, senses * sens, disambiguator * dsb, 
		 chart_parser * parser, dependency_parser * dep)
{
  string text, form, lemma, tag, sn, spr;
  sentence av;
  double prob;
  list < sentence > ls;
  unsigned long totlen = 0;

  while (std::getline (std::cin, text))
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
	  if (cfg.InputFormat == MORFO)
	    {
	      while (sin >> lemma >> tag >> spr)
		{
		  analysis an (lemma, tag);
		  prob = util::string2double (spr);
		  an.set_prob (prob);
		  w.add_analysis (an);
		}
	    }
	  else if (cfg.InputFormat == SENSES)
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
	  else if (cfg.InputFormat == TAGGED)
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

	  if (cfg.InputFormat < MORFO and cfg.OutputFormat >= MORFO)
	    morfo->analyze (ls);
	  if (cfg.InputFormat < SENSE and cfg.OutputFormat >= MORFO and 
	      (cfg.SENSE_SenseAnnotation == MFS or cfg.SENSE_SenseAnnotation == ALL))
	    sens->analyze (ls);
	  if (cfg.InputFormat < TAGGED and cfg.OutputFormat >= TAGGED)
	    tagger->analyze (ls);
	  if (cfg.OutputFormat >= TAGGED and (cfg.SENSE_SenseAnnotation == UKB))  
	    dsb->analyze (ls);
	  if (cfg.InputFormat <= TAGGED and cfg.OutputFormat >= TAGGED
	      and cfg.NEC_NEClassification)
	    neclass->analyze (ls);
	  if (cfg.InputFormat < PARSED and cfg.OutputFormat >= PARSED)
	    parser->analyze (ls);
	  if (cfg.InputFormat < DEP and cfg.OutputFormat >= DEP)
	    dep->analyze (ls);

	  PrintResults (ls, cfg);

	  av.clear ();		// clear list of words for next use
	  ls.clear ();		// clear list of sentences for next use
	}
    }

  // process last sentence in buffer (if any)
  ls.push_back (av);		// last sentence (may not have blank line after it)

  if (cfg.InputFormat < MORFO and cfg.OutputFormat >= MORFO)
    morfo->analyze (ls);
  if (cfg.InputFormat < SENSE and cfg.OutputFormat >= MORFO and 
      (cfg.SENSE_SenseAnnotation == MFS or cfg.SENSE_SenseAnnotation == ALL))
    sens->analyze (ls);
  if (cfg.InputFormat < TAGGED and cfg.OutputFormat >= TAGGED)
    tagger->analyze (ls);
  if (cfg.OutputFormat >= TAGGED and (cfg.SENSE_SenseAnnotation == UKB))  
    dsb->analyze (ls);
  if (cfg.InputFormat <= TAGGED and cfg.OutputFormat >= TAGGED
      and cfg.NEC_NEClassification)
    neclass->analyze (ls);
  if (cfg.InputFormat < PARSED and cfg.OutputFormat >= PARSED)
    parser->analyze (ls);
  if (cfg.InputFormat < DEP and cfg.OutputFormat >= DEP)
    dep->analyze (ls);

  PrintResults (ls, cfg);
}


//---------------------------------------------
// Sample main program
//---------------------------------------------
int main (int argc, char **argv) {

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

  // use first parameter as the name for the named pipe
  string FIFO_in = string(argv[1])+".in";
  string FIFO_out = string(argv[1])+".out";

  argv++; argc--;   // use the rest of parameters normally

  // read configuration file and command-line options
  config cfg (argv);

  if (!((cfg.InputFormat < cfg.OutputFormat) ||
	(cfg.InputFormat == cfg.OutputFormat && cfg.InputFormat == TAGGED
	 && cfg.NEC_NEClassification)))
    {
      cerr <<"Error - Input format cannot be more complex than desired output."<<endl;
      exit (1);
    }

  if (cfg.COREF_CoreferenceResolution && cfg.OutputFormat<=TAGGED) {
    cerr <<"Error - Requested coreference resolution is only compatible with output format 'parsed' or 'dep'." <<endl;
    exit (1);
  }

  if (cfg.OutputFormat < TAGGED and (cfg.SENSE_SenseAnnotation == UKB))   {
    cerr <<"Error - UKB word sense disambiguation requires PoS tagging. Specify 'tagged', 'parsed' or 'dep' output format." <<endl;
    exit (1);
  }
  //--- create needed analyzers, depending on given options ---//

  // tokenizer requested
  if (cfg.InputFormat < TOKEN && cfg.OutputFormat >= TOKEN)
    tk = new tokenizer (cfg.TOK_TokenizerFile);
  // splitter requested
  if (cfg.InputFormat < SPLITTED && cfg.OutputFormat >= SPLITTED)
    sp = new splitter (cfg.SPLIT_SplitterFile);

  // morfological analysis requested
  if (cfg.InputFormat < MORFO && cfg.OutputFormat >= MORFO) {
      // the morfo class requires several options at creation time.
      // they are passed packed in a maco_options object.
      maco_options opt (cfg.Lang);
      // boolean options to activate/desactivate modules
      // default: all modules deactivated (options set to "false")
      opt.set_active_modules (cfg.MACO_AffixAnalysis,
			      cfg.MACO_MultiwordsDetection,
			      cfg.MACO_NumbersDetection,
			      cfg.MACO_PunctuationDetection,
			      cfg.MACO_DatesDetection,
			      cfg.MACO_QuantitiesDetection,
			      cfg.MACO_DictionarySearch,
			      cfg.MACO_ProbabilityAssignment,
			      cfg.MACO_NER_which,
			      cfg.MACO_OrthographicCorrection);
      // decimal/thousand separators used by number detection
      opt.set_nummerical_points (cfg.MACO_Decimal, cfg.MACO_Thousand);
      // Minimum probability for a tag for an unkown word
      opt.set_threshold (cfg.MACO_ProbabilityThreshold);
      // Data files for morphological submodules. by default set to ""
      // Only files for active modules have to be specified 
      opt.set_data_files (cfg.MACO_LocutionsFile, cfg.MACO_QuantitiesFile,
			  cfg.MACO_AffixFile, cfg.MACO_ProbabilityFile,
			  cfg.MACO_DictionaryFile, cfg.MACO_NPdataFile,
			  cfg.MACO_PunctuationFile, cfg.MACO_CorrectorFile);

      // create analyzer with desired options
      morfo = new maco (opt);
  }

  // sense annotation requested
  if (cfg.InputFormat < SENSE and cfg.OutputFormat >= MORFO
      and (cfg.SENSE_SenseAnnotation == MFS or cfg.SENSE_SenseAnnotation == ALL))
    sens = new senses (cfg.SENSE_SenseFile, cfg.SENSE_DuplicateAnalysis);
  else if (cfg.InputFormat < SENSE and cfg.OutputFormat >= TAGGED
      and (cfg.SENSE_SenseAnnotation == UKB))      
    dsb = new disambiguator (cfg.UKB_BinFile, cfg.UKB_DictFile, cfg.UKB_Epsilon, cfg.UKB_MaxIter);

  // tagger requested, see which method
  if (cfg.InputFormat < TAGGED && cfg.OutputFormat >= TAGGED) {
      if (cfg.TAGGER_which == HMM)
	tagger =
	  new hmm_tagger (cfg.Lang, cfg.TAGGER_HMMFile, cfg.TAGGER_Retokenize,
			  cfg.TAGGER_ForceSelect);
      else if (cfg.TAGGER_which == RELAX)
	tagger =
	  new relax_tagger (cfg.TAGGER_RelaxFile, cfg.TAGGER_RelaxMaxIter,
			    cfg.TAGGER_RelaxScaleFactor,
			    cfg.TAGGER_RelaxEpsilon, cfg.TAGGER_Retokenize,
			    cfg.TAGGER_ForceSelect);
  }

  // NEC requested
  if (cfg.InputFormat <= TAGGED && cfg.OutputFormat >= TAGGED && 
          (cfg.NEC_NEClassification || cfg.COREF_CoreferenceResolution)) {
      neclass = new nec ("NP", cfg.NEC_FilePrefix);
  }

  // Chunking requested
  if (cfg.InputFormat < PARSED && (cfg.OutputFormat >= PARSED || cfg.COREF_CoreferenceResolution)) {
      parser = new chart_parser (cfg.PARSER_GrammarFile);
  }

  // Dependency parsing requested
  if (cfg.InputFormat < PARSED && cfg.OutputFormat >= DEP) 
    dep = new dep_txala (cfg.DEP_TxalaFile, parser->get_start_symbol ());

  cerr<<"SERVER: Analyzers loaded."<<endl;

  // create named pipe for input and output
  if (mkfifo(FIFO_in.c_str(),0666)) {
    switch (errno) {
    case EEXIST: cerr<<"SERVER: Named pipe "<<FIFO_in<<" already exists."<<endl;
                   cerr<<"        If no other server is using it, remove the file and try again."<<endl;
                   exit(1); break;
      default: cerr<<"Error creating named pipe "<<FIFO_in<<endl;
               exit(1);
    }
  }

  if (mkfifo(FIFO_out.c_str(),0666)) {
    switch (errno) {
    case EEXIST: cerr<<"SERVER: Named pipe "<<FIFO_out<<" already exists."<<endl;
                   cerr<<"        If no other server is using it, remove the file and try again."<<endl;
                   exit(1); break;
      default: cerr<<"Error creating named pipe "<<FIFO_out<<endl;
               exit(1);
    }
  }

  cerr<<"SERVER: Pipes "<<FIFO_in<<" and "<<FIFO_out<<" created."<<endl;

  // serve client requests until server is killed
  while (1) {
          
    cerr<<"SERVER: opening channels. Waiting for new connection"<<endl;
    
    // --- open named pipes for a new client-server interaction

    // open input pipe and connect it to cin
    ifstream fin(FIFO_in.c_str());
    streambuf *inbkp=cin.rdbuf();
    streambuf *newin=fin.rdbuf();
    cin.rdbuf(newin);

    // open output pipe and connect it to cout
    ofstream fout(FIFO_out.c_str());
    streambuf *outbkp=cout.rdbuf();
    streambuf *newout=fout.rdbuf();
    cout.rdbuf(newout);

    //--- get and process client input up to EOF ---

    // coreference requested, plain text input
    if (cfg.COREF_CoreferenceResolution) {
      int vectors = COREFEX_DIST | COREFEX_IPRON | COREFEX_JPRON | COREFEX_IPRONM | COREFEX_JPRONM
	| COREFEX_STRMATCH | COREFEX_DEFNP | COREFEX_DEMNP | COREFEX_GENDER | COREFEX_NUMBER
	| COREFEX_SEMCLASS | COREFEX_PROPNAME | COREFEX_ALIAS | COREFEX_APPOS;
      corfc = new coref(cfg.COREF_CorefFile, vectors);
      ProcessCoreference (cfg, tk, sp, morfo, tagger, neclass, sens, parser, dep, corfc);
    } 
    // Input is plain text.
    else if (cfg.InputFormat == PLAIN)
      ProcessPlain (cfg, tk, sp, morfo, tagger, neclass, sens, dsb, parser, dep);
    // Input is tokenized. 
    else if (cfg.InputFormat == TOKEN)
      ProcessToken (cfg, sp, morfo, tagger, neclass, sens, dsb, parser, dep);
    // Input is (at least) tokenized and splitted.
    else if (cfg.InputFormat >= SPLITTED)
      ProcessSplitted (cfg, morfo, tagger, neclass, sens, dsb, parser, dep);
    
    cerr<<"SERVER: client ended. Closing connection."<<endl;

    // --- Client sent EOF, end interaction.
    cin.rdbuf(inbkp); fin.close(); 
    cout.rdbuf(outbkp); fout.close(); 
  }

  // Actually, we'll never reach here, but a fine server should trap 
  // the signals and clean up before exiting.

  // --- shutting down server
  cerr<<"SERVER: Signal received. Stopping"<<endl;

  // remove named pipes
  remove(FIFO_in.c_str());
  remove(FIFO_out.c_str());

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
}
