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

#include "freeling/semdb.h"
#include "freeling/traces.h"

using namespace std;

#define MOD_TRACENAME "SEMDB"
#define MOD_TRACECODE SENSES_TRACE


///////////////////////////////////////////////////////////////
///  Constructor of the auxiliary class "sense_info"
///////////////////////////////////////////////////////////////

sense_info::sense_info(const string &syn, const string &p, const string &data) {

  // store sense identifier
  sense=syn;  pos=p;

  // split data string into fields
  list<string> fields=util::string2list(data," ");

  // iterate over field list and store each appropriately
  list<string>::iterator f=fields.begin();

  //first field: list of hypernyms
  if ((*f)!="-") parents=util::string2list(*f,":");   
  // second field: WN semantic file
  f++; semfile=(*f);   
  // third filed: List of EWN top-ontology labels  
  f++; if ((*f)!="-") tonto=util::string2list(*f,":");
}


///////////////////////////////////////////////////////////////
///  Obtain parent list as string (useful for Java API
///////////////////////////////////////////////////////////////

string sense_info::get_parents_string() const {
  return util::list2string(parents,":");
}

///////////////////////////////////////////////////////////////
///  Create the sense annotator
///////////////////////////////////////////////////////////////

semanticDB::semanticDB(const std::string &SensesFile, const std::string &WNFile) {

  if (SensesFile!="") sensesdb.open_database(SensesFile.c_str());
  if (WNFile!="") wndb.open_database(WNFile.c_str());

  TRACE(1,"module succesfully created");
}

////////////////////////////////////////////////
/// Destroy sense annotator module, close database.
////////////////////////////////////////////////

semanticDB::~semanticDB() {
  //Close the databases
  sensesdb.close_database();
  wndb.close_database();
}


///////////////////////////////////////////////////////////////
///  Get senses for a lemma+pos
///////////////////////////////////////////////////////////////  

list<string> semanticDB::get_word_senses(const string &lemma, const string &pos) {
  return util::string2list(sensesdb.access_database("W:"+lemma+":"+pos), " ");
}


///////////////////////////////////////////////////////////////
///  Get synonyms for a sense+pos
///////////////////////////////////////////////////////////////  

list<string> semanticDB::get_sense_words(const string &sens, const string &pos) {
  return util::string2list(sensesdb.access_database("S:"+sens+":"+pos), " ");
}


///////////////////////////////////////////////////////////////
///  Get info for a sense+pos
///////////////////////////////////////////////////////////////  

sense_info semanticDB::get_sense_info(const string &syn, const string &pos) {
  /// access DB and split obtained data_string into fields
  /// and store appropriately in a sense_info object
  sense_info sinf(syn,pos, wndb.access_database(syn+":"+pos) );
  /// add also list of synset words
  sinf.words=get_sense_words(syn,pos);
  return sinf;
}

