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

#include <locale>
#include <sstream>
#include <fstream>
#include <string>

#include "freeling/tokenizer.h"
#include "fries/util.h"
#include "freeling/traces.h"

using namespace std;

#define MOD_TRACENAME "TOKENIZER"
#define MOD_TRACECODE TOKEN_TRACE

///////////////////////////////////////////////////////////////
/// Create a tokenizer, using the abreviation and patterns 
/// file indicated in given options.
///////////////////////////////////////////////////////////////

tokenizer::tokenizer(const std::string &TokFile)
{
  string line;
  string mname, mvalue, comment,re;
  list<pair<string,string> > macros;
  list<pair<string,string> >::iterator i;
  unsigned int reading,substr;
  string::size_type p;
  bool rul;

  // open abreviations file
  ifstream fabr (TokFile.c_str());
  if (!fabr) {
    ERROR_CRASH("Error opening file "+TokFile);
  };

  // load and store abreviations file
  reading=0; rul=false;
  while (getline(fabr,line)) {

    istringstream sin;
    sin.str(line);
    
    if (line == "<Macros>") reading=1;
    else if (line == "</Macros>") reading=0;
    else if (line == "<RegExps>") reading=2;
    else if (line == "</RegExps>") reading=0;
    else if (line == "<Abbreviations>") reading=3;
    else if (line == "<Abbreviations>") reading=0;
    else if (reading==1 && rul) {
      ERROR_CRASH("Error reading tokenizer rules. Macros must be defined before rules.");
    }
    else if (reading==1) {
      // reading macros
      sin>>mname>>mvalue;
      macros.push_back(make_pair(mname,mvalue));
      TRACE(3,"Read macro "+mname+": "+mvalue);
    }
    else if (reading==2) {
      // read tokenization rule
      sin>>comment>>substr>>re;
      // at least a rule found
      rul=true;
      // check all macros
      for (i=macros.begin(); i!=macros.end(); i++) {
	mname="{"+i->first+"}"; 
	mvalue=i->second;
	// replace all macro occurrences in the rule with its value
	p=re.find(mname,0);
	while (p!=string::npos) { 
          //re=re.substr(0,p)+mvalue+re.substr(p+mname.length());
	  re.replace(p, mname.length(), mvalue);
	  p=re.find(mname,p);
	}
      }	

      // create and store Regexp rule in rules vector.
      re.insert(0,"^");
      RegEx x(re);
      rules.push_back(make_pair(comment,x));
      matches.insert(make_pair(comment,substr));
      TRACE(3,"Stored rule "+comment+" "+re);
    }
    else if (reading==3) {
      abrevs.insert(line);
    }
  }
  fabr.close();

  TRACE(3,"analyzer succesfully created");
}

///////////////////////////////////////////////////////////////
/// Split the string into tokens using RegExps from
/// configuration file, returning a word object list.
///////////////////////////////////////////////////////////////

list<word> tokenizer::tokenize(const std::string &p, unsigned long &offset) {
  string t[10];
  string s;
  string::iterator c;
  vector<pair<string,RegEx> >::iterator i;
  bool match;
  int j,start;
  int len=0;
  list<word> v;

  // Loop until line is completely processed
  s=p;
  while (!s.empty()) {

    // find first non-white space and erase leading whitespaces
    for (c=s.begin(); isspace(*c); c++,offset++);
    s.erase(s.begin(),c);

    // find first matching rule
    match=false;
    start = offset;
    for (i=rules.begin(); i!=rules.end() && !match; i++) {
      if (i->second.Search(s)) {
        // regexp matches, extract substrings
	match=true; len=0;
	for (j=(matches[i->first]==0? 0 : 1); j<=matches[i->first] && match; j++) {
	  // get each requested  substring
	  t[j] = i->second.Match(j);
	  len += t[j].length();
	  TRACE(2,"Found match "+util::int2string(j)+" ["+t[j]+"] for rule "+i->first);
	  // if special rule, match must be in abbrev file
	  if ((i->first)[0]=='*' && abrevs.find(util::lowercase(t[j]))==abrevs.end()) {
	    match = false;
	    TRACE(2,"Special rule and found match not in abbrev list. Rule not satisfied");
	  }
	}
      }
    }

    if (match) {
      i--;
      // create word for each matched substring and append it to token list
      for (j=(matches[i->first]==0? 0 : 1); j<=matches[i->first]; j++) {
	if (t[j].length() > 0) {
	  TRACE(2,"Accepting matched substring ["+t[j]+"]");
	  word w(t[j]);
	  w.set_span(offset,offset+t[j].length());
	  offset += t[j].length();
	  v.push_back(w);
	}
	else
	  TRACE(2,"Skipping matched null substring ["+t[j]+"]");
      }  
      // remaining substring
      s=s.substr(len);
      TRACE(3,"  remaining... ["+s+"]");
    }
  }

  TRACE_WORD_LIST(1,v);
 
  return(v);
}


list<word> tokenizer::tokenize(const std::string &p) {
  unsigned long aux=0;
  return(tokenize(p,aux));
}
