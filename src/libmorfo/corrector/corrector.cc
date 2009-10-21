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
#include "freeling/traces.h"
#include "fries/util.h"

using namespace std;

#define MOD_TRACENAME "CORRECTOR"
#define MOD_TRACECODE CORRECTOR_TRACE

#define DISTANCE_LIMIT 0.5
#define MAX_SIZE_DIFF 3


///////////////////////////////////////////////////////////////
///  Create a corrector module, open database. 
///////////////////////////////////////////////////////////////

corrector::corrector(const std::string &correctorLang,  dictionary &dict1, const std::string &correctorCommon, const std::string &dist): dictionaryCheck("") {
 
  dict=&dict1;
  distanceMethod=dist;
  string soundDicFile=correctorLang+".db"; // indexed dictionary of similar-sounding words.
  string soundChangeRules=correctorLang+".rules";  // language-specific phonetic rules.

  string soundChangeDicFile=correctorCommon+".soundDicFile";   // /????
  string sampaFile=correctorCommon+".sampa";                   // sampa codes for sounds
  string phoneticDistanceFile=correctorCommon+".phdistances";  // phonetic distance tables
  string configFile=correctorCommon+".config"; // define specific parameters

  //string phoneticDistanceFile=correctorCommon+".similarity";
  
  // Opening  BerkeleyDB database
  similar_words.open_database(soundDicFile);
  
  if ((distanceMethod!="similarity") and (distanceMethod!="phonetic")) 
    ERROR_CRASH("Error distance value: "+distanceMethod+", is not valid");
  
  ph= new phonetics(soundChangeRules,soundChangeDicFile,sampaFile,false);
  
  if (distanceMethod=="similarity") sm= new similarity();
  else if (distanceMethod=="phonetic") phd= new phoneticDistance(phoneticDistanceFile);
  
  ifstream F;
  F.open(configFile.c_str()); 
  if (!F) 
    ERROR_CRASH("Error opening file "+configFile);

  string line;
  while(getline(F,line)){
    vector <string> vs=util::split(line," ");

    if (vs[0]=="dictionary")
      dictionaryCheck = RegEx(vs[1]);
    else if (vs[0]=="noDictionary")
      noDictionaryCheck = (util::lowercase(vs[1])=="yes" or util::lowercase(vs[1])=="y");
  }
  
  F.close();
  
  TRACE(3,"corrector succesfully created");
  TRACE(3,"distance method: "+distanceMethod);
}


////////////////////////////////////////////////
/// Destroy corrector module, close database, delete objects.
////////////////////////////////////////////////

corrector::~corrector(){
  // close the database
  similar_words.close_database();
  // delete dict;
  delete ph;
  delete phd;
}


////////////////////////////////////////////////////////////////////////
/// returns the phonema's transcription of a word
////////////////////////////////////////////////////////////////////////

string corrector:: getSound(string word){ 
	
  string result= ph->getSound(word);
  result=util::eliminateChars(result,"aeiou@AEIOU"); // eliminate vowels for generating a key for the database that will embrace more words
  if (result.size()==0) result="aeiou"; // if the word only contains vowels we use the keyword aeiou
  TRACE(3,"Getting the sound of word: "+word+", result: "+result);
  
  return result;
}


////////////////////////////////////////////////////////////////////////
/// adds the new words that are posible correct spellings from original 
/// word to the word analysys data
////////////////////////////////////////////////////////////////////////

void corrector:: putWords(string listaPal, word &w, string wordOrig) {

  list<string> tokens = util::string2list(listaPal,",");
  
  list<string>::iterator wd;  
  for( wd=tokens.begin(); wd!=tokens.end(); wd++) {
    
    int diff = wordOrig.size() - wd->size();
    if (diff<0) diff = -diff;

    if (diff<MAX_SIZE_DIFF) {      
      double simil=0.0;      
      if (distanceMethod=="similarity") 
	simil=(double) sm->getSimilarity(util::lowercase(wordOrig),util::lowercase(*wd));

      else if (distanceMethod=="phonetic") { // phonetic distance	
	string word1=util::lowercase(wordOrig);
	string word2=util::lowercase(*wd);
	word1= ph->getSound(word1);
	word2= ph->getSound(word2);
	simil=(double) phd->getPhoneticDistance(word1,word2);
	double simMax=(double) phd->getPhoneticDistance(word1,word1);
	simil=simil/simMax;	
      }

      if (simil>DISTANCE_LIMIT) {	
	list<analysis> la;	
	dict->search_form(*wd,la);
	
	for (list<analysis>::iterator an=la.begin(); an!=la.end(); an++) {
	  an->set_distance(simil);
	  an->set_lemma(an->get_lemma()+":"+(*wd));
	  w.add_analysis(*an);
	  TRACE(4,"   added analysis "+an->get_lemma()+" "+an->get_parole());
	}	
      }
    }
  }

}

////////////////////////////////////////////////////////////////////////
/// Navigates the sentence adding alternative words (possible correct spelling data)
////////////////////////////////////////////////////////////////////////

void corrector:: annotate(sentence &se) 
{
  bool spellCheck;
  sentence::iterator pos;
    
  for (pos=se.begin(); pos!=se.end(); ++pos)  {    

    if (!pos->get_n_analysis())
      spellCheck = noDictionaryCheck;
    else {
      spellCheck=false;
      for (word::iterator a=pos->begin(); a!=pos->end() and !spellCheck; a++) 
	spellCheck= dictionaryCheck.Search(a->get_parole());
    }
		      
    TRACE(3,"Checking word "+pos->get_form()+": "+(spellCheck?"yes":"no"));

    if (spellCheck){
      string listaPal;
      string wordOrig=pos->get_form();
      string keyword = getSound(wordOrig); // calculate the sound without vowels of the current word

      TRACE(3,"   Sound for word "+pos->get_form()+": "+keyword);
      if (keyword.size()>0){
	// we obatin the list of word separated by coma that match the keyword in the databse
	listaPal= similar_words.access_database(keyword); 
	TRACE(3,"   Obtained words: ["+listaPal+"]");

	if (listaPal.size()>0)
	  putWords(listaPal,*pos, wordOrig); // we add the new words to the POS of the word
      }
    }
  }  
}

