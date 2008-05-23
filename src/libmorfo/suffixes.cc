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

#include "freeling/suffixes.h"
#include "freeling/accents.h"
#include "freeling/dictionary.h"
#include "fries/util.h"
#include "freeling/traces.h"

using namespace std;

#define MOD_TRACENAME "SUFFIXES"
#define MOD_TRACECODE SUFF_TRACE


///////////////////////////////////////////////////////////////
///     Create a suffixed words analyzer. 
///////////////////////////////////////////////////////////////

suffixes::suffixes(const std::string &Lang, const std::string &sufFile): accen(Lang)
{
  string line,key,data;

  // open suffix file
  ifstream fabr (sufFile.c_str());
  if (!fabr) { 
    ERROR_CRASH("Error opening file "+sufFile);
  }
  
  // load suffix rules
  LongestSuf=0;    
  while (getline(fabr, line)) {
    // skip comentaries and empty lines       
    if(line[0]!='#' && !line.empty()) {
      
      // read suffix info from line
      string cond,term,output,retok;
      int last_acc,lem_form,enc,always;
      istringstream sin;
      sin.str(line);
      sin>>key>>term>>cond>>output>>last_acc>>enc>>lem_form>>always>>retok;

      sufrule suf(cond);
      suf.term=term; suf.output=output; suf.last_acc=last_acc;
      suf.enc=enc; suf.lem_form=lem_form; suf.always=always; 
      suf.retok=retok;
      if (suf.retok=="-") suf.retok.clear();
      
      // insert sufix data in multimap
      suffix.insert(make_pair(key,suf));
      if (suf.always) suffix_always.insert(make_pair(key,suf));
      
      ExistingLength.insert(key.size());
      if (key.size()>LongestSuf) LongestSuf=key.size();
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

void suffixes::look_for_suffixes(word &w, dictionary &dic) {

  if (w.get_n_analysis()>0) {
    // word in dictionary. Check only "always-checkable" suffixes
    TRACE(2,"Known word, with "+util::int2string(w.get_n_analysis())+" analysis. Looking only for 'always' suffixes");
    look_for_suffixes_in_list(suffix_always,w,dic);
  }
  else {
    // word not in dictionary. Check all suffixes
    TRACE(2,"Unknown word. Looking for any suffix");
    look_for_suffixes_in_list(suffix,w,dic);
  }
}


//////////////////////////////////////////////////////////////////////////////////////////
// Look for suffix the form lws, using the given multimap.
// The corresponding word pointed by the iterator it is annotated.
//////////////////////////////////////////////////////////////////////////////////////////

void suffixes::look_for_suffixes_in_list(std::multimap<std::string,sufrule> &suff, word &w, dictionary &dic) const
{
  typedef multimap<string,sufrule>::iterator T1;
  T1 sufit;
  pair<T1,T1> rules;
  vector<string> candidates;
  string lws,form_term,form_root;
  unsigned int i,j;

  lws=w.get_form();
  j=lws.length();
  for (i=1; i<=LongestSuf && i<lws.size(); i++) {
    // advance backwards in form
    j--;

    // check whether there are any suffixes of that size
    if (ExistingLength.find(i)==ExistingLength.end()) {
      TRACE(3,"No suffixes of size "+util::int2string(i));
    }
    else {
      // get the termination
      form_term = lws.substr(j);

      // get all rules for that suffix
      rules=suff.equal_range(form_term);
      if (rules.first==suff.end() || rules.first->first!=form_term) {
	TRACE(3,"No rules for suffix "+form_term+" (size "+util::int2string(i)+")");
      }
      else {
	TRACE(3,"Found "+util::int2string(suff.count(form_term))+" rules for suffix "+form_term+" (size "+util::int2string(i)+")");
	for (sufit=rules.first; sufit!=rules.second; sufit++) {
	  form_root = lws.substr(0,j);
	  TRACE(3,"Trying a new rule for that suffix. Building root with "+form_root);

	  // all possible roots, using terminations provided in suffix rule
	  candidates = GenerateRoots(sufit->second, form_root);
	  // fix accentuation patterns of obtained roots
	  accen.fix_accentuation(candidates, sufit->second);
	  // enrich word analysis list with dictionary entries for valid roots
	  SearchRootsList(candidates, sufit->second, w, dic);
	}
      }
    }
  }
}


//////////////////////////////////////////////////////////////////////////////////////////
/// Generate all possible forms expanding root rt with all possible terminations
/// according to the given suffix rule.
//////////////////////////////////////////////////////////////////////////////////////////

vector<string> suffixes::GenerateRoots(const sufrule &suf, const std::string &rt) const
{
  vector<string> cand;
  string term,r;
  size_t pe;

  cand.clear();
  term=suf.term;
  TRACE(3,"possible terminations: "+term);

  // fill the vector of roots
  pe=term.find_first_of("|");
  while (pe!=string::npos) {
    r=term.substr(0,pe);
    if (r=="*")  r=""; // null termination
    TRACE(3,"Adding to t_roots the element: "+rt+r);
    cand.push_back(rt+r);
    term=term.substr(pe+1);
    pe=term.find_first_of("|");
  }

  // adding the last termination in the list
  if (term=="*")  term=""; // null termination
  cand.push_back(rt+term);

  return(cand);
}


//////////////////////////////////////////////////////////////////////////////////////////
/// Search candidate forms in dictionary, discarding invalid forms
/// and annotating the valid ones.
//////////////////////////////////////////////////////////////////////////////////////////

void suffixes::SearchRootsList(const std::vector<std::string> &roots, sufrule &suf, word &wd, dictionary &dic) const
{
  vector<string>::const_iterator r;
  list<analysis> la;
  list<analysis>::iterator pos;
  string tag, lem;

  TRACE(3,"Checking a list of "+util::int2string(roots.size())+" roots.");
  for (r=roots.begin(); r!=roots.end(); r++){
    // look into the dictionary for that root
    la.clear();
    dic.search_form(*r,la);

    // if found, we must construct the analysis for the suffix
    if (la.empty()) {
      TRACE(3,"Root "+(*r)+" not found.");
    }
    else {
      TRACE(3,"Root "+(*r)+" found in dictionary.");

      for (pos=la.begin(); pos!=la.end(); pos++) {
	// test condition over parole
        if (not suf.cond.Search(pos->get_parole()) ) {
	  TRACE(3," Parole "+pos->get_parole()+" fails input conditon "+suf.cond.expression);
	}
	else {
	  TRACE(3," Parole "+pos->get_parole()+" satisfies input conditon "+suf.cond.expression);

	  if (suf.output=="*") {
	    // we have to keep parole untouched
	    TRACE(3,"Output parole as found in dictionary");
	    tag = pos->get_parole();
	  }
	  else {
	    TRACE(3,"Output parole provided by suffix rule");
	    tag = suf.output;
	  }

	  if (suf.lem_form==0) {
	    TRACE(3,"Output lemma is lemma found in dictionary");
	    lem = pos->get_lemma();
	  }
          else if (suf.lem_form==1) {
	    // lemma is the same as initial form
	    TRACE(3,"Output lemma is original word form");
	    lem = wd.get_form();
	  }
	  else if (suf.lem_form==2) {
	    TRACE(3,"Output lemma is root found in dictionary");
            lem = (*r);
	  }

	  TRACE(3,"Analysis for the suffixed form ("+lem+","+tag+")");

	  list<word> rtk;
	  CheckRetokenizable(suf,*r,lem,tag,dic,rtk);

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
  }
}


//////////////////////////////////////////////////////////////////////////////////////////
/// Check whether the suffix carries retokenization information, and create alternative
/// word list if necessary.
//////////////////////////////////////////////////////////////////////////////////////////

void suffixes::CheckRetokenizable(const sufrule &suf, const std::string &form, const std::string &lem, const std::string &tag, dictionary &dic, std::list<word> &rtk) const
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
