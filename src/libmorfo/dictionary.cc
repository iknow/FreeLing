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


///////////////////////////////////////////////////////////////
///  Create a dictionary module, open database.
///////////////////////////////////////////////////////////////

dictionary::dictionary(const std::string &Lang, const std::string &dicFile, bool activateAff, const std::string &sufFile) {

  // remember if affix analysis is to be performed
  AffixAnalysis = activateAff;
  // create affix analyzer if required
  suf = (AffixAnalysis ? new affixes(Lang, sufFile) : NULL);

  // Opening a 4.0 or higher BerkeleyDB database, somewhat slower, but saves RAM.
  morfodb.open_database(dicFile);
  
  TRACE(3,"analyzer succesfully created");
}


////////////////////////////////////////////////
/// Destroy dictionary module, close database.
////////////////////////////////////////////////

dictionary::~dictionary(){
  // Close the database
  morfodb.close_database();
  // delete affix analyzer, if any
  delete suf;
}

/////////////////////////////////////////////////////////////////////////////
///  Search form in the dictionary, according to given options,
///  *Add* found analysis to the given list
/////////////////////////////////////////////////////////////////////////////

void dictionary::search_form(const std::string &s, std::list<analysis> &la) {

  // lowercase the string
  string key = util::lowercase(s);

  // search word in the active dictionary  
  string data = morfodb.access_database(key);

  if (not data.empty()) {
    // process the data string into analysis list
    string::size_type p=0, q=0;
    while (p!=string::npos) {
      TRACE(3,"word '"+s+"'. remaining data: ["+data.substr(p)+"]");
      // get lemma
      q=data.find_first_of(" ",p);
      string lem=data.substr(p,q-p);
      TRACE(4,"   got lemma="+lem+" p="+util::int2string(p)+" q="+util::int2string(q));
      // get tag
      p=q+1;
      q=data.find_first_of(" ",p);
      string tag=data.substr(p,q-p);
      TRACE(4,"   got tag="+tag+" p="+util::int2string(p)+" q="+util::int2string(q));
      // prepare next
      p= (q==string::npos? string::npos : q+1);
      // insert analysis
      TRACE(3,"Adding ("+lem+","+tag+") to analysis list");
      la.push_back(analysis(lem,tag));
    }
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
   if (AffixAnalysis) {
     TRACE(2,"Affix analisys active. SEARCHING FOR AFFIX. word n_analysis="+util::int2string(w.get_n_analysis()));
     suf->look_for_affixes(w, *this);
   }
 }


////////////////////////////////////////////////////////////////////////
/// Check whether the given word is a contraction, if so, obtain 
/// composing words (and store them into lw).
////////////////////////////////////////////////////////////////////////

bool dictionary::check_contracted(const word &w, std::list<word> &lw) {
  string lem,tag,cl;
  list<string> ct;
  size_t pl,pt;
  list<analysis>::const_iterator a;
  bool contr;

  // we check only the first analysis, since contractions can have only one analysis.
  lem=w.get_lemma(); tag=w.get_parole();
  contr=false;

  pl=lem.find_first_of("+");   pt=tag.find_first_of("+");

  if (pl!=string::npos && pt!=string::npos && w.get_n_analysis()>1) 
    WARNING("Contraction "+w.get_form()+" has two analysis in dictionary. All but first ignored.");

  while (pl!=string::npos && pt!=string::npos) {
    contr=true;    // at least one "+" found. it's a contraction

    // split contracted component out of "lem" and "tag" strings
    cl=lem.substr(0,pl);   ct=util::string2list(tag.substr(0,pt),"/");
    lem=lem.substr(pl+1);  tag=tag.substr(pt+1);
    TRACE(3,"Searching contraction component... "+cl+"_"+util::list2string(ct,"/"));

    // obtain analysis for contracted component, and keep analysis matching the given tag/s
    list<analysis> la;
    search_form(cl,la);

    // create new word for the component
    word c(cl);
    // add all matching analysis for the component
    for (a=la.begin(); a!=la.end(); a++) {
      for (list<string>::const_iterator t=ct.begin(); t!=ct.end(); t++) {
	if (a->get_parole().find(*t)==0 || (*t)=="*") {
	  c.add_analysis(*a); 
	  TRACE(3,"  Matching analysis: "+a->get_parole());
	}
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

    cl=lem.substr(0,pl);   ct=util::string2list(tag.substr(0,pt),"/");
    lem=lem.substr(pl+1); tag=tag.substr(pt+1);
    TRACE(3,"Searching contraction component... "+cl+"_"+util::list2string(ct,"/"));

    list<analysis> la;
    search_form(cl,la);

    word c(cl);
    for (a=la.begin(); a!=la.end(); a++) {
      for (list<string>::const_iterator t=ct.begin(); t!=ct.end(); t++) {
	if (a->get_parole().find(*t)==0 || (*t)=="*") {
	  c.add_analysis(*a); 
	  TRACE(3,"  Matching analysis: "+a->get_parole());
	}
      }
    }
    lw.push_back(c);

    if (c.get_n_analysis()==0) 
       ERROR_CRASH("Tag not found for contraction component. Check dictionary entry for "+w.get_form());
  }

  return (contr);
}


////////////////////////////////////////////////////////////////////////
///  Dictionary search and affix analysis for all words
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

      // check whether the word is a contraction, if so, obtain composing words (into lw)
      // and replace it with "uncontracted" components.
      list<word> lw;
      if (check_contracted(*pos,lw)) {        

        TRACE(2,"Contraction found, replacing... "+pos->get_form()+". span=("+util::int2string(pos->get_span_start())+","+util::int2string(pos->get_span_finish())+")");

        int st=pos->get_span_start(); 
        int step=(pos->get_span_finish()-st+1)/lw.size(); 
        if (step<1) step=1;

        for (i=lw.begin(); i!=lw.end(); i++) {
	  i->set_span(st,st+step-1);
	  i->user=pos->user;

          TRACE(2,"  Inserting "+i->get_form()+". span=("+util::int2string(i->get_span_start())+","+util::int2string(i->get_span_finish())+")");
          pos=se.insert(pos,*i); 
          pos++;
	  st=st+step;
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

