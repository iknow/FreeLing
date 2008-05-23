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

#include <cmath>

#include "freeling/relax.h"
#include "fries/util.h"
#include "freeling/traces.h"

using namespace std;

#define MOD_TRACENAME "RELAX"
#define MOD_TRACECODE RELAX_TRACE


//---------- Class problem ----------------------------------

////////////////////////////////////////////////////////////////
///
/// Constructor: allocate space for nv variables, 
/// and a label weigth list for each
///
///////////////////////////////////////////////////////////////

problem::problem(int nv) {
  this->reserve(nv);
  for (int i=0; i<nv; i++) {
    list<double> ld;
    this->push_back(ld);
  }
};


///////////////////////////////////////////////////////////////
///
///  Add a label (and its weight) to the i-th variable
///
///////////////////////////////////////////////////////////////

void problem::add_label(int i, double w) {
  (*this)[i].push_back(w);
};



//---------- Class label ----------------------------------

////////////////////////////////////////////////////////////////
///  The class label stores all information related to a 
/// variable label in the relaxation labelling algorithm.
////////////////////////////////////////////////////////////////

label::label() {};



//---------- Class constraint ----------------------------------

////////////////////////////////////////////////////////////////
///  The class constraint stores all information related to a 
/// constraint on a label in the relaxation labelling algorithm.
////////////////////////////////////////////////////////////////

constraint::constraint() {}


////////////////////////////////////////////////
/// set compatibility value
////////////////////////////////////////////////

void constraint::set_compatibility(double c) {
  compatibility=c;
}

////////////////////////////////////////////////
/// get compatibility value
////////////////////////////////////////////////

double constraint::get_compatibility() const {
  return(compatibility);
}


//---------- Class relax ----------------------------------

///////////////////////////////////////////////////////////////
///  Constructor: Build a relax solver
///////////////////////////////////////////////////////////////

relax::relax(int m, double f, double r) : CURRENT(0), NEXT(1), MaxIter(m), ScaleFactor(f), Epsilon(r) {}


////////////////////////////////////////////////
/// Prepare for a new problem (ie, free old tables 
/// and alloc new for the new problem). 
/// Reset main variables table
////////////////////////////////////////////////

void relax::reset(const problem &p) {
int j;
problem::const_iterator v;
list<double>::const_iterator lb;

 // free old tables
 vars.clear();

 CURRENT=0; NEXT=1;

 // allocate tables for the new sentence. One variable for each word in the sentence
 vars.reserve(p.size()); 

 // Then, for each variable, as many labels as analysis in the word
 for (v=p.begin();  v!=p.end();  v++) {
   vector<label> vlab(v->size());
   for (j=0,lb=v->begin(); lb!=v->end(); j++,lb++) {
     vlab[j].weight[CURRENT] = (*lb);
     vlab[j].weight[NEXT] = (*lb);
   }
 
   vars.push_back(vlab);
 } 
}


////////////////////////////////////////////////
///  Add a new constraint to the problem, afecting 
/// the i.j pair
////////////////////////////////////////////////

void relax::add_constraint(int v, int l, const list<list<pair<int,int> > > &lp, double comp) {
int i,j;
list<list<pair<int,int> > >::const_iterator x;
list<pair<int,int> >::const_iterator y;

  constraint ct;
  ct.reserve(lp.size());

  // translate the given list of coordinates (v,l) to pointers to the actual label weights, to speed later access.
  for (i=0,x=lp.begin();  x!=lp.end();  i++,x++) {

    vector<double*> ld;
    ld.reserve(x->size());

    for (j=0,y=x->begin();  y!=x->end();  j++,y++) {
      TRACE(2,"added constraint for ("+util::int2string(v)+","+util::int2string(l)+") pointing to ("+util::int2string(y->first)+","+util::int2string(y->second)+")");

      ld.push_back(vars[y->first][y->second].weight);
    }

    ct.push_back(ld);
  }

  ct.set_compatibility(comp);
  vars[v][l].constraints.push_back(ct);
}


////////////////////////////////////////////////
/// Solve the consistent labelling problem
////////////////////////////////////////////////

void relax::solve() {
  vector<vector<label> >::iterator var;
  vector<label>::iterator lab;
  list<constraint>::const_iterator r;
  constraint::const_iterator p;
  vector<double*>::const_iterator wg;
  int n,j,v;
  double fnorm, inf, tw;
  double *support;
  
  // iterate until convercence (no changes)
  n=0; 
  while ((n==0 || there_are_changes()) && n<MaxIter) {
    TRACE(1,"relaxation iteration number "+util::int2string(n));

    // for each label of each variable
    for (v=0,var=vars.begin(); var!=vars.end(); var++,v++) {

      TRACE(1,"   Variable "+util::int2string(v));
      fnorm=0;
      if (var->size() > 1) { //  only proceed if the word is ambiguous.
	
	support = new double[var->size()];
	for (j=0,lab=var->begin(); lab!=var->end();  j++,lab++) {

	  double CurrW = lab->weight[CURRENT];
	  TRACE(2,"     Label "+util::int2string(j)+" weight="+util::double2string(CurrW));
	  if (CurrW>0) { // if weight==0 don't bother to compute supports, since the weight won't change
	    
	    support[j]=0.0;
	    // apply each constraint affecting the label
	    for (r=lab->constraints.begin(); r!=lab->constraints.end(); r++) {
	      
	      // each constraint is a list of terms to be multiplied
	      for (inf=1, p=r->begin(); p!=r->end(); p++) {
		
		// each term is a list (of lenght one except on negative or wildcarded conditions) 
		// of label weights to be added.
		for (tw=0, wg=p->begin(); wg!=p->end(); wg++)
		  tw += (*wg)[CURRENT];
		
		inf *= tw;
	      }
	      
	      // add constraint influence*compatibility to label support
	      support[j] += r->get_compatibility() * inf;
	      TRACE(3,"       constraint done (comp:"+util::double2string(r->get_compatibility())+"), inf="+util::double2string(inf));
	    }
	    
	    // normalize supports to a unified range
	    TRACE(3,"        total support="+util::double2string(support[j]));
	    support[j] = NormalizeSupport(support[j]);	    
	    TRACE(3,"        normalized support="+util::double2string(support[j]));
	    // compute normalization factor for updating function below
	    fnorm += CurrW * (1+support[j]);
	  }
	}
	
	// update label weigths
	for (j=0,lab=var->begin(); lab!=var->end();  j++,lab++) {
	  double CurrW = lab->weight[CURRENT];
	  lab->weight[NEXT] = (CurrW>0 ? CurrW*(1+support[j])/fnorm : 0);
	}
	
	delete support;
      }
    }
    
    n++; 
    CURRENT = NEXT;  
    NEXT = 1-NEXT;    // 'NEXT' becomes 'CURRENT'. A new 'NEXT' will be computed
  }
}


////////////////////////////////////////////////
/// Once the problem is solved, obtain the solution.
/// Get the best label(s) for variable v, and return 
/// them via list<int>&
////////////////////////////////////////////////

list<int> relax::best_label(int v) const {
  vector<label>::const_iterator lab;
  int j;
  double max;
  list<int> best;

  // build list of labels with highest weight
  max=0.0; 
  for (j=0,lab=vars[v].begin(); lab!=vars[v].end(); j++,lab++) {

    if (lab->weight[CURRENT] > max) {
      max=lab->weight[CURRENT];
      // if new maximum, restart list from scratch
      best.clear();
      best.push_back(j);
    }
    else if (lab->weight[CURRENT] == max) {
      // if equals current maximum, add to the list.
      best.push_back(j);
    }
  }

  return(best);
}




//--------------- private methods -------------

////////////////////////////////////////////////
/// Normalize support for a label in a fixed range
////////////////////////////////////////////////

double relax::NormalizeSupport(double x) const {

  if ((x>ScaleFactor)||(x<-ScaleFactor)) 
    {
      if (x>0) x = ScaleFactor; 
      else x = -ScaleFactor;
    }
  return (x/ScaleFactor);
}


////////////////////////////////////////////////
/// Check whether any weight changed more than Epsilon
////////////////////////////////////////////////

bool relax::there_are_changes() const {
  vector<vector<label> >::const_iterator i;
  vector<label>::const_iterator j;
  bool b;

   b=true;
   for (i=vars.begin(); b && i!=vars.end(); i++) 
     if (i->size() > 1) 
       for (j=i->begin(); b && j!=i->end(); j++)
	 b = fabs(j->weight[NEXT] - j->weight[CURRENT]) < Epsilon;

   return(!b);
}

