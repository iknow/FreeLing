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

// ####  Learn a abm model using a training corpus
// ####  argv[2] must be a class coding string in a format like:
// ####         0 MyClass  1 MyOtherClass  2 MyThirdClass  3 MyLastClass  (...)
// ####  or either:
// ####         0 MyClass  1 MyOtherClass  2 MyThirdClass  (...) <others> MyDefaultClass
// ####
// ####  Output file to argv[1].abm

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <sstream>
#include "freeling.h"
#include "fries.h"
#include "omlet.h"

adaboost* learner;
fex* extractor;
dataset* ds;
map<string,int> lexicon;
map<string,string> codes;

// encode examples in the given sentece, and add them to the 
// dataset to be used as train
void ProcessResults(const sentence &s) {
 
  sentence::const_iterator w;
  vector<set<int> > features;
  set<int>::iterator f;
  int i;
  map<string,string>::iterator q;

  // extract sentence features
  features.clear();
  extractor->encode_int(s,features);
  
  for (w=s.begin(),i=0; w!=s.end(); w++,i++) {
          
      // build an example 
      example exmp(ds->get_nlabels());

      // find out correct B-I-O tag for current word, and set it as the right
      // answer for this example
      q=codes.find(w->user[0]);
      if (q!=codes.end())
	exmp.set_belongs(util::string2int(q->second), true);

      // fill example with obtained features 
      for (f=features[i].begin(); f!=features[i].end(); f++)
	exmp.add_feature(*f);

      // add example to train set
      ds->add_example(exmp);
  }
}

// ----------------------------------------------------

int main(int argc, char* argv[]) {

  string text,form,lemma,tag,BIOtag,prob;
  sentence av;
  string num,name,freq;
  int nlabels,ns=0;
  string snam;
  int total_lex_examples=0;

  cerr << "train.e:: rgf:" << argv[1] << " codes: \"" << argv[2] << "\"" << endl;

  // analyze class codes string
  string cod(argv[2]);
  istringstream ps(cod);
  nlabels=0;
  while (ps>>num>>name) {
    if (num!="<others>") {
	 nlabels++;
	 codes.insert(make_pair(name,num));
    }
  }
  
  // create dataset to store examples;
  ds = new dataset(nlabels);
  
  // load feature extraction rules and feature lexicon
  snam = string(argv[1]);
  extractor = new fex(snam+".rgf", snam+".lex");

  // load input into train data set, in the form of feature vectors.
  while (std::getline(std::cin,text)) {
    
    if (text != "") { // got a word line
      istringstream sin; sin.str(text);

      // first field is the B-I-O tag
      sin>>BIOtag;

      // get word form
      sin>>form; 	
      
      // build new word
      word w(form);

      // add BIOtag as "user data"
      w.user.push_back(BIOtag);
      //cerr<<"BIO "<<BIOtag<<" "<<w.user[0]<<endl;;


      // add all analysis in line to the word.
      while (sin>>lemma>>tag>>prob){
	analysis an(lemma,tag);
	an.set_prob(atof(prob.c_str()));
	w.add_analysis(an);
      }
      
      // append new word to sentence
      av.push_back(w);
    }
    else { // blank line, sentence end.
      ProcessResults(av);
      av.clear(); // clear list of words for next use
      if (++ns%1000==0) cerr<<"Loaded sentence "<<ns<<endl;     
    }
  }

  // last sentence (may not have blank line after it)
  ProcessResults(av);    
  
  cerr<<"corpus encoded"<<endl;     

  // Set weak rule type to be used.  Note that the wr_type must
  // correspond to a registered weak rule type.  "mlDTree" is
  // preregistered in libomlet, but you can write code for your own WR
  // and register it without recompiling the library.  Check libomlet
  // documentation for details on how to do this.
  string wr_type="mlDTree";

  // Set parameters for WRs, nlab and epsilon are generic for all WRs.
  // Third parameter here is max_depth, specific to mlDTree.
  mlDTree_params wp(nlabels, 0.001, 3);

  // create and learn adaboost model
  adaboost learner(nlabels, wr_type);
  
  // set learned model output file
  string mod=snam+".abm"; ofstream abm(mod.c_str());

  // write class descriptions on first line
  abm<<argv[2]<<endl;
  // write Weak rule type on second line
  abm<<wr_type<<endl;

  // learn model     
  cerr<<"learning"<<endl;     
  learner.set_output((ostream*)&abm);
  learner.learn(*ds, 1000, true, (wr_params *)&wp);
  
  abm.close();
}


