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

#include "freeling/hmm_tagger.h"
#include "fries/util.h"
#include "freeling/traces.h"

using namespace std;

#define MOD_TRACENAME "HMMTAGGER"
#define MOD_TRACECODE HMM_TRACE

//---------- Viterbi Class  ----------------------------------

////////////////////////////////////////////////////////////////
/// Constructor: Create dynammic storage for delta and phi 
/// tables used by Viterbi algortihm.
////////////////////////////////////////////////////////////////

viterbi::viterbi(int T) {  
   delta_log=new map <string, double> [T];
   phi=new map <string, string> [T];
}

////////////////////////////////////////////////////////////////
/// Destructor: Free dynammic storage for delta and phi tables 
////////////////////////////////////////////////////////////////

viterbi::~viterbi() {
  delete[] delta_log;
  delete[] phi;
}


//---------- HMMTagger Class  ----------------------------------

///////////////////////////////////////////////////////////////
///  Constructor: Build a HMM tagger, loading probability tables.
///////////////////////////////////////////////////////////////

hmm_tagger::hmm_tagger(const std::string &lang, const std::string &HMM_File, bool rtk, unsigned int force) : POS_tagger(rtk,force)
{
  double prob, coef;
  string nom1,nom2,categ,line,aux;
  int reading;

  Language = lang;

  ZERO_logprob = log(0); // -inf

  ifstream model (HMM_File.c_str());
  if (!model) {
    ERROR_CRASH("Error opening file "+HMM_File);
  }

  reading=0;
  while (getline(model,line)) {

    istringstream sin;
    sin.str(line);
    
    if (line == "<Tag>") reading=1;
    else if (line == "</Tag>") reading=0;

    else if (line == "<Bigram>") reading=2;
    else if (line == "</Bigram>") reading=0;

    else if (line == "<Trigram>") reading=3;
    else if (line == "</Trigram>") reading=0;

    else if (line == "<Initial>") reading=4;
    else if (line == "</Initial>") reading=0;

    else if (line == "<Word>") reading=5;
    else if (line == "</Word>") reading=0;
 
    else if (line == "<Smoothing>") reading=6;
    else if (line == "</Smoothing>") reading=0;

    else if (line == "<Forbidden>") reading=7;
    else if (line == "</Forbidden>") reading=0;
 
    else if (reading == 1) {
      // Reading tag probabilities
      sin>>nom1>>prob;
      PTag.insert(make_pair(nom1,prob));
    }

    else if (reading == 2) {
      // Reading bigram probabilities
      sin>>nom1>>prob;
      PBg.insert(make_pair(nom1,prob));
    }

    else if (reading == 3) {
      // Reading trigram probabilities
      sin>>nom1>>prob;
      PTrg.insert(make_pair(nom1,prob));
    }

    else if (reading == 4) {
      // Reading initial probabilities
      sin>>nom1>>prob;
      PInitial.insert(make_pair(nom1,prob));
    }

    else if (reading == 5) {
      // Reading word probabilities
      sin>>nom1>>prob;
      PWord.insert(make_pair(nom1,prob));
    }

    else if (reading == 6) {
      // Reading the coeficients
      sin>>nom1>>coef;
      if (nom1=="c1") c[0]=coef;
      else if (nom1=="c2") c[1]=coef;
      else if (nom1=="c3") c[2]=coef;
    }

    else if (reading == 7) {
      // Reading forbidden transitions
      sin>>aux;
      bool err=false;
      vector<string> l;
      l.push_back("");l.push_back("");l.push_back("");

      TRACE(4," reading forbidden ("+aux+")");
      vector<string> ltg=util::string2vector(aux,".");
      for (int i=0; i<3; i++) {
        TRACE(4,"    ... processing  ("+ltg[i]+")");
	size_t p=ltg[i].find("<"); 
	if (p!=string::npos) {
	  l[i]=ltg[i].substr(p);
	  ltg[i]=ltg[i].substr(0,p);
	}
	// it is illegal to use a lema with * or 0, and
        // to use * or 0 out of first position
        err=((not l[i].empty() or i!=0) and (ltg[i]=="*" or ltg[i]=="0"));
      }

      if (err)
	WARNING("Wrong format for forbidden trigram '"+aux+"'. Ignored.");
      else {

	vector<string> stg;
	for (int i=0; i<3; i++) {     
	  if (ltg[i]=="*" and ltg[i]=="0") {
	    stg.push_back(ltg[i]);
	    ltg[i]= "";
	  }
	  else {
	    analysis an("",ltg[i]);
	    stg.push_back(an.get_short_parole(Language));
	    if (stg[i]==ltg[i]) ltg[i]="";
	  }
	}
      
        string tr=util::vector2string(stg,".");
	string lt=util::vector2string(ltg,".");
	string lm=util::vector2string(l,".");
		
	string s=""; 
        if (lt!=".." or lm!="..") s=lm+"#"+lt;

	TRACE(4,"Inserting forbidden ("+tr+","+s+")");
	Forbidden.insert(make_pair(tr,s));
      }
    }
  }

  TRACE(3,"analyzer succesfully created");
}


////////////////////////////////////////////////
/// check if a trigram is in the forbidden list.
////////////////////////////////////////////////

bool hmm_tagger::is_forbidden(const string &trig, sentence::const_iterator w) const {

  string lemm0,lemm1,lemm2;

  bool fbd=false;

  pair<map<string,string>::const_iterator,map<string,string>::const_iterator> range;
  range = Forbidden.equal_range(trig);

  // check all possible rules for that trigram, if any.
  for (map<string,string>::const_iterator t=range.first;
       t!=range.second and not fbd;
       t++) { 

    TRACE(4,"       Checking rule for forbidden trigram "+t->first+":"+t->second);
    if (t->second.empty())  // no lemmas, the trigram is always forbidden
      fbd=true;

    else  { 
      // if we get here, the trigram is forbidden only 
      // for some specific lemmas. Check if it's the case.

      vector<string> stags=util::string2vector(t->first,".");
      vector<string> vaux=util::string2vector(t->second,"#");
      vector<string> lems=util::string2vector(vaux[0],".");
      vector<string> ltags=util::string2vector(vaux[1],".");

      TRACE(4,"       check rule: stags=["+util::vector2string(stags,",")+"] lems=["+util::vector2string(lems,",")+"] ltags=["+util::vector2string(ltags,",")+"]");

      sentence::const_iterator wd = w;
      fbd=true; int i=3;
      while (i>0 and fbd) {
	i--;

	// if more detail than short tags is required, look for 
        // the pair tag-lemma in the corresponding word.
	if (not (lems[i].empty() and ltags[i].empty())) {

	  fbd=false;
	  for (word::const_iterator an=wd->begin(); an!=wd->end() and not fbd; an++) {
	    if (not ltags[i].empty() and not lems[i].empty()) {
              // we have both lemma and long tag
	      fbd = (ltags[i]==an->get_parole()) and (lems[i]=="<"+an->get_lemma()+">");
	      TRACE(4,"       ... checking "+ltags[i]+","+lems[i]+": "+(fbd?"forbidden":"allowed"));
	    }
	    else if (ltags[i].empty()) {  // we have lemma, but not long tag.
	      fbd = (stags[i]==an->get_short_parole(Language)) and 
                    (lems[i]=="<"+an->get_lemma()+">");
	      TRACE(4,"       ... checking "+lems[i]+" (no ltag): "+(fbd?"forbidden":"allowed"));
	    }
	    else { // we have long tag, but not lemma
	      fbd = (ltags[i]==an->get_parole());
	      TRACE(4,"       ... checking "+ltags[i]+" (no lemm): "+(fbd?"forbidden":"allowed"));
	    }
	  }	 
	}
	
	// check position 0 only if there is a lemma to check
        // (i.e. prevent checking for wildcards at sentence beggining)
	if (i>1 or not lems[0].empty()) wd--;
      }
    }
  }

  TRACE(4,"       ...trigram "+trig+" is "+(fbd?"forbidden.":"not forbidden."));
  return fbd;
}


////////////////////////////////////////////////
///  Compute transition log_probability from state_i to state_j,
///  returning appropriate smoothed values if no evidence is available.
///  If the trigram is in the "forbidden" list, result is probability zero.
////////////////////////////////////////////////

double hmm_tagger::ProbA_log(const std::string &state_i, const std::string &state_j, sentence::const_iterator w) const
{
  double prob;
  map <string, double>::const_iterator k;
  string t3, t2t3, t1t2t3; 

  prob=0;
  
  // state_i=t1.t2 --  state_j=t2.t3  
  t3=state_j.substr(state_j.find_last_of('.')+1); 
  t2t3=state_j;  
  t1t2t3=state_i+"."+t3;
  
  if (is_forbidden("*."+t2t3,w) or is_forbidden(t1t2t3,w)) 
    // if it's a forbidden transition, set zero probability
    prob=0;
  else {
    
    k=PTag.find(t3);
    if (k!=PTag.end())    // Tag found in map
      prob+=c[0]*(k->second);
    else {     // unobserved tag look for a generic one
      k=PTag.find("x");
      prob+=c[0]*(k->second);  
    }
    
    k=PBg.find(t2t3);
    if (k!=PBg.end())      // Bigram found in map
      prob+=c[1]*(k->second);
    
    k=PTrg.find(t1t2t3);
    if (k!=PTrg.end())    // Trigram found in map
      prob+=c[2]*(k->second);  
  }
  
  prob=log(prob);
    
  return prob;
}


///////////////////////////////////////////////////////////
/// Compute emission log_probability for observation obs
/// from state_i.
///   Pb=P(state|word)*P(word)/P(state)
///   approximating P(s|w) by: P(s|w)~=P(t3|w)*P(s)/P(t3)
///   thus,  Pb ~= P(t3|w)*P(w)/P(t3)
///////////////////////////////////////////////////////////

double hmm_tagger::ProbB_log(const std::string &state_i, const word & obs) const
{
  double pb_log,plog_word_tag,plog_word,plog_tag; 
  string tag;
  word::const_iterator a;
  map <string, double>::const_iterator k;

  // last tag in state_i (states are bigrams t1.t2)
  tag=state_i.substr(state_i.find_last_of('.')+1);

  // get observed word probability
  k=PWord.find(obs.get_form());
  if(k!=PWord.end())
    plog_word=k->second;
  else {
    // unobserved word, backoff probability
    k=PWord.find("<UNOBSERVED_WORD>");
    plog_word=k->second;
  }
  
  // get tag t2 probability
  k=PTag.find(tag);
  if(k!=PTag.end())
    plog_tag=log(k->second);
  else {
    // unobserved tag
    k=PTag.find("x");
    plog_tag=log(k->second);  
  }

  // Compute emission probability pb. 

  // We need first P(t2|w). (The tag will be in the list, since
  // the states are computed consistently with observations).
  for(a=obs.begin(); a->get_short_parole(Language)!=tag; a++);
  plog_word_tag=log(a->get_prob());

  // Compute pb  Pb ~= P(t2|w)*P(w)/P(t2)
  pb_log = plog_word_tag + plog_word - plog_tag;

  return pb_log;
}

  
///////////////////////////////////////////////////////////
/// Compute initial log_probability for  state_i.
///////////////////////////////////////////////////////////

double hmm_tagger::ProbPi_log(const std::string &state_i) const 
{
  double ppi_log;
  map <string, double>::const_iterator k;

  // Initial state probability
  k=PInitial.find(state_i);
  if(k!=PInitial.end())
    ppi_log=(*k).second;
  else if(state_i.find("0.")==0) {
    // Unobserved (but possible) initial state
    k=PInitial.find("0.x");
    ppi_log=k->second;
  }
  else {
    // non-initial state, zero probability, but log(0) = -inf, 
    ppi_log = ZERO_logprob;
  }

  return ppi_log;
}


///////////////////////////////////////////////////////////////  
///  Disambiguate given sentences with provided options  
///////////////////////////////////////////////////////////////  

void hmm_tagger::analyze(std::list<sentence> &ls) {
  list<emission_states> lemm;
  list<emission_states>::iterator emms,emmsant;
  emission_states::iterator k,kant;
  list<sentence>::iterator is;
  sentence::iterator w;
  word::iterator ka;
  list<word> av;
  double max, aux;  
  map<string,double>::iterator kd;
  map<string,string>::iterator ks;
  list<string> q;
  string tag, st, state_ant_max;
  int t;

  
  // tag each sentence in list.
  for (is=ls.begin(); is!=ls.end(); is++) {

    TRACE(3,"Analyze one sentence using Viterbi algorithm");

    // create tables to disambiguate current sentece
    viterbi v(is->size());

    // Compute possible emission states for each word
    lemm=FindStates(*is);

    // initialitation (first observation in sequence)
    w=is->begin();
    TRACE(3,"probability for initial word "+w->get_form());
    emms=lemm.begin();
    for (k=emms->begin(); k!=emms->end(); k++) {
      aux = ProbPi_log(*k)+ProbB_log(*k,*w);
      TRACE(3,"    possible emission from "+(*k)+" prob: "+util::double2string(aux));
      v.delta_log[0].insert(make_pair(*k,aux));
      v.phi[0].insert(make_pair(*k,"0"));
    }
    
    // compute best path
    TRACE(3,"Computing the best path");
    t=1;
    emmsant=lemm.begin();  emms=++lemm.begin(); 
    for (w=++is->begin(); w!=is->end(); w++) {

      TRACE(3,"probability for "+w->get_form());
      for(k=emms->begin(); k!=emms->end(); k++) {

        TRACE(3,"    possible emission from "+(*k));
	max= ZERO_logprob;
	for (kant=emmsant->begin(); kant!=emmsant->end(); kant++) {
	  // ignore nonsense transitions. E.g, check A.B->B.C
          // but not transition  A.B->C.D
          if(kant->substr(kant->find_last_of('.')+1)==k->substr(0,k->find_last_of('.'))) {
	    kd=v.delta_log[t-1].find(*kant);
	    if(kd!=v.delta_log[t-1].end()) {
	      aux=(kd->second)+ProbA_log(*kant,*k,w);
	    }
	    else 
	      aux = ZERO_logprob;
            TRACE(3,"       coming from "+(*kant)+"  prob "+util::double2string(aux)); 
	    if (max <= aux) {
	      max=aux;
	      state_ant_max=*kant;
              TRACE(3,"        new max");
	    }
	  }
	}

	aux = max+ProbB_log(*k,*w);
	
	TRACE(3,"       **probability for "+w->get_form()+","+(*k)+": "+util::double2string(aux)+", from:"+state_ant_max+" emm prob="+(((aux==max)&&(max==ZERO_logprob))? util::double2string(max) : util::double2string(aux-max)));
	v.delta_log[t].insert(make_pair(*k,aux));
	v.phi[t].insert(make_pair(*k,state_ant_max));
      }
      
      t++;      
      emmsant=emms;
      emms++;
    }
    
    // Termination state, last word.
    max=ZERO_logprob;
    w=--is->end();
    emms=--lemm.end();
    for(k=emms->begin(); k!=emms->end(); k++) {
	kd=v.delta_log[is->size()-1].find(*k);
	if(kd!=v.delta_log[is->size()-1].end())
	  aux=kd->second;
	else 
	  aux=ZERO_logprob;
	
	TRACE(4, "       delta for "+(*k)+"is "+util::double2string(aux));
	if(max<=aux) {
	  max=aux;
	  st=*k; //last state with higher probability
	  TRACE(4, "       better state found "+st);
	}
    }

    TRACE(3, "Recovering the path, last state="+st);
    tag=st.substr(st.find_last_of(".")+1); //last tag of the st
                                          //(most likely tag for the last word)
    w=--is->end();
    for (t=is->size()-1; t>=0; t--) {  
      // erase any previous selection 
      w->unselect_all_analysis();

      // get the tags with highest prob among those possible
      list<word::iterator> bestk;
      max=0.0;
      TRACE(3, "Word: "+w->get_form()+" with tag "+tag);
      for (ka=w->begin();  ka!=w->end();  ka++) {
	TRACE(3, "   Cheking analysis: "+ka->get_lemma()+" "+ka->get_parole());
	if (ka->get_short_parole(Language)==tag) {
          if (ka->get_prob()>max) {
	    TRACE(3, "   ** selected! (new max)");
	    max=ka->get_prob();
	    bestk.clear();
	    bestk.push_back(ka);
	  }
	  else if (ka->get_prob()==max) {
	    TRACE(3, "   ** selected (added)");
	    bestk.push_back(ka);
	  }
	}
      }

      // mark them as selected analysis for that word
      for (list<word::iterator>::iterator k=bestk.begin(); k!=bestk.end(); k++)
	w->select_analysis(*k);

      // backtrack one more step if possible
      if (t>0) {
	ks=v.phi[t].find(st);
	if (ks != v.phi[t].end()) st=ks->second;
	tag=st.substr(st.find_last_of(".")+1);
	w--;
      }
    }

    TRACE(3,"sentence analyzed"); 
  }

  // once everything is tagged, force select if required
  force_select(ls);
  // ... and perform retokenization if needed.
  retokenize(ls);
}

///////////////////////////////////////////////////////////////  
///  Disambiguate given sentences, return analyzed copy
///////////////////////////////////////////////////////////////  

std::list<sentence> hmm_tagger::analyze(const std::list<sentence> &ls) {
  list<sentence> al;

  al=ls;
  analyze(al);
  return(al);
}



///////////////////////////////////////////////////////////////  
///  Obtain a list with the states that *may* have emmited 
///  current observation (a sentence).
///////////////////////////////////////////////////////////////  

list<emission_states> hmm_tagger::FindStates(const sentence & sent) const {

  emission_states st; //list with the states that may have emmited two consecutive words
  list<emission_states> ls;
  sentence::const_iterator w1,w2;
  string t1,t2;
  word::const_iterator a1,a2;

  // note that we only consider *selected* analysis for each word, which
  // may be all if the previous step was a morpho analyzer, or just a few 
  // if some kind of predesambiguation has been performed.

  // deal with first word
  w2=sent.begin();
  TRACE(3,"obtaining the states that may have emited the word: "+w2->get_form());
  for (a2=w2->selected_begin(); a2!=w2->selected_end(); a2++) 
    st.insert("0."+a2->get_short_parole(Language));
  ls.push_back(st);

  // step to second word
  w1=w2; w2++;

  // deal with each word in sentence (from 2nd to last)
  for (; w2!=sent.end();  w1=w2, w2++) {
    // compute list of possible trigrams according to two previous words.
    TRACE(3,"obtaining the states that may have emited the word: "+w2->get_form());
    st.clear();

    for (a1=w1->selected_begin(); a1!=w1->selected_end(); a1++)
      for (a2=w2->selected_begin(); a2!=w2->selected_end(); a2++)
	st.insert(a1->get_short_parole(Language) + "." + a2->get_short_parole(Language));

    // add list for current word to global list of lists.
    ls.push_back(st);
  }

  return ls;
}




