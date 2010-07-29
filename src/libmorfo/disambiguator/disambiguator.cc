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


#include "freeling/disambiguator.h"

#include "ukb/common.h"
#include "ukb/kbGraph.h"
#include "ukb/csentence.h"
#include "ukb/globalVars.h"

#include "fries/util.h"
#include "freeling/traces.h"

using namespace std;
using namespace ukb;

#define MOD_TRACENAME "WSD"
#define MOD_TRACECODE SENSES_TRACE

//constructor
disambiguator::disambiguator(const string & relFile, const string & dictFile, double eps, int iter) {

  // load graph
  Kb::create_from_binfile(relFile);

  // set UKB stopping parameters
  glVars::prank::threshold = eps;
  glVars::prank::num_iterations = iter;

  // adding the dictionary to the UKB graph
  glVars::dict_filename = dictFile;  
  Kb::instance().add_dictionary(false);
}

//destructor
disambiguator::~disambiguator() {}

//retrieve numeric part of synset code
string disambiguator::convert_synset(const string &syns) const {
  return syns.substr(0, 8);
}

//comparison between pair<string, double> elements to sort a list of such elements in *decreasing* order
bool comp_pair(pair<string,double> p1, pair<string,double> p2) { return (p1.second > p2.second); }

void disambiguator::analyze(std::list<sentence> & ls) const {
  CSentence cs;
	
  //create one context from ls
  list<sentence>::iterator is;
  int id_numb = 0;
  
  list<sentence::iterator> lwctx;
  
  for (is=ls.begin(); is!=ls.end(); is++) {
    
    sentence::iterator w;
    for (w=is->begin(); w!=is->end(); w++) {
      
      char pos(0);
      pos = (util::lowercase(w->get_parole())).at(0);
      if (pos=='j') pos='a';
      
      if ((pos == 'a') || (pos == 'n') || (pos == 'r') || (pos == 'v')) {
	const string & w_id = "" + id_numb;
	CWord new_cw(w->get_lemma(), w_id, pos, CWord::cwdist);
	cs.push_back(new_cw);
	lwctx.push_back(w);
      }
      
      id_numb++;
    }
  }
  
  // no sentences (or no words) in given list.
  if (id_numb==0) return;
  
  //calling ukb library to disambiguate
  vector<float> ranks;
  
  bool ok = calculate_kb_ppr(cs,ranks);
  if (!ok) 
    WARNING("No word links to KB when calculating ranks for sentence "+cs.id());
  
  //ranks attributed to synsets for each word
  disamb_csentence_kb(cs, ranks);

  //copying the synsets back into FreeLing analysis
  list<sentence::iterator>::iterator ilwctx = lwctx.begin();
  
  vector<CWord>::iterator cw_it = cs.begin();
  vector<CWord>::iterator cw_end = cs.end();
  size_t num_syns;
  
  for (; cw_it != cw_end; ++cw_it, ++ilwctx) {
    list<pair<string,double> > lsen;
    vector<string> & vsyns = cw_it->get_syns_vector();
    num_syns = vsyns.size();
    TRACE(2,"Word "+cw_it->word()+". Found "+util::int2string(num_syns)+" synsets.");
    
    for (size_t i = 0; i != num_syns; ++i) {
      lsen.push_back(make_pair(convert_synset(vsyns[i]), cw_it->rank(i)));
      TRACE(2,"   Added synset "+vsyns[i]+" with rank "+util::double2string(cw_it->rank(i)));
    }
    //sorting the synsets by decreasing rank
    lsen.sort(comp_pair);
    
    //copying selected analyses to lsen 
    word::iterator a;
    for (a = (*ilwctx)->selected_begin(); a!= (*ilwctx)->selected_end(); a++) {
      a->set_senses(lsen);
    }
  }
}


list<sentence> disambiguator::analyze(const list<sentence> & ls) const {
  list<sentence> s=ls;
  analyze(s);
  return s;
}
