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

#include "freeling/suffixes.h"
#include "freeling/accents.h"
#include "freeling/dictionary.h"
#include "fries/util.h"
#include "freeling/traces.h"

using namespace std;

#define MOD_TRACENAME "AFFIXES"
#define MOD_TRACECODE AFF_TRACE


///////////////////////////////////////////////////////////////
///     Create a suffixed words analyzer. 
///////////////////////////////////////////////////////////////

affixes::affixes(const std::string &Lang, const std::string &sufFile): accen(Lang)
{
  string line,key,data;
  string cond,term,output,retok,lema;
  int acc,enc,nomore,always;

  // open suffix file
  ifstream fabr (sufFile.c_str());
  if (!fabr) { 
    ERROR_CRASH("Error opening file "+sufFile);
  }
  
  int kind= -1;
  Longest[SUF]=0;  Longest[PREF]=0;    
  // load suffix rules
  while (getline(fabr, line)) {
    // skip comentaries and empty lines       
    if(line[0]!='#' && !line.empty()) {

      if (line=="<Suffixes>") kind=SUF;
      else if (line=="</Suffixes>") kind= -1;
      else if (line=="<Prefixes>") kind=PREF;
      else if (line=="</Prefixes>") kind= -1;
      else if (kind==SUF or kind==PREF) {
	// read affix info from line
	istringstream sin;
	sin.str(line);
	sin>>key>>term>>cond>>output>>acc>>enc>>nomore>>lema>>always>>retok;

	TRACE(4,"Loading rule: "+key+" "+term+" "+cond+" "+output+" "
                +util::int2string(acc)+" "+util::int2string(enc)+" "
                +util::int2string(nomore)+" "+lema+" "+util::int2string(always)+" "+retok);
	// create rule.
	sufrule suf(cond);
	suf.term=term; suf.output=output; suf.acc=acc; suf.enc=enc; 
	suf.nomore=nomore; suf.lema=lema; suf.always=always; 
	suf.retok=retok;
	if (suf.retok=="-") suf.retok.clear();
	// Insert rule in appropriate affix multimaps.
	affix[kind].insert(make_pair(key,suf));
	if (suf.always) affix_always[kind].insert(make_pair(key,suf));	
	ExistingLength[kind].insert(key.size());
	if (key.size()>Longest[kind]) Longest[kind]=key.size();
      }
    }
  }
  
  fabr.close();

  TRACE(3,"analyzer succesfully created");
}


//////////////////////////////////////////////////////////////////////////////////////////
/// Look up possible roots of a suffixed form.
/// Words already analyzed are only applied the "always"-marked suffix rules.
/// So-far unrecognized words, are applied all the sufix rules.
//////////////////////////////////////////////////////////////////////////////////////////

void affixes::look_for_affixes(word &w, dictionary &dic) {

  if (w.get_n_analysis()>0) {
    // word with analysys already. Check only "always-checkable" affixes
    TRACE(2,"=== Known word '"+w.get_form()+"', with "+util::int2string(w.get_n_analysis())+" analysis. Looking only for 'always' affixes");
    TRACE(3," --- Cheking SUF ---");
    look_for_affixes_in_list(SUF,affix_always[SUF],w,dic);
    TRACE(3," --- Cheking PREF ---");
    look_for_affixes_in_list(PREF,affix_always[PREF],w,dic);
    TRACE(3," --- Cheking PREF+SUF ---");
    look_for_combined_affixes(affix_always[SUF],affix_always[PREF],w,dic);
  }
  else {
    // word not in dictionary. Check all affixes
    TRACE(2,"===Unknown word '"+w.get_form()+"'. Looking for any affix");
    TRACE(3," --- Cheking SUF ---");
    look_for_affixes_in_list(SUF,affix[SUF],w,dic);
    TRACE(3," --- Cheking PREF ---");
    look_for_affixes_in_list(PREF,affix[PREF],w,dic);
    TRACE(3," --- Cheking PREF+SUF ---");
    look_for_combined_affixes(affix[SUF],affix[PREF],w,dic);
  }
}


//////////////////////////////////////////////////////////////////////////////////////////
// Look for suffixes of the word w, using the given multimap.
// The word is annotated with new analysis, if any.
//////////////////////////////////////////////////////////////////////////////////////////

void affixes::look_for_affixes_in_list(int kind, std::multimap<std::string,sufrule> &suff, word &w, dictionary &dic) const
{
  typedef multimap<string,sufrule>::iterator T1;
  T1 sufit;
  pair<T1,T1> rules;
  set<string> candidates;
  string lws,form_term,form_root;
  unsigned int i,j, len;

  lws=util::lowercase(w.get_form());
  len=lws.length();
  for (i=1; i<=Longest[kind] && i<len; i++) {
    // advance backwards in form
    j=len-i;

    // check whether there are any affixes of that size
    if (ExistingLength[kind].find(i)==ExistingLength[kind].end()) {
      TRACE(4,"No affixes of size "+util::int2string(i));
    }
    else {
      // get the termination/beggining
      if (kind==SUF) form_term = lws.substr(j);
      else if (kind==PREF) form_term = lws.substr(0,i);

      // get all rules for that suffix
      rules=suff.equal_range(form_term);
      if (rules.first==suff.end() || rules.first->first!=form_term) {
	TRACE(3,"No rules for affix "+form_term+" (size "+util::int2string(i)+")");
      }
      else {
	TRACE(3,"Found "+util::int2string(suff.count(form_term))+" rules for affix "+form_term+" (size "+util::int2string(i)+")");
	for (sufit=rules.first; sufit!=rules.second; sufit++) {
	  // get the stem, after removing the affix
	  if (kind==SUF) form_root = lws.substr(0,j);
	  else if (kind==PREF) form_root = lws.substr(i);
	  TRACE(3,"Trying rule ["+form_term+" "+sufit->second.term+" "+sufit->second.cond.expression+" "+sufit->second.output+"] on root "+form_root);
	  
	  // complete all possible roots, using terminations/begginings provided in suffix rule
	  candidates = GenerateRoots(kind, sufit->second, form_root);
	  // fix accentuation patterns of obtained roots
	  accen.fix_accentuation(candidates, sufit->second);
	  // enrich word analysis list with dictionary entries for valid roots
	  SearchRootsList(candidates, form_term, sufit->second, w, dic);
	}
      }
    }
  }
}


//////////////////////////////////////////////////////////////////////////////////////////
// Check if the word w is decomposable with one prefix+one suffix
// The word is annotated with new analysis, if any.
//////////////////////////////////////////////////////////////////////////////////////////

void affixes::look_for_combined_affixes(std::multimap<std::string,sufrule> &suff, std::multimap<std::string,sufrule> &pref, word &w, dictionary &dic) const
{
  typedef multimap<string,sufrule>::iterator T1;
  T1 sufit,prefit;
  pair<T1,T1> rules_S,rules_P;
  set<string> candidates,cand1,cand2;
  string lws,form_suf,form_pref,form_root;
  unsigned int i,j,len;

  lws=w.get_form();
  len=lws.length();
  for (i=1; i<=Longest[SUF] && i<len; i++) {
    // check whether there are any suffixes of that size. 
    if (ExistingLength[SUF].find(i)==ExistingLength[SUF].end()) {
      TRACE(4,"No suffixes of size "+util::int2string(i));
      continue; // Skip to next size if none found
    }
 
    // check prefixes, only to len-i, since i+j>=len leaves no space for a root.
    for (j=1; j<=Longest[PREF] && j<len-i; j++) {  
      // check whether there are any suffixes of that size. Skip to next if none found
      if (ExistingLength[PREF].find(j)==ExistingLength[PREF].end()) {
	TRACE(4,"No prefixes of size "+util::int2string(j));
	continue; // Skip to next size if none found
      }
      
      // get sufix and prefix of current lengths
      form_suf = lws.substr(len-i);
      form_pref = lws.substr(0,j);
      // get all rules for those affixes
      rules_S = suff.equal_range(form_suf);
      if (rules_S.first==suff.end() || rules_S.first->first!=form_suf) {
	TRACE(3,"No rules for suffix "+form_suf+" (size "+util::int2string(i)+")");
	continue; // Skip to next size if no rules found
      }
      rules_P = pref.equal_range(form_pref);
      if (rules_P.first==pref.end() || rules_P.first->first!=form_pref) {
	TRACE(3,"No rules for prefix "+form_pref+" (size "+util::int2string(j)+")");
	continue; // Skip to next size if no rules found
      }

      // get the stem, after removing the suffix and prefix
      form_root = lws.substr(0,len-i).substr(j);
      TRACE(3,"Trying a decomposition: "+form_pref+"+"+form_root+"+"+form_suf);

      // if we reach here, there are rules for a sufix and a prefix of the word.      
      TRACE(3,"Found "+util::int2string(suff.count(form_suf))+" rules for suffix "+form_suf+" (size "+util::int2string(i)+")");
      TRACE(3,"Found "+util::int2string(pref.count(form_pref))+" rules for prefix "+form_pref+" (size "+util::int2string(j)+")");
      
      bool wfid=w.found_in_dict();

      for (sufit=rules_S.first; sufit!=rules_S.second; sufit++) {
	for (prefit=rules_P.first; prefit!=rules_P.second; prefit++) {
	  
	  candidates.clear();
	  // cand1: all possible completions with suffix rule
	  cand1 = GenerateRoots(SUF, sufit->second, form_root);
	  // fix accentuation patterns of obtained roots
	  accen.fix_accentuation(cand1, sufit->second);
          for (set<string>::iterator s=cand1.begin(); s!=cand1.end(); s++) {
	    // cand2: for each cand1, generate all possible completions with pref rule 
	    cand2 = GenerateRoots(PREF, prefit->second, (*s));
	    // fix accentuation patterns of obtained roots
	    accen.fix_accentuation(cand2, prefit->second);
	    // accumulate cand2 to candidate list.
	    candidates.insert(cand2.begin(),cand2.end());
	  }

	  // enrich word analysis list with dictionary entries for valid roots
	  word waux=w;
	  // apply prefix rules and generate analysis in waux.
	  SearchRootsList(candidates, form_pref, prefit->second, waux, dic);
	  // use analysis in waux as base to apply suffix rule
          for (set<string>::iterator s=candidates.begin(); s!=candidates.end(); s++) 
	    ApplyRule(form_pref+(*s), waux, form_suf, sufit->second, w, dic);

	  // unless both rules stated nomore, leave everything as it was
	  if (not (sufit->second.nomore and prefit->second.nomore))
	    w.set_found_in_dict(wfid);  
	}
      }
    }
  }
}


//////////////////////////////////////////////////////////////////////////////////////////
/// Generate all possible forms expanding root rt with all possible terminations
/// according to the given suffix rule.
//////////////////////////////////////////////////////////////////////////////////////////

set<string> affixes::GenerateRoots(int kind, const sufrule &suf, const std::string &rt) const
{
  set<string> cand;
  string term,r;
  size_t pe;

  cand.clear();
  term=suf.term;
  TRACE(3,"possible terminations/begginings: "+term);

  // fill the set of completed roots
  pe=term.find_first_of("|");
  while (pe!=string::npos) {
    r=term.substr(0,pe);
    if (r=="*")  r=""; // null termination

    if (kind==SUF) {
      TRACE(3,"Adding to t_roots the element: "+rt+r);
      cand.insert(rt+r);
    }
    else if (kind==PREF) {
      TRACE(3,"Adding to t_roots the element: "+r+rt);
      cand.insert(r+rt);
    }

    term=term.substr(pe+1);
    pe=term.find_first_of("|");
  }

  // adding the last termination/beggining in the list
  if (term=="*") term="";
  if (kind==SUF) {
    TRACE(3,"Adding to t_roots the element: "+rt+term);
    cand.insert(rt+term);
  }
  else if (kind==PREF) {
    TRACE(3,"Adding to t_roots the element: "+term+rt);
    cand.insert(term+rt);
  }

  return(cand);
}


//////////////////////////////////////////////////////////////////////////////////////////
/// Search candidate forms in dictionary, discarding invalid forms
/// and annotating the valid ones.
//////////////////////////////////////////////////////////////////////////////////////////

void affixes::SearchRootsList(set<string> &roots, const string &aff, sufrule &suf, word &wd, dictionary &dic) const
{
  set<string> remain;
  set<string>::iterator r;
  list<analysis> la;

  TRACE(3,"Checking a list of "+util::int2string(roots.size())+" roots.");
#ifdef VERBOSE
  for (r=roots.begin(); r!=roots.end(); r++) TRACE(3,"        "+(*r));
#endif

  remain=roots;

  while (not remain.empty()) {

    r=remain.begin();

    // look into the dictionary for that root
    la.clear();
    dic.search_form(*r,la);

    // if found, we must construct the analysis for the suffix
    if (la.empty()) {
      TRACE(3,"Root "+(*r)+" not found.");
      roots.erase(*r);  // useless root, remove from list.
    }
    else {
      TRACE(3,"Root "+(*r)+" found in dictionary.");      
      // apply the rule -if matching- and enrich the word analysis list.
      ApplyRule(*r,la,aff,suf,wd,dic);
    }

    // Root procesed. Remove from remaining candidates list.
    remain.erase(*r);
  }
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Actually apply a rule
//////////////////////////////////////////////////////////////////////////////////////////

void affixes::ApplyRule(const string &r, const list<analysis> &la, const string &aff, sufrule &suf, word &wd, dictionary &dic) const 
{
 list<analysis>::const_iterator pos;
 string tag,lem;

    for (pos=la.begin(); pos!=la.end(); pos++) {
      // test condition over parole
      if (not suf.cond.Search(pos->get_parole()) ) {
	TRACE(3," Parole "+pos->get_parole()+" fails input conditon "+suf.cond.expression);
      }
      else {
	TRACE(3," Parole "+pos->get_parole()+" satisfies input conditon "+suf.cond.expression);
	// We're applying the rule. If it says so, avoid assignation 
	// of more tags by later modules (e.g. probabilities).
	if (suf.nomore) wd.set_found_in_dict(true); 
	
	if (suf.output=="*") {
	  // we have to keep parole untouched
	  TRACE(3,"Output parole as found in dictionary");
	  tag = pos->get_parole();
	}
	else {
	  TRACE(3,"Output parole provided by suffix rule");
	  tag = suf.output;
	}

	// process suf.lema to build output lema
	list<string> suflem=util::string2list(suf.lema,"+");
	lem = "";
	for (list<string>::iterator s=suflem.begin(); s!=suflem.end(); s++) {
	  if ( *s=="F") {
	    TRACE(3,"Output lemma: add original word form");
	    lem = lem + util::lowecase(wd.get_form());
	  }
	  else if ( *s=="R") {
	    TRACE(3,"Output lemma: add root found in dictionary");
	    lem = lem + r;
	  }
	  else if ( *s=="L") {
	    TRACE(3,"Output lemma: add lemma found in dictionary");
	    lem = lem + pos->get_lemma();
	  }
	  else if ( *s=="A") {
	    TRACE(3,"Output lemma: add affix");
	    lem = lem + aff;
	  }
	  else {
	    TRACE(3,"Output lemma: add string '"+(*s)+"'");
	    lem = lem + (*s);
	  }
	}

	TRACE(3,"Analysis for the affixed form "+r+" ("+lem+","+tag+")");
	
	list<word> rtk;
	CheckRetokenizable(suf,r,lem,tag,dic,rtk);
	
	word::iterator p;
	// check whether the found analysis was already there
	for (p=wd.begin(); p!=wd.end() && !(p->get_lemma()==lem && p->get_parole()==tag); p++);
	
	// new analysis, analysis to the word, with retokenization info.
	if (p==wd.end()) {
	  analysis a(lem,tag);
	  a.set_retokenizable(rtk);
	  wd.add_analysis(a);
	}
	else { // if the analysis was already there, make sure it gets the retokenization
	  // info, unless it already has some.
	  TRACE(3,"Analysis was already there, adding RTK info");
	  if (!rtk.empty() && !p->is_retokenizable()) 
	    p->set_retokenizable(rtk);
	}
      }
    }
}
 
//////////////////////////////////////////////////////////////////////////////////////////
/// Check whether the suffix carries retokenization information, and create alternative
/// word list if necessary.
//////////////////////////////////////////////////////////////////////////////////////////

void affixes::CheckRetokenizable(const sufrule &suf, const std::string &form, const std::string &lem, const std::string &tag, dictionary &dic, std::list<word> &rtk) const
{
 string::size_type i;

  TRACE(3,"Check retokenizable.");
  if (!suf.retok.empty()) {

    TRACE(3," - sufrule has RTK: "+suf.retok);
    i=suf.retok.find_first_of(":");
    
    list<string> forms=util::string2list(suf.retok.substr(0,i),"+");
    list<string> tags=util::string2list(suf.retok.substr(i+1),"+");
    
    list<string>::iterator k,j;
    for (k=forms.begin(),j=tags.begin(); k!=forms.end(); k++,j++) {
      // create a word for each pair form/tag in retok 
      word w(*k);
      if ((*k)=="$$") {  // base form (referred to as "$$" in retok)
	w.set_form(form);
	analysis a(lem,tag);
	w.add_analysis(a);
      }
      else { // other retok forms
	list<analysis> la;
	list<analysis>::iterator a;
	// search each form in dict
	dic.search_form(*k,la);
	// use only analysis matching the appropriate tag in retok 
	for (a=la.begin(); a!=la.end(); a++) {
	  if (a->get_parole().find(*j)==0)
	    w.add_analysis(*a);
	}
      }
      rtk.push_back(w);
      TRACE(3,"    word "+w.get_form()+"("+w.get_lemma()+","+w.get_parole()+") added to decomposition list");
    }
  }
}
