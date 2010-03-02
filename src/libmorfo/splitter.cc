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

#include "freeling/splitter.h"
#include "fries/util.h"
#include "freeling/traces.h"

using namespace std;

#define MOD_TRACENAME "SPLITTER"
#define MOD_TRACECODE SPLIT_TRACE

#define SAME 1000
#define VERY_LONG 2000

///////////////////////////////////////////////////////////////
/// Create a sentence splitter.
///////////////////////////////////////////////////////////////

splitter::splitter(const std::string &SplitFile) { 
  int reading;
  int nmk;
  string name,line,open,close;
  bool value;  

    ifstream fabr(SplitFile.c_str());
    if (!fabr) { 
      ERROR_CRASH("Error opening file "+SplitFile);
    }

    // default values    
    SPLIT_AllowBetweenMarkers = true;
    SPLIT_MaxLines = 0;

    // current state
    betweenMrk=false;
    no_split_count=0; 
    mark_type=0;

    nmk=1;
    reading=0;
    while (getline(fabr,line)) {

      istringstream sin;
      sin.str(line);

      if (line == "<General>") reading=1;
      else if (line == "</General>") reading=0;
      else if (line == "<Markers>") reading=2;
      else if (line == "</Markers>") reading=0;
      else if (line == "<SentenceEnd>") reading=3;
      else if (line == "</SentenceEnd>") reading=0;
      else if (line == "<SentenceStart>") reading=4;
      else if (line == "</SentenceStart>") reading=0;
      else if (reading==1) {
	// reading general options
	sin>>name;
        if (name=="AllowBetweenMarkers") sin>>SPLIT_AllowBetweenMarkers;
	else if (name=="MaxLines") sin>>SPLIT_MaxLines;
        else ERROR_CRASH("Unexpected splitter option in file "+SplitFile);
      }
      else if (reading==2) {
	// reading open-close marker pairs (parenthesis, etc)
	sin>>open>>close;
	if (open!=close) {
	  markers.insert(make_pair(open,nmk));   // positive for open, negative for close
	  markers.insert(make_pair(close,-nmk));
	}
	else {  // open and close are the same (e.g. double "quotes")
	  markers.insert(make_pair(open, SAME+nmk));   // both share the same code, but its distinguishable
	  markers.insert(make_pair(close, SAME+nmk));
	}
	nmk++;
      }
      else if (reading==3) {
	// reading sentence delimiters
	sin>>name>>value;
	enders.insert(make_pair(name,value));
      }
      else if (reading==4) {
	// reading sentence delimiters
	starters.insert(line);
      }
    }

    fabr.close(); 

}


///////////////////////////////////////////////////////////////
///  Accumulate the word list v into the internal buffer.
///  If a sentence marker is reached (or flush flag is set), 
///  return all sentences currently in buffer, and clean buffer.
///  If a new sentence is started but not completed, keep
///  in buffer, and wait for further calls with more data.
///////////////////////////////////////////////////////////////

void splitter::split(const std::vector<word> &v, bool flush, list<sentence> &ls) {
  vector<word>::const_iterator w;
  map<string,bool>::const_iterator e;
  map<string,int>::const_iterator m;

  TRACE(3,"Looking for a sentence marker. Max no split is: "+util::int2string(SPLIT_MaxLines));
  TRACE_WORD_LIST(3,v);

  // clear list of sentences from previous use
  ls.clear();
  // look for a sentence marker
  for(w=v.begin(); w!=v.end(); w++){

    // check whether we are entering "between markers" state
    m=markers.find(w->get_form());
    if (m!=markers.end() && m->second>0 && !betweenMrk && !SPLIT_AllowBetweenMarkers) {
      mark_form = m->first;
      mark_type = m->second;
      TRACE(3,"Start no split period, marker "+m->first+" code:"+util::int2string(m->second));
      betweenMrk=true;
      buffer.push_back(*w);
    }
    // check whether we are leaving "between markers" state
    else if (betweenMrk && !SPLIT_AllowBetweenMarkers){
      TRACE(3,"no-split flag continues set. word="+w->get_form()+" expecting code:"+util::int2string(mark_type)+" (closing "+mark_form+")");
      
      no_split_count++;
      // check if we are closing the opened marker
      if ((m!=markers.end() && m->second==(m->second>SAME?1:-1)*mark_type) || (SPLIT_MaxLines>0 && no_split_count>SPLIT_MaxLines)) {
        TRACE(3,"End no split period. marker="+m->first+" code:"+util::int2string(m->second));
	betweenMrk=false; mark_type=0; no_split_count=0; 
      }

      if (no_split_count>VERY_LONG && no_split_count<=VERY_LONG+5) {
	WARNING("Ridiculously long sentence between markers at token '"+w->get_form()+"' at input offset "+util::int2string(w->get_span_start())+".");
	if (no_split_count==VERY_LONG+5) {
	  WARNING("...etc...");
          WARNING("Expecting code "+util::int2string(mark_type)+" (closing "+mark_form+") for over "+util::int2string(VERY_LONG)+" words. Probable marker mismatch in document.");
          WARNING("If this causes crashes try setting AllowBetweenMarkers=1 in");
          WARNING("your splitter configuration file (e.g. splitter.dat)");
	}
      }

      buffer.push_back(*w);
    }
    else{
      TRACE(3,"Outside Marks");

      e = enders.find(w->get_form());
      if (e!=enders.end()) {
	// Possible sentence ending. 
        // We split if the delimiter is "sure" (e->second==true) or if the context checking returns true. 
	if (e->second || end_of_sentence(w,v)) {
	  TRACE(2,"Sentence marker ["+w->get_form()+"] found");
	  // Complete the sentence
	  buffer.push_back(*w);
	  // store it in the results list
	  ls.push_back(buffer);

	  // Clear sentence, look for a new one
	  buffer.clear();
	  // reset state
          betweenMrk=false; mark_type=0; no_split_count=0; 
	}
	else {
	  // context indicated it was not a sentence ending.
	  TRACE(3,w->get_form()+" is not a sentence marker here");
	  buffer.push_back(*w);
	}
      }
      else{
       // Normal word. Accumulate to the buffer.
	TRACE(3,w->get_form()+" is not a sentence marker here");
	buffer.push_back(*w);
      }
    }
  }

  if (flush && !buffer.empty()) { // if flush is set, do not retain anything
     TRACE(3,"Flushing the remaining words into a sentence");
     ls.push_back(buffer);         // add sentence to return list
     buffer.clear();
     betweenMrk=false; mark_type=0; no_split_count=0; 
  }

  TRACE_SENTENCE_LIST(1,ls);

  //  return(ls);        // return found sentences
}



///////////////////////////////////////////////////////////////
///  Split and return a copy of the sentences
///////////////////////////////////////////////////////////////

list<sentence> splitter::split(const std::vector<word> &v, bool flush) {

  list<sentence> ls;
  split (v, flush, ls);
  return ls;
}


///////////////////////////////////////////////////////////////
///  Check whether a word is a sentence end (eg a dot followed
/// by a capitalized word).
///////////////////////////////////////////////////////////////

bool splitter::end_of_sentence (std::vector<word>::const_iterator w, const std::vector<word> &v) const {
  vector<word>::const_iterator r;
  string f,g;

  // Search at given position w for the next word, to decide whether w contains a marker.

  if (w==--v.end()) {
    // Last word in the list. We don't know whether we can consider a sentence end. 
    // Probably we'd better wait for next line, but we don't in this version.
    return true;
  }
  else {
    // Not last word, check the following word to see whether it is uppercase
    r=w; r++;
    f=r->get_form();

    // See if next word a typical sentence starter or its first letter is uppercase
    return (util::isuppercase(f[0]) || starters.find(f)!=starters.end());
  }
}




