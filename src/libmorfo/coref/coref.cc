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

#include "fries/util.h"
#include "freeling/coref.h"
#include "freeling/traces.h"

using namespace std;

#define MOD_TRACENAME "COREF"
#define MOD_TRACECODE COREF_TRACE

//////////////////////////////////////////////////////////////////
///    Outputs a sample. Only for debug purposes
//////////////////////////////////////////////////////////////////

void outSample(SAMPLE &sa){
  cout << "SAMPLE" << endl;
  cout << "	Sent: " << sa.sent << endl;
  cout << "	Beg: " << sa.posbegin << endl;
  cout << "	End: " << sa.posend << endl;
  cout << "	Text: " << sa.text << endl;
  cout << "	Tok: ";
  for(vector<string>::iterator it = sa.texttok.begin(); it!= sa.texttok.end();++it)
    cout << (*it) << " ";
  cout << endl;
  cout << "	Tag: ";
  for(vector<string>::iterator it = sa.tags.begin(); it!= sa.tags.end();++it)
    cout << (*it) << " ";
  cout << endl;
}

///////////////////////////////////////////////////////////////
/// Create a coreference module, loading
/// appropriate files.
///////////////////////////////////////////////////////////////

coref::coref(const std::string &filename, const int vectors) {
  int lnum=0;
  string path=filename.substr(0,filename.find_last_of("/\\")+1);
  string line,sf,wf;
  
  ifstream fin;
  fin.open(filename.c_str());  
  if (fin.fail()) ERROR_CRASH("Cannot open Coreference config file "+filename);

  int reading=0; 
  while (getline(fin,line)) {
    lnum++;
    
    if (line.empty() || line[0]=='%') {} // ignore comment and empty lines

    else if (line == "<ABModel>") reading=1;
    else if (line == "</ABModel>") reading=0;

    else if (line == "<MaxDistance>") reading=2;
    else if (line == "</MaxDistance>") reading=0;

    else if (line == "<SEMDB>") reading=3;
    else if (line == "</SEMDB>") reading=0;

    else if (reading==1) {
      ////// get .abm file absolute name
      istringstream sin;  sin.str(line);
      string fname;
      sin>>fname;
      fname = util::absolute(fname,path); 
      // create AdaBoost classifier
      TRACE(3," Loading adaboost model "+fname);
      classifier = new adaboost(fname);
    }
    else if (reading==2) {
      // Read MaxDistance value
      istringstream sin;  sin.str(line);
      sin>>MaxDistance ;
    }
    else if (reading==3) {
      ////// load SEMDB section 
      string key,fname;
      istringstream sin;  sin.str(line);
      sin>>key>>fname;
      
      if (key=="SenseFile")   sf= util::absolute(fname,path); 
      else if (key=="WNFile") wf= util::absolute(fname,path); 
      else 
	WARNING("Unknown parameter "+key+" in SEMDB section of file "+filename+". SemDB not loaded");      
    }
  }

  // create feature extractor
  extractor = new coref_fex(TYPE_TWO, vectors, sf, wf);

  TRACE(3,"analyzer succesfully created");
}

///////////////////////////////////////////////////////////////
/// Fills a sample from a node info
///////////////////////////////////////////////////////////////

void coref::set_sample(parse_tree::iterator pt, SAMPLE & sample) const{
  parse_tree::sibling_iterator d;

  if (pt->num_children()==0) {
    word wo=pt->info.get_word();
    string w = util::lowercase(wo.get_form());
    string t = util::lowercase(wo.get_parole());
    
    sample.posend++;
    sample.text += " " + w;
    sample.texttok.push_back(w);
    sample.tags.push_back(t);
  } 
  else {
    for (d=pt->sibling_begin(); d!=pt->sibling_end(); ++d) {
      set_sample(d, sample);
    }
  }
}

///////////////////////////////////////////////////////////////
/// Finds recursively all the SN and put in a list of samples
///////////////////////////////////////////////////////////////

void coref::add_candidates(int sent, int & word, parse_tree::iterator pt, list<SAMPLE> & candidates) const{
  parse_tree::sibling_iterator d;
  
  if (pt->num_children()==0) {
    word++;
  } 
  else {
    if (pt->info.get_label() == "sn"){
      SAMPLE candidate;
      candidate.sent = sent;
      candidate.posbegin = word;
      candidate.posend = word;
      set_sample(pt, candidate);
      word = candidate.posend;
      candidate.node1 = &(pt->info);
      candidates.push_back(candidate);
    }
    else {
      for (d=pt->sibling_begin(); d!=pt->sibling_end(); ++d)
	add_candidates(sent, word, d, candidates);
    }
  }
}

///////////////////////////////////////////////////////////////
/// Check if the two samples are coreferents. Uses the classifier.
///////////////////////////////////////////////////////////////

bool coref::check_coref(const SAMPLE & sa1, const SAMPLE & sa2) const{
  int j;
  double pred[classifier->get_nlabels()];
  std::vector<int> encoded;
  EXAMPLE ex;
  
  TRACE(5,"    -Encoding example");
  ex.sample1 = sa1;
  ex.sample2 = sa2;
  ex.sent = ex.sample2.sent - ex.sample1.sent;
  //outSample(ex.sample1);
  //outSample(ex.sample2);
  extractor->extract(ex, encoded);
  example exampl(classifier->get_nlabels());
  for(std::vector<int>::iterator it = encoded.begin(); it!= encoded.end(); ++it) {
    exampl.add_feature((*it));
  }

  TRACE(5,"    -Classifying");  
  // classify current example
  classifier->classify(exampl,pred);
  
  // find out which class has highest weight (alternatively, we
  // could select all classes with positive weight, and propose
  // more than one class per example)
  double max=pred[0];
  string tag=classifier->get_label(0);
  for (j=1; j<classifier->get_nlabels(); j++) {
    if (pred[j]>max) {
      max=pred[j];
      tag=classifier->get_label(j);
    }
  }

  // if no label has a positive prediction and <others> is defined, select <others> label.
  string def = classifier->default_class();
  if (max<0 && def!="") tag = def;
  
  TRACE(4,"    -Selected class: "+tag);

  return (tag == "Positive");
}


/////////////////////////////////////////////////////////////////////////////
/// Classify the SN is our group of coreference
/////////////////////////////////////////////////////////////////////////////

void coref::analyze(document & doc) const {
  
  list<SAMPLE> candidates;
  
  TRACE(3,"Searching for candidate noun phrases");
  int sent1 = 0;
  int word1 = 0;
  for (document::iterator par = doc.begin(); par != doc.end(); ++par) {
    for (paragraph::iterator se = par->begin(); se != par->end(); ++se) {
      add_candidates(sent1, word1, se->get_parse_tree().begin(), candidates);
      sent1++;
    }
  }
  
  TRACE(3,"Pairing candidates ("+util::int2string(candidates.size())+")");
  list<SAMPLE>::const_iterator i = candidates.begin();
  ++i;
  while (i != candidates.end()) {

    TRACE(4,"   pairing "+i->text+" with all previous");
    bool found = false; 
    bool end = false;
    int count = MaxDistance;
    list<SAMPLE>::const_iterator j = i; 
    --j;
    while (!end && !found && count > 0) {
      TRACE(4,"   checking pair ("+j->text+","+i->text+")");
      found = check_coref(*j, *i);
      if (found) doc.add_positive(*(j->node1), *(i->node1));
      
      if (j==candidates.begin()) end=true;
      else --j;

      count--;
    }
    ++i;
  }
}


