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

#include "freeling/senses.h"
#include "freeling/traces.h"

using namespace std;

#define MOD_TRACENAME "SENSES"
#define MOD_TRACECODE SENSES_TRACE


///////////////////////////////////////////////////////////////
///  Create the sense annotator
///////////////////////////////////////////////////////////////

senses::senses(const string &fsense, bool dup) : duplicate(dup) {
  semdb = new semanticDB(fsense, "");
  TRACE(1,"analyzer succesfully created");
}

///////////////////////////////////////////////////////////////
///  Destructor
///////////////////////////////////////////////////////////////

senses::~senses() {
  delete semdb;
}

///////////////////////////////////////////////////////////////
///  Analyze given sentences.
///////////////////////////////////////////////////////////////  

void senses::analyze(std::list<sentence> &ls) {
  
  // anotate with all possible senses, no disambiguation
  list<sentence>::iterator is;     
  for (is=ls.begin(); is!=ls.end(); is++) {
    
    sentence::iterator w;
    for (w=is->begin(); w!=is->end(); w++) {

      list<analysis> newla;
      word::iterator a;
      for (a=w->begin(); a!=w->end(); a++) {

	// search senses for the word
	list<string> lsen=semdb->get_word_senses(a->get_lemma(),a->get_parole().substr(0,1));
	
        if (lsen.size()==0) {
          // no senses found for that lemma.
	  if (duplicate) newla.push_back(*a);
	}
	else {
	  // senses found for that lemma
          if (duplicate) {
            list<string> ss;
	    list<string>::iterator s=lsen.begin(); 
            double newpr= a->get_prob()/lsen.size();
          
	    for (s=lsen.begin(); s!=lsen.end(); s++) {
	      // create a copy of the analysis for each sense
	      analysis *newan = new analysis(*a);
	      // add current sense to new analysis, overwriting.
              ss.clear(); ss.push_back(*s);  

 	      list<string>::iterator lsi;
	      list<pair<string, double> > lsen_noranks;
	      for (lsi = ss.begin(); lsi != ss.end(); lsi++) {
	    	  lsen_noranks.push_back(make_pair(*lsi, 0.0));
	      }
              newan->set_senses(lsen_noranks); 

	      newan->set_prob(newpr);
	      // add new analysis to the new list
	      newla.push_back(*newan);

	      TRACE(3, "  Duplicating analysis for sense "+(*s));
	    }
	  }
	  else {
	    // duplicate not set. Add the the whole sense list to current analysis
	    list<string>::iterator lsi;
	    list<pair<string, double> > lsen_noranks;
	    for (lsi = lsen.begin(); lsi != lsen.end(); lsi++) {
	    	lsen_noranks.push_back(make_pair(*lsi, 0.0));
	    }
	    a->set_senses(lsen_noranks);
	  }
	}
      }

      // if duplicate is on, use the newly rebuild analysis list
      if (duplicate) w->set_analysis(newla);
    }
  }

  TRACE(1,"Sentences annotated by the senses module.");
}


///////////////////////////////////////////////////////////////
///  Analyze given sentences. Return analyzed copy.
///////////////////////////////////////////////////////////////  

std::list<sentence> senses::analyze(const std::list<sentence> &ls) {
  
  list<sentence> y=ls;
  analyze(y);
  return y;
}

