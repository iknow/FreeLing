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

#ifndef _CORRECTOR
#define _CORRECTOR

#include <db_cxx.h>  // header of BerkeleyDB C++ interface

#include "fries/language.h"
#include "freeling/dictionary.h"
#include "freeling/phonetics.h"
#include "freeling/phoneticDistance.h"
#include "freeling/similarity.h"



////////////////////////////////////////////////////////////////
///
///  The class corrector implements a spell checker that will try 
/// to correct spelling errors
///
////////////////////////////////////////////////////////////////

class corrector {
   private:
      
      /// C++ Interface to BerkeleyDB C API
      Db morfodb;
	
	/// The dictionary that is currently using freeling
	dictionary* dict; 
	/// The class that translate a word into phonetic sounds
	phonetics* ph;
	/// The class that calculate the phonetic distante between two phonetic transcriptions
	phoneticDistance* phd;
	/// The method to calculate the distance between words
	std::string distanceMethod;
	/// the class that calculate the similarity between two words
	similarity* sm;
	/// contains the tags for the words to be checked that are present in the dictionary
	vector <string> dictionaryCheck;
	/// contains the tags for the words to be checked that are not present in the dictionary
	vector <string> noDictionaryCheck;
	
	
      	/// returns the phonema's transcription of a word
	std::string getSound(std::string);
	/// returns the list of words from the database that have the string as a key
	std::string getListWords(std::string);
	/// adds the new words that are posible correct spellings from original word to the word analysys data
	void putWords(std::string, word &, std::string);
      
      

   public:
      /// Constructor
      corrector(const std::string &, dictionary &, const std::string &,const std::string &);
      /// Destructor
      ~corrector();

      /// Navigates the sentence adding alternative words (possible correct spelling data)
      void annotate(sentence &);
};

#endif
