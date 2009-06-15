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

void PrintTree (parse_tree::iterator n, int depth) {
  parse_tree::sibling_iterator d;

  cout << string (depth * 2, ' ');
  if (n->num_children () == 0) {
    if (n->info.is_head ()) cout << "+";
    word w = n->info.get_word ();
    cout << "(" << w.get_form () << " " << w.get_lemma () << " " << w.get_parole ();
    cout << ")" << endl;
  }
  else {
    if (n->info.is_head ()) cout << "+";

    cout<<n->info.get_label();
    cout << "_[" << endl;

    for (d = n->sibling_begin (); d != n->sibling_end (); ++d) 
      PrintTree (d, depth + 1);
    cout << string (depth * 2, ' ') << "]" << endl;
  }
}


void PrintResults (list<sentence> &ls) {
  word::const_iterator ait;
  sentence::const_iterator w;
  list < sentence >::iterator is;
  int nsentence = 0;

  for (is = ls.begin (); is != ls.end (); is++, ++nsentence) {
      parse_tree & tr = is->get_parse_tree ();
      PrintTree (tr.begin (), 0);
      cout << endl;
  }
}


///---------------------------------------------
/// Sample main program
///
///   The following program reads from stdin a PoS-tagged text, and 
///   calls the chart parser to perform a shallow parsing.
///
///   The input format should be one word per line, with the right lemma-PoS
///   for each word, and a blank line as sentence separator.  For example:
///
///    The the DT 1
///    cats cat NNS 1
///    eat eat VBP 0.25
///    fish fish NN 0.916667
///    . . Fp 1
///  
///    The the DT 1
///    kids kid NNS 0.982759
///    are be VBP 1
///    playing play VBG 1
///    . . Fp 1
/// 
///---------------------------------------------

int main (int argc, char **argv) {
  chart_parser *parser = NULL;

  if(argc < 2) { 
    cout << "fl-chunker" << endl;
    cout << "Usage: fl-chunker [grammar]" << endl;
    cout << endl; 
    return 1;
  }
 
// #### Parser options
// GrammarFile=$FREELINGSHARE/cy/chunker/grammar.dat

  parser = new chart_parser(argv[1]);

  // read and process input
  string text, form, lemma, tag, sn, spr;
  sentence av;
  list < sentence > ls;
  unsigned long totlen = 0;

  while (std::getline (std::cin, text))  {
    
    if (text != "")  {	// got a word line
      istringstream sin;
      sin.str (text);
      sin >> form;       // get word form
      
      // build new word
      word w (form);
      w.set_span (totlen, totlen + form.size ());
      totlen += text.size () + 1;

      // process word line, according to input format.
      // add all analysis in line to the word.
      w.clear ();
      while (sin >> lemma >> tag >> spr) {
	sin >> lemma >> tag;
	analysis an (lemma, tag);
	an.set_prob (1.0);
	w.add_analysis (an);
      }
      
      // append new word to sentence
      av.push_back (w);
    }
    else {// blank line, sentence end.
      totlen += 2;
      
      ls.push_back (av);      
      parser->analyze (ls);
      PrintResults (ls);

      av.clear ();		// clear list of words for next use
      ls.clear ();		// clear list of sentences for next use
    }
  }
  
  // process last sentence in buffer (if any)
  ls.push_back (av);		// last sentence (may not have blank line after it)
  parser->analyze (ls);
  PrintResults (ls);

  // clean up. 
  delete parser;

  return 0;
}
