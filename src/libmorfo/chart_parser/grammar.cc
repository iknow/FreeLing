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

using namespace std;
#include <fstream>

#undef yyFlexLexer
#define yyFlexLexer Gram_FlexLexer
#include "freeling/FlexLexer.h"
#include "freeling/grammar.h"
#include "freeling/traces.h"

using namespace std;

#define MOD_TRACENAME "GRAMMAR"
#define MOD_TRACECODE GRAMMAR_TRACE


//-------- Class rule implementation -----------//
 
////////////////////////////////////////////////////////////////
/// Default constructor of rule.
////////////////////////////////////////////////////////////////

//rule::rule(const string &s, const list<string> &ls, const int posgov): head(s),right(ls) {gov=posgov;}

rule::rule(const string &s, const list<string> &ls, const int p): head(s),right(ls),gov(p) {}

rule::rule(const rule & r) { 
  gov=r.gov;
  head=r.head;
  right=r.right;
}

rule::rule() { gov=0;}

rule &  rule::operator=(const rule & r) {
  gov=r.gov;
  head=r.head;
  right=r.right;
  return *this;
}


////////////////////////////////////////////////////////////////
/// Set rule governor
////////////////////////////////////////////////////////////////

void rule::set_governor(const int p) { 
 gov=p;
}

////////////////////////////////////////////////////////////////
/// Get rule governor
////////////////////////////////////////////////////////////////

unsigned int rule::get_governor(void) const {
 return gov;
}

////////////////////////////////////////////////////////////////
/// Get head for  rule.
////////////////////////////////////////////////////////////////

string rule::get_head() const {
 return(head);
}

////////////////////////////////////////////////////////////////
/// get right part of the rule.
////////////////////////////////////////////////////////////////

list<string> rule::get_right() const {
  return(right);
}


//-------- Class grammar implementation -----------//

#define ParseError(f,l,x) {cerr<<"Grammar "<<f<<", line "<<l<<": "<<x<<endl;}

/// no governor mark
unsigned int grammar::NOGOV=99999;  
/// default governor is first element in rule
unsigned int grammar::DEFGOV=0;

////////////////////////////////////////////////////////////////
/// Create a grammar, loading a file.
////////////////////////////////////////////////////////////////

grammar::grammar(const string &fname) {
  const int MAX= 32;
  int tok,stat,newstat,i,j;
  int what=0;
  int trans[MAX][MAX];
  bool first=false, wildcard=false;
  string head, err, categ, name;
  list<string> ls;
  int prior_val;

  // We use a FSA to read grammar rules. Fill transition tables
  for (i=0; i<MAX; i++) for (j=0; j<MAX; j++) trans[i][j]=0;
  // State 1. Initial state. Any line may come
  trans[1][COMMENT]=1; trans[1][CATEGORY]=2; trans[1][PRIOR]=6;
  trans[1][START]=8;   trans[1][HIDDEN]=6;   trans[1][FLAT]=6;
  trans[1][NOTOP]=6;   trans[1][ONLYTOP]=6;
  // State 2. A rule has started
  trans[2][ARROW]=3; 
  // State 3. Right side of a rule
  trans[3][CATEGORY]=4; 
  trans[3][HEAD]=10;
  // State 4. Right side of a rule, category found
  trans[4][COMMA]=3;  trans[4][BAR]=3;  trans[4][DOT]=1; 
  trans[4][LEMMA]=5;  trans[4][FORM]=5; trans[4][FILENAME]=5;
  // State 5. Right side of a rule, specificied category found
  trans[5][COMMA]=3;  trans[5][BAR]=3;  trans[5][DOT]=1; 
  // State 6. List directive found (@PRIOR, @HIDDEN, etc)
  trans[6][CATEGORY]=7;
  // State 7. Processing list directive
  trans[7][CATEGORY]=7;  trans[7][DOT]=1;
  // State 8. @START directive found 
  trans[8][CATEGORY]=9;
  // State 9. Processing list directive
  trans[9][DOT]=1;
  // State 10. Processing governor tag
  trans[10][CATEGORY]=4; 

  ifstream gf(fname.c_str());
  Gram_FlexLexer fl(&gf);

  // Governor default 0 (for unary rules. All others must have an explicit governor)
  int gov=0; 
  bool havegov=false;

  // FS automata to check rule file syntax and load rules.
  stat=1; prior_val=1; err="";
  while( (tok=fl.yylex()) ) {

    newstat = trans[stat][tok];

    // do appropriate processing depending on the state
    switch(newstat){
      case 0:
	// error state
	if (tok==COMMENT) err="Unexpected comment. Missing dot ending previous rule/directive ?";
        if (err=="") err="Unexpected '"+string(fl.YYText())+"' found.";
	ParseError(fname,fl.lineno(),err);
	err="";
	  
	// skip until first end_of_rule/directive or EOF, and continue from there.
	while (tok && tok!=DOT) tok=fl.yylex();
	newstat=1;
        break;

      case 1:
        if (tok==DOT && (stat==4 || stat==5)) { 
	  // a rule has just finished. Store it.
	  ls.insert(ls.end(),categ);

          // If a governor has not been defined, use default.
          // If the rule was not unary, issue a parsing warning
          if (!havegov) {
	    gov=DEFGOV;
            if (ls.size()!=1)
	      ParseError(fname,fl.lineno(),"Non-unary rule with no governor. First component taken as governor.");
	  }

          // store rule
	  new_rule(head,ls,wildcard,gov);

          // prepare for next rule
          gov=NOGOV; havegov=false;
	}
        break;

      case 2:
	// start of a rule, since there is a rule to rewrite 
        // this symbol, assume it is  non-terminal. Remember the head
	head=string(fl.YYText());
	nonterminal.insert(head); 
	break;

      case 3:
	if (tok==ARROW) {
	  // the right-hand side of a rule is starting. Initialize
	  ls.clear();
	  first=true; wildcard=false;
	}
	else if (tok==COMMA) { 
	  // new category for the same rule, add the category to the rule right part list
	  ls.insert(ls.end(),categ);
	}
	else if (tok==BAR) { 
	  // rule finishes here
	  ls.insert(ls.end(),categ);

          // If a governor has not been defined, use default.
          // If the rule was not unary, issue a parsing warning
          if (!havegov) {
	    gov=DEFGOV;
            if (ls.size()!=1)
	      ParseError(fname,fl.lineno(),"Non-unary rule with no governor. First component taken as governor.");
	  }

          // store rule
	  new_rule(head,ls,wildcard, gov);

	  // go for a new rule, with the same parent.
          gov=NOGOV; havegov=false;

	  ls.clear();
	}
        break;

      case 4:
	// store right-hand part of the rule
        categ= string(fl.YYText());
        // if first token in right part, check for wildcards
	if (first && categ.find_first_of("*")!=string::npos) wildcard = true;
	// next will no longer be first
	first = false; 
        break;

      case 5:
        name=string(fl.YYText());
	// category is specified with lemma or form. Remember that.
        categ = categ+name;

        // if a file name is given, load it into filemap
	if (tok==FILENAME) {

	  string sname;
	  // remove brackets
	  sname=name.substr(1,name.size()-2); 
          // convert relative file name to absolute, usig directory of grammar file as base.
          sname = util::absolute(sname, fname.substr(0,fname.find_last_of("/")+1) );

	  ifstream fs(sname.c_str());
	  if (!fs) ERROR_CRASH("Error opening file "+sname);

	  string op, clo;
	  if (name[0]=='<') {op="<"; clo=">";}
          else if (name[0]=='(') {op="("; clo=")";}

	  string line;
	  while (getline(fs,line))
	    filemap.insert(make_pair(op+line+clo,name));

	  fs.close();
	}

        break;

      case 6:
	// list directive start. Remember which directive.
	what=tok;
        break;

      case 7:
	categ=string(fl.YYText());
	if (nonterminal.find(categ)!=nonterminal.end()) {
	  switch (what) {
	    case PRIOR:   prior.insert(make_pair(categ,prior_val++)); break;
	    case HIDDEN:  hidden.insert(categ); break;
	    case FLAT:    flat.insert(categ); break;
	    case NOTOP:   notop.insert(categ); break;
  	    case ONLYTOP: onlytop.insert(categ); break;
	  }
	}
	else  {
	  err="Terminal symbol '"+string(fl.YYText())+"' not allowed in directive.";
	  newstat=0;
	}
        break;

      case 8:
	if (start!="") {
	  err="@START specified more than once.";
	  newstat=0;
	}
        break;

      case 9:
	start=string(fl.YYText());
	if (nonterminal.find(start)==nonterminal.end()) nonterminal.insert(start);
        break;

     case 10:

       // Found governor mark, store current position
       gov=ls.size();
       havegov=true;
       break;

    } // End switch

    stat=newstat;
  }  

  if (start=="") ParseError(fname,fl.lineno(),"@START symbol not specified.");
  if (hidden.find(start)!=hidden.end()) ParseError(fname,fl.lineno(),"@START symbol cannot be @HIDDEN.");
  if (notop.find(start)!=notop.end()) ParseError(fname,fl.lineno(),"@START symbol cannot be @NOTOP.");

  set<string>::const_iterator x;
  for (x=onlytop.begin(); x!=onlytop.end(); x++) {
    if (hidden.find(*x)!=hidden.end()) {
      ParseError(fname,fl.lineno(),"@HIDDEN directive for '"+(*x)+"' overrides @ONLYTOP.");
    }
  }

  gf.close();

  TRACE(3," Grammar loaded.");
}


////////////////////////////////////////////////////////////////
/// Create and store a new rule, indexed by 1st category in its right part.
////////////////////////////////////////////////////////////////

void grammar::new_rule(const string &h, const list<string> &ls, bool w, const int ngov) {
  
  rule r(h,ls,ngov);    // create
  //  r.set_governor(ngov);
  this->insert(make_pair(*ls.begin(),r)); // store

  // if appropriate, insert rule in wildcarded rules list
  // indexed by 1st char in wildcarded category
  if (w) wild.insert(make_pair(ls.begin()->substr(0,1),r));
}


////////////////////////////////////////////////////////////////
/// Obtain specificity for a terminal symbol.
/// Lower value, higher specificity.
/// highest is "TAG(form)", then "TAG<lemma>", then "TAG"
////////////////////////////////////////////////////////////////

int grammar::get_specificity(const string &s) const {

  // Form: most specific (spec=0)
  if (s.find_first_of("(")!=string::npos &&  s.find_first_of(")")==s.size()-1) return (0);
  // Lemma: second most specific (spec=1)
  if (s.find_first_of("<")!=string::npos && s.find_first_of(">")==s.size()-1) return (1);
  // Other (single tags): less specific (spec=2)
  return (2);
}

////////////////////////////////////////////////////////////////
/// Obtain priority for a non-terminal symbol.
/// Lower value, higher priority.
////////////////////////////////////////////////////////////////

int grammar::get_priority(const string &sym) const {
map<string,int>::const_iterator x;

  x=prior.find(sym);
  if (x!=prior.end()) return(x->second);
  else return 9999;
}


////////////////////////////////////////////////////////////////
/// Check whether a symbol is terminal or not.
////////////////////////////////////////////////////////////////

bool grammar::is_terminal(const string &sym) const {
  return(nonterminal.find(sym)==nonterminal.end());
}

////////////////////////////////////////////////////////////////
/// Check whether a symbol must disappear of final tree
////////////////////////////////////////////////////////////////

bool grammar::is_hidden(const string &sym) const {
  return(hidden.find(sym)!=hidden.end());
}

////////////////////////////////////////////////////////////////
/// Check whether a symbol must be flattened when recursive
////////////////////////////////////////////////////////////////

bool grammar::is_flat(const string &sym) const {
  return(flat.find(sym)!=flat.end());
}

////////////////////////////////////////////////////////////////
/// Check whether a symbol can not be used as a tree root
////////////////////////////////////////////////////////////////

bool grammar::is_notop(const string &sym) const {
  return(notop.find(sym)!=notop.end());
}

////////////////////////////////////////////////////////////////
/// Check whether a symbol is hidden unless when at tree root
////////////////////////////////////////////////////////////////

bool grammar::is_onlytop(const string &sym) const {
  return(onlytop.find(sym)!=onlytop.end());
}



////////////////////////////////////////////////////////////////
/// Return the grammar start symbol.
////////////////////////////////////////////////////////////////

string grammar::get_start_symbol() const {
  return(start);
}


////////////////////////////////////////////////////////////////
/// Get all rules with a right part beggining with the given category.
////////////////////////////////////////////////////////////////

list<rule> grammar::get_rules_right(const string &cat) const {
  typedef multimap<string,rule>::const_iterator T1;
  pair<T1,T1> i;
  T1 j;
  list<rule> lr;

  // find appropriate rules
  i=this->equal_range(cat);
  // If any rules found, return them in a list
  if (i.first!=this->end() && i.first->first==cat) {
    for (j=i.first; j!=i.second; j++) {
       lr.push_back(j->second);
    }
  }
  return lr;
}


////////////////////////////////////////////////////////////////
/// Get all rules with a right part beggining with a wilcarded category.
////////////////////////////////////////////////////////////////

list<rule> grammar::get_rules_right_wildcard(const string &c) const {
  typedef multimap<string,rule>::const_iterator T1;
  pair<T1,T1> i;
  T1 j;
  list<rule> lr;

  // find appropriate rules
  i=wild.equal_range(c);
  // If any rules found, return them in a list
  if (i.first!=wild.end() && i.first->first==c) {
    for (j=i.first; j!=i.second; j++) {
       lr.push_back(j->second);
    }
  }
  return lr;
}

////////////////////////////////////////////////////////////////
/// search given string in filemap, and check whether it maps to the second
////////////////////////////////////////////////////////////////

bool grammar::in_filemap(const string &key, const string &val) const {
  typedef multimap<string,string>::const_iterator T1;
  pair<T1,T1> i;
  T1 j;

  // search in multimap
  i=filemap.equal_range(key);  

  if (i.first==filemap.end() || i.first->first!=key)
    return false;     // pair not found

  // key found, check pairs.
  bool b=false;
  for (j=i.first; j!=i.second && !b; j++) b=(j->second==val);  
  return b;
}
