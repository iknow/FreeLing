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
#include <list>

/// headers to call freeling library
#include "freeling.h"

/// config file/options handler for this particular sample application
#include "config.h"
#define MODE
#include "analyzer_common.h"

// server mode, we'll need this
#include "socket.h"

// communicate with clients;
socket_CS *sock;


//---------------------------------------------
// print obtained analysis.
//---------------------------------------------

void SendResults (list <sentence> &ls, const document &doc=document() ) {
  word::const_iterator ait;
  sentence::const_iterator w;
  list < sentence >::iterator is;

  if (ls.empty()) {
    sock->write_message("FL-SERVER-READY");  
    return;
  }

  ostringstream sout;
  PrintResults(sout,ls,doc);
  sock->write_message(sout.str());
}


//---------------------------------------------
// Coreference resolution. Input is plain text, whole document.
//---------------------------------------------
void ProcessCoreference () {

  string text;
  list < word > av;
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
    SendResults(*par, doc);

}


//---------------------------------------------
// Plain text input, incremental processing
//---------------------------------------------
void ProcessPlain (double &cpuTime_total, int &sentences, int &words){
  string text;
  unsigned long offs = 0;
  list < word > av;
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
    
    if (cfg->OutputFormat >= TOKEN) tk->tokenize (text, offs, av);
    if (cfg->OutputFormat >= SPLITTED) sp->split (av, cfg->AlwaysFlush, ls);
    AnalyzeSentences(ls);
    
    // update stats
    sentences+=ls.size();          
    list<sentence>::iterator is;
    for (is=ls.begin(); is!=ls.end(); is++) {
      sentence se=*is;
      words+=se.size();
    }

    SendResults (ls);

    clock_t end = clock();
    cpuTime_total += (end-(double)start)/(CLOCKS_PER_SEC);            
    
    av.clear ();		// clear list of words for next use
    ls.clear ();		// clear list of sentences for next use
  }
  
  clock_t start = clock();

  //flush splitter buffer
  if (cfg->OutputFormat >= SPLITTED) sp->split (av, true, ls);
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
  SendResults (ls);

  clock_t end = clock();
  cpuTime_total += (end-(double)start)/(CLOCKS_PER_SEC);            

}


//----------------------------------------------
// Process already tokenized text.
// This sample program expects one token per line.
//----------------------------------------------
void ProcessToken () {
  string text;
  list < word > av;
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
      
      SendResults (ls);
      
      av.clear ();		// clear list of words for next use
      ls.clear ();		// clear list of sentences for next use
    }
  }
  
  //flush splitter buffer
  if (cfg->OutputFormat >= SPLITTED) sp->split (av, true, ls);	
  // process last sentence in buffer (if any)
  AnalyzeSentences(ls);

  SendResults (ls);
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

	  SendResults (ls);

	  av.clear ();		// clear list of words for next use
	  ls.clear ();		// clear list of sentences for next use
	}
    }

  // process last sentence in buffer (if any)
  ls.push_back (av);		// last sentence (may not have blank line after it)

  AnalyzeSentences(ls);

  SendResults (ls);
}


//---------------------------------------------
// Capture signal to terminate process
//---------------------------------------------
void terminate (int param)
{

  // --- shutting down server
  cerr<<"SERVER: Signal received. Stopping"<<endl;

  // clean up. 
  cleanup();

  // terminate the server
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

  // skip port number
  argv++; argc--;
  // read configuration file and command-line options, 
  // and create appropriate analyzers
  CreateAnalyzers(argv);

  cerr<<"SERVER: Analyzers loaded."<<endl;

  // crear socket
  sock = new socket_CS(server_port);

  // set ending signals processing to allow clean exits;
  signal (SIGTERM,terminate);
  signal (SIGQUIT,terminate);
  
  double cpuTime_total=0.0;
  int sentences=0;
  int words=0;

  // serve client requests until server is stopped
  while (true) {
          
    cerr<<"SERVER: opening channels. Waiting connections"<<endl;
    sock->wait_client();
    
    //--- get and process client input up to EOF ---

    // coreference requested, plain text input
    if (cfg->COREF_CoreferenceResolution)
      ProcessCoreference ();
    // Input is plain text.
    else if (cfg->InputFormat == PLAIN)
      ProcessPlain (cpuTime_total, sentences, words);
    // Input is tokenized. 
    else if (cfg->InputFormat == TOKEN)
      ProcessToken ();
    // Input is (at least) tokenized and splitted.
    else if (cfg->InputFormat >= SPLITTED)
      ProcessSplitted ();
    
    cerr<<"SERVER: client ended. Closing connection."<<endl;
    sock->close_connection();
  }

}
