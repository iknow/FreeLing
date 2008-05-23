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

#undef yyFlexLexer
#define yyFlexLexer CG_FlexLexer
#include "freeling/FlexLexer.h"

#include "freeling/constraint_grammar.h"
#include "freeling/traces.h"
#include "fries/util.h"

using namespace std;

#define MOD_TRACENAME "CONST_GRAMMAR"
#define MOD_TRACECODE CONST_GRAMMAR_TRACE


//-------- Class condition implementation -----------//
 
////////////////////////////////////////////////////////////////
/// Default constructor of condition
////////////////////////////////////////////////////////////////

condition::condition() {};

////////////////////////////////////////////////////////////////
/// clear condition contents
////////////////////////////////////////////////////////////////

void condition::clear() {
  neg = false;
  pos = 0;
  starpos = false;
  terms.clear();
  barrier.clear();
}

////////////////////////////////////////////////////////////////
/// Set negation value
////////////////////////////////////////////////////////////////

void condition::set_neg(bool n) {neg = n;}

////////////////////////////////////////////////////////////////
/// Set position value (and star condition)
////////////////////////////////////////////////////////////////

void condition::set_pos(int p, bool s) {pos = p; starpos = s;}

////////////////////////////////////////////////////////////////
/// Set list of condition terms
////////////////////////////////////////////////////////////////

void condition::set_terms(const list<string>& t) {terms = t;}

////////////////////////////////////////////////////////////////
/// Set list of barrier terms. Empty list means no barrier.
////////////////////////////////////////////////////////////////

void condition::set_barrier(const list<string>& b) {barrier = b;}

////////////////////////////////////////////////////////////////
/// Get negation value
////////////////////////////////////////////////////////////////

bool condition::is_neg() const {return(neg);}

////////////////////////////////////////////////////////////////
/// Get position value 
////////////////////////////////////////////////////////////////

int condition::get_pos() const {return(pos);}

////////////////////////////////////////////////////////////////
/// Get star value for position
////////////////////////////////////////////////////////////////

bool condition::has_star() const {return(starpos);}

////////////////////////////////////////////////////////////////
/// Get list of condition terms
////////////////////////////////////////////////////////////////

list<string> condition::get_terms() const {return(terms);}

////////////////////////////////////////////////////////////////
/// Find out whether there ara barrier conditions
////////////////////////////////////////////////////////////////

bool condition::has_barrier() const {return(!barrier.empty());}

////////////////////////////////////////////////////////////////
/// Get list of barrier terms. Empty list means no barrier.
////////////////////////////////////////////////////////////////

list<string> condition::get_barrier() const {return(barrier);}

//-------- Class ruleCG implementation -----------//
 
////////////////////////////////////////////////////////////////
/// Default constructor of ruleCG.
////////////////////////////////////////////////////////////////

ruleCG::ruleCG() {}

////////////////////////////////////////////////////////////////
/// Get head for  rule.
////////////////////////////////////////////////////////////////

string ruleCG::get_head() const {return(head);}

////////////////////////////////////////////////////////////////
/// get rule compatibility value
////////////////////////////////////////////////////////////////

double ruleCG::get_weight() const {return(weight);}

////////////////////////////////////////////////////////////////
/// set head for  rule.
////////////////////////////////////////////////////////////////

void ruleCG::set_head(string const &h) {head=h;}

////////////////////////////////////////////////////////////////
/// set rule compatibility value
////////////////////////////////////////////////////////////////

void ruleCG::set_weight(double w) {weight = w;}


//-------- Class constraint_grammar implementation -----------//

#define ERROR_ST   0
#define INIT       1
#define S_SETS     2
#define S_CONSTR   3
#define WAIT_IS    4
#define INILIST    5
#define F_LIST     6
#define L_LIST     7
#define C_LIST     8
#define S_LIST     9
#define INIRULE   10
#define PHEAD     11
#define FHEAD     12
#define INICOND   13
#define NOTCOND   14
#define TERMS     15
#define MORETERMS 16
#define TERMLIST  17
#define S_BARR    18
#define MOREBARR  19
#define ENDBARR   20
#define ENDRULE   21

////////////////////////////////////////////////////////////////
/// Create a constraint_grammar, loading a file.
////////////////////////////////////////////////////////////////

constraint_grammar::constraint_grammar(const string &fname) {
  const int MAX=50;
  int tok,stat,newstat,i,j;
  int section=0;
  int trans[MAX][MAX];
  bool star;
  string err;
  list<string> lt;
  list<string>::reverse_iterator x;
  map<string,setCG>::iterator p;
  ruleCG rul;
  condition cond;

  // We use a FSA to read grammar rules. Fill transition tables
  for (i=0; i<MAX; i++) for (j=0; j<MAX; j++) trans[i][j]=ERROR_ST;

  // State INIT. Initial state. SETS or CONSTRAINTS may come
  trans[INIT][COMMENT]=INIT;  trans[INIT][SETS]=S_SETS; trans[INIT][CONSTRAINTS]=S_CONSTR;

  // Sets section started, read SETS
  trans[S_SETS][COMMENT]=S_SETS; trans[S_SETS][CATEGORY]=WAIT_IS; trans[S_SETS][CONSTRAINTS]=S_CONSTR;
  // Set name read, expect "is"
  trans[WAIT_IS][IS]=INILIST;
  // start of list, see what it contains
  trans[INILIST][FORM]=F_LIST;  trans[INILIST][LEMMA]=L_LIST;  
  trans[INILIST][CATEGORY]=C_LIST; trans[INILIST][SENSE]=S_LIST; 
  // list of forms
  trans[F_LIST][FORM]=F_LIST;  trans[F_LIST][SEMICOLON]=S_SETS;
  // list of lemmas
  trans[L_LIST][LEMMA]=L_LIST;  trans[L_LIST][SEMICOLON]=S_SETS;
  // list of categories
  trans[C_LIST][CATEGORY]=C_LIST;  trans[C_LIST][SEMICOLON]=S_SETS;
  // list of senses
  trans[S_LIST][SENSE]=S_LIST;  trans[S_LIST][SEMICOLON]=S_SETS;

    // Constraints section started, expect first constraint
  trans[S_CONSTR][COMMENT]=S_CONSTR; trans[S_CONSTR][FLOAT]=INIRULE;
  // State INIRULE. A rule has started, the head is coming
  trans[INIRULE][CATEGORY]=PHEAD;   
  trans[INIRULE][LEMMA]=FHEAD;  trans[INIRULE][USER]=FHEAD; trans[INIRULE][SENSE]=FHEAD; 
  // State PHEAD. Partial head, it may be continuing
  trans[PHEAD][FORM]=PHEAD;  
  trans[PHEAD][LEMMA]=FHEAD;  trans[PHEAD][SENSE]=FHEAD;  trans[PHEAD][OPAR]=INICOND; 
  // State FHEAD. Full head readed, conditions coming.
  trans[FHEAD][OPAR]=INICOND; 
  // States INICOND, NOTCOND. start of a condition
  trans[INICOND][NOT]=NOTCOND; trans[INICOND][POSITION]=TERMS; 
  trans[NOTCOND][POSITION]=TERMS;  
  // State TERMS. OR-ed term list
  trans[TERMS][CATEGORY]=MORETERMS; trans[TERMS][FORM]=TERMLIST;  trans[TERMS][LEMMA]=TERMLIST; 
  trans[TERMS][SENSE]=TERMLIST;     trans[TERMS][USER]=TERMLIST;  trans[TERMS][OUT]=TERMLIST; 
  trans[TERMS][SETREF]=TERMLIST;
  // State MORETERMS. OR-ed term list
  trans[MORETERMS][FORM]=TERMLIST; trans[MORETERMS][LEMMA]=TERMLIST; trans[MORETERMS][SENSE]=TERMLIST;   
  trans[MORETERMS][OR]=TERMS;      trans[MORETERMS][BARRIER]=S_BARR; trans[MORETERMS][CPAR]=ENDRULE;
  // State TERMLIST. Processing term list
  trans[TERMLIST][OR]=TERMS;        trans[TERMLIST][BARRIER]=S_BARR;  trans[TERMLIST][CPAR]=ENDRULE;
  // State S_BARR. Barrier term list starting
  trans[S_BARR][CATEGORY]=MOREBARR; trans[S_BARR][FORM]=ENDBARR;    trans[S_BARR][LEMMA]=ENDBARR;
  trans[S_BARR][SENSE]=ENDBARR;     trans[S_BARR][USER]=ENDBARR;    trans[S_BARR][SETREF]=ENDBARR; 
  // State MOREBARR. Barrier term list continuing
  trans[MOREBARR][FORM]=ENDBARR;    trans[MOREBARR][LEMMA]=ENDBARR; trans[MOREBARR][SENSE]=ENDBARR;
  trans[MOREBARR][OR]=S_BARR;       trans[MOREBARR][CPAR]=ENDRULE;
  // State ENDBARR. Processing barrier term list
  trans[ENDBARR][OR]=S_BARR;  trans[ENDBARR][CPAR]=ENDRULE;
  // State ENDRULE. End of condition
  trans[ENDRULE][OPAR]=INICOND;  trans[ENDRULE][SEMICOLON]=S_CONSTR;

  ifstream cgf(fname.c_str());
  CG_FlexLexer fl(&cgf);

  senses_used=false;

  // FS automata to check rule file syntax and load rules.
  stat=INIT; err="";
  while( (tok=fl.yylex()) ) {

    newstat = trans[stat][tok];

    // do appropriate processing depending on the state
    switch(newstat){
      case ERROR_ST:
	// error state
	if (tok==COMMENT) err="Unexpected comment. Missing semicolon ending previous rule?";
        if (err=="") err="Unexpected '"+string(fl.YYText())+"' found.";
	cerr<<"Constraint Grammar '"<<fname<<"'. Line "<<fl.lineno()<<". Syntax error: "<<err<<endl;
	err="";
	  
	// skip until first end_of_rule or EOF, and continue from there.
	while (tok && tok!=SEMICOLON) tok=fl.yylex();
	newstat=section;
        break;

      case INIT:  // only to expect SETS or CONSTRAINTS
	break;

      case WAIT_IS: {
	// A new set is starting, create set associated to its name, keep iterator, wait for "="
        setCG myset;  
        pair<map<string,setCG>::iterator,bool> q=sets.insert(make_pair(string(fl.YYText()),myset));
        p=q.first;
	break;
      }

      case S_SETS:
	section=S_SETS; // where to sync in case of error
	break;

      case INILIST:  //  just read "=", wait to see what are the set members
	break;

      case F_LIST:   // store type of set elements (FORM, LEMMA, CATEGORY)
      case L_LIST:
      case C_LIST:
      case S_LIST:
	p->second.type = tok;
        p->second.insert(string(fl.YYText()));
	break;

      case S_CONSTR:
	section=S_CONSTR; // where to sync in case of error

        if (tok==SEMICOLON && stat==ENDRULE) { 
	  // store last condition and tidy up
	  rul.push_back(cond);
	  cond.clear(); lt.clear();
	  // a rule has just finished. Store it, indexed by first char in head if TAG, 
          // or by whole head if <lemma>.  (form) is not allowed in constraint head.
	  TRACE(3,"Inserted new rule:  ("+rul.get_head()+", "+util::int2string(rul.size())+" conditions)");
	  this->insert(make_pair(rul.get_head(),rul)); 
	  rul.clear();	  
	}
        break;

      case INIRULE:
	// start of a rule, store weight
	rul.set_weight(util::string2double(string(fl.YYText())));
	rul.set_head("");
	break;

      case PHEAD:
	// got head of a rule, store it
	rul.set_head(string(fl.YYText()));
	break;

      case FHEAD:
	// head is continuing, update.
	rul.set_head(rul.get_head()+string(fl.YYText()));
	if (tok==SENSE) senses_used=true;
	break;

      case INICOND:
	// If a condition has finished, store it
	if (stat==ENDRULE) rul.push_back(cond);
	// in any case, prepare to start a new condition
	cond.clear();
	lt.clear();
        break;

      case NOTCOND:
	// Negative condition
        cond.set_neg(true);
        break;

      case TERMS:	
	// condition is starting
	if (tok==POSITION && (stat==INICOND || stat==NOTCOND)) {
	  // Separate position value from star, if any.
	  string p = string(fl.YYText());
	  string::size_type s = p.find_first_of("*"); 
	  if (s!=string::npos) {
	    star=true;
	    p=p.substr(0,s);
	  }
	  else star=false;
	  s = util::string2int(p);
	  cond.set_pos(s,star);
	  // prepare to accumulate condition terms
	  lt.clear();
	}
        break;

      case MORETERMS:
	// new term, accumulate
	lt.push_back(string(fl.YYText()));
        break;

      case TERMLIST:
	if (tok==SENSE) senses_used=true;

	if (stat==TERMS) {
          string tkform(fl.YYText());
	  // new term
          if (tok==SETREF && sets.find(tkform.substr(1,tkform.size()-2))==sets.end()) {
	    err="Reference to undefined SET '"+tkform+"'.--"+tkform.substr(1,tkform.size()-2)+"--";
	    newstat=ERROR_ST;
	  }
	  else
	    lt.push_back(tkform);
	}
	else if (stat==MORETERMS)  { 
	  // term is continuing, update last added element
	  x = lt.rbegin(); 
	  (*x)=(*x)+string(fl.YYText());
	}
        break;

      case S_BARR:
	if (cond.has_star()) {
	  if (tok==BARRIER && (stat==MORETERMS || stat==TERMLIST)) {
	    // start of barrier, store accumulated terms
	    cond.set_terms(lt);
	    // start accumulating barrier terms.
	    lt.clear();
	  }
	}
	else  {
	  err="BARRIER is nonsense in a fixed position condition.";
	  newstat=ERROR_ST;
	}
	break;
      
      case MOREBARR:
        // new barrier term, accumulate
	lt.push_back(string(fl.YYText()));
        break;

      case ENDBARR:
	if (tok==SENSE) senses_used=true;

	if (stat==S_BARR) {
          string tkform(fl.YYText());
	  // new term
	  if (tok==SETREF && sets.find(tkform.substr(1,tkform.size()-2))==sets.end()) {
	    err="Reference to undefined SET '"+tkform+"'.";
	    newstat=ERROR_ST;
	  }
	  else
	    lt.push_back(tkform);
	}
	else if (stat==MOREBARR)  { 
	  // term is continuing, update last added element
	  x = lt.rbegin();
	  (*x)=(*x)+string(fl.YYText());
	}
        break;

      case ENDRULE:
	// end of condition
	if (stat==MORETERMS || stat==TERMLIST) // we come from condition states, no barrier	
	  cond.set_terms(lt);  

	else if (stat==S_BARR || stat==MOREBARR || stat==ENDBARR) // we come from barrier states  
          cond.set_barrier(lt);  
        break;

      default: // should never happen
	ERROR_CRASH("Bad internal state ("+util::int2string(newstat)+")");

    } // End switch

    stat=newstat;
  }  

  cgf.close();

  TRACE(3," Constraint Grammar loaded.");
}


////////////////////////////////////////////////////////////////
/// Add to the given list all rules with a head starting with the given string.
////////////////////////////////////////////////////////////////

void constraint_grammar::get_rules_head(const string &h, list<ruleCG> &lr) const {
  typedef multimap<string,ruleCG>::const_iterator T1;
  pair<T1,T1> i;
  T1 j;

  // find appropriate rules
  i=this->equal_range(h);
  // If any rules found, return them in a list
  if (i.first!=this->end() && i.first->first==h) {
    for (j=i.first; j!=i.second; j++) {
       lr.push_back(j->second);
    }
  }
}


