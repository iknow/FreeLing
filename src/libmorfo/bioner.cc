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
#include <math.h>

#include "freeling/ner.h"
#include "freeling/bioner.h"
#include "fries/util.h"
#include "freeling/traces.h"

using namespace std;

#define MOD_TRACENAME "NP"
#define MOD_TRACECODE NP_TRACE

///////////////////////////////////////////////////////////////
/// Perform named entity recognition using AdaBoost
///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////
/// Create a named entity recognition module, loading
/// appropriate files.
///////////////////////////////////////////////////////////////

bioner::bioner(const std::string &conf_file) : ner(), vit(conf_file) {
  string feat,lex_file,rgf_file, abm_file, line, name;
  int num;

  map <string, int> class_num;
  
  // default
  Title_length = 0;
  splitNPs=false;
  
  // read configuration file and store information
   
  ifstream file (conf_file.c_str());
  if (!file) {
    ERROR_CRASH("Error opening file "+conf_file);
  }
  
  int reading=0;
  while (getline(file,line)) {

    istringstream sin;
    sin.str(line);

    if (line == "<Lexicon>") reading=1;
    else if (line == "</Lexicon>") reading=0;

    else if (line == "<RGF>") reading=2;
    else if (line == "</RGF>") reading=0;

    else if (line == "<AdaBoostModel>") reading=3;
    else if (line == "</AdaBoostModel>") reading=0;

    else if (line == "<Classes>") reading=4;
    else if (line == "</Classes>") reading=0;
    
    else if (line == "<NE_Tag>") reading=5;
    else if (line == "</NE_Tag>") reading=0;
    
    else if (line == "<TitleLimit>") reading=6;
    else if (line == "</TitleLimit>") reading=0;
    
    else if (line == "<SplitMultiwords>") reading = 7;
    else if (line == "</SplitMultiwords>") reading = 0;

    else if (reading == 1) {
      // Reading lexicon file name
      sin>>lex_file;
    }

    else if (reading == 2) {
      // Reading RGF file name
      sin>>rgf_file;
    }

    else if (reading == 3) {
      // Reading AdaBoost model file name
      sin>>abm_file;
    }

    else if (reading == 4) {
      // Reading class name and numbers: e.g. 0 B 1 I 2 O
      while(sin>>num>>name)
	class_name.insert(make_pair(num,name));   // to be used by analyzer
    }
    
    else if (reading==5)  // reading tag to assign to detected NEs
      NE_tag=line;
    
    else if (reading==6)  // reading value for Title_length 
      Title_length = util::string2int(line);
    
    else if (reading==7)
      splitNPs = (line.compare("yes")==0);
  }
   
  // create feature extractor with appropriate rules and lexicon
  TRACE(3," Loading feature rules "+rgf_file+" and lexicon file "+lex_file);
  extractor = new fex(rgf_file,lex_file);

  // create AdaBoost classifier
  TRACE(3," Loading adaboost model "+abm_file);
  classifier = new adaboost(abm_file);

  TRACE(3,"analyzer succesfully created");
}

/////////////////////////////////////////////////////////////////////////////
/// Destructor: deletes created pointers
bioner::~bioner(){
  delete classifier;
  delete extractor;
}
   
/////////////////////////////////////////////////////////////////////////////
/// Recognize NEs in given sentence
/////////////////////////////////////////////////////////////////////////////

void bioner::annotate(sentence &se) {
  sentence::iterator w;
  word::iterator a;
  set<int>::iterator f;
  vector<set<int> > features;
  map<string,int>::const_iterator p;  
  int i;
  string tag,def;
  
  // allocate prediction array (reused for all sentences)
  double pred[classifier->get_nlabels()];
  
    // all the predictions of the sentence will be stored in an array of arrays.
    // there are the weights for each class and each word. First index is the class_num
    // the second one, the word.
    double** all_pred=new double*[classifier->get_nlabels()]; 
    // initialize matrix 
    for (i = 0; i < classifier->get_nlabels(); ++i)
      all_pred[i] = new double[se.size()];

    // extract sentence features
    features.clear();
    extractor->encode_int(se,features);
    TRACE(2,"Sentence encoded.");

    // process each word
    for (w=se.begin(),i=0; w!=se.end(); w++,i++) {
      example exmp(classifier->get_nlabels());

      // add all extracted features to example
      for (f=features[i].begin(); f!=features[i].end(); f++) exmp.add_feature(*f);
      TRACE(4,"   example build, with "+util::int2string(exmp.size())+" features");

      // classify example
      classifier->classify(exmp,pred);
      TRACE(3,"Example classified");
 
      // add the predictions for this word to the vector of arrays
      for (int j=0; j<classifier->get_nlabels(); j++)
	all_pred[j][i]=pred[j];
    }

    // Once all sentece has been encoded, use Viterbi algorithm to 
    // determine which is the most likely class combination
    vector<int> best;
    best = vit.find_best_path(all_pred,se.size());
    
    // process obtained best_path and join detected NEs, syncronize it with sentence
    bool inNE=false;
    sentence::iterator beg;
    bool built=false;
    
    // for each word
    for (w=se.begin(),i=0; w!=se.end(); w++,i++) { 
      // look for the BIOtag choosen for this word
      map <int,string>::const_iterator tag;
      tag=class_name.find(best[i]);
      if (tag!=class_name.end()) {
	TRACE(3, "Word "+w->get_form()+" has BIO tag "+tag->second);
	// if we were inside NE, and the chosen class (best[i]) for this word is "B" or "O", 
	//  previous NE is finished: build multiword. 	
	if ((inNE && tag->second=="B") || (inNE && tag->second=="O")) {
	  w=BuildMultiword(se, beg, w-1, built);
	  inNE=false;
	}
	// if we found "B", start new NE (previous if joins possible previous NE that finishes here)
	if (tag->second=="B") {
	  inNE=true;
	  beg=w;
	}
      }	
    }
    
    TRACE_SENTENCE(1,se);
    
    // free memory 
    for (i = 0; i < classifier->get_nlabels(); ++i)
      delete(all_pred[i]);
    delete(all_pred);
}

///////////////////////////////////////////////////////////////
///  Perform last minute validation before effectively building multiword
///////////////////////////////////////////////////////////////

bool bioner::ValidMultiWord(const word &w) const {
	
  // We do not consider a valid proper noun if all words are capitalized and there 
  // are more than Title_length words (it's probably a news title, e.g. "TITANIC WRECKS IN ARTIC AS!!")
  // Title_length==0 deactivates this feature

  list<word> mw = w.get_words_mw();
  if (Title_length>0 && mw.size() >= Title_length) {
    list<word>::const_iterator p;
    bool lw=false;
    for (p=mw.begin(); p!=mw.end() && !lw; p++) lw=util::has_lowercase(p->get_form());
    // if a word with lowercase chars is found, it is not a title, so it is a valid proper noun.
    return (lw);
  }
  else
    return(true);

}

///////////////////////////////////////////////////////////////
///   Set the appropriate lemma and parole for the
///   new multiword.
///////////////////////////////////////////////////////////////

void bioner::SetMultiwordAnalysis(sentence::iterator i) const {

  // Setting the analysis for the Named Entity. 
  // The new MW is just created, so its list is empty.
  
//     // if the MW has only one word, and is an initial noun, copy its possible analysis.
//   if (initialNoun && i->get_n_words_mw()==1) {
//     TRACE(3,"copying first word analysis list");
//     i->copy_analysis(i->get_words_mw().front());
//   }
    
  TRACE(3,"adding NP analysis");
    // Add an NP analysis, with the compound form as lemma.
  i->add_analysis(analysis(util::lowercase(i->get_form()),NE_tag));

}

///////////////////////////////////////////////////////////////
///   BuildMultiword: given a sentence and the position where
///   a multiword starts and ends, build the multiword
///////////////////////////////////////////////////////////////

sentence::iterator bioner::BuildMultiword(sentence &se, sentence::iterator start, sentence::iterator end, bool &built) const {
  sentence::iterator i;
  list<word> mw;
  string form;
	
  TRACE(3,"Building multiword");
  for (i=start; i!=end; i++){
    mw.push_back(*i);           
    form += i->get_form()+"_";
    TRACE(3,"added next ["+form+"]");
  }
  // don't forget last word
  mw.push_back(*i);           
  form += i->get_form();
  TRACE(3,"added last ["+form+"]");
	
  // build new word with the mw list, and check whether it is acceptable
  word w(form,mw);
	
  if (ValidMultiWord(w)) {  
    if (splitNPs) {
      for (sentence::iterator j=start; j!=se.end() && j!=end+1; j++){
	if (util::isuppercase(j->get_form()[0]))
	{
	  j->set_analysis(analysis(util::lowercase(j->get_form()),NE_tag));
	}
      }
      i=end+1;			
      built=true;
    }
    else {
      TRACE(3,"Valid Multiword. Modifying the sentence");
      // erasing from the sentence the words that composed the multiword
      end++;
      i=se.erase(start, end);
      // insert new multiword it into the sentence
      i=se.insert(i,w); 
      TRACE(3,"New word inserted");
      // Set morphological info for new MW
      SetMultiwordAnalysis(i);
      built=true;
    }
  }
  else {
    TRACE(3,"Multiword found, but rejected. Sentence untouched");
    i=end+1;
    built=false;
  }
	
  return(i);
}



//---------- Visible Viterbi Class  ----------------------------------

////////////////////////////////////////////////////////////////
/// Constructor: Create dynammic storage for the best path
////////////////////////////////////////////////////////////////

vis_viterbi::vis_viterbi(const string &P_file) {
  
  string line, name, name1, prob;
  int num;
  
  N=0;
  
  map <string, int> class_num;

  // read probabilities file and store information
   
  ifstream file (P_file.c_str());
  if (!file)
    ERROR_CRASH("Error opening file "+P_file);
  
  int reading=0;
  while (getline(file,line)) {

    istringstream sin;
    sin.str(line);
  
    if (line == "<Classes>") reading=1;
    else if (line == "</Classes>") reading=0;
    
    else if (line == "<InitialProb>") reading=2;
    else if (line == "</InitialProb>") reading=0;

    else if (line == "<TransitionProb>") reading=3;
    else if (line == "</TransitionProb>") reading=0;

    else if (reading == 1) {
      // Reading class name and numbers: e.g. 0 B 1 I 2 O
      while(sin>>num>>name)
	class_num.insert(make_pair(name,num));   // to be used when reading probabilities
    // now we know number of classes, set N
      N=class_num.size();
    }

    else if (reading == 2) {
    // Reading initial probabilities for each class (one class per line)
      if (N==0) // we have not seen reading==1
	ERROR_CRASH("File "+P_file+" need to have classes name/numbers before probability values");
      if (p_ini.size()==0) { // first line, initialize vector
	for (int i=0; i<N; i++)
	  p_ini.push_back(-1.0); // default value, must be changed
      }
      
      sin>>name>>prob;
      map<string, int>:: iterator it_class=class_num.find(name);
      if (it_class!=class_num.end())
	p_ini[it_class->second]=atof(prob.c_str());
      else
	ERROR_CRASH("Class name for NER: \""+name+"\" not found in class/int key");

    }
    
    else if (reading == 3) {
    // Reading transition probabilities for each class (one transition per line: "B I prob(B->I)")
      if (N==0) // we have not seen reading==1
	ERROR_CRASH("File "+P_file+" need to have classes name/numbers before probability values");
      if (p_trans.size()==0) { // first line, initialize vector
	vector <double> aux;
	for (int i=0; i<N; i++)
	  aux.push_back(-1.0); // default value, must be changed
	for (int i=0; i<N; i++)
	  p_trans.push_back(aux);
      }
      
      sin>>name>>name1>>prob;
      map<string, int>:: iterator it_class=class_num.find(name);
      map<string, int>:: iterator it_class1=class_num.find(name1);
      if (it_class!=class_num.end() && it_class1!=class_num.end())
	p_trans[it_class->second][it_class1->second]=atof(prob.c_str());
      else
	ERROR_CRASH("Class name for NER \""+name+"\" or \""+name1+"\", not found in class/int key");
    }
  }

  // Check that p_ini and p_trans have received correct values (between 0 and 1)
  for (int i=0; i<N; i++){
    if(p_ini[i]<0 || p_ini[i]>1)
      ERROR_CRASH("Invalid probability value for some initial probability");
    for (int j=0; j<N; j++) {
      if(p_trans[i][j]<0 || p_trans[i][j]>1)
	ERROR_CRASH("Invalid probability value for some transition probability");
    }
  }

  TRACE(3,"Viterbi created, from file "+P_file);
}

////////////////////////////////////////////////////////////////
/// find_best_path: perform viterbi algortihm given the weights matrix
////////////////////////////////////////////////////////////////

vector<int> vis_viterbi::find_best_path(double** predictions, int sent_size) const {

  long double p, max, sum;
  int argmax=0;

  TRACE(3,"  processing sentence of size: "+util::int2string(sent_size));
  
  // vector with the most likely class for each word
  vector<int> best_path[N];
  
  // array with the best path for reaching current word with each 
  //  possible class
  long double p_path[N];
  long double p_path_new[N];
  
  // initialize this array with the weights for the first word
  //  multiplied by initial probability
  
  // normalizing factor to convert weights in probabilities -- p(i)=exp(w(i))/sum(exp(w(j)))
  sum=0;
  for (int i=0; i<N; i++)
    sum+=exp(predictions[i][0]);
  for (int i=0; i<N; i++) {
    p_path[i]=p_ini[i]*exp(predictions[i][0])/sum;
    TRACE(4,"   initial prob for class "+util::int2string(i)+": "+util::int2string(p_path[i]));
    TRACE(4,"         p_ini["+util::int2string(i)+"]: "+util::double2string(p_ini[i]));
    TRACE(4,"         p_pred: "+util::double2string(exp(predictions[i][0])/sum));
  }

  // predictions contains the weights for each class and each word,
  //  use these weights  and transition probabilities to compute 
  //  most likely path.
  for (int w=1; w<sent_size; w++){ // for each word starting in the second one
    TRACE(4," studying word in position "+util::int2string(w));
    
    // sumarize previous probabilities
    TRACE(4," previous probabilities: ");
    for (int i=0; i<N; i++){ 
      TRACE(4,"   "+util::double2string(p_path[i]));
    }
    
    // normalizing factor to convert weights in probabilities -- p(i)=exp(w(i))/sum(exp(w(j)))
    sum=0;
    for (int i=0; i<N; i++)
      sum+=exp(predictions[i][w]);
    
    for (int i=0; i<N; i++){ // for each class, store the best probability
      TRACE(5,"    store best probability for class "+util::int2string(i));
      TRACE(5,"      predictions: "+util::double2string(predictions[i][w])+" p_pred: "+ util::double2string(exp(predictions[i][w])/sum)); 
      max=0;
      for (int j=0; j<N; j++) { 
	p=p_path[j]*(exp(predictions[i][w])/sum)*p_trans[j][i];
	TRACE(5,"      p_trans: "+util::double2string(p_trans[j][i])+" p_path["+util::int2string(j)+"]: "+ util::double2string(p_path[j]));
	TRACE(5,"     new p: "+util::double2string(p));
	if(max<p) {
	  max=p;
	  argmax=j;
	}
      }
      // store most likely path for this class.
      TRACE(5,"    best probability: "+util::double2string(max)+" best_path: "+util::int2string(argmax));
      best_path[i].push_back(argmax);
      p_path_new[i]=max;
    }
    
    // store p_path_new as p_path
    for (int i=0; i<N; i++)
      p_path[i]=p_path_new[i];
  }
  
  // once the last word is reached, choose the best path
  max=0;
  for (int i=0; i<N; i++) {
    if(max<p_path[i]) {
      max=p_path[i];
      argmax=i;
    }
  }
  
  // store best class for last word in best_path
  best_path[argmax].push_back(argmax);
  
  TRACE(3,"Best path found");

  return (best_path[argmax]);
}

