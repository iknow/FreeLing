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


///////////////////////////////////////////////////////////////
///  Create a corrector module, open database. 
///////////////////////////////////////////////////////////////

corrector::corrector(const std::string &configfile, dictionary &mydict): dictionaryCheck("") {
 
  string path=configfile.substr(0,configfile.find_last_of("/\\")+1);

  string line; 
  ifstream fabr (configfile.c_str());
  if (!fabr) { 
    ERROR_CRASH("Error opening file "+configfile);
  }
  
  // default
  useSoundDict=false;
  SimThresholdKnown=0.5;
  SimThresholdUnknownHigh=0.5;
  SimThresholdUnknownLow=0.5;
  MaxSizeDiff=3;
  distanceMethod=PHONETIC_DISTANCE;
  noDictionaryCheck=true;

  string ph_rules_file, wd_sound_dict, sampa_file, ph_dist_file;

  // load list of functional words that may be included in a NE.
  while (getline(fabr,line)) {

    istringstream sin;
    sin.str(line);
    string key,value;
    sin>>key;
    if (key.size()>0 && key[0]!='#') {
      if (key=="SimilarityThresholdKnown") 
	sin>>SimThresholdKnown;
      else if (key=="SimilarityThresholdUnknownHigh") 
	sin>>SimThresholdUnknownHigh;
      else if (key=="SimilarityThresholdUnknownLow") 
	sin>>SimThresholdUnknownLow;
      else if (key=="MaxSizeDiff") 
	sin>>MaxSizeDiff;
      else if (key=="CheckDictWords") {
	sin>>value;
	dictionaryCheck = RegEx(value);
      }
      else if (key=="CheckUnknown") {
	sin>>value;
	value=util::lowercase(value);
	noDictionaryCheck = (value=="yes" or value=="y");
      }
      else if (key=="SimilarWordsDict") {
	sin>>value;
	similar_words.open_database(util::absolute(value,path));
      }
      else if (key=="PhoneticRules") {
	sin>>ph_rules_file;
	ph_rules_file=util::absolute(ph_rules_file,path);
      }
      else if (key=="WordSoundDict") {
	sin>>wd_sound_dict;
	if (util::lowercase(wd_sound_dict)=="none"
             || util::lowercase(wd_sound_dict)=="no") {
	  wd_sound_dict="";
	  useSoundDict=false;
	}	
	else {
	  wd_sound_dict=util::absolute(wd_sound_dict,path);
	  useSoundDict=true;
	}
      }
      else if (key=="SampaCodes") {
	sin>>sampa_file;
	sampa_file=util::absolute(sampa_file,path);
      }
      else if (key=="PhoneticDist") {
	sin>>ph_dist_file;
	if (util::lowercase(ph_dist_file)=="none" 
             || util::lowercase(ph_dist_file)=="no") {
	  ph_dist_file="";
	  distanceMethod=EDIT_DISTANCE;
	}	
	else {
	  ph_dist_file=util::absolute(ph_dist_file,path);
	  distanceMethod=PHONETIC_DISTANCE;
	}
      }
      else 
	WARNING("Unexpected keyword '"+key+"' in config file. Ignored.");
    }
  }
  fabr.close(); 

  if (SimThresholdUnknownHigh<SimThresholdUnknownLow) {
    WARNING("SimilarityThresholdUnknownHigh can't be lower than SimThresholdUnknownLow. Ignored");
    SimThresholdUnknownHigh=SimThresholdUnknownLow;
  }

  ph= new phonetics(ph_rules_file, wd_sound_dict, sampa_file, useSoundDict);

  if (distanceMethod==EDIT_DISTANCE) sm= new similarity();
  else if (distanceMethod==PHONETIC_DISTANCE) phd= new phoneticDistance(ph_dist_file);

  dict=&mydict;
    
  TRACE(1,"corrector succesfully created");
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

string corrector:: getKey(string word){ 
	
  string result= ph->getSound(word);

  TRACE(3," Getting the sound of word: "+word+", result: "+result);

  // eliminate vowels to generate a key for the database 
  // that will find similar words
  result=util::eliminateChars(result,"aeiou@AEIOU"); 
 // if the word only contains vowels we use the key aeiou
  if (result.size()==0) result="aeiou";
  TRACE(3,"Getting the key of word: "+word+", result: "+result);
  
  return result;
}


////////////////////////////////////////////////////////////////////////
/// adds the new words that are posible correct spellings from original 
/// word to the word analysys data
////////////////////////////////////////////////////////////////////////

void corrector:: putWords(string listaPal, word &w) {

  list<string> tokens = util::string2list(listaPal,",");
  string wform = util::lowercase(w.get_form());

  int na=w.get_n_analysis();
  
  bool found=false;
  list<pair<word,double> > lalt;
  list<string>::iterator wd;  
  for( wd=tokens.begin(); wd!=tokens.end(); wd++) {
    
    int diff = wform.size() - wd->size();
    if (diff<0) diff = -diff;

    if (diff<MaxSizeDiff && wform!=(*wd)) { // do not repeat analysis in dict for a known form
      double simil=0.0;      
      if (distanceMethod==EDIT_DISTANCE) {
	simil=(double) sm->getSimilarity(wform,*wd);	
	TRACE(4,"   Simil "+util::double2string(simil)+" for ("+wform+") vs ("+(*wd)+")");
      }
      else if (distanceMethod==PHONETIC_DISTANCE) { // phonetic distance	
	string word1=ph->getSound(wform);
	string word2=ph->getSound(*wd);

	simil=(double) phd->getPhoneticDistance(word1,word2);
	double simMax=(double) phd->getPhoneticDistance(word1,word1);
	simil=simil/simMax;	
	TRACE(4,"   Simil "+util::double2string(simil)+" for ("+wform+","+word1+") vs ("+(*wd)+","+word2+")");
      }

      if (simil==1.0) found=true;

      if ((!na && simil>SimThresholdUnknownLow) || (na && simil>SimThresholdKnown)) {

	list<analysis> la;	
	dict->search_form(*wd,la);

	word alt(*wd);	
        alt.set_analysis(la);

	lalt.push_back(make_pair(alt,simil));
	TRACE(3,"    - added alternative ("+(*wd)+","+util::double2string(simil)+")"); 
      }
    }
  }

  // If the word was unkown but there was a perfect sound match
  // filter the list using SimThresholdUnknownHigh
  if (!na and found and SimThresholdUnknownHigh>SimThresholdUnknownLow) {
    list<pair<word,double> >::iterator pa;
    for (list<pair<word,double> >::iterator a=lalt.begin(); a!=lalt.end(); a++) {
      if (a->second<=SimThresholdUnknownHigh) {	
	TRACE(3,"    - threshold raised. Erasing alternative "+a->first.get_form());
	pa=a; pa--;
	lalt.erase(a);
	a=pa;
      }
    }
  }

  // add selected alternatives to the word
  w.set_alternatives(lalt);
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
      // calculate the sound without vowels of the current word
      string key = getKey(pos->get_form()); 

      TRACE(3,"   Key for word "+pos->get_form()+": "+key);
      if (key.size()>0){
	// Get the list of words with same key in the database
	listaPal= similar_words.access_database(key); 
	TRACE(3,"   Obtained words: ["+listaPal+"]");

	// filter and add the obtained words as new analysis
	if (listaPal.size()>0) putWords(listaPal,*pos); 
      }
    }
  }  
}

