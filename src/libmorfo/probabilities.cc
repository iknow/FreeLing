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
#include <set>

#include "freeling/probabilities.h"
#include "fries/util.h"
#include "freeling/traces.h"

using namespace std;

#define MOD_TRACENAME "PROBABILITIES"
#define MOD_TRACECODE PROB_TRACE


///////////////////////////////////////////////////////////////
/// Create a probability assignation module, loading
/// appropriate file.
///////////////////////////////////////////////////////////////

probabilities::probabilities(const std::string &Lang, const std::string &probFile, double Threshold) : RE_PunctNum(RE_FZ) {
  string line;
  string key,clas,frq,tag;
  map<string,double>::iterator k;
  map<string,double> temp_map;
  double probab, sumUnk, numUnk, count;
  int reading;

  Language = Lang;
  ProbabilityThreshold = Threshold;

  // default values, in case config file doesn't specify them. Values of 1.0 mean no effect.
  CheckerOverGuesser = 1.0;
  ExactMatchBonus = 1.0; 
  AlternativeAnalysisMass = 1.0;

  ifstream fabr (probFile.c_str());
  if (!fabr) { 
    ERROR_CRASH("Error opening file "+probFile);
  }
  
  reading=0; sumUnk=0; numUnk=0; long_suff=0;
  while (getline(fabr,line)) {
    
    istringstream sin;
    sin.str(line);
    
    if (line == "<SingleTagFreq>") reading=1;
    else if (line == "</SingleTagFreq>") reading=0;
    else if (line == "<ClassTagFreq>") reading=2;
    else if (line == "</ClassTagFreq>") reading=0;
    else if (line == "<FormTagFreq>")  reading=3;
    else if (line == "</FormTagFreq>") reading=0;
    else if (line == "<UnknownTags>")    reading=4;
    else if (line == "</UnknownTags>")   reading=0;
    else if (line == "<Theeta>")  reading=5;
    else if (line == "</Theeta>") reading=0;
    else if (line == "<Suffixes>")  reading=6;
    else if (line == "</Suffixes>") reading=0;    
    else if (line == "<CheckerOverGuesser>") reading=7;    
    else if (line == "</CheckerOverGuesser>") reading=0;    
    else if (line == "<ExactMatchBonus>") reading=8;    
    else if (line == "</ExactMatchBonus>") reading=0;    
    else if (line == "<AlternativeAnalysisMass>") reading=9;    
    else if (line == "</AlternativeAnalysisMass>") reading=0;    
    else if (reading==1) {
      // reading Single tag frequencies
      sin>>key>>frq;
      probab=util::string2double(frq);
      single_tags.insert(make_pair(key,probab));
    }
    else if (reading==2) {
      // reading tag class frequencies
      temp_map.clear();
      sin>>key;
      while (sin>>tag>>frq) {
	probab=util::string2double(frq);
	temp_map.insert(make_pair(tag,probab));
      }
      class_tags.insert(make_pair(key,temp_map));
    }
    else if (reading==3) {
      // reading form tag frequencies
      temp_map.clear();
      sin>>key>>clas;
      while (sin>>tag>>frq) {
	probab=util::string2double(frq);
	temp_map.insert(make_pair(tag,probab));
      }
      lexical_tags.insert(make_pair(key,temp_map));
    }
    else if (reading==4) {
      // reading tags for unknown words
      sin>>tag>>frq;
      probab=util::string2double(frq);
      sumUnk += probab;
      unk_tags.insert(make_pair(tag,probab));
    }
    else if (reading==5) {
      // reading theeta parameter
      sin>>frq;
      theeta=util::string2double(frq);
    }
    else if (reading==6) {
      // reading suffixes for unknown words
      temp_map.clear();
      sin>>key>>frq;
      long_suff = (key.length()>long_suff ? key.length() : long_suff);
      count = util::string2double(frq);
      while (sin>>tag>>frq) {
	probab=util::string2double(frq)/count;
	temp_map.insert(make_pair(tag,probab));
      }
      unk_suffs.insert(make_pair(key,temp_map));
    }
    else if (reading==7) {
      // reading CheckerOverGuesser factor
      sin>>frq;
      CheckerOverGuesser=util::string2double(frq);
    }
    else if (reading==8) {
      // reading ExactMacthBonus factor
      sin>>frq;
      ExactMatchBonus=util::string2double(frq);
    }
    else if (reading==9) {
      // reading AlternativeAnalysisMass threshold
      sin>>frq;
      AlternativeAnalysisMass=util::string2double(frq);
    }
  }
  fabr.close(); 
    
  // normalizing the probabilities in unk_tags map
  for (k=unk_tags.begin(); k!=unk_tags.end(); k++)
    k->second = k->second / sumUnk;

  TRACE(3,"analyzer succesfully created");
}

/////////////////////////////////////////////////////////////////////////////
/// Annotate probabilities for each analysis of given word
/////////////////////////////////////////////////////////////////////////////

void probabilities::annotate_word(word &w) {
  
  bool knw;
  double sum;
  int na=w.get_n_analysis();
  int nb=0;
  TRACE(2,"--Assigning probabilitites to: "+w.get_form());
  
  if (w.has_alternatives()) {

    // if the spell cheker added some alternatives, smooth each of them individually
    double s=0;
    for (list<pair<word,double> >::iterator a=w.alternatives_begin(); a!=w.alternatives_end(); a++) {
      TRACE(3,"....Smoothing alternative: "+a->first.get_form());
      smoothing(a->first);
 
      // compute in n the number of relevant analysis for the 
      // alternative (biggest that sum more than Thrs). This is to
      // reduce the analysis count for alternatives with one very probable
      // analysis and many low-probability. 
      a->first.sort();
      int n=0;
      double pa=0;
      for (word::iterator j=a->first.begin(); pa<AlternativeAnalysisMass && j!=a->first.end(); j++) {
        pa += j->get_prob();
	n++;
      }

      nb += a->first.get_n_analysis();
      for (word::iterator j=a->first.begin(); j!=a->first.end(); j++) {
        // give bonus weight to exact phonetic matches
	double sim=(a->second==1.0 ? ExactMatchBonus : a->second);
	// ponderate probability for each analisys of the alternative with the 
        // word similarity, and taking into account the number of analysis
	double p= j->get_prob() * sim * n ;
	TRACE(4,"    weighting alternative analysis "+j->get_lemma()+" p="+util::double2string(p)+" n="+util::int2string(n)+" p0="+util::double2string(j->get_prob())+" sim="+util::double2string(sim));  

	j->set_prob(p);
	s += p;
      }
    }

    // normalize probabilities for analysis of the alternatives.
    for (list<pair<word,double> >::iterator a=w.alternatives_begin(); a!=w.alternatives_end(); a++) {
      for (word::iterator j=a->first.begin(); j!=a->first.end(); j++) {
	double p= j->get_prob()/s;
	j->set_prob(p);
      }
    }
  }

  // word found in dictionary, punctuation mark, number, or with retokenizable analysis 
  //  and with some analysis
  if ( na>0 && (w.found_in_dict() || w.find_tag_match(RE_PunctNum) || w.has_retokenizable())) {
    TRACE(2,"Form with analysis. Found in dict ("+string(w.found_in_dict()?"yes":"no")+") or punctuation ("+string(w.find_tag_match(RE_PunctNum)?"yes":"no")+") or has_retok ("+string(w.has_retokenizable()?"yes":"no")+")");
    knw=true;

    // smooth probabilities for original analysis
    smoothing(w);
    sum=1;
  }
  // word not found in dictionary, may (or may not) have
  // tags set by other modules (NE, dates, suffixes...)
  else {
    // form is unknown in the dictionary
    TRACE(2,"Form with NO analysis. Guessing");
    knw=false;
    
    // set uniform distribution for analysis from previous modules.
    const double mass=1.0;
    for (word::iterator li=w.begin(); li!=w.end(); li++)
      li->set_prob(mass/w.get_n_analysis());

    // guess possible tags, keeping some mass for previously assigned tags.
    // setting mass to higher values above, will give more weight to 
    // existing tags.
    sum=guesser(w,mass);
    
    // normalize probabilities of all accumulated tags
    for (word::iterator li=w.begin();  li!=w.end(); li++)
      li->set_prob(li->get_prob()/sum);

    // get number of analysis again, in case the guesser added some.
    na=w.get_n_analysis();
  }

  if (w.has_alternatives()) {
    /// move alternatives proposed by the spell checker to the analysis
    /// list, so that the tagger may take them into account.
    
    // take into account the relative amount of analysis vs alternatives
    double f=1.0;
    if (knw) {
      // For known words (knw), increase weitgh for dicc over spell checker 
      if (na>nb) f=(double)nb/(double)na; 
    }
    else {
      // For unknown words increase weight for spell checker (nb/na) 
      // over guesser, with bonus.
      f=CheckerOverGuesser*(double)nb/(double)na; 
      // make sure we are not *reducing* the weight for the checker
      if (f<1.0) f=1.0;
    }

    sum=1.0; // mass already assigned to analysis
    // copy alternatives to analysis, setting probability
    for (list<pair<word,double> >::iterator a=w.alternatives_begin(); a!=w.alternatives_end(); a++) {
      for (word::iterator j=a->first.begin(); j!=a->first.end(); j++) {
	double p= f*j->get_prob();
	j->set_prob(p);
	w.add_analysis(*j);

	sum += p;
	TRACE(3,"    copied alternative "+j->get_lemma()+" p="+util::double2string(p)+" sum="+util::double2string(sum)+" f="+util::double2string(f));  
      }
    }    
    
    TRACE(3,"Normalizing alternatives. Sum="+util::double2string(sum));
    // normalize probabilities of analysis + added alternatives
    for (word::iterator li=w.begin();  li!=w.end(); li++)
      li->set_prob(li->get_prob()/sum); 
  }

  w.sort(); // sort analysis by decreasing probability,

  //sorting may scramble selected/unselected list, and the tagger will
  // need all analysis to be selected. This should be transparent here
  // (probably move it to class 'word').
  w.select_all_analysis();

  // if any of the analisys of the word was retokenizable, assign probabilities
  // to its sub-words, just in case they are selected.
  for (word::iterator li=w.begin();  li!=w.end(); li++) {
    list<word> rtk=li->get_retokenizable();
    for (list<word>::iterator k=rtk.begin(); k!=rtk.end(); k++) 
      annotate_word(*k);
    li->set_retokenizable(rtk);
  }

}


/////////////////////////////////////////////////////////////////////////////
/// Annotate probabilities for each analysis of each word in given sentence,
/// using given options.
/////////////////////////////////////////////////////////////////////////////

void probabilities::annotate(sentence &se) {
sentence::iterator i;

  for (i=se.begin(); i!=se.end(); i++) {
    annotate_word(*i);    
  }

  TRACE_SENTENCE(1,se);
}

/////////////////////////////////////////////////////////////////////////////
/// Smooth probabilities for the analysis of given word
/////////////////////////////////////////////////////////////////////////////

void probabilities::smoothing(word &w) {

  int na=w.get_n_analysis();
  if (na==1) {
    // form is inambiguous
    TRACE(2,"Inambiguous form, set prob to 1");
    w.begin()->set_prob(1);
    return; // we're done here
  }

  // form has analysis. begin probability back-off
  TRACE(2,"Form with analysis. Smoothing probabilites.");

  // count occurrences of short tags 
  map<string,double> tags_short;
  for (word::iterator li=w.begin(); li!=w.end(); li++) {
    map<string,double>::iterator p=tags_short.find(li->get_short_parole(Language));
    if (p==tags_short.end())
       tags_short.insert(make_pair(li->get_short_parole(Language),1.0));
    else 
      p->second++;
  }
  
  // build word ambiguity class with and without NP tags
  string c=""; string cNP="";
  for (map<string,double>::iterator x=tags_short.begin(); x!=tags_short.end(); x++) {
    cNP += "-"+x->first;
    if ( x->first != "NP" ) c += "-"+x->first;
  }

  bool trysec=false;

  if (c==cNP) { // no NP tags, only one class
    cNP=cNP.substr(1);   
    TRACE(3,"Ambiguity class ("+cNP+")");
  }
  else {
    // there is one NP, check both classes
    cNP=cNP.substr(1);
    TRACE(3,"Ambiguity class ("+cNP+")");

    if (not c.empty()) {
      c=c.substr(1);
      TRACE(3,"Secondary ambiguity class ("+c+")");
      trysec=true;   // try secondary class if primary fails.
    }
    else 
      WARNING("Empty ambiguity class for word '"+w.get_form()+"'. Duplicate NP analysis??");

  }  

  map<string,double> temp_map;
  string form=util::lowercase(w.get_form());

  if (lexical_tags.find(form)!=lexical_tags.end()) {
    // word found in lexical probabilities list. Use them straightforwardly.
    TRACE(2,"Form '"+form+"' contained in the lexical_tags map");
    temp_map=lexical_tags[form];
  }  
  else if (class_tags.find(cNP)!=class_tags.end()){
    // Word not in lexical probs list. Back off to ambiguity class
    TRACE(2,"Ambiguity class '"+cNP+"' found in class_tags map");
    temp_map=class_tags[cNP];
  }
  else if (trysec and class_tags.find(c)!=class_tags.end()){
    // Ambiguity class not found. Try secondary class, if any.
    TRACE(2,"Secondary ambiguity class '"+c+"' found in class_tags map");
    temp_map=class_tags[c];
  }
  else {
    // Ambiguity class not found. Back off to unigram probs.
    TRACE(2,"unknown word and class. Using unigram probs");
    temp_map=single_tags;
  }	      
  
  double sum=0;
  for (map<string,double>::iterator x=tags_short.begin(); x!=tags_short.end(); x++) 
    sum += temp_map[x->first] * x->second;

  for (word::iterator li=w.begin(); li!=w.end(); li++)
    li->set_prob((temp_map[li->get_short_parole(Language)]+(1/(double)na))/(sum+1));
  
}



/////////////////////////////////////////////////////////////////////////////
/// Compute probability of a tag given a word suffix.
/////////////////////////////////////////////////////////////////////////////

double probabilities::compute_probability(const std::string &tag, double prob, const std::string &s) {
  map<string,map<string,double> >::const_iterator is;
  map<string,double>::const_iterator it;
  double x,pt;


  if (s == "") 
    return (prob);
  else {
    x = compute_probability(tag, prob, s.substr(1));
    
    // search suffix in map.
    pt = 0;
    is = unk_suffs.find(s);
    if (is != unk_suffs.end()) {
      // search tag in suffix probability list. It should be there.
      it = (is->second).find(tag);
      if (it != (is->second).end()) pt = it->second;
    }

    // obtain probability
    return (pt + theeta*x)/(1+theeta);
  }
}


/////////////////////////////////////////////////////////////////////////////
/// Guess possible tags, keeping some mass for previously assigned tags    
/////////////////////////////////////////////////////////////////////////////

double probabilities::guesser(word &w, double mass) {

  string form=util::lowercase(w.get_form());

  // find longest suffix of word in unk_suffs map
  string::size_type ln = form.length();
  string::size_type mx = (ln<long_suff ? ln : long_suff);
  string suf= form;
  for (int j=mx; j>=0 && unk_suffs.find(suf)==unk_suffs.end(); j--) {
    suf= form.substr(ln-j);
  }
  
  TRACE(2,"longest suf: "+suf);
  
  // mass = mass assigned so far.  This gives some more probability to the 
  // preassigned tags than to those computed from suffixes.
  double sum= (w.get_n_analysis()>0 ? mass : 0);
  double sum2=0;
    
  TRACE(2,"initial sum=: "+util::double2string(sum));
  
  // to store analysis under threshold, just in case
  list<analysis> la;
  // for each possible tag, compute probability
  for (map<string,double>::iterator t=unk_tags.begin(); t!=unk_tags.end(); t++) {
    
    // See if it was already there, set by some other module
    bool hasit=false;
    for (word::iterator li=w.begin(); li!=w.end() && !hasit; li++)
      hasit= (t->first.find(li->get_short_parole(Language))==0);
    
    // if we don't have it, consider including it in the list
    if (!hasit) {
      
      double p = compute_probability(t->first,t->second,suf);
      analysis a(form,t->first);
      a.set_prob(p);
      
      TRACE(2,"   tag:"+t->first+" ("+(hasit?"had it":"new")+")  pr="+util::double2string(p)+" "+util::double2string(t->second));
      
      // if the tag is new and higher than the threshold, keep it.
      if (p >= ProbabilityThreshold) {
	sum += p;
	w.add_analysis(a);
	TRACE(2,"    added. sum is: "+util::double2string(sum));
      }
      // store candidates under treshold, just in case we need them later
      else  {
	sum2 += p;
	la.push_back(a);
      }
    }
  }
  
  // if the word had no analysis, and no candidates passed the threshold, assign them
  // anyway, to avoid words with zero analysis
  if (w.get_n_analysis()==0) {    
    w.set_analysis(la);
    sum = sum2;
  }

  return sum;
}
