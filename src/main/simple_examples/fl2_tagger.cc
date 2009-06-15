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
///   The following program reads from stdin a morphologically analyzed
///   text, and calls the tagger to disambiguate the PoS of each word.
///
///   The input format should be one word per line, with all possible lemma-PoS
///   for each word, and a blank line as sentence separator.  For example:
///
///   The the DT 1
///   cats cat NNS 1
///   eat eat VB 0.75 eat VBP 0.25
///   fish fish NN 0.916667 fish NNS 0.0277778 fish VB 0.0277778 fish VBP 0.0277778
///   . . Fp 1
///  
///   The the DT 1
///   kids kid NNS 0.982759 kid VBZ 0.0172414
///   are be VBP 1
///   playing play VBG 1
///   . . Fp 1
///  
///---------------------------------------------
int main (int argc, char **argv) {
  POS_tagger *tagger = NULL;

  if(argc < 2) { 
    cout << "fl-tagger" << endl;
    cout << "Usage: fl-tagger [relax file]" << endl;
    cout << endl; 
    return 1;
  }
 
  //TaggerRelaxFile=constr_gram.dat
  //TaggerRelaxMaxIter=500
  //TaggerRelaxScaleFactor=670.0
  //TaggerRelaxEpsilon=0.001
  //TaggerRetokenize=no
  //TaggerForceSelect=retok

  // create tagger
  tagger = new relax_tagger (argv[1], 500, 670.0, 0.001, false, true);

  /// read and tag input
  string text, form, lemma, tag, sn, spr;
  sentence av;
  double prob;
  list < sentence > ls;
  unsigned long totlen = 0;

  while (std::getline (std::cin, text)) {
    if (text != "")  {	// got a word line
      istringstream sin;
      sin.str (text);

      sin >> form;    // get word form

      // build new word
      word w (form);
      w.set_span (totlen, totlen + form.size ());
      totlen += text.size () + 1;
      
      // process word line, according to input format.
      // add all analysis in line to the word.
      w.clear ();
      while (sin >> lemma >> tag >> spr)  {
	analysis an (lemma, tag);
	prob = util::string2double (spr);
	an.set_prob (prob);
	w.add_analysis (an);
      }
      
      av.push_back (w);   // append new word to sentence
    }
    else {  // blank line, sentence end.
	totlen += 2;
	
	ls.push_back (av);	  
	tagger->analyze (ls);	 
	PrintResults (ls);
	
	av.clear ();		// clear list of words for next use
	ls.clear ();		// clear list of sentences for next use
      }
  }
  
  // process last sentence in buffer (if any)
  ls.push_back (av);		// last sentence (may not have blank line after it)
  tagger->analyze (ls);
  PrintResults (ls);

  // clean up. 
  delete tagger;

  return 0;
}
