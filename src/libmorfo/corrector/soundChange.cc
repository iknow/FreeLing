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
//    General Public License for more details.º
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


#include <iostream>
#include <fstream>
#include "regexp-pcre++.h"
#include "freeling/soundChange.h"
#include "freeling/traces.h"

#define MOD_TRACENAME "SOUNDCHANGE"
#define MOD_TRACECODE PHONETICS_TRACE

using namespace std;


///////////////////////////////////////////////////////////////
///  Create
///////////////////////////////////////////////////////////////

soundChange::soundChange(const map<string,string> &vars, const vector<string> &rls) {

  for (vector<string>::const_iterator r=rls.begin(); r!=rls.end(); r++) {

    RegEx re("^([^/]+)/([^/]*)/(.+)$");

    if (re.Search(*r)) {
      ph_rule newrule;
      newrule.from = re.Match(1);
      newrule.to   = re.Match(2);
      newrule.env  = re.Match(3);
      
      // escape $ letter to not be confused with end of word in regular expressions
      util::find_and_replace(newrule.env,"$","\\$");
      if (newrule.env[0]=='#') newrule.env[0]='^';
      if (newrule.env[newrule.env.size()-1]=='#') newrule.env[newrule.env.size()-1]='$';
            
      // We substitue any var in the rule for its value
      for (map<string,string>::const_iterator v=vars.begin(); v!=vars.end(); v++) {
	string var=v->first;
	string val="["+v->second+"]";

	util::find_and_replace(newrule.from, var, val);
	util::find_and_replace(newrule.to, var, val);
	util::find_and_replace(newrule.env, var, val);	
      }

      // store the resulting rule.
      rules.push_back(newrule);
    }
    else 
      WARNING("Wrongly formatted phonetic rule '"+(*r)+"' ignored.");
  }  
}



///////////////////////////////////////////////////////////////
///  Destroy 
///////////////////////////////////////////////////////////////

soundChange::~soundChange(){}


///////////////////////////////////////////////////////////////
///  returns the sound changed
///////////////////////////////////////////////////////////////

string soundChange::change(const string &word) const {

  string result=word;

  for (vector<ph_rule>::const_iterator r=rules.begin(); r!=rules.end(); r++) {
    TRACE(4,"appling rule ("+r->from+"/"+r->to+"/"+r->env+") to word '"+word+"'");
    result= apply_rule(result,(*r));
    TRACE(4,"  result: "+result);
  }
  return result;
}


///////////////////////////////////////////////////////////////
///  check that the enviorment is true
///////////////////////////////////////////////////////////////

bool soundChange::check_cond(const string &text, const string &opt, int loc, const string &findStr) const {
	
  if (opt.size()<=1) return true; // condition "_" always holds
  
  string pattern="("+opt+")";
  util::find_and_replace(pattern,"_","("+findStr+")"); // we substitute _ for findStr
  
  RegEx re(pattern);
  if (re.Search(text)){
    int ini,fin;
    re.MatchPositions(1,ini,fin);
    if (ini!=-1) 
      return (ini==loc); // the sign _ and the loc of the char to replace must be in te same location
    else 
      ERROR_CRASH("Error in MatchPositions");
  }

  return false; // no match  
}


///////////////////////////////////////////////////////////////
///  Replace all ocurrences in text of "findStr" with replaceStr, 
/// if "[findStr]" then replaces every char(i) of findStr for replaceStr(i)
///////////////////////////////////////////////////////////////

string soundChange::apply_rule(const string &text, const ph_rule &rul) const {
  
  size_t j;
  string result=text;
  
  if ((rul.from.substr(0,1)!="[") and (rul.from!=rul.to)) {
    for (int x=0;(j = result.find(rul.from,x)) != string::npos ;) {
      if (check_cond(result,rul.env,j,rul.from)){
	string auxtext=result;
	result.erase(j, rul.from.size());
	result.insert(j, rul.to);
	
      }
      if (j!=string::npos ){
	if (rul.to.size()>0) x=j+rul.to.size();
	else x=j+1;
	
      }
      else break;
    }
  }
  else {
    string f=rul.from.substr(1,rul.from.size()-2);
    string r;
    if (rul.to.size()>1) r=rul.to.substr(1,rul.to.size()-2);
    else {
      r=rul.to;
      for (size_t ii=r.size();ii<f.size();ii++)	{ // @ to @@@@@@@@@@@@@@ so change aeiou to @@@@@
	r=r+r.substr(0,1);
      }			
    }
    for (size_t z=0;z<f.size();z++){
      if (f.at(z)!=r.at(z)){	
	string cc=f.substr(z,1);
	for (int x=0; (j = result.find(cc,x)) != string::npos ; ) {
	  if (check_cond(result,rul.env,j,rul.from)){
	    string auxtext=result;
	    result.erase(j,1);
	    result.insert(j, r.substr(z,1));
	  }
	  if (j!=string::npos ) x=j+1;
	  else break;
	}
      }
    }
  }
  
  return result;
}
