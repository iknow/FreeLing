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

#include "freeling/database.h"
#include "freeling/traces.h"

using namespace std;

#define MOD_TRACENAME "DATABASE"
#define MOD_TRACECODE DATABASE_TRACE

///////////////////////////////////////////////////////////////
///  Create a database accessing module
///////////////////////////////////////////////////////////////

#ifdef USE_LIBDB
database::database() : Db(NULL,DB_CXX_NO_EXCEPTIONS) {}
#else
database::database() {}
#endif

///////////////////////////////////////////////////////////////
///  Destructor
///////////////////////////////////////////////////////////////

database::~database() {}


///////////////////////////////////////////////////////////////
///  Open a BerkeleyDB file and link it to the database object
///////////////////////////////////////////////////////////////

void database::open_database(const string &file) {
  int res;

  if (file.substr(file.size()-3)==".db") {

    #ifdef USE_LIBDB
      // open a Berkeley DB 
      if ((res=this->OPEN(file.c_str(),NULL,DB_UNKNOWN,DB_RDONLY,0))) {
        ERROR_CRASH("Error '"+string(db_strerror(res))+"' while opening database "+file);
      }
      usingDB=true;
    #else
      ERROR_CRASH("Error opening database "+file+". BerkeleyDB support was not compiled in this FreeLing installation.");
    #endif
  }

  else if (file.substr(file.size()-4)==".src") {
    // Load plain dictionary in a RAM map. 
    ifstream fdic (file.c_str());
    if (!fdic) ERROR_CRASH("Error opening file "+file);
    
    string line; 
    dbmap.clear();
    while (getline(fdic,line)) {
      // split line in key+data
      string form=line.substr(0,line.find(" "));
      string analysis=line.substr(line.find(" ")+1);	
      
      dbmap.insert(make_pair(form,analysis));
    }
    
    usingDB=false;
  }
  else {
    ERROR_CRASH("Unknown dictionary type "+file+". Please provide either .db or .src file name");
  }

}

///////////////////////////////////////////////////////////////
///  close the database.
///////////////////////////////////////////////////////////////

void database::close_database() {
  int res;

  #ifdef USE_LIBDB
    if (usingDB)
      if ((res=this->close(0))) {
        ERROR_CRASH("Error '"+string(db_strerror(res))+"' while closing database");
      }
  #endif
}

///////////////////////////////////////////////////////////////
///  search for a string key in the DB, return associated string data.
///////////////////////////////////////////////////////////////

string database::access_database(const string &clau) {

  string data_string;

  if (usingDB) {
    // database is in berkeley DB

    #ifdef USE_LIBDB
      int error;
      char buff[1024];
      unsigned int p;
      Dbt data, key;
      
      // Access the DB
      key.set_data((void *)clau.c_str());
      key.set_size(clau.length());
      error = this->get (NULL, &key, &data, 0);
      
      list<string> lsen;
      if (!error) {  // key found
	// copy the data associated to the key to the buffer
	p=data.get_size();
	memcpy((void *)buff, data.get_data(), p);
	
	// convert char* to string into data_string
	buff[p]=0; data_string=buff;    
      }
      else if (error == DB_NOTFOUND) {
	data_string="";
      }
      else {
	ERROR_CRASH("Error '"+string(db_strerror(error))+"' while accessing database");
      }
    #else
      ERROR_CRASH("BerkeleyDB support was not compiled in this FreeLing installation.");
    #endif
  }
  else {
    // database is in RAM map
    map<string,string>::iterator p=dbmap.find(clau);
    if (p!=dbmap.end()) data_string=p->second;
    else data_string="";
  }

  return data_string;
}


