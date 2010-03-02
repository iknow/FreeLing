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

#include "freeling/automat.h"
#include "freeling/traces.h"

using namespace std;

#define MOD_TRACENAME "AUTOMAT"
#define MOD_TRACECODE AUTOMAT_TRACE

///////////////////////////////////////////////////////////////
/// Create an instance of the class, initializing options member.
/// Since automat is an abstract class, this is called always 
/// from child constructors.
///////////////////////////////////////////////////////////////

automat::automat() {};


///////////////////////////////////////////////////////////////
/// Check each word in sentece as a possible pattern start. 
/// Recognize the longest pattern starting at first possible
/// start found.
/// Repeat the process starting from first word after recognized
/// pattern, until sentence ends.
///////////////////////////////////////////////////////////////

void automat::annotate(sentence &se)
{
  sentence::iterator i,j,sMatch,eMatch; 
  int newstate, state, token, fstate;
   
  fstate=0;

  // check whether there is a match starting at each position i
  for (i=se.begin(); i!=se.end(); i++) {

    // reset automaton
    state=initialState;
    ResetActions();

    sMatch=i; eMatch=se.end();
    for (j=i;state != stopState && j!=se.end(); j++) {
      // request the child class to compute the token
      // code for current word in current state
      token = ComputeToken(state,j,se);
      // do the transition to new state
      newstate = trans[state][token];
      // let the child class perform any actions 
      // for the new state (e.g. computing date value...)
      StateActions(state, newstate, token, j);
      // change state
      state = newstate;
      // if the state codes a valid match, remember it
      //  as the longest match found so long.
      if (Final.find(state)!=Final.end()) {
        eMatch=j;
	fstate=state;
	TRACE(3,"New candidate found");
      }
    }

    TRACE(3,"STOP state reached. Check longest match");
    // stop state reached. find longest match (if any) and build a multiword
    if (eMatch!=se.end()) {
      TRACE(3,"Match found");
      bool found;
      i=BuildMultiword(se,sMatch,eMatch,fstate,found);
      TRACE_SENTENCE(3,se);
    }
  }

  // Printing partial module results
  TRACE_SENTENCE(1,se);
}


///////////////////////////////////////////////////////////////
/// Check given word in sentece as a possible pattern start. 
/// Recognize the longest pattern.
///////////////////////////////////////////////////////////////

bool automat::annotate(sentence &se, sentence::iterator &i)
{
  sentence::iterator j,sMatch,eMatch; 
  int newstate, state, token, fstate;
  bool found=false;
 
    // reset automaton
  state=initialState;
  fstate=0;
  ResetActions();
  
  sMatch=i; eMatch=se.end();
  for (j=i;state != stopState && j!=se.end(); j++) {
    // request the child class to compute the token
    // code for current word in current state
    token = ComputeToken(state,j,se);
    // do the transition to new state
    newstate = trans[state][token];
    // let the child class perform any actions 
    // for the new state (e.g. computing date value...)
    StateActions(state, newstate, token, j);
    // change state
    state = newstate;
    // if the state codes a valid match, remember it
    //  as the longest match found so long.
    if (Final.find(state)!=Final.end()) {
      eMatch=j;
      fstate=state;
      TRACE(3,"New candidate found");
    }
  }
  
  TRACE(3,"STOP state reached. Check longest match");
  // stop state reached. find longest match (if any) and build a multiword
  if (eMatch!=se.end()) {
    TRACE(3,"Match found");
    i=BuildMultiword(se,sMatch,eMatch,fstate,found);
    TRACE_SENTENCE(3,se);
  }

  return(found);
}





///////////////////////////////////////////////////////////////
/// Perform last minute validation before effectively building multiword.
/// Child classes can redefine this function to perform desired checks.
///////////////////////////////////////////////////////////////

bool automat::ValidMultiWord(const word & w) {
  return(true);
}


///////////////////////////////////////////////////////////////
///  Arrange the sentence grouping all words from start to end
///  in a multiword.
///////////////////////////////////////////////////////////////

sentence::iterator automat::BuildMultiword(sentence &se, sentence::iterator start, sentence::iterator end, int fs, bool &built)
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
  else {
    TRACE(3,"Multiword found, but rejected. Sentence untouched");
    ResetActions();
    i=start;
    built=false;
  }

  return(i);
}
