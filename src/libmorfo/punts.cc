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

#include <sstream>
#include <fstream>

#include "freeling/punts.h"
#include "freeling/traces.h"

using namespace std;

#define MOD_TRACENAME "PUNCTUATION"
#define MOD_TRACECODE PUNCT_TRACE

////////////////////////////////////////////////////////////////
/// Create a punctuation sign recognizer.
////////////////////////////////////////////////////////////////

punts::punts(const std::string &puntFile)
{ 
  string line,key,data;

  // opening punctuation symbols file
  ifstream fabr (puntFile.c_str());
  if (!fabr) { 
    ERROR_CRASH("Error opening file "+puntFile);
  }
  
  // loading file into map
  while(getline(fabr, line)) {
    istringstream sin;
    sin.str(line);
    sin>>key>>data;
    punct.insert(make_pair(key,data));       
  }
  fabr.close(); 
  
  TRACE(3,"analyzer succesfully created");
}

////////////////////////////////////////////////////////////////
/// Detect and annotate punctuation signs in given sentence,
/// using given options.
////////////////////////////////////////////////////////////////

void punts::annotate(sentence &se) {
  sentence::iterator i;
  map <string,string>::const_iterator im;
  string form;
  string::iterator ch;

  for(i=se.begin();i!=se.end();++i) {

    form=i->get_form();
    TRACE(3,"checking "+form);
    // search for alphanumeric char in the word
    for(ch=form.begin(); ch!=form.end() && !util::isalphanum(*ch); ++ch);
    TRACE(3," search alphanum char ");
 
    if (ch==form.end()) {
      TRACE(3,"   ...but not found ");
      // No alphanum found.  Must be a punctuation sign
      im = punct.find(form);
      TRACE(3,"     ["+form+"] searched in map");
      if (im!=punct.end()) {
        TRACE(3,"         ...and found: known punctuation");
	// punctuation sign found in the hash
	i->set_analysis(analysis(form,(*im).second));
      }
      else { 
        TRACE(3,"         ...and not found: other punctuation");
	// Not found. Tag it as "others"
	i->set_analysis(analysis(form,punct[OTHER]));
      }
    }
  }
  
  TRACE(3,"done ");
  TRACE_SENTENCE(1,se);
}


