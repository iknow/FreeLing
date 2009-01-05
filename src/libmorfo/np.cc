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
#include <iostream>  

#include "freeling/np.h"
#include "fries/util.h"
#include "freeling/traces.h"

using namespace std;

#define MOD_TRACENAME "NP"
#define MOD_TRACECODE NP_TRACE

//-----------------------------------------------//
//       Proper noun recognizer                  //
//-----------------------------------------------//

// State codes
// recognize named entities (NE).
#define IN 1   // initial
#define NP 2   // Capitalized word, much likely a NE component
#define FUN 3  // Functional word inside a NE
#define STOP 4

// Token codes
#define TK_sUnkUpp  1   // non-functional, unknown word, begining of sentence, capitalized, with no analysis yet
#define TK_sNounUpp 2   // non-functional, known as noun, begining of sentence, capitalized, with no analysis yet
#define TK_mUpper 3     // capitalized, not at beggining of sentence
#define TK_mFun  4   // functional word, non-capitalized, not at beggining of sentence
#define TK_other 5



///////////////////////////////////////////////////////////////
///     Create a proper noun recognizer
///////////////////////////////////////////////////////////////

np::np(const std::string &npFile): ner(), automat(), RE_NounAdj(RE_NA), RE_Closed(RE_CLO), RE_DateNumPunct(RE_DNP)
{
  int reading;

  string line; 
  ifstream fabr (npFile.c_str());
  if (!fabr) { 
    ERROR_CRASH("Error opening file "+npFile);
  }
  
  // default
  Title_length = 0;
  splitNPs=false;
  
  // load list of functional words that may be included in a NE.
  reading=0;
  while (fabr >> line) {
    if (line == "<FunctionWords>") reading=1;
    else if (line == "</FunctionWords>") reading=0;
    else if (line == "<SpecialPunct>") reading=2;
    else if (line == "</SpecialPunct>") reading=0;
    else if (line == "<NE_Tag>") reading=3;
    else if (line == "</NE_Tag>") reading=0;
    else if (line == "<TitleLimit>") reading=4;
    else if (line == "</TitleLimit>") reading=0;
    else if (line == "<Names>") reading=5;
    else if (line == "</Names>") reading=0;
    else if (line == "<Ignore>") reading=6;
    else if (line == "</Ignore>") reading=0;
    else if (line == "<RE_NounAdj>") reading=7;
    else if (line == "</RE_NounAdj>") reading=0;
    else if (line == "<RE_Closed>") reading=8;
    else if (line == "</RE_Closed>") reading=0;
    else if (line == "<RE_DateNumPunct>") reading=9;
    else if (line == "</RE_DateNumPunct>") reading=0;
    else if (line == "<SplitMultiwords>") reading = 10;
    else if (line == "</SplitMultiwords>") reading = 0;
    else if (reading==1)   // reading Function words
      func.insert(line);
    else if (reading==2)   // reading special punctuation tags
      punct.insert(line); 
    else if (reading==3)  // reading tag to assign to detected NEs
      NE_tag=line;
    else if (reading==4)  // reading value for Title_length 
      Title_length = util::string2int(line);
    else if (reading==5)  // reading list of words to consider names when at line beggining
      names.insert(line);
    else if (reading==6)  // reading list of words to ignore as possible NEs, even if they are capitalized
      ignore.insert(line);
    else if (reading==7) {
      RegEx re(line);     
      RE_NounAdj=re;
    }
    else if (reading==8) {
      RegEx re(line);     
      RE_Closed=re;
    }
    else if (reading==9) {
      RegEx re(line);     
      RE_DateNumPunct=re;
    }
    else if (reading==10)
      splitNPs = (line.compare("yes")==0);
  }
  fabr.close(); 
  
  // Initialize special state attributes
  initialState=IN; stopState=STOP;
  // Initialize Final state set 
  Final.insert(NP); 
  
  // Initialize transitions table. By default, stop state
  int s,t;
  for(s=0;s<MAX_STATES;s++) for(t=0;t<MAX_TOKENS;t++) trans[s][t]=STOP;
  
  // Initializing transitions table
  // State IN
  trans[IN][TK_sUnkUpp]=NP; trans[IN][TK_sNounUpp]=NP; trans[IN][TK_mUpper]=NP;
  // State NP
  trans[NP][TK_mUpper]=NP;
  trans[NP][TK_mFun]=FUN;
  // State FUN
  trans[FUN][TK_sUnkUpp]=NP; trans[FUN][TK_sNounUpp]=NP; trans[FUN][TK_mUpper]=NP;
  trans[FUN][TK_mFun]=FUN;

  TRACE(3,"analyzer succesfully created");
}


//-- Implementation of virtual functions from class automat --//

///////////////////////////////////////////////////////////////
/// Specify that "annotate" must be inherited from "automat" and not from "ner"
///////////////////////////////////////////////////////////////
void np::annotate(sentence &se){
  automat::annotate(se);
}

///////////////////////////////////////////////////////////////
///  Compute the right token code for word j from given state.
///////////////////////////////////////////////////////////////

int np::ComputeToken(int state, sentence::iterator &j, sentence &se)
{
  string form, formU, tagant;
  int token;
  bool sbegin;

  formU = j->get_form();
  form = util::lowercase(formU);

  token = TK_other;

  if (j==se.begin()) {
    // we are the first word in sentence
    sbegin=true;
  }
  else {
     // not the first, locate previous word...
    sentence::const_iterator ant=j; ant--; 
    // ...and check if it has any of the listed punctuation tags 
    sbegin=false;    
    for (word::const_iterator a=ant->begin(); a!=ant->end() && !sbegin; a++)
      sbegin = (punct.find(a->get_parole())!=punct.end());    
  }
  
  // ignorable word
  if (ignore.find(form)!=ignore.end()) {
    TRACE(3,"Ignorable word ("+form+")");
    if (state==NP) 
      token=TK_mUpper;  // if inside a NE, do not ignore
    else {
      // if not inside, only form NE if followed by another capitalized word
      sentence::iterator nxt=j; nxt++;
      if (nxt!=se.end() && util::isuppercase(nxt->get_form()[0]))
	// set token depending on if it's first word in sentence
	token= (sbegin? TK_sNounUpp: TK_mUpper);
    }
  }
  // non-ignorable
  else if (sbegin) { 
    TRACE(3,"non-ignorable word, sbegin ("+form+")");
    // first word in sentence (or word preceded by special punctuation sign), and not locked
    if (!j->is_locked() && util::isuppercase(formU[0]) &&
        func.find(form)==func.end() &&
        !j->is_multiword() && !j->find_tag_match(RE_DateNumPunct)) {
      // capitalized, not in function word list, no analysis except dictionary.

      // check for unknown/known word
      if (j->get_n_analysis()==0) {
	// not found in dictionary
	token = TK_sUnkUpp;
      }
      else if ( !j->find_tag_match(RE_Closed) && (j->find_tag_match(RE_NounAdj) || names.find(form)!=names.end() ) ) {
	// found as noun with no closed category
        // (prep, determiner, conjunction...) readings
	token = TK_sNounUpp;
      }
    }
  }
  else if (!j->is_locked()) {
    TRACE(3,"non-ignorable word, non-locked ("+form+")");
    // non-ignorable, not at sentence beggining, non-locked word
    if (util::isuppercase(formU[0]) && !j->find_tag_match(RE_DateNumPunct))
      // Capitalized and not number/date
      token=TK_mUpper;
    else if (func.find(form)!=func.end())
      // in list of functional words
      token=TK_mFun;
  }
 
  TRACE(3,"Next word form is: ["+formU+"] token="+util::int2string(token));
  TRACE(3,"Leaving state "+util::int2string(state)+" with token "+util::int2string(token)); 
  return (token);
}


///////////////////////////////////////////////////////////////
///   Reset flag about capitalized noun at sentence start.
///////////////////////////////////////////////////////////////

void np::ResetActions() 
{
  initialNoun=false;
}



///////////////////////////////////////////////////////////////
///  Perform necessary actions in "state" reached from state 
///  "origin" via word j interpreted as code "token":
///  Basically, set flag about capitalized noun at sentence start.
///////////////////////////////////////////////////////////////

void np::StateActions(int origin, int state, int token, sentence::const_iterator j)
{

  // if we found a capitalized noun at sentence beginning, remember it.
  if (state==NP) {
    TRACE(3,"actions for state NP");
    initialNoun = (token==TK_sNounUpp);
  }

  TRACE(3,"State actions completed. initialNoun="+util::int2string(initialNoun));
}


///////////////////////////////////////////////////////////////
///  Perform last minute validation before effectively building multiword
///////////////////////////////////////////////////////////////

bool np::ValidMultiWord(const word &w) {
	
  // We do not consider a valid proper noun if all words are capitalized and there 
  // are more than Title_length words (it's probably a news title, e.g. "TITANIC WRECKS IN ARTIC SEAS!!")
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

void np::SetMultiwordAnalysis(sentence::iterator i, int fstate) {

  // Setting the analysis for the Named Entity. 
  // The new MW is just created, so its list is empty.
  
    // if the MW has only one word, and is an initial noun, copy its possible analysis.
    if (initialNoun && i->get_n_words_mw()==1) {
      TRACE(3,"copying first word analysis list");
      i->copy_analysis(i->get_words_mw().front());
    }
    
    TRACE(3,"adding NP analysis");
    // Add an NP analysis, with the compound form as lemma.
    i->add_analysis(analysis(util::lowercase(i->get_form()),NE_tag));

}

sentence::iterator np::BuildMultiword(sentence &se, sentence::iterator start, sentence::iterator end, int fs, bool &built)
{
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
	    TRACE(3,"Valid Multiword. Split NP is on: keeping separate words");
	    for (sentence::iterator j=start; j!=end; j++) {
	      if (util::isuppercase(j->get_form()[0]))
		j->set_analysis(analysis(util::lowercase(j->get_form()),NE_tag));
	    }
            // don't forget last word in MW 
	    if (util::isuppercase(end->get_form()[0]))
	      end->set_analysis(analysis(util::lowercase(end->get_form()),NE_tag));

	    ResetActions();
	    i=end;
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
	    SetMultiwordAnalysis(i,fs);
	    built=true;
	  }
	}
	else {
	  TRACE(3,"Multiword found, but rejected. Sentence untouched");
	  ResetActions();
	  i=start;
	  built=false;
	}
	
	return(i);
}

