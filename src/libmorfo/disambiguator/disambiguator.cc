#include "freeling/disambiguator.h"
#include "ukb/common.h"
#include "ukb/kbGraph.h"
#include "ukb/csentence.h"
#include "ukb/globalVars.h"

#include "fries/util.h"

using namespace std;
using namespace ukb;

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
	CWord new_cw(w->get_lemma(), w_id, pos, CWord::cwtoken);
	cs.push_back(new_cw);
	lwctx.push_back(w);
      }
      
      id_numb++;
    }
  }
  
  
  //calling ukb library to disambiguate
  vector<float> ranks;
  
  /*bool ok = */calculate_kb_ppr(cs,ranks);
  //If an error is output here then you have an output for every empty line, not very agreeable.
  //  if (!ok) {
  //	cerr << "Error when calculating ranks for sentence " << cs.id() << "\n";
  //	cerr << "(No word links to KB ?)\n";
  //  }
  
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
    
    for (size_t i = 0; i != num_syns; ++i)
      lsen.push_back(make_pair(convert_synset(vsyns[i]), cw_it->rank(i)));
    
    //sorting the synsets by decreasing rank
    lsen.sort(comp_pair);
    
    //copying lsen to the selected analyses
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
