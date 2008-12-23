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

using namespace std;


// ####  Convert a corpus prepared for NER into features and produce
// ####  lexicon files.  See TRAIN.sh in this directory to learn more.

#include <iostream>
#include <fstream>
#include <stdlib.h>
//#include <map>
#include "freeling.h"
//#include "freeling/traces.h"
//#include "fries/language.h"
#include "fries.h"
#include "omlet.h"
//#include "fries/fex.h"



fex* extractor;
map <string,int> lexicon;

void ProcessResults(const list<sentence> &ls) {
  
  list<sentence>::const_iterator s;
  word::const_iterator a;
  sentence::const_iterator w;
  vector<set<string> > features;
  set<string>::iterator f;
  int i;
  map<string,int>::iterator p;

  // for each sentence in list
  for (s=ls.begin(); s!=ls.end(); s++) {

    // extract sentence features
    features.clear();
    extractor->encode_name(*s,features, true); // boolean "true" means we are building the lexicon

    for (w=s->begin(),i=0; w!=s->end(); w++,i++) {	
      // store and print features (print is for debugging
      // purposes only, it is not actually necessary)
      cout<<w->get_form()
      for (f=features[i].begin(); f!=features[i].end(); f++) {
	extractor->add_lexicon(*f);
	cout<<" "+(*f);
      }
      cout<<endl;
    }
  }
}


// ----- Main program: read a sentence, encode it and store features appearing

int main(int argc, char* argv[]) {

  string text,form,lemma,tag, BIOtag, prob;
  sentence av;
  list<sentence> ls;
  string snam;

  snam = string(argv[1]);
  extractor = new fex(snam+".rgf");
  
  while (std::getline(std::cin,text)) {
    
    if (text != "") { // got a word line
      istringstream sin; sin.str(text);
      //cerr<<"["<<text<<"]"<<endl;

      // first field is the B-I-O tag
      sin>>BIOtag;
      //cerr<<"tag: "<<BIOtag<<endl;


      // get word form
      sin>>form; 
      //cerr<<"form: "<<form<<endl;

      // build new word
      word w(form);

      // add all analysis in line to the word.
      while (sin>>lemma>>tag>>prob){
	//cerr<<"  analisis: "<<lemma<<" "<<tag<<" "<<prob<<endl;
	analysis an(lemma,tag);
	an.set_prob(atof(prob.c_str()));
	w.add_analysis(an);
      }
      // cerr<<"   number of analysis: "<<w.get_n_analysis()<<endl;
      // append new word to sentence
      av.push_back(w);
    }
    else { // blank line, sentence end.
      ls.push_back(av);
      
      ProcessResults(ls);
	
      av.clear(); // clear list of words for next use
      ls.clear(); // clear list of sentences for next use
    }
  }
  
  // last sentence (may not have blank line after it)
  ls.push_back(av);
  ProcessResults(ls);    
  
  // save the lexicon to disk, no features filtered
  extractor->save_lexicon(snam+"-all.lex");
  // save the lexicon to disk, features occurring three or less times are filtered
  extractor->save_lexicon(snam+"-3abs.lex", 3);
  // save the lexicon to disk, features with 1% of the occurrences or less are filtered
  extractor->save_lexicon(snam+"-1pc.lex", 0.01);
}


