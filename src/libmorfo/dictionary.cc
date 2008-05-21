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

#include "freeling/dictionary.h"
#include "fries/util.h"
#include "freeling/traces.h"

using namespace std;

#define MOD_TRACENAME "DICTIONARY"
#define MOD_TRACECODE DICT_TRACE

// define apropriate open calls depending on Berkeley-DB version.
#if (DB_VERSION_MAJOR==4 && DB_VERSION_MINOR==0)
  #define OPEN(name,db,type,flags,mode)  open(name,db,type,flags,mode)
#else
#if DB_VERSION_MAJOR>=4
  #define OPEN(name,db,type,flags,mode)  open(NULL,name,db,type,flags,mode)
#endif
#endif


///////////////////////////////////////////////////////////////
///  Create a dictionary module, open database.
///////////////////////////////////////////////////////////////

dictionary::dictionary(const std::string &Lang, const std::string &dicFile, bool activateSuf, const std::string &sufFile): morfodb(NULL,DB_CXX_NO_EXCEPTIONS)
{
  int res;

  // remember if suffix analysis is to be performed
  MACO_SuffixAnalysis = activateSuf;
  // create suffix analyzer if required
  if (MACO_SuffixAnalysis) suf = new suffixes(Lang, sufFile);

  // Opening a 4.0 or higher BerkeleyDB database
  if ((res=morfodb.OPEN(dicFile.c_str(),NULL,DB_UNKNOWN,DB_RDONLY,0))) {
    ERROR_CRASH("Error "+util::int2string(res)+" while opening database "+dicFile);
  }

  TRACE(3,"analyzer succesfully created");
}


////////////////////////////////////////////////
/// Destroy dictionary module, close database.
////////////////////////////////////////////////

dictionary::~dictionary(){
int res;
  // Close the database
  if ((res=morfodb.close(0))) {
    ERROR_CRASH("Error "+util::int2string(res)+" while closing database");
  }
  // delete suffix analyzer, if any
  delete suf;
}

/////////////////////////////////////////////////////////////////////////////
///  Search form in the dictionary, according to given options,
///  *Add* found analysis to the given list
/////////////////////////////////////////////////////////////////////////////

void dictionary::search_form(const std::string &s, std::list<analysis> &la) {
  string data_string,lem,tag,lws;
  char buff[1024];
  string::size_type p;
  int error;
  Dbt data, key;

  // lowercase the string
  lws=util::lowercase(s);

  // Get data of the analysis associated to the key passed as argument
  key.set_data((void *)lws.c_str());
  key.set_size(lws.length());
  error = morfodb.get (NULL, &key, &data, 0);

  if (!error) {
     // copy the data associated to the key to the buffer
     p=data.get_size();
     memcpy((void *)buff, data.get_data(), p);
     // convert char* to string into data_string
     buff[p]=0;
     data_string=buff;
     // process the data string into analysis list
     p=0;
     while(p!=string::npos){
       TRACE(3,"data_string contains now: ["+data_string+"]");
       // get lemma
       p=data_string.find_first_of(" ");
       lem=data_string.substr(0,p);
       data_string=data_string.substr(p+1);
       // get tag
       p=data_string.find_first_of(" ");
       tag=data_string.substr(0,p);
       data_string=data_string.substr(p+1);
       // insert analysis
       TRACE(3,"Adding ("+lem+","+tag+") to analysis list");
       la.push_back(analysis(lem,tag));
     }
  }
  else if (error != DB_NOTFOUND) {
    ERROR_CRASH("Error "+util::int2string(error)+" while accessing database");
  }

}

/////////////////////////////////////////////////////////////////////////////
///  Search form in the dictionary, according to given options,
///  *Add* found analysis to the given word.
/////////////////////////////////////////////////////////////////////////////

void dictionary::annotate_word(word &w) {
  string data_string,lem,tag,lws;
  Dbt data, key;
  list<analysis> la;
  list<analysis>::const_iterator a;

  search_form(w.get_form(), la);
  w.set_found_in_dict( la.size()>0 );  // set "found_in_dict" accordingly to results
  TRACE(3,"   Found "+util::int2string(la.size())+" analysis.");

  for (a=la.begin(); a!=la.end(); a++) {
     w.add_analysis(*a);
     TRACE(4,"   added analysis "+a->get_lemma());
  }

  // check whether the word is a derived form via suffixation
  if (MACO_SuffixAnalysis) {
    TRACE(2,"Suffix analisys active. SEARCHING FOR A SUFFIX. word n_analysis="+util::int2string(w.get_n_analysis()));
    suf->look_for_suffixes(w, *this);
  }
}


////////////////////////////////////////////////////////////////////////
/// Check whether the given word is a contraction, if so, obtain 
/// of what words (and store them into lw).
////////////////////////////////////////////////////////////////////////

bool dictionary::check_contracted(const word &w, std::list<word> &lw) {
  string lem,tag,cl,ct;
  size_t pl,pt;
  list<analysis>::const_iterator a;
  bool contr;

  // we check only the first analysis, since contractions can have only one analysis.
  lem=w.get_lemma(); tag=w.get_parole();
  contr=false;

  pl=lem.find_first_of("+");   pt=tag.find_first_of("+");
  while (pl!=string::npos && pt!=string::npos) {
    contr=true;    // at least one "+" found. it's a contraction

    // split contracted component out of "lem" and "tag" strings
    cl=lem.substr(0,pl);   ct=tag.substr(0,pt);
    lem=lem.substr(pl+1);  tag=tag.substr(pt+1);

    TRACE(3,"Searching contraction component... "+cl+"_"+ct);

    // obtain analysis for contracted component, and keep the first matching the given tag
    list<analysis> la;
    search_form(cl,la);

    // create new word for the component
    word c(cl);
    // add all matching analysis for the component
    for (a=la.begin(); a!=la.end(); a++) {
      if (a->get_parole().find(ct)==0 || ct=="*") {
        c.add_analysis(*a); 
        TRACE(3,"  Matching analysis: "+a->get_parole());
      }
    }
    lw.push_back(c);

    if (c.get_n_analysis()==0) 
       ERROR_CRASH("Tag not found for contraction component. Check dictionary entry for "+w.get_form());
  
    // look for next component
    pl=lem.find_first_of("+");  pt=tag.find_first_of("+");
  }

  // process last component (only if it was a contraction)
  if (contr) {
    cl=lem.substr(0,pl);  ct=tag.substr(0,pt);
    lem=lem.substr(pl+1); tag=tag.substr(pt+1);
    TRACE(3,"Searching contraction component... "+cl+"_"+ct);

    list<analysis> la;
    search_form(cl,la);

    word c(cl);
    for (a=la.begin(); a!=la.end(); a++) {
      if (a->get_parole().find(ct)==0 || ct=="*") {
        c.add_analysis(*a); 
        TRACE(3,"  Matching analysis: "+a->get_parole());
      }
    }
    lw.push_back(c);

    if (c.get_n_analysis()==0) 
       ERROR_CRASH("Tag not found for contraction component. Check dictionary entry for "+w.get_form());
  }

  return (contr);
}


////////////////////////////////////////////////////////////////////////
///  Dictionary search and suffix analysis for all words
/// in a sentence, using given options.
////////////////////////////////////////////////////////////////////////

void dictionary:: annotate(sentence &se) {
  string temp;
  sentence::iterator pos,q;
  list<word>::iterator i;

  for (pos=se.begin(); pos!=se.end(); ++pos){
    // Process the word if it hasn't been annotated by previous modules,
    // except if annotated by "numbers" module, ("uno","one" are numbers 
    // but also pronouns, "one must do whatever must be done")
    if (!pos->get_n_analysis() || (pos->get_n_analysis() && pos->get_parole()[0]=='Z')) {
      TRACE(1,"Searching in the dictionary the WORD: "+pos->get_form());
      annotate_word(*pos);

      // check whether the word is a contraction, if so, obtain of what words (into lw)
      // and replace it with "uncontracted" components.
      list<word> lw;
      if (check_contracted(*pos,lw)) {        

        TRACE(2,"Contraction found, replacing... "+pos->get_form()+". span=("+util::int2string(pos->get_span_start())+","+util::int2string(pos->get_span_finish())+")");
        for (i=lw.begin(); i!=lw.end(); i++) {
	  i->set_span(pos->get_span_start(),pos->get_span_finish());

          TRACE(2,"  Inserting "+i->get_form()+". span=("+util::int2string(i->get_span_start())+","+util::int2string(i->get_span_finish())+")");
          pos=se.insert(pos,*i); 
          pos++;
         }

	TRACE(2,"  Erasing "+pos->get_form());
        q=pos; q--;     // save pos of previous word
        se.erase(pos);  // erase contracted word
        pos=q;          // fix iteration control
      }
    }

  }

  TRACE_SENTENCE(1,se);
}

