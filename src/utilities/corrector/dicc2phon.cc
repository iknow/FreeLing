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

////////////////////////////////////////////////////////////////
//   
//   From a dictionary of freeling create a text that indexdict can
//
//   use to create a database with 
//  
//
//      key1(consonant fonemas)  data1(words that match) 
//
//	The dictionary is passed as stdin and the text is the stdout
//
//	The paramaters are the 3 necesary files soundChangeRules
//	soundChangeDicFile and sampaFile
//
////////////////////////////////////////////////////////////////

#include <iostream>
#include <string>
#include <cstdlib>
#include "freeling/phonetics.h"
#include "fries/util.h"


using namespace std;


int main(int argc, char *argv[]){
	
	
	if (!argc==3) { cout << "You need to specify the 3 necesary files: soundChangeRules, soundChangeDicFile and sampaFile" << endl; exit(0);}
	
	string line;
	vector<string> words;
		
	// we take the word form the dictionary skipping all other info
	while(getline(cin,line)){
		vector<string> vs;
		vs=util::split(line," \t");
		words.push_back(vs.at(0));
	}
	
			
	string soundChangeRules=argv[1];
	string soundChangeDicFile=argv[2];
	string sampaFile=argv[3];
	
	phonetics ph(soundChangeRules,soundChangeDicFile,sampaFile,true);
	
	// we translate every word for his phonetic sound
	
	 map<string,string> bd;
	
	
	for( vector<string>::iterator iter = words.begin(); iter != words.end(); iter++ ) {
		string word=*iter;
		string sound=ph.getSound(word);
		sound=util::eliminateChars(sound,"aeiou");
		if (sound.size()==0) { sound="aeiou";}
		bd[sound]+=","+word;
	}
	
	
    	for( map<string,string>::iterator iter=bd.begin(); iter!=bd.end(); ++iter ) {
      		string keyword=iter->first;
		string data=iter->second;
		data=data.substr(1);
		cout << keyword << " " << data << endl;
	}
		
}
