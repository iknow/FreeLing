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
//  This file contains a sample main program to illustrate 
//  usage of FreeLing analyzers library.
//
//  This sample main program may be used straightforwardly as 
//  a basic front-end for the analyzers (e.g. to analyze corpora)
//
//  Neverthless, if you want embed the FreeLing libraries inside
//  a larger application, or you want to deal with other 
//  input/output formats (e.g. XML,HTML,...), the efficient and elegant 
//  way to do so is consider this file as a mere example, and call 
//  the library from your your own main code.
//
//  See README file to find out how to compile and execute this sample
//
//------------------------------------------------------------------//


#include "errno.h"
#include "sys/stat.h"
#include <iostream>

#include "freeling.h"
#include "freeling/traces.h"

using namespace std;

// predeclarations
void PrintMorfo(list<sentence> &ls);
void PrintTree(list<sentence> &ls);
void PrintDepTree(list<sentence> &ls);


/////////   MAIN SAMPLE PROGRAM  -- begin

int main(int argc, char **argv) {
  string text;
  list<word> lw;
  list<sentence> ls;

  // use first parameter as the name for the named pipe
  string FIFO_in = string(argv[1])+".in";
  string FIFO_out = string(argv[1])+".out";

  string path="/usr/local/share/FreeLing/es/";

  // --- Create analyzers

  tokenizer tk(path+"tokenizer.dat"); 
  splitter sp(path+"splitter.dat");
  
  // morphological analysis has a lot of options, and for simplicity they are packed up
  // in a maco_options object. First, create the maco_options object with default values.
  maco_options opt("es");  
  // then, set required options on/off  
  opt.set_active_modules(true, true, true, true, true, false, true, true, NER_BASIC);
  // and provide files for morphological submodules. Note that it is not necessary
  // to set opt.QuantitiesFile, since Quantities module was deactivated.
  opt.set_data_files(path+"locucions.dat", "", path+"afixos.dat",
                     path+"probabilitats.dat", opt.DictionaryFile=path+"maco.db",
                     path+"np.dat", path+"../common/punct.dat");
  // create the analyzer with the just build set of maco_options
  maco morfo(opt); 

  // create a hmm tagger for spanish (with retokenization ability, and forced 
  // to choose only one tag per word)
  hmm_tagger tagger("es", path+"tagger.dat", true, true); 
  // create chunker
  chart_parser parser(path+"grammar-dep.dat");
  // create dependency parser 
  dep_txala dep(path+"dep/dependences.dat", parser.get_start_symbol());
  

  // --- Create named pipe for client/server input and output

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

  // --- Main server loop: serve client requests until server is killed

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

    // --- Process client text up to EOF.
    while (getline(cin,text)) {
    
      // tokenize input line into a list of words
      lw=tk.tokenize(text);
      
      // accumulate list of words in splitter buffer, returning a list of sentences.
      // The resulting list of sentences may be empty if the splitter has still not 
      // enough evidence to decide that a complete sentence has been found. The list
      // may contain more than one sentence (since a single input line may consist 
      // of several complete sentences).
      ls=sp.split(lw, false);
      
      // perform and output morphosyntactic analysis and disambiguation
      morfo.analyze(ls);
      tagger.analyze(ls);
      PrintMorfo(ls);
      
      // perform and output Chunking
      parser.analyze(ls);
      PrintTree(ls);
      
      // Dep. parsing
      dep.analyze(ls);
      PrintDepTree(ls);
      
      // clear temporary lists;
      lw.clear(); ls.clear();    
    }
    
    // No more lines to read. Make sure the splitter doesn't retain anything  
    ls=sp.split(lw, true);   
    
    // analyze sentence(s) which might be lingering in the buffer, if any.
    morfo.analyze(ls);
    tagger.analyze(ls);
    PrintMorfo(ls);
    
    parser.analyze(ls);
    PrintTree(ls);
    
    dep.analyze(ls);
    PrintDepTree(ls);

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
}

/////////   MAIN SAMPLE PROGRAM  -- end


//----------------------------------
/// Result processing functions
//----------------------------------


//---------------------------------------------
// print morphological information
//---------------------------------------------

void PrintMorfo(list<sentence> &ls) {
  word::const_iterator a;
  sentence::const_iterator w;

  cout<<"----------- MORPHOLOGICAL INFORMATION -------------"<<endl;

  // print sentence XML tag
  for (list<sentence>::iterator is=ls.begin(); is!=ls.end(); is++) {

    cout<<"<SENT>"<<endl;
    // for each word in sentence
    for (w=is->begin(); w!=is->end(); w++) {
      
      // print word form, with PoS and lemma chosen by the tagger
      cout<<"  <WORD form=\""<<w->get_form();
      cout<<"\" lemma=\""<<w->get_lemma();
      cout<<"\" pos=\""<<w->get_parole();
      cout<<"\">"<<endl;
      
      // for each possible analysis in word, output lemma, parole and probability
      for (a=w->analysis_begin(); a!=w->analysis_end(); ++a) {
	
	// print analysis info
	cout<<"    <ANALYSIS lemma=\""<<a->get_lemma();
	cout<<"\" pos=\""<<a->get_parole();
	cout<<"\" prob=\""<<a->get_prob();
	cout<<"\"/>"<<endl;
      }
      
      // close word XML tag after list of analysis
      cout<<"  </WORD>"<<endl;
    }
    
    // close sentence XML tag
    cout<<"</SENT>"<<endl;
  }
}  


//---------------------------------------------
// print syntactic tree
//---------------------------------------------

void rec_PrintTree(parse_tree::iterator n, int depth) {
  parse_tree::sibling_iterator d;
  
  cout<<string(depth*2,' '); 
  if (n->num_children()==0) { 
    if (n->info.is_head()) { cout<<"+";}
    word w=n->info.get_word();
    cout<<"("<<w.get_form()<<" "<<w.get_lemma()<<" "<<w.get_parole();
    cout<<")"<<endl;
  }
  else { 
    if (n->info.is_head()) { cout<<"+";}
    cout<<n->info.get_label()<<"_["<<endl;
    for (d=n->sibling_begin(); d!=n->sibling_end(); ++d)
      rec_PrintTree(d, depth+1);
    cout<<string(depth*2,' ')<<"]"<<endl;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - -
void PrintTree(list<sentence> &ls) {

  cout<<"----------- CHUNKING -------------"<<endl; 
  for (list<sentence>::iterator is=ls.begin(); is!=ls.end(); is++) {
    parse_tree &tr = is->get_parse_tree();
    rec_PrintTree(tr.begin(), 0);
  }
}

//---------------------------------------------
// print dependency tree
//---------------------------------------------

void rec_PrintDepTree(dep_tree::iterator n, int depth) {
  dep_tree::sibling_iterator d,dm;
  int last, min;
  bool trob;
  
    cout<<string(depth*2,' '); 

    cout<<n->info.get_link()->info.get_label()<<"/"<<n->info.get_label()<<"/";
    word w=n->info.get_word();
    cout<<"("<<w.get_form()<<" "<<w.get_lemma()<<" "<<w.get_parole()<<")";  

    if (n->num_children()>0) { 
       cout<<" ["<<endl; 

       //Print Nodes
       for (d=n->sibling_begin(); d!=n->sibling_end(); ++d)
	 if(!d->info.is_chunk())
	   rec_PrintDepTree(d, depth+1);

       // print CHUNKS (in order)
       last=0; trob=true;
       while (trob) { 
	 // while an unprinted chunk is found look, for the one with lower chunk_ord value
	 trob=false; min=9999;  
	 for (d=n->sibling_begin(); d!=n->sibling_end(); ++d) {
	   if(d->info.is_chunk()) {
	     if (d->info.get_chunk_ord()>last && d->info.get_chunk_ord()<min) {
	       min=d->info.get_chunk_ord();
	       dm=d;
	       trob=true;
	     }
	   }
	 }
	 if (trob) rec_PrintDepTree(dm, depth+1);
	 last=min;
       }

       cout<<string(depth*2,' ')<<"]"; 
    }
    cout<<endl;
}

// - - - - - - - - - - - - - - - - - - - - - - -
void PrintDepTree(list<sentence> &ls) {

  cout<<"----------- DEPENDENCIES -------------"<<endl;
  for (list<sentence>::iterator is=ls.begin(); is!=ls.end(); is++) {
    dep_tree &dep = is->get_dep_tree();
    rec_PrintDepTree(dep.begin(), 0);
  }
}


