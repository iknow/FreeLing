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


#include <iostream>
#include <fstream>
#include <string>
#include "regexp-pcre++.h"
#include <map>
#include <stack>
#include "fries/util.h"
#include "freeling/phonetics.h"
#include "freeling/traces.h"

#define MOD_TRACENAME "PHONETICS"
#define MOD_TRACECODE PHONETICS_TRACE

using namespace std;


/////////////////////////////////////////////////////////////////////////////
/// Delete the last character of a string, similiar to chomp in perl
/////////////////////////////////////////////////////////////////////////////

string chomp (string e){
	string s=e;
	s.erase(s.size()); 
	return s;
}


///////////////////////////////////////////////////////////////
///  Create the phonetic translator
///////////////////////////////////////////////////////////////

phonetics::phonetics(string rulesFileIn, string dictFileIn, string sampaFileIn, bool useDictIn) {

	string rulesFile=rulesFileIn;
	string dictFile=dictFileIn;
	string sampaFile=sampaFileIn;
	useDict=useDictIn;


	// loading rules
	TRACE(3,"loading rules from "+rulesFile);
	ifstream SC;
	SC.open(rulesFile.c_str());
	if(!SC) {   ERROR_CRASH("Error opening file "+rulesFile);}

	string line;
	RegEx re0("^\\*");
	RegEx re1("(^\\s[\t]*$)");
	RegEx re2("(^(.)=(.+)$)");
	RegEx re3("(^[^\\/]+\\/[^\\/]*\\/.+$)");
	
	while(getline(SC, line)) {
		if (re0.Search(line)) continue; 
		if (re1.Search(line)) continue;  
		chomp(line); 
		if (re2.Search(line)) {
			vars[re2.Match(2)]=re2.Match(3); 
		}
		else if (re3.Search(line)) { 
			rules.push_back(line); 
		}
	}

	SC.close();

	// loading the dictionary
	
	if (useDict)
	{
		TRACE(3,"loading dictionary from "+dictFile);
		ifstream D;
		D.open(dictFile.c_str()); 
		if(!D) { ERROR_CRASH("Error opening file "+dictFile);}
		RegEx re4("(^\\#)");
		RegEx re5("(^$)");

		vector<string> vs;

		while(getline(D, line)) {
			chomp(line);
			if (re4.Search(line)) continue; 
			if (re5.Search(line)) continue; 
	
			vs=util::string2vector(line," ");
			DIC[vs.at(0)]=vs.at(1);
			
		}
		D.close();
	}


	// loading sampa file
	TRACE(3,"loading sampa file from "+sampaFile);

	ifstream S;
	S.open(sampaFile.c_str()); 
	if(!S) { ERROR_CRASH("Error opening file "+sampaFile);}
	RegEx re6("(^\\#)");
	RegEx re7("(^$)");
	vector<string> vs2,vr1,vr2;
	string r1,r2;

	while(getline(S, line)){
		// File format: CMU \t SC SAMPA asciiN
		chomp(line);
		if (re6.Search(line)) continue;
		if (re7.Search(line)) continue;
		vs2=util::string2vector(line,"\t");
		if (vs2.size()>2){
			vr1=util::string2vector(vs2.at(1),"/");
			if (vr1.size()>0) r1=vr1.at(0);
			vr2=util::string2vector(vs2.at(2),"/");
			if (vr2.size()>0) r2=vr2.at(0);
			if ((vr2.size()>0) and (vr1.size()>0)){
				SAMPA[r1]=r2;
				
			}
		}
	}
	S.close();

	// creating the soundchanguer
	sc= new soundChange(vars,rules);

}


///////////////////////////////////////////////////////////////
///  Destroy 
///////////////////////////////////////////////////////////////

phonetics::~phonetics(){
	delete sc;
 
}


/////////////////////////////////////////////////////////////////////////////
/// getSound return the phonetic translation of a word
/////////////////////////////////////////////////////////////////////////////
string phonetics::getSound(string word){

	string form=word;
	string form1=form;
	string sound;
	
	if (form1.empty()) return "";
	sound=DIC[form1];
	if (useDict and !sound.empty()) { return sound;}
	else {
		string aux=cache[form1];
		if (aux.empty()){	
			sound=sc->change(form1);
			TRACE(4,"soundChange returns: "+sound+" from word "+form1);
			string result;
			for (int i=0; i<sound.size(); i++) {
				string it=sound.substr(i,1);
				string res=SAMPA[it];
				result=result+res;
			}
			TRACE(4,"sampa translation converts "+sound+" into "+result);
			sound=result;
			cache[form1]=sound;
		}
		//Set
		sound=cache[form1];
	}
	
	return sound;

}
