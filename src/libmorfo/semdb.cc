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

// define apropriate open calls depending on Berkeley-DB version.
#if (DB_VERSION_MAJOR==4 && DB_VERSION_MINOR==0)
  #define OPEN(name,db,type,flags,mode)  open(name,db,type,flags,mode)
#else
#if DB_VERSION_MAJOR>=4
  #define OPEN(name,db,type,flags,mode)  open(NULL,name,db,type,flags,mode)
#endif
#endif


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
///  Create the sense annotator
///////////////////////////////////////////////////////////////

semanticDB::semanticDB(const std::string &SensesFile, const std::string &WNFile): sensesdb(NULL,DB_CXX_NO_EXCEPTIONS),wndb(NULL,DB_CXX_NO_EXCEPTIONS)
{
  int res;

  // Opening a 4.0 or higher BerkeleyDB database
  if (SensesFile!="") {
    if ((res=sensesdb.OPEN(SensesFile.c_str(),NULL,DB_UNKNOWN,DB_RDONLY,0))) {
      ERROR_CRASH("Error "+util::int2string(res)+" while opening database "+SensesFile);
    }
  }

  // Opening a 4.0 or higher BerkeleyDB database
  if (WNFile!="") {
    if ((res=wndb.OPEN(WNFile.c_str(),NULL,DB_UNKNOWN,DB_RDONLY,0))) {
      ERROR_CRASH("Error "+util::int2string(res)+" while opening database "+WNFile);
    }
  }

  TRACE(1,"module succesfully created");
}

////////////////////////////////////////////////
/// Destroy sense annotator module, close database.
////////////////////////////////////////////////

semanticDB::~semanticDB() {
  int res;
  //Close the database
  if ((res=sensesdb.close(0))) {
    ERROR_CRASH("Error "+util::int2string(res)+" while closing sense database");
  }
  if ((res=wndb.close(0))) {
    ERROR_CRASH("Error "+util::int2string(res)+" while closing wn database");
  }
}


///////////////////////////////////////////////////////////////
///  Get senses for a lemma+pos
///////////////////////////////////////////////////////////////  

list<string> semanticDB::get_word_senses(const string &lemma, const string &pos) {
  return util::string2list(access_semantic_db(sensesdb,"W:"+lemma+":"+pos), " ");
}


///////////////////////////////////////////////////////////////
///  Get synonyms for a sense+pos
///////////////////////////////////////////////////////////////  

list<string> semanticDB::get_sense_words(const string &sens, const string &pos) {
  return util::string2list(access_semantic_db(sensesdb,"S:"+sens+":"+pos), " ");
}


///////////////////////////////////////////////////////////////
///  Get info for a sense+pos
///////////////////////////////////////////////////////////////  

sense_info semanticDB::get_sense_info(const string &syn, const string &pos) {
  /// access DB and split obtained data_string into fields
  /// and store appropriately in a sense_info object
  sense_info sinf(syn,pos, access_semantic_db(wndb,syn+":"+pos) );
  /// add also list of synset words
  sinf.words=get_sense_words(syn,pos);
  return sinf;
}


//--------- Private functions --------//

///////////////////////////////////////////////////////////////
///  Get senses for a lemma+pos or words for a sense+pos
///////////////////////////////////////////////////////////////  

string semanticDB::access_semantic_db(Db &database, const string &clau) {
  int error;
  string data_string;
  char buff[1024];
  unsigned int p;
  Dbt data, key;

  // Access the DB
  key.set_data((void *)clau.c_str());
  key.set_size(clau.length());
  error = database.get (NULL, &key, &data, 0);

  list<string> lsen;
  if (!error) {  // key found
    // copy the data associated to the key to the buffer
    p=data.get_size();
    memcpy((void *)buff, data.get_data(), p);

    // convert char* to string into data_string
    buff[p]=0; data_string=buff;    

    TRACE(3,clau+" Found: "+data_string);
  }
  else if (error == DB_NOTFOUND) {
    TRACE(3,clau+" NOT found.");
    data_string="";
  }
  else {
    ERROR_CRASH("Error "+util::int2string(error)+" while accessing sense database");
  }

  return data_string;
}

