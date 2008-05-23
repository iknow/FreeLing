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

#include <sstream>
#include <fstream>

#include "freeling/locutions.h"
#include "fries/util.h"
#include "freeling/traces.h"

using namespace std;

#define MOD_TRACENAME "LOCUTIONS"
#define MOD_TRACECODE LOCUT_TRACE

//-----------------------------------------------//
//        Multiword recognizer                   //
//-----------------------------------------------//

// State codes 
// states for a multiword recognizer
#define P 1  // initial state. A mw prefix is found
#define M 2  // Complete multiword found
// stop state
#define STOP 3

// Token codes
#define TK_pref   1 // the arriving form with the accumulated mw forms a valid mw prefix
#define TK_mw     2 // the arriving form with the accumulated mw forms a valid complete mw
#define TK_prefL  3 // the arriving lemma with the accumulated mw forms a valid mw prefix
#define TK_mwL    4 // the arriving lemma with the accumulated mw forms a valid complete mw
#define TK_prefP  5 // the arriving tag with the accumulated mw forms a valid mw prefix
#define TK_mwP    6 // the arriving tag with the accumulated mw forms a valid complete mw
#define TK_other  7


///////////////////////////////////////////////////////////////
///  Create a multiword recognizer, loading multiword file.
///////////////////////////////////////////////////////////////

locutions::locutions(const std::string &locFile): automat()
{
  string line;

  if (locFile!="") { // if no file given, wait for later manual locution loading
    // open locutions file
    ifstream fabr (locFile.c_str());
    if (!fabr) { 
      ERROR_CRASH("Error opening file "+locFile);
    }
    
    // loading locutions and prefixes map
    while (getline(fabr, line))
      add_locution(line);
    
    fabr.close(); 
  }

  // Initialize special state attributes
  initialState=P; stopState=STOP;
  
  // Initialize Final state set 
  Final.insert(M);
  
  // Initialize transitions table. By default, stop state
  int s,t;
  for(s=0;s<MAX_STATES;s++) for(t=0;t<MAX_TOKENS;t++) trans[s][t]=STOP;
  
  // Initializing transitions table
  // State P
  trans[P][TK_pref]=P; trans[P][TK_mw]=M;   
  trans[P][TK_prefL]=P; trans[P][TK_mwL]=M;   
  trans[P][TK_prefP]=P; trans[P][TK_mwP]=M;   
  // State M
  trans[M][TK_pref]=P; trans[M][TK_mw]=M; 
  trans[M][TK_prefL]=P; trans[M][TK_mwL]=M; 
  trans[M][TK_prefP]=P; trans[M][TK_mwP]=M; 

  TRACE(3,"analyzer succesfully created");
}


///////////////////////////////////////////////////////////////
///  Add a locution rule to the multiword recognizer
///////////////////////////////////////////////////////////////

void locutions::add_locution(const std::string &line) {
string prefix, key, lemma, tag;
string::size_type p;   

  istringstream sin;
  sin.str(line);
  sin>>key>>lemma>>tag;
  
  // store multiword in multiwords map
  locut.insert(make_pair(key,lemma+" "+tag));
  
  // store all its prefixes (xxx_, xxx_yyy_, xxx_yyy_zzz_, ...) in prefixes map 
  prefix="";
  p = key.find_first_of("_");
  while (p!=string::npos) {
    
    prefix += key.substr(0,p+1);                         	 
    prefixes.insert(prefix);  
    
    key = key.substr(p+1);
    p = key.find_first_of("_");
  }  
}


//-- Implementation of virtual functions from class automat --//


void locutions::check(const std::string s, std::set<std::string> &acc, bool &mw, bool &pref) 
{  
  if (locut.find(s)!=locut.end()) {
    acc.insert(s); 
    longest_mw=acc;
    over_longest=0;
    TRACE(3,"  Added MW: "+s);
    mw=true;
  }
  else if (prefixes.find(s+"_")!=prefixes.end()) {
    acc.insert(s); 
    TRACE(3,"  Added PRF: "+s);
    pref=true;
  }
}


///////////////////////////////////////////////////////////////
///  Compute the right token code for word j from given state.
///////////////////////////////////////////////////////////////

int locutions::ComputeToken(int state, sentence::iterator &j, sentence &se)
{
  set<string> acc;
  string form, lem, tag;
  set<string>::const_iterator i;
  word::const_iterator a;
  int token;
  bool mw,pref;
  string s;
 
  // store component
  components.push_back(*j);
  form = util::lowercase(j->get_form());
  
  token = TK_other;

  // look for first analysis matching some locution or prefix

  // if no analysis, check only the form
  mw=false; pref=false;
  if (j->size() == 0) {
      TRACE(3,"checking ("+form+")");
      if (acc_mw.empty()) {
	check(form,acc,mw,pref); 
      }
      else {
	for (i=acc_mw.begin(); i!=acc_mw.end(); i++) {
	  TRACE(3,"   acc_mw: ["+(*i)+"]");
	  check((*i)+"_"+form,acc,mw,pref); 
	}
      }
  }
  else { // there are anylsis, check all multiword patterns
    for (a=j->begin(); a!=j->end(); a++) {
      bool bm=false,bp=false;
      lem = "<"+a->get_lemma()+">";
      tag = a->get_parole();
      TRACE(3,"checking ("+form+","+lem+","+tag+")");
      
      if (acc_mw.empty()) {
	check(form,acc,bm,bp); 
	check(lem,acc,bm,bp);  
	check(tag,acc,bm,bp);  
	mw=mw||bm; pref=pref||bp; 
      }
      else {
	for (i=acc_mw.begin(); i!=acc_mw.end(); i++) {
	  TRACE(3,"   acc_mw: ["+(*i)+"]");
	  check((*i)+"_"+form,acc,bm,bp); 
	  check((*i)+"_"+lem,acc,bm,bp);  
	  check((*i)+"_"+tag,acc,bm,bp);  
	  mw=mw||bm; pref=pref||bp; 
	}
      }
    }
  }

  TRACE(3," fora: "+(mw?string("MW"):string("noMW"))+","+(pref?string("PREF"):string("noPREF")));


  if (mw) token=TK_mw;
  else if (pref) token=TK_pref;

  over_longest++;
  acc_mw = acc;

  TRACE(3,"Next word is: ["+form+","+lem+","+tag+"] token="+util::int2string(token));
  return (token);
}

///////////////////////////////////////////////////////////////
///   Reset current multiword acumulator.
///////////////////////////////////////////////////////////////

void locutions::ResetActions() 
{
  longest_mw=acc_mw;
  acc_mw.clear();
  components.clear();
  mw_analysis.clear();
}



///////////////////////////////////////////////////////////////
///  Perform necessary actions in "state" reached from state 
///  "origin" via word j interpreted as code "token":
///  Basically, when reaching a state, update accumulated multiword.
///////////////////////////////////////////////////////////////

void locutions::StateActions(int origin, int state, int token, sentence::const_iterator j)
{
  string form, lem, tag;

  // longest_mw=acc_mw;

  TRACE(3,"State actions completed. mw="+util::int2string(longest_mw.size()));

  if (!longest_mw.empty())  TRACE(3,"State actions completed. LMW="+*longest_mw.begin());
}


///////////////////////////////////////////////////////////////
/// Perform last minute validation before effectively building multiword.
///////////////////////////////////////////////////////////////

bool locutions::ValidMultiWord(const word & w) {

  string form,lemma,tag,check,par;
  map<string,string>::const_iterator mw_data;
  unsigned int n,nc;
  string::size_type p,q;
  word::const_iterator a;
  list<analysis> la;
  bool valid;

  // Setting the analysis for the multiword
  
  TRACE(3," Form MW: ("+util::lowercase(w.get_form())+") "+" comp="+util::int2string(components.size()-over_longest+1));
  
  TRACE(3," longest_mw #candidates: ("+util::int2string(longest_mw.size())+")");
  string lmw=*longest_mw.begin();  
  TRACE(3," longest mw: ("+lmw+")");

  q = lmw.find("_",0);
  if (q!=string::npos) {
    form=lmw.substr(0,q);
    for (n=1; n<components.size()-over_longest+1; n++) {
      p = q; q = lmw.find("_",q+1);
      form=form+lmw.substr(p,q-p);
    }
  }
  else form=lmw;
  TRACE(3," matched locution: ("+form+")");
   
  mw_data=locut.find(form);
  if (mw_data!=locut.end()) {
    istringstream sin;
    sin.str(mw_data->second);
    sin>>lemma>>tag;
  }

  // the tag is straighforward, use as is.
  if (tag[0]!='$') {
    la.push_back(analysis(lemma,tag));    
    valid = true;
  }
  else {
    // the tag is NOT straighforward, must be recovered from the components

    // locate end of component number, and extract the number
    p = tag.find(":",0);
    if (p==string::npos) ERROR_CRASH("Invalid tag in locution entry: "+form+" "+lemma+" "+tag);
    check=tag.substr(p+1);
    // get the condition on the PoS after the component number (e.g. $1:NC)
    nc=util::string2int(tag.substr(1,p-1));
    TRACE(3,"Getting tag from word $"+util::int2string(nc)+", constraint: "+check);

    // search all analysis in the given component that match the PoS condition,
    bool found=false;
    for (a=components[nc-1].begin(); a!=components[nc-1].end(); a++) { 
      TRACE(4,"  checking analysis: "+a->get_lemma()+" "+a->get_parole());
      par=a->get_parole();
      if (par.find(check)==0) {
        found=true;
        la.push_back(analysis(lemma,par));
      }
    }

    if (!found) TRACE(2,"Validation Failed: Tag "+tag+" not found in word. Locution entry: "+form+" "+lemma+" "+tag);
    valid = found;
  }

  mw_analysis = la;
  return(valid);
}


///////////////////////////////////////////////////////////////
///   Set the appropriate lemma and parole for the
///   new multiword.
///////////////////////////////////////////////////////////////

void locutions::SetMultiwordAnalysis(sentence::iterator i, int fstate) {
  i->set_analysis(mw_analysis);
  TRACE(3,"Analysis set to: ("+i->get_lemma()+","+i->get_parole()+")");
}

