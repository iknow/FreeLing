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

#include "freeling/traces.h"
#include "fries/util.h"
#include "freeling/relax_tagger.h"
#include "freeling/relax.h"

using namespace std;

#define MOD_TRACENAME "RELAX_TAGGER"
#define MOD_TRACECODE RELAX_TAGGER_TRACE


//---------- Class relax_tagger ----------------------------------

///////////////////////////////////////////////////////////////
///  Constructor: Build a relax PoS tagger
///////////////////////////////////////////////////////////////

relax_tagger::relax_tagger(const string &cg_file, int m, double f, double r, bool rtk, unsigned int force) : POS_tagger(rtk,force),solver(m,f,r),c_gram(cg_file), RE_user(USER_RE)  {}

////////////////////////////////////////////////
///  Perform PoS tagging on given sentences
////////////////////////////////////////////////

void relax_tagger::analyze(list<sentence> &ls) {
  list<sentence>::iterator s;
  sentence::iterator w;
  word::iterator tag, a;
  list<ruleCG> cand;
  list<ruleCG>::const_iterator cs;
  unsigned int lb,i;
  int v;
  
  // tag each sentence in list.
  for (s=ls.begin(); s!=ls.end(); s++) {
    
    TRACE(1, "Creating CLP");
    // Create a variable for each word, and a label foreach analysis.
    // Instantiate applicable constraints and add them to CLP

    // add an OUT_OF_BOUNDS mark at the begining of the sequence
    analysis an("OUT_OF_BOUNDS","OUT_OF_BOUNDS"); an.set_prob(1.0);
    word outofbounds("OUT_OF_BOUNDS"); outofbounds.add_analysis(an);
    s->insert(s->begin(),outofbounds);
    s->insert(s->end(),outofbounds);

    // build a RL problem for the current sentence (basically a table Vars x Labels)
    problem prb(s->size());
    for (v=0,w=s->begin();  w!=s->end();  v++,w++) {
      for (tag=w->selected_begin(); tag!=w->selected_end(); tag++) {
	prb.add_label(v,tag->get_prob());
      }
    }      

    // inform the solver about the problem sizes and initial weights
    solver.reset(prb);

    // precompute which contraints affect each analysis for each word, and add them to the CLP
    for (v=0,w=s->begin(); w!=s->end(); v++,w++) {

      TRACE(2, "   Adding constraints for word "+w->get_form());

      // constraints are needed only for ambiguous words
      if (w->get_n_selected()>1) {

	for (lb=0,tag=w->selected_begin(); tag!=w->selected_end(); lb++,tag++) {

	  TRACE(2, "     Adding label "+tag->get_parole());
	  
	  /// look for suitable candidate rules for this word-analysis
	  string t=tag->get_parole(); string lm="<"+tag->get_lemma()+">";
	  cand.clear();
          c_gram.get_rules_head(t, cand);    // constraints for the TAG of the analysis.
	  c_gram.get_rules_head(lm, cand);   // add constraints for the <lemma>
	  c_gram.get_rules_head(t+lm, cand); // add constraints for the TAG<lemma> 
	  c_gram.get_rules_head(t+"("+w->get_form()+")", cand); // add constraints for the TAG(form)

	  string sen="";
	  list<string> lsen=tag->get_senses();
	  if (c_gram.senses_used && lsen.size()>1) {
	    ERROR_CRASH("Conditions on 'sense' field used in constraint grammar, but 'DuplicateAnalysis' option was off during sense annotation. "); 
	  }
	  else if (lsen.size()>0) {
	    sen="[" + *(lsen.begin()) + "]";
	    c_gram.get_rules_head(sen, cand);    // constraints for [sense]
	    c_gram.get_rules_head(t+sen, cand);  // constraints for TAG[sense]
	  }

          // add constraints for user[i] fields:  u.0=stuff, u.1=foo, etc
          vector<string>::const_iterator uit; int u;
          for (u=0,uit=tag->user.begin(); uit!=tag->user.end(); u++,uit++) {
	    string us= "u."+util::int2string(u)+"="+(*uit);
  	    c_gram.get_rules_head(us, cand);
	  }
	  
	  // for all possible prefixes of the tag, look for constraints 
          // for TAGPREF*, TAGPREF*<lemma>, and TAGPREF*(form)
	  for (i=1; i<t.size(); i++) {
	    string pref=t.substr(0,i)+"*";
	    c_gram.get_rules_head(pref, cand);                         // TAGPREF*
	    c_gram.get_rules_head(pref+lm, cand);                      // TAGPREF*<lemma>
	    c_gram.get_rules_head(pref+"("+w->get_form()+")", cand);   // TAGPREF*(form)
	    if (lsen.size()>0) c_gram.get_rules_head(t+sen, cand);     // TAGPREF*[sense]
	  }
	  
	  TRACE(2, "    Found "+util::int2string(cand.size()) +" candidate rules.");
	
	  // see whether each candidate constraint applies to the current context.
	  for (cs=cand.begin(); cs!=cand.end(); cs++) {

	    // The constraint applies if all its conditions match
	    TRACE(3, "---- Checking candidate for "+cs->get_head()+" -----------");
	    ruleCG::const_iterator x;
	    list<list<pair<int,int> > > cnstr;
	    bool applies=true;
	    for (x=cs->begin(); applies && x!=cs->end(); x++) 
	      applies = CheckCondition(*s, w, v, *x, cnstr);

	    // if the constraint applies, create a solver constraint to add to the label
	    TRACE(3, (applies? "Conditions match." : "Conditions do not match."));
	    if (applies) solver.add_constraint(v, lb, cnstr, cs->get_weight());
	  }
	}
      }
    }

    TRACE(1, "Solving CLP");
    // Solve CLP
    solver.solve();

    TRACE(1, "Interpreting CLP solution");
    // Fetch results and apply them to our sentence

    s->erase(s->begin());  // remove the OUT_OF_BOUNDS marks.
    w=s->end(); w--; s->erase(w);

    v=1;   // skip first mark in the table
    for (w=s->begin(); w!=s->end(); w++) {
      if (w->get_n_selected()>1) {
	// erase any previous selection 
	w->unselect_all_analysis();

	// get best labels (may be more than one) for the ambiguous word. 
        list<int> bests=solver.best_label(v);
        list<word::iterator> bi;
	for(list<int>::iterator b=bests.begin(); b!=bests.end(); b++) {
	  // locate analysis corresponding to that label
          int j;
	  for (j=0,a=w->unselected_begin(); j!=(*b); j++,a++);
          bi.push_back(a);
	}

	// mark them as selected analysis for that word
	for (list<word::iterator>::iterator k=bi.begin(); k!=bi.end(); k++)
	  w->select_analysis(*k);

      }

      v++;  // next variable
    }    
  }

  // once everything is tagged, force select if required
  force_select(ls);
  // ... and perform retokenization if needed.
  retokenize(ls);
}

///////////////////////////////////////////////////////////////  
///  Disambiguate given sentences, return analyzed copy
///////////////////////////////////////////////////////////////  

std::list<sentence> relax_tagger::analyze(const std::list<sentence> &ls) {
  list<sentence> al;

  al=ls;
  analyze(al);
  return(al);
}


//--- private methods --

////////////////////////////////////////////////////////////////
/// auxiliary macro
////////////////////////////////////////////////////////////////

#define AdvanceNext(x,w,v)  {if (x.get_pos()>0) {w++; v++;} else if (x.get_pos()<0) {w--; v--;}}


////////////////////////////////////////////////////////////////
/// Find the match of a condition against the context
/// and add to the given constraint the list of terms to evaluate.
////////////////////////////////////////////////////////////////

bool relax_tagger::CheckCondition(const sentence &s, sentence::const_iterator w, int v, 
                                  const condition &x, list<list<pair<int,int> > > &res) {
  bool b;
  int nv, bv;
  sentence::const_iterator nw,bw;
  list<pair<int,int> > lsum;

  // position before first in the sentence is controlled with nv.
  // it would be nicer to use s.rend(), but that's not so easy with a single iterator

  // locate the position where to start matching
  TRACE(3, "    Locating position "+util::int2string(x.get_pos())+" relative to "+util::int2string(v));
  nw=w; nv=v; 
  while (nv-v!=x.get_pos() && nw!=s.end() && nv>=0) {
    AdvanceNext(x,nw,nv);    
  }
  TRACE(3, "    Stopped at relative: "+util::int2string(nv-v)+" absolute: "+util::int2string(nv));

  b=false;
  if (nw!=s.end() && nv>=0) {
    // it is a valid sentece position, let's check the word.
    do { 
      b = CheckWordMatchCondition(x.get_terms(), x.is_neg(), nv, nw, lsum);

      if (!b && x.has_star()) AdvanceNext(x,nw,nv);     // if it didn't match but the position 
    }                                                   // had a star, try the next word until
    while (!b && x.has_star() && nw!=s.end() && nv>=0); // one matches or the sentence ends.
  }
  else if (x.is_neg()) {
    // the position is outside the sentence, but the condition had a NOT, thus, it is a match.
    TRACE(3,"    Out of of bounds, but negative condition: match");
    b=true;
    lsum.push_back(make_pair(0,0));  // word 0, tag 0 is the OUT_OF_BOUNDS mark
  }

  // add result for the matching word to the constraint
  if (b) res.push_back(lsum);

  // if there was a barrier, compute its weight, and add them to the result
  if (b && x.has_barrier()) {

    TRACE(3,"  Checking BARRIER");
    list<list<pair<int,int> > > rbar;

    bool mayapply=true;
    bw=w; bv=v;  AdvanceNext(x,bw,bv);
    while (bw!=nw && mayapply) {
      if (CheckWordMatchCondition(x.get_barrier(), true, bv, bw, lsum)) 
	rbar.push_back(lsum);
      else 
        mayapply = false;  // found a word such that *all* analysis violate the barrier

      AdvanceNext(x,bw,bv);
    }

    if (!mayapply) {  
      // if there was any word such that *all* its analysis 
      // violated the barrier, the rule does not apply
      b=false;
      TRACE(3,"  Barrier violated, rule not applying");
    }
    else if (!rbar.empty()) {
      // if there were words in between, add the weights 
      // of their analysis that do not violate the barrier
      for (list<list<pair<int,int> > >::iterator k=rbar.begin(); k!=rbar.end(); k++) 
        res.push_back(*k);
      TRACE(3,"  Barrier only partially violated, rule may apply");
    }
    else {
      // else there were no words in between, the rule applies normally.
      TRACE(3,"  No words in between, barrier not violated, rule may apply");
    }
  }
  
  return b;
}



////////////////////////////////////////////////////////////////
/// check whether a word matches a simple condition
////////////////////////////////////////////////////////////////

bool relax_tagger::CheckWordMatchCondition(const list<string> &terms, bool is_neg, int nv,
					   sentence::const_iterator nw,
                                           list<pair<int,int> > &lsum) {
 bool amatch,b;
 int lb;
 list<string>::const_iterator t;
 word::const_iterator a;

 lsum.clear();
 b=false;
 for (a=nw->analysis_begin(),lb=0;  a!=nw->analysis_end();  a++,lb++) { // check each analysis of the word
  
   TRACE(3,"    Checking condition for ("+nw->get_form()+","+a->get_parole()+")"); 
   amatch=false;
   for (t=terms.begin(); !amatch && t!=terms.end(); t++) { // check each term in the condition
     TRACE(3,"    -- Checking term "+(*t)); 
     amatch = check_possible_matching(*t, a, nw);
   }
   
   // If the analysis matches (or if it doesn't and the condition was negated),
   // put the pair var-label in the result list.
   if (amatch && !is_neg || !amatch && is_neg) {
     lsum.push_back(make_pair(nv,lb));
     b = true;
     TRACE(3,"  word matches condition");
   }

   // Note: Although one matching analysis is enough to satisfy the condition, 
   // we go through all of them, because we want to add the weights of all matching analysis.
 }

 return(b);
}


////////////////////////////////////////////////////////////////
/// check whether a word matches one of all possible condition patterns
////////////////////////////////////////////////////////////////

bool relax_tagger::check_possible_matching(const string &s, word::const_iterator a,
                                           sentence::const_iterator w) {
  bool b;
  list<string> lsen=a->get_senses();

  // If the condition has a "<>" pair, check lemma stuff
  if (s.find_first_of("<")!=string::npos && s[s.size()-1]=='>')
    b= check_match(s, "<"+a->get_lemma()+">") || 
       check_match(s, a->get_parole()+"<"+a->get_lemma()+">");

  // If the condition has a "()" pair, check form stuff
  else if (s.find_first_of("(")!=string::npos  && s[s.size()-1]==')') {
    string form=util::lowercase(w->get_form());
    b= check_match(s, "("+form+")") || 
       check_match(s, a->get_parole()+"("+form+")"); 
  }

  // If the condition has a "[]" pair, check sense stuff
  else if (s.find_first_of("[")!=string::npos  && s[s.size()-1]==']') {
    if (lsen.size()>1) {
      ERROR_CRASH("Conditions on 'sense' field used in constraint grammar, but 'DuplicateAnalysis' option was off during sense annotation. "); 
    }
    else if (lsen.size()>0) {
      string sens= "[" + *(lsen.begin()) + "]";
      b= check_match(s,sens) || check_match(s, a->get_parole() + sens); 
    }
    else 
      b=false;
  }

  // If the condition has a "{}" pair, look in the sets map
  else if (s.find_first_of("{")!=string::npos  && s[s.size()-1]=='}') {
    map<string,setCG>::iterator p = c_gram.sets.find(s.substr(1,s.size()-2));
    string search;
    switch (p->second.type) {
      case FORM:  search= "("+util::lowercase(w->get_form())+")"; 
	          break;
      case LEMMA: search= "<"+util::lowercase(a->get_lemma())+">"; 
	          break;
      case SENSE: if (lsen.size()>1) {
	            ERROR_CRASH("Conditions on 'sense' field used in constraint grammar, but 'DuplicateAnalysis' option was off during sense annotation. "); 
                  }
                  else 
	            search= "["+ (lsen.size()>0 ? *(lsen.begin()) : "") +"]"; 
 	          break;
      case CATEGORY: search= a->get_parole(); 
                     break;
    }
    b= (p->second.find(search) != p->second.end());
  }

  // If the condition has pattern "u.9=XXXXXX", check user stuff
  else if (RE_user.Search(s)) {
    // find position indexed in the rule
    string ps=RE_user.Match(1);  string::size_type p= util::string2int(ps); 
    // check condition if position exists
    b = (a->user.size()>p) && check_match(s, "u."+ps+"="+a->user[p]);  
  }

  // otherwise, check tag
  else 
    b = check_match(s, a->get_parole());

  return (b);
}


////////////////////////////////////////////////////////////////
/// check match between a (possibly) wildcarded string and a literal.
/// ------------------------------------------------------------
/// This method is identical to chart::check_match. 
/// Maybe some kind of generalization into a new class would be a good idea ?
/// Maybe it should be moved to util.cc or like ?
////////////////////////////////////////////////////////////////

bool relax_tagger::check_match(const string &searched, const string &found) const {
  string s,m;
  string::size_type n;

  TRACE(3,"       matching "+searched+" and "+found);

  if (searched==found) return true;

  // not equal, check for a wildcard
  n = searched.find_first_of("*");
  if (n == string::npos)  return false;  // no wildcard, forget it.

  // check for wildcard match 
  if ( found.find(searched.substr(0,n)) != 0 ) return false;  //no match, forget it.

  // the start of the wildcard expression matches found string (e.g. VMI* matches VMI3SP0) 
  // Now, make sure the whole conditions hold (may be VMI*<lemma> )

  // check for lemma or form conditions: Actual searched is the expanded 
  // wildcard plus the original lemma/form condition (if any)
  n=found.find_first_of("(<[");
  if (n==string::npos) s=found; else s=found.substr(0,n);  
  n=searched.find_first_of("(<[");
  if (n==string::npos) m=""; else m=searched.substr(n);

  return (s+m == found);
}





