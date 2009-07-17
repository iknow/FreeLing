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
//   Indexate a raw-text dictionary in a BerkeleyDB database
//
//   Data read from stdin.  argv[1] specifies the name of the DB to be created.
//   Expected format for raw text file (read from stdin):
//
//      key1 data1 
//      key2 data2
//      ...
//
////////////////////////////////////////////////////////////////

#include <iostream>
#include <string>
#include <db_cxx.h>
#include <cstdlib>

using namespace std;


int main(int argc, char *argv[]) 
{
  int res, nlin;
  string s, s_form, s_analysis;
  Dbt key, data;

  // create database
  Db mydbase(NULL,DB_CXX_NO_EXCEPTIONS);
  if ((res=mydbase.open(NULL,argv[1],NULL,DB_HASH,DB_CREATE,0644))) {
    mydbase.err(res,"Error %d while creating database '%s'",res,argv[1]);
    exit(res);
  }

  // read stdin and insert records in database
  for (nlin=0; getline(cin,s); nlin++) {
 
    // split line in key+data
    s_form=s.substr(0,s.find(" "));
    s_analysis=s.substr(s.find(" ")+1);	

    // build db record
    key.set_data((void *)s_form.c_str());
    key.set_size(s_form.length());
    data.set_data((void*)s_analysis.c_str());
    data.set_size(s_analysis.length());
    
    // insert record in DB.
    res = mydbase.put(NULL, &key, &data, DB_NOOVERWRITE);
    if (res == DB_KEYEXIST) {
      mydbase.err(res,"Unexpected duplicate key %s at line %d",
 	 	      s_form.c_str(),nlin);
      exit(res);
    }
  }
  
  if ((res=mydbase.close(0))) {
    mydbase.err(res,"Error %d while closing database",res);
    exit(res);
  }
}
