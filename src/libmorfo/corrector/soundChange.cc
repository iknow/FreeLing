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
#include "pcre.h"
#include "freeling/traces.h"

#define OVECCOUNT 300   /* should be a multiple of 3 */


#define MOD_TRACENAME "SOUNDCHANGE"
#define MOD_TRACECODE SOUNDCHANGE_TRACE

using namespace std;


///////////////////////////////////////////////////////////////
///  Create
///////////////////////////////////////////////////////////////

soundChange::soundChange(map<string, string> vars1, vector <string> rules1){

	vars=vars1;
	rules=rules1;
	compile_vars();
	compile_rules();
}



///////////////////////////////////////////////////////////////
///  Destroy 
///////////////////////////////////////////////////////////////

soundChange::~soundChange(){
 
}


///////////////////////////////////////////////////////////////
///  returns the sound changed
///////////////////////////////////////////////////////////////

string soundChange::change(string word)
{
	string result=word;
	for (size_t i=0;i<rules.size();i++){
		TRACE(4,"from: "+from[i]+" to: "+to[i]+" env: "+env[i]);
		result=find_and_replace(result,from[i],to[i],env[i]);
		TRACE(4,"result: "+result);
	}
	return result;
}


///////////////////////////////////////////////////////////////
/// compile the rules
///////////////////////////////////////////////////////////////

void soundChange::compile_rules(){

	for (size_t i=0;i<rules.size();i++){
  		string rule= rules.at(i); 
		map<string,string>::iterator iter;
		
		RegEx re("^([^/]+)/([^/]*)/(.+)$");
		RegEx re2("^(#?)([^_#]*)(_)([^_#]*)(#?)$");
  		if(re.Search(rule)){
			string fromS=re.Match(1);
			string toS=re.Match(2);
			string envS=re.Match(3);

			// escape $ letter to not be confused with end of word in regular expressions
			for (size_t x=0;x<envS.size();x++) {
      				string c=envS.substr(x,1);
				if (c=="$") { envS.erase(x,1); envS.insert(x,"\\$"); x=x+1;; }
				//else if (c=="@") { envS.erase(x,1); envS.insert(x,"\\@"); x=x+1; }
			}

			if (envS.substr(0,1)=="#") envS.replace( 0, 1,"^");
			if (envS.substr(envS.size()-1,1)=="#") envS.replace(envS.size()-1, 1,"$");

						
			// We substitue the var for his value on every rule
			for( iter = vars.begin(); iter != vars.end(); ++iter ) {
      				string var=iter->first;
				string valor=iter->second;
				fromS=find_and_replace(fromS,var,valor,"");
				toS=find_and_replace(toS,var,valor,"");
				envS=find_and_replace(envS,var,valor,"");
			}
		
			from.push_back(fromS);
			to.push_back(toS);
			env.push_back(envS);

			
		}	
			
	}

}



///////////////////////////////////////////////////////////////
///  compile de variables
///////////////////////////////////////////////////////////////

void soundChange::compile_vars(){


	map<string,string>::iterator iter;
	
    	for( iter = vars.begin(); iter != vars.end(); ++iter ) {
      		string var=iter->first;
		string list=iter->second;
		vars[var]="["+list+"]"; 
				
   	}
}


///////////////////////////////////////////////////////////////
///  calculate the location of the _ in a pattern
///////////////////////////////////////////////////////////////

int soundChange::calculatePosition( string pattern){



	string aux=pattern;
		
	// replace [...] with C to count chars to _
    	for (size_t x=0;x<aux.size();x++) {
      		string c=aux.substr(x,1);
		if (c=="[") { 
			int pos=aux.find("]",0);
			aux.erase(x,pos-x+1);
			aux.insert(x,"C");
		}
		else if (c=="^") { aux.erase(x,1); x--;}
		
		
		
   	}
	int loc= aux.find("_",0);
	return loc;
}



///////////////////////////////////////////////////////////////
///  check that the enviorment is true
///////////////////////////////////////////////////////////////

bool soundChange::check_cond(string text, string opt, int loc, string findStr){

	
  if (opt.size()<2) return true; // empty condition
  
  const char *error;
  int   erroffset;
  pcre *re;
  int   rc;
  const char* str = text.c_str();  /* String to match */
  int   ovector[OVECCOUNT];
  
  string aux="("+opt+")";
  aux=find_and_replace(aux,"_",findStr,"");
  
  const char* pattern=aux.c_str();
  
  re = pcre_compile (
		     pattern,       /* the pattern */
		     0,                    /* default options */
		     &error,               /* for error message */
		     &erroffset,           /* for error offset */
		     0);                   /* use default character tables */
  
  if (!re) {
    ERROR_CRASH("pcre_compile failed");
    return false;
  }
  
  rc = pcre_exec (
		  re,                   /* the compiled pattern */
		  0,                    /* no extra data - pattern was not studied */
		  str,                  /* the string to match */
		  strlen(str),          /* the length of the string */
		  0,                    /* start at offset 0 in the subject */
		  0,                    /* default options */
		  ovector,              /* output vector for substring information */
		  OVECCOUNT);           /* number of elements in the output vector */

  bool res;    
  if (rc == PCRE_ERROR_NOMATCH) res=false; 
  else if (rc < 0) ERROR_CRASH("pcre: Error while matching");
  else {
    // the sign _ and the loc of the char to replace must be in te same location
    int loc_= calculatePosition(opt);
    res = (ovector[0]+loc_==loc);
  }
  
  free(re);
  return res;

}


///////////////////////////////////////////////////////////////
///  Replace all ocurrences in text of "findStr" with replaceStr, 
/// if "[findStr]" then replaces every char(i) of findStr for replaceStr(i)
///////////////////////////////////////////////////////////////

string soundChange::find_and_replace(string text, string findStr, string replaceStr, string opt){

  size_t j;
  string result=text;
  
  if ((findStr.substr(0,1)!="[") and (findStr!=replaceStr)) {
    for (int x=0;(j = result.find(findStr,x)) != string::npos ;) {
      if (check_cond(result,opt,j,findStr)){
	string auxtext=result;
	result.erase(j, findStr.size());
	result.insert(j, replaceStr);
	
      }
      if (j!=string::npos ){
	if (replaceStr.size()>0) x=j+replaceStr.size();
	else x=j+1;
	
      }
      else break;
    }
  }
  else {
    string f=findStr.substr(1,findStr.size()-2);
    string r;
    if (replaceStr.size()>1) r=replaceStr.substr(1,replaceStr.size()-2);
    else {
      r=replaceStr;
      for (size_t ii=r.size();ii<f.size();ii++)	{ // @ to @@@@@@@@@@@@@@ so change aeiou to @@@@@
	r=r+r.substr(0,1);
      }			
    }
    for (size_t z=0;z<f.size();z++){
      if (f.at(z)!=r.at(z)){	
	string cc=f.substr(z,1);
	for (int x=0; (j = result.find(cc,x)) != string::npos ; ) {
	  if (check_cond(result,opt,j,findStr)){
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
