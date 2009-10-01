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
#include <vector>

#include "freeling/corrector.h"
#include "fries/util.h"
#include "freeling/traces.h"

using namespace std;

#define MOD_TRACENAME "CORRECTOR"
#define MOD_TRACECODE CORRECTOR_TRACE

#define DISTANCE_LIMIT 0.5
#define MAX_SIZE_DIFF 3


// define apropriate open calls depending on Berkeley-DB version.
#if (DB_VERSION_MAJOR==4 && DB_VERSION_MINOR==0)
  #define OPEN(name,db,type,flags,mode)  open(name,db,type,flags,mode)
#else
#if DB_VERSION_MAJOR>=4
  #define OPEN(name,db,type,flags,mode)  open(NULL,name,db,type,flags,mode)
#endif
#endif


///////////////////////////////////////////////////////////////
///  Create a corrector module, open database. 
///////////////////////////////////////////////////////////////

corrector::corrector(const std::string &correctorLang,  dictionary &dict1, const std::string &correctorCommon, const std::string &dist): morfodb(NULL,DB_CXX_NO_EXCEPTIONS){
 
	int res;
	dict=&dict1;
	distanceMethod=dist;
	string soundDicFile=correctorLang+".db";
	string soundChangeRules=correctorLang+".rules";
	string soundChangeDicFile=correctorCommon+".soundDicFile";
	string sampaFile=correctorCommon+".sampa";
	string phoneticDistanceFile=correctorCommon+".phdistances";
	//string phoneticDistanceFile=correctorCommon+".similarity";

	// Opening a 4.0 or higher BerkeleyDB database
  	if ((res=morfodb.OPEN(soundDicFile.c_str(),NULL,DB_UNKNOWN,DB_RDONLY,0))){
    		ERROR_CRASH("Error "+util::int2string(res)+" while opening database "+soundDicFile);
  	}
	
	if ((distanceMethod!="similarity") and (distanceMethod!="phonetic")) ERROR_CRASH("Error distance value: "+distanceMethod+", is not valid");
	
	ph= new phonetics(soundChangeRules,soundChangeDicFile,sampaFile,true);
		
	if (distanceMethod=="similarity") sm= new similarity();
	else if (distanceMethod=="phonetic") phd= new phoneticDistance(phoneticDistanceFile);

	TRACE(3,"corrector succesfully created");
	TRACE(3,"distance method: "+distanceMethod);
}


////////////////////////////////////////////////
/// Destroy corrector module, close database, delete objects.
////////////////////////////////////////////////

corrector::~corrector(){
int res;
  // Close the database
  if ((res=morfodb.close(0))) {
    ERROR_CRASH("Error "+util::int2string(res)+" while closing database");
  }
	// delete dict;
	delete ph;
	delete phd;
}


////////////////////////////////////////////////////////////////////////
/// returns the phonema's transcription of a word
////////////////////////////////////////////////////////////////////////

string corrector:: getSound(string word){ 
	
	string result= ph->getSound(word);
	result=util::eliminateChars(result,"aeiou"); // eliminate vowels for generating a key for the database that will embrace more words
	if (result.size()==0) result="aeiou"; // if the word only contains vowels we use the keyword aeiou
	TRACE(3,"Getting the sound of word: "+word+", result: "+result);

return result;

}


////////////////////////////////////////////////////////////////////////
/// returns the list of words from the database that have the string as a key
////////////////////////////////////////////////////////////////////////

string corrector:: getListWords(string keyword) {

  string data_string;
  Dbt data, key;
  int error;
  string::size_type p;
  char buff[100000];
  
  key.set_data((void *) keyword.c_str());
  key.set_size(keyword.length());
  error = morfodb.get (NULL, &key, &data, 0);
  
  if (!error){
    // copy the data associated to the key to the buffer
    p=data.get_size();
    memcpy((void *)buff, data.get_data(), p);
    // convert char* to string into data_string
    buff[p]=0;
    data_string=buff;
    TRACE(3,"Accesing database with keyword: "+keyword+", result: "+data_string);
  }
  else if (error != DB_NOTFOUND) 
    ERROR_CRASH("Error "+util::int2string(error)+" while accessing database");
  
  return data_string; // the word will be sperated by coma 
}

////////////////////////////////////////////////////////////////////////
/// adds the new words that are posible correct spellings from original 
/// word to the word analysys data
////////////////////////////////////////////////////////////////////////

void corrector:: putWords(string listaPal, word &w, string wordOrig){


	char delims[2] = ",";

	list<analysis> la;
	
	list <string> tokens; 
	char *token;

	string aux=listaPal+",";
	
	token = strtok((char *) aux.c_str(),delims);
	if (token!=NULL) { tokens.push_back(string(token)); /*tokens[nTokens]=string(token); nTokens++;*/}
	while((token=strtok(NULL,delims))!=NULL){
	 	//tokens[nTokens]=string(token);
		tokens.push_back(string(token));
	}
	
	
	list<string>::iterator it;

	 for( it=tokens.begin(); it!=tokens.end(); ++it){

 		string word=*it;
		string lema;
		string tags;
	
		int diff=wordOrig.size()-word.size();
		if (diff<0) diff=-1*diff;
		if (diff<MAX_SIZE_DIFF){
		
			double simil;
			
			
			if (distanceMethod=="similarity") simil=(double) sm->getSimilarity(util::lowercase(wordOrig),util::lowercase(word));
			else if (distanceMethod=="phonetic") { // phonetic distance
			
				string word1=util::lowercase(wordOrig);
				string word2=util::lowercase(word);
				word1= ph->getSound(word1);
				word2= ph->getSound(word2);
				simil=(double) phd->getPhoneticDistance(word1,word2);
				double simMax=(double) phd->getPhoneticDistance(word1,word1);
				simil=simil/simMax;
				
			}
			else ERROR_CRASH("Error unknown distance method: "+distanceMethod);

			if (simil>DISTANCE_LIMIT){	
				dict->getInfoWord(word,lema,tags);
				TRACE(3,"Adding ("+word+","+tags+") to analysis list");
       				analysis a=analysis(lema+":"+word,tags);
				a.set_distance(simil);
				la.push_back(a);
			}
		
		}

	}


 	list<analysis>::const_iterator a;

	for (a=la.begin(); a!=la.end(); a++){
     		w.add_analysis(*a);
     		TRACE(4,"   added analysis "+a->get_lemma());
	}
	
}



////////////////////////////////////////////////////////////////////////
/// Navigates the sentence adding alternative words (possible correct spelling data)
////////////////////////////////////////////////////////////////////////

void corrector:: annotate(sentence &se) 
{
  sentence::iterator pos;
  
  
  for (pos=se.begin(); pos!=se.end(); ++pos)  {
	// If a find it in the dictionary and it's a verb or if i didn't find it in the dictionary 
	if (((pos->get_n_analysis()) && (pos->get_parole()[0]=='V' || (pos->get_parole()[0]=='N' && pos->get_parole()[1]=='P'))) || (!pos->get_n_analysis())){
		string wordOrig=pos->get_form();
		string keyword = getSound(wordOrig); // calculate the sound without vowels of the current word
		if (keyword.size()>0){
			string listaPal=getListWords(keyword); // we obatin the list of word separated by coma that match the keyword in the databse
			if (listaPal.size()>0){
				putWords(listaPal,*pos, wordOrig); // we add the new words to the POS of the word
			}
		}
	}
    }

}


