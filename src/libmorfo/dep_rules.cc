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
#include "freeling/dep_rules.h"

using namespace std;

#define MOD_TRACENAME "DEPENDENCIES"
#define MOD_TRACECODE DEP_TRACE


//---------- Class completerRule ----------------------------------

////////////////////////////////////////////////////////////////
///  Constructor
////////////////////////////////////////////////////////////////

completerRule::completerRule() : leftRE(""), rightRE("") {
  context="$$";
  weight=0;
}

////////////////////////////////////////////////////////////////
///  Constructor
////////////////////////////////////////////////////////////////

completerRule::completerRule(const string &pnewNode1, const string &pnewNode2, const string &poperation) : leftRE(""), rightRE("") {
  operation=poperation;
  newNode1=pnewNode1;
  newNode2=pnewNode2;
  line=0;
  context="$$";
  context_neg=false;
  weight=0;
}


////////////////////////////////////////////////////////////////
///  Constructor
////////////////////////////////////////////////////////////////

completerRule::completerRule(const completerRule & cr) : leftRE(""), rightRE("") {
  leftChk=cr.leftChk;     rightChk=cr.rightChk;
  leftConds=cr.leftConds; rightConds=cr.rightConds;
  leftRE=cr.leftRE;       rightRE=cr.rightRE;
  newNode1=cr.newNode1;   newNode2=cr.newNode2;
  operation=cr.operation;
  weight=cr.weight;
  last=cr.last;
  context=cr.context;
  context_neg=cr.context_neg;
  line=cr.line;
  enabling_flags = cr.enabling_flags;
  flags_toggle_on = cr.flags_toggle_on;
  flags_toggle_off = cr.flags_toggle_off;
}

////////////////////////////////////////////////////////////////
///  Assignment
////////////////////////////////////////////////////////////////

completerRule & completerRule::operator=( const completerRule & cr) {
  leftChk=cr.leftChk;     rightChk=cr.rightChk;
  leftConds=cr.leftConds; rightConds=cr.rightConds;
  leftRE=cr.leftRE;       rightRE=cr.rightRE;
  newNode1=cr.newNode1;   newNode2=cr.newNode2;
  operation=cr.operation;
  weight=cr.weight;
  last=cr.last;
  context=cr.context;
  context_neg=cr.context_neg;
  line=cr.line;
  enabling_flags = cr.enabling_flags;
  flags_toggle_on = cr.flags_toggle_on;
  flags_toggle_off = cr.flags_toggle_off;
  
  return *this;
}

////////////////////////////////////////////////////////////////
///  Comparison. The smaller weight, the higher priority 
////////////////////////////////////////////////////////////////

int completerRule::operator<(const  completerRule & a ) const  { 
  return ((weight<a.weight) && (weight>0));
}


//---------- Class rule_expression (and derived) -------------------

///////////////////////////////////////////////////////////////
/// Constructor
///////////////////////////////////////////////////////////////

rule_expression::rule_expression() {};

///////////////////////////////////////////////////////////////
/// Constructor
///////////////////////////////////////////////////////////////

rule_expression::rule_expression(const string &n,const string &v) {
  node=n;
  valueList=util::string2set(v,"|");
};


///////////////////////////////////////////////////////////////
/// Search for a value in the list of an expression
///////////////////////////////////////////////////////////////

bool rule_expression::find(const string &v) const {
  return (valueList.find(v) != valueList.end());
}

///////////////////////////////////////////////////////////////
/// Match the value against a RegExp
///////////////////////////////////////////////////////////////

bool rule_expression::match(const string &v) const {
  RegEx re(util::set2string(valueList,"|"));
  return (re.Search(v));
}


///////////////////////////////////////////////////////////////
/// Search for any value of a list in the list of an expression
///////////////////////////////////////////////////////////////

bool rule_expression::find_any(const list<string> &ls) const {
  bool found=false;
  for (list<string>::const_iterator s=ls.begin(); !found && s!=ls.end(); s++)
    found=find(*s);
  return(found);
}


///////////////////////////////////////////////////////////////
/// Search for a value in the list of an expression, 
/// taking into account wildcards
///////////////////////////////////////////////////////////////

bool rule_expression::find_match(const string &v) const {
  for (set<string>::const_iterator i=valueList.begin(); i!=valueList.end(); i++) {
    TRACE(4,"      eval "+node+".label="+(*i)+" (it is "+v+")");
    // check for plain match
    if (v==(*i)) return true;  
    // not straight, check for a wildcard
    string::size_type p=i->find_first_of("*");
    if (p!=string::npos && i->substr(0,p)==v.substr(0,p)) 
      // there is a wildcard and matches
      return true;
  }
  return false;
}


///////////////////////////////////////////////////////////////
/// Search for any value of a list in the list of an expression,
/// taking into account wildcards
///////////////////////////////////////////////////////////////

bool rule_expression::find_any_match(const list<string> &ls) const {
  bool found=false;
  for (list<string>::const_iterator s=ls.begin(); !found && s!=ls.end(); s++)
    found=find_match(*s);
  return(found);
}


///////////////////////////////////////////////////////////////
/// Recursive disassembly of node reference string (e.g. p-sn-sajd)
/// to get the right iterator.  
///////////////////////////////////////////////////////////////

dep_tree::iterator rule_expression::parse_node_ref(string nd, dep_tree::iterator k) const {

  string top;
  if (nd.size()==0)
    return k;
  else {
    TRACE(4,"       recursing at "+nd+", have parent "+k->info.get_link()->info.get_label());
    string::size_type t=nd.find(':');
    if (t==string::npos) {
      top=nd;
      nd="";
    }
    else {
      top=nd.substr(0,t);
      nd=nd.substr(t+1);
    }
    
    TRACE(4,"        need child "+nd);
    dep_tree::sibling_iterator j;
    for (j=k->sibling_begin(); j!=k->sibling_end(); ++j) {
       TRACE(4,"           looking for "+top+", found child with: "+j->info.get_link()->info.get_label());
       if (j->info.get_link()->info.get_label() == top)
	 return parse_node_ref(nd,j);
    }  
  }
  
  return NULL;
}


///////////////////////////////////////////////////////////////
/// Givent parent and daughter iterators, resolve which of them 
/// is to be checked in this condition.
///////////////////////////////////////////////////////////////

bool rule_expression::nodes_to_check(dep_tree::iterator p, dep_tree::iterator d, list<dep_tree::iterator> &res) const{

  string top, nd;
  string::size_type t=node.find(':');
  if (t==string::npos) {
    top=node;
    nd="";
  }
  else {
    top=node.substr(0,t);
    nd=node.substr(t+1);
  }

  if (top=="p") {
    dep_tree::iterator x=parse_node_ref(nd,p);
    if (x!=NULL) res.push_back(x);   
  }
  else if (top=="d") {
    dep_tree::iterator x=parse_node_ref(nd,d);
    if (x!=NULL) res.push_back(x);
  }
  else if (top=="As" or top=="Es") {
    // add to the list all children (except d) of the same parent (p)
    for (dep_tree::sibling_iterator s=p->sibling_begin(); s!=p->sibling_end(); ++s) {
      if (s!=d) {
	dep_tree::iterator x=parse_node_ref(nd,s);
	if (x!=NULL) res.push_back(x);
      }
    }
  }

  return (top=="As"); // return true for AND, false for OR.
}



///////////////////////////////////////////////////////////////
/// Check wheter a rule_expression can be applied to the
/// given pair of nodes
///////////////////////////////////////////////////////////////

bool rule_expression::check(dep_tree::iterator ancestor, dep_tree::iterator descendant) const {

  list<dep_tree::iterator> ln;

  // if "which_ao"=true then "eval" of all nodes in "ln" must be joined with AND
  // if it is false, and OR must be used 
  bool which_ao = nodes_to_check(ancestor, descendant, ln);  
  if (ln.empty()) return false;

  TRACE(4,"      found nodes to check.");

  // start with "true" for AND and "false" for OR
  bool res= which_ao; 
  // the loop goes on when res==true for AND and when res==false for OR
  for (list<dep_tree::iterator>::iterator n=ln.begin(); n!=ln.end() and (res==which_ao); n++) {
    TRACE(4,"      checking node.");
    res=eval(*n);
  }

  return res;
}

///////////////////////////////////////////////////////////////
/// eval whether a single node matches a condition
/// only called from check if needed. The abstract class
/// version should never be reached.
///////////////////////////////////////////////////////////////

bool rule_expression::eval(dep_tree::iterator n) const {
  return false;
}


//---------- Classes derived from rule_expresion ----------------------------------

/// check_and

void check_and::add(rule_expression * re) {check_list.push_back(re);}
bool check_and::check(dep_tree::iterator ancestor, dep_tree::iterator descendant) const {
  TRACE(4,"      eval AND");
  bool result=true;
  list<rule_expression *>::const_iterator ci=check_list.begin();
  while(result && ci!=check_list.end()) { 
    result=(*ci)->check(ancestor,descendant); 
    ++ci;
  }
  return result;
}

/// check_not

check_not::check_not(rule_expression * re) {check_op=re;}
bool check_not::check(dep_tree::iterator  ancestor, dep_tree::iterator  descendant) const {
  TRACE(4,"      eval NOT");
  return (! check_op->check(ancestor,descendant));  
}

/// check_side

check_side::check_side(const string &n,const string &s) : rule_expression(n,s) {
  if (valueList.size()>1 || (s!="right" && s!="left")) 
    WARNING("Error reading dependency rules. Invalid condition "+node+".side="+s+". Must be one of 'left' or 'right'.");
};
bool check_side::check(dep_tree::iterator ancestor, dep_tree::iterator descendant) const {
  string side=*valueList.begin();
  TRACE(4,"      eval SIDE="+side);
  if ((side=="left" && node=="d") || (side=="right" && node=="p")) {
    return ((descendant->info.get_word().get_span_start())<(ancestor->info.get_word().get_span_start()));
  }
  else if ((side=="left" && node=="p") || (side=="right" && node=="d")) {
    return ((descendant->info.get_word().get_span_start())>(ancestor->info.get_word().get_span_start()));
  }
  else return false;
}

/// check_wordclass

check_wordclass::check_wordclass(const string &n,const string &c) : rule_expression(n,c) {}
bool check_wordclass::eval (dep_tree::iterator n) const {

  TRACE(4,"      Checking "+node+".class="+util::set2string(valueList,"|")+" ? lemma="+ n->info.get_word().get_lemma());

  bool found=false;
  for (set<string>::iterator wclass=valueList.begin(); !found && wclass!=valueList.end(); wclass++)
    found= (wordclasses.find((*wclass)+"#"+(n->info.get_word().get_lemma())) != wordclasses.end());
  return found;
}

/// check_lemma

check_lemma::check_lemma(const string &n,const string &l) : rule_expression(n,l) {}
bool check_lemma::eval(dep_tree::iterator n) const {
  TRACE(4,"      eval. "+node+".lemma "+n->info.get_word().get_lemma());
  return (find(n->info.get_word().get_lemma()));
}

/// check head PoS

check_pos::check_pos(const string &n,const string &l) : rule_expression(n,l) {}
bool check_pos::eval(dep_tree::iterator n) const {
  TRACE(4,"      eval. "+node+".pos "+n->info.get_word().get_parole());
  return (match(n->info.get_word().get_parole()));
}


/// check chunk label

check_category::check_category(const string &n,const string &p) : rule_expression(n,p) {}
bool check_category::eval(dep_tree::iterator n) const {
  TRACE(4,"      eval. "+node+".label "+n->info.get_link()->info.get_label());
  return (find_match(n->info.get_link()->info.get_label()));
}


/// check top-ontology

check_tonto::check_tonto(semanticDB &db, const string &n, const string &t) : rule_expression(n,t), semdb(db) {}
bool check_tonto::eval (dep_tree::iterator n) const {
  TRACE(4,"      eval "+node+".tonto "+n->info.get_link()->info.get_label());
  string lem=n->info.get_word().get_lemma();
  string pos=n->info.get_word().get_parole().substr(0,1);
  list<string> sens = semdb.get_word_senses(lem,pos);
  bool found=false;
  for (list<string>::iterator s=sens.begin(); !found && s!=sens.end(); s++) {
    found=find_any(semdb.get_sense_info(*s,pos).tonto);
  }
  return(found);
}

/// check semantic file

check_semfile::check_semfile(semanticDB &db, const string &n, const string &f) : rule_expression(n,f), semdb(db) {}
bool check_semfile::eval (dep_tree::iterator n) const {
  string lem=n->info.get_word().get_lemma();
  string pos=n->info.get_word().get_parole().substr(0,1);
  list<string> sens = semdb.get_word_senses(lem,pos);
  bool found=false;
  for (list<string>::iterator s=sens.begin(); !found && s!=sens.end(); s++) {
    found=find(semdb.get_sense_info(*s,pos).semfile);
  }
  return(found);
}

/// check synonyms

check_synon::check_synon(semanticDB &db, const string &n, const string &w) : rule_expression(n,w), semdb(db) {}
bool check_synon::eval (dep_tree::iterator n) const {
  string lem=n->info.get_word().get_lemma();
  string pos=n->info.get_word().get_parole().substr(0,1);
  list<string> sens = semdb.get_word_senses(lem,pos);
  bool found=false;
  for (list<string>::iterator s=sens.begin(); !found && s!=sens.end(); s++) {
    found=find_any(semdb.get_sense_info(*s,pos).words);
  }
  return(found);
}

/// check ancestor synonyms

check_asynon::check_asynon(semanticDB &db, const string &n, const string &w) : rule_expression(n,w), semdb(db) {}
bool check_asynon::eval (dep_tree::iterator n) const {
  string lem=n->info.get_word().get_lemma();
  string pos=n->info.get_word().get_parole().substr(0,1);

  // start checking for a synonym in the senses of the word
  list<string> sens = semdb.get_word_senses(lem,pos);
  bool found=false;
  for (list<string>::iterator s=sens.begin(); !found && s!=sens.end(); s++) {
    found=find_any(semdb.get_sense_info(*s,pos).words);
    // if not found, enlarge the list of senses to explore
    // with all parents of the current sense
    if (!found) {
      list<string> lpar=semdb.get_sense_info(*s,pos).parents;
      if (lpar.size()>0) sens.splice(sens.end(),lpar);
    }
  }
  return(found);
}


//---------- Class ruleLabeler -------------------

////////////////////////////////////////////////////////////////
///  Constructor
////////////////////////////////////////////////////////////////

ruleLabeler::ruleLabeler(void) {};

////////////////////////////////////////////////////////////////
///  Constructor
////////////////////////////////////////////////////////////////

ruleLabeler::ruleLabeler(const string &plabel, rule_expression* pre) {
  re=pre;
  label=plabel; 
}

////////////////////////////////////////////////////////////////
///  Evaluate rule conditions
////////////////////////////////////////////////////////////////

bool ruleLabeler::check(dep_tree::iterator ancestor, dep_tree::iterator descendant) const {
  TRACE(4,"      eval ruleLabeler");
  return re->check(ancestor, descendant);
}

