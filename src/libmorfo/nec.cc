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

#include <fstream>
#include <sstream>

#include "freeling/nec.h"
#include "fries/util.h"
#include "freeling/traces.h"

using namespace std;

#define MOD_TRACENAME "NEC"
#define MOD_TRACECODE NEC_TRACE


///////////////////////////////////////////////////////////////
/// Create a named entity classification module, loading
/// appropriate files.
///////////////////////////////////////////////////////////////

nec::nec(const std::string &tag, const std::string &filepref) : NPtag(tag)
{
  string feat,file,line;

  // create feature extractor with appropriate rules and lexicon
  TRACE(3," Loading feature rules "+filepref+".rgf and lexicon file "+filepref+".lex");
  extractor = new fex(filepref+".rgf",filepref+".lex");

  // create AdaBoost classifier
  TRACE(3," Loading adaboost model "+filepref+".abm");
  classifier = new adaboost(filepref+".abm");
   
  TRACE(3,"analyzer succesfully created");
}

/////////////////////////////////////////////////////////////////////////////
/// Destructor: delete pointers
/////////////////////////////////////////////////////////////////////////////
nec::~nec(){
  delete extractor;
  delete classifier;
}

/////////////////////////////////////////////////////////////////////////////
/// Classify NEs in given sentence
/////////////////////////////////////////////////////////////////////////////

void nec::analyze(std::list<sentence> &ls) const {
  list<sentence>::iterator se;
  sentence::iterator w;
  word::iterator a;
  set<int>::iterator f;
  vector<set<int> > features;
  map<string,int>::const_iterator p;  
  int i,j;
  double max;
  string tag,def;

  // allocate prediction array (reused for all sentences)
  double pred[classifier->get_nlabels()];

  for (se=ls.begin(); se!=ls.end(); se++) {
    
    // extract sentence features
    features.clear();
    extractor->encode_int((*se),features);
    TRACE(1,"Sentence encoded.");
    
    // process each word
    for (w=se->begin(),i=0; w!=se->end(); w++,i++) {
      
      // for any analysis (selected by the tagger) that has NEtag, create and classify new example
      for (a=w->selected_begin(); a!=w->selected_end(); a++) {
	if (a->get_parole().find(NPtag,0)==0) {
	  
	  TRACE(2,"NP found ("+w->get_form()+"). building example");
	  example exmp(classifier->get_nlabels());
	  // add all extracted features to example
	  for (f=features[i].begin(); f!=features[i].end(); f++) exmp.add_feature(*f);
	  TRACE(3,"   example build, with "+util::int2string(exmp.size())+" features");
	  
	  // classify example
	  classifier->classify(exmp,pred);
	  TRACE(2,"Example classified");
	  
	  // find out which class has highest weight,
	  max=pred[0]; tag=classifier->get_label(0);
	  TRACE(3,"   label:"+classifier->get_label(0)+" weight:"+util::double2string(pred[0]));
	  for (j=1; j<classifier->get_nlabels(); j++) {
	    TRACE(3,"   label:"+classifier->get_label(j)+" weight:"+util::double2string(pred[j]));
	    if (pred[j]>max) {
	      max=pred[j];
	      tag=classifier->get_label(j);
	    }
	  } 
	  
	  // if no label has a positive prediction and <others> is defined, select <others> label.
	  def = classifier->default_class();
	  if (max<0 && def!="") tag = def;
	  
	  // tag NE appropriately: modify NP tag to be the right one... 
	  a->set_parole(tag);
	}
      } 
    }
    
    TRACE_SENTENCE(1,*se);
  }
}

list<sentence> nec::analyze(const list<sentence> &ls) const {
  list<sentence> s=ls;
  analyze(s);
  return s;
}

