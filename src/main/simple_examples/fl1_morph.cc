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

/// Author:  Francis Tyers

using namespace std;

#include <sstream>
#include <iostream>

#include <map>
#include <vector>

/// headers to call freeling library
#include "freeling.h"

void PrintResults (list<sentence> &ls) {
  word::const_iterator ait;
  sentence::const_iterator w;
  list < sentence >::iterator is;
  int nsentence = 0;

  for (is = ls.begin (); is != ls.end (); is++, ++nsentence) {

      for (w = is->begin (); w != is->end (); w++) {
	cout << w->get_form ();
	
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
	  }
	cout << endl;	
      }
    // sentence separator: blank line.
    cout << endl;
  }
}



///---------------------------------------------
/// Sample main program
///
///   The following program reads from stdin a plain text, tokenizes it, splits on
///   sentence boundaries, and looks up each word in the dictionary
///  
///---------------------------------------------
int main (int argc, char **argv) {
  tokenizer *tk = NULL;
  splitter *sp = NULL;
  maco *morfo = NULL;

  if(argc < 4) { 
    cout << "fl-morph" << endl;
    cout << "Usage: fl-morph [tokeniser] [splitter] [maco.db]" << endl;
    cout << endl; 
    return 1;
  }
 
  tk = new tokenizer (argv[1]);
  sp = new splitter (argv[2]);

  // "xx" should be the language of the input text, but since this is a 
  // very simple example, it doesn't matter much.
  maco_options opt ("xx");     
  // We use the bare minimum of modules (ner = NONE = no NER)
  // set_active_modules(bool suf, bool mw, bool num, bool pun, bool dat, bool qt, bool dic, bool prb, int ner) 
  opt.set_active_modules(false, false, false, false, false, false, true, false, NER_NONE);
  opt.set_data_files ("", "", "", "", argv[3], "", "");
  
  // create tha analyzer with the above options.
  morfo = new maco (opt);

  // read and process text
  string text;
  unsigned long offs = 0;
  list < word > av;
  list < word >::const_iterator i;
  list < sentence > ls;

  while (std::getline (std::cin, text))  {
    av = tk->tokenize (text, offs);
    ls = sp->split (av, false);
    morfo->analyze (ls);
    PrintResults (ls);
    
    av.clear ();		// clear list of words for next use
    ls.clear ();		// clear list of sentences for next use
  }
  
  // process last sentence in buffer (if any)
  ls = sp->split (av, false);	//flush splitter buffer
  morfo->analyze (ls);
  PrintResults (ls);

  // clean up. 
  delete tk;
  delete sp;
  delete morfo;

  return 0;
}
