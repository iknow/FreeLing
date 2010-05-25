//////////////////////////////////////////////////////////////////
//
//    FreeLing - Open Source Language Analyzers
//
//    Copyright (C) 2004   TALP Research Center
//                         Universitat Politecnica de Catalunya
//
//    This library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU Lesser General Public
//    License as published by the Free Software Foundation; either
//    version 2.1 of the License, or (at your option) any later version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    Lesser General Public License for more details.
//
//    You should have received a copy of the GNU Lesser General Public
//    License along with this library; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
//    contact: Lluis Padro (padro@lsi.upc.es)
//             TALP Research Center
//             despatx C6.212 - Campus Nord UPC
//             08034 Barcelona.  SPAIN
//
////////////////////////////////////////////////////////////////


//------------------------------------------------------------------//
//
//                    IMPORTANT NOTICE
//
//  This file contains a simple main program to illustrate 
//  usage of FreeLing analyzers library.
//
//  This sample main program may be used straightforwardly as 
//  a basic front-end for the analyzers (e.g. to analyze corpora)
//
//  Neverthless, if you want embed the FreeLing libraries inside
//  a larger application, or you want to deal with other 
//  input/output formats (e.g. XML), the efficient and elegant 
//  way to do so is consider this file as a mere example, and call 
//  the library from your your own main code.
//
//------------------------------------------------------------------//



using namespace std;

#include <sstream>
#include <iostream>

#include <set>
#include <map>
#include <vector>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <sstream>

#include "fries/util.h"
#include "freeling/tokenizer.h"
#include "freeling/splitter.h"
#include "freeling/maco.h"
#include "freeling/nec.h"
#include "freeling/tagger.h"
#include "freeling/hmm_tagger.h"
#include "freeling/relax_tagger.h"
#include "freeling/maco_options.h"
#include "LKBconfig.h"

const char FORMFEED=0x0C;

// we use pointers to the analyzers, so we
// can create only those strictly necessary.
tokenizer *tk=NULL;
splitter *sp=NULL;
maco *morfo=NULL;
nec *neclass=NULL;
POS_tagger *tagger=NULL;

// FL configuration options
config *cfg=NULL;

// Variables and classes to hold transformation rules FL->SPPP
class SPPP_rule {
  public:
    RegEx form1;
    RegEx lemma;
    RegEx tag;
    bool any_form, pn_form;
    bool any_lemma, pn_lemma;
    bool any_tag, pn_tag;
 
    string stem;    
    string rule_id;
    string form2;

     SPPP_rule() : form1(""),lemma(""),tag("") {any_form=true; any_lemma=true; any_tag=true; 
                                                pn_form=true;  pn_lemma=true;  pn_tag=true;}
    ~SPPP_rule() {};
};

set<string> noTag;
list<SPPP_rule> rules;
map<string,list<analysis> > replaces;
list<list<string> > fusion;




//---------------------------------------------
// output a string both in cout and cerr
//---------------------------------------------
void say(const string &s) {
  cout<<s<<endl;
  cerr<<s<<endl;
}


//---------------------------------------------
// encode special chars to XML
//---------------------------------------------
void toXML(string &s){
  util::find_and_replace(s, "&", "&amp;");
  util::find_and_replace(s, "\"", "&quot;");
  util::find_and_replace(s, "<", "&lt;");
  util::find_and_replace(s, ">", "&gt;");
  util::find_and_replace(s, "'", "&apos;");
}

//---------------------------------------------
// decode special chars from XML
//---------------------------------------------
void fromXML(string &s){
  util::find_and_replace(s, "&quot;", "\"");
  util::find_and_replace(s, "&lt;", "<");
  util::find_and_replace(s, "&gt;", ">");
  util::find_and_replace(s, "&apos;", "'");
  util::find_and_replace(s, "&amp;", "&");
}

//---------------------------------------------
// print one analysis.
//---------------------------------------------
void print_analysis(const analysis &a, const string &form) {
 
  string lcform = util::lowercase(form);
  string parole = a.get_parole();
  string alemma = a.get_lemma();
  toXML(alemma);

  if (a.is_retokenizable()) {            
    say("    <analysis stem=\""+alemma+"\" >");
    list<word> rtk=a.get_retokenizable();
    list<word>::iterator r;
    string rform;
    for (r=rtk.begin(); r!= rtk.end(); r++) {
      rform = r->get_form();
      toXML(rform);
      say("      <rule id=\""
	  + string(r == rtk.begin() ? "" : "+") + r->get_parole() 
	  + "\" form=\"" + rform +"\" />");
    }
    say("    </analysis>");
  }  

  else {

    string stem="NO-RULE-FOUND"; string rid="NO-RULE-FOUND"; string frm="NO-RULE-FOUND";
    bool trobat = false;
    for (list<SPPP_rule>::iterator r=rules.begin(); r!=rules.end() and not trobat; r++) {
      
      if ( (r->any_form or r->form1.Search(lcform)==r->pn_form) and
	   (r->any_lemma or r->lemma.Search(alemma)==r->pn_lemma) and
	   (r->any_tag or r->tag.Search(parole)==r->pn_tag) ) {
	
	// matching rule found, apply right hand side.
	trobat = true;
	
	// compute stem
	if (r->stem=="L") stem=alemma;
	else if (r->stem=="T") stem=parole;
	else if (r->stem=="F") stem=lcform;
	else stem=r->stem;
	
	// compute rule_id
	if (r->rule_id=="L") rid=alemma;
	else if (r->rule_id=="T") rid=parole;
	else if (r->rule_id=="F") rid=lcform;
	
	// compute form
	string f="";
	for (size_t i=0; i<r->form2.size(); i++) {
	  if (r->form2[i]=='L') f=f+"#"+alemma;
	  else if (r->form2[i]=='T') f=f+"#"+parole;
	  else if (r->form2[i]=='F') f=f+"#"+form;
	}
	if (not f.empty()) frm=f.substr(1);      
      }
    }
    
    say("    <analysis stem=\""+stem+"\" >");
    say("      <rule id=\""+rid+"\" form=\""+frm+"\" />");
    say("    </analysis>");
  }
}


//---------------------------------------------
// check if the word matches some Fusion rule,
// and apply it if so.
//---------------------------------------------
void CheckFusion(word &w, bool tagged) {
  word::iterator wb,we,a;
  set<string> common;

  if (not tagged) {
     wb = w.analysis_begin();
     we = w.analysis_end();
  }
  else {
     wb = w.selected_begin();
     we = w.selected_end();
  }

  // check all fusion rules.
  list<list<string> >::iterator r;
  for (r=fusion.begin(); r!=fusion.end(); r++) {
 
    common.clear(); // clear set of common lemmas.

    // check rule
    bool ok=true;
    list<string>::iterator tagout=r->begin(); // first tag is the output
    list<string>::iterator tag1=r->begin(); tag1++; // second tag is first condition

    list<string>::iterator tr;
    for (tr=tag1; tr!=r->end() and ok; tr++) {
      // build a set with lemmas for current rule tag
      set<string> lems; 
      for (a=wb; a!=we; a++) 
	if ((*tr)==a->get_parole()) 
	  lems.insert(a->get_lemma());

      if (tr==tag1) 
	// first iteration, intersection so far is lem.
	common=lems; 
      else {
	// further iterations, accumulate intersection.
	set<string> is;
	set_intersection(common.begin(), common.end(), lems.begin(), lems.end(), inserter(is,is.begin()) );
	common=is;
      }

      // if acumulated intersection is empty, rule won't match.
      ok = not common.empty();
    }

    if (ok) {  // rule matched. Apply it      
      // for each lemma matching rule tags
      for (set<string>::iterator lem=common.begin(); lem!=common.end(); lem++) {

	// Locate and erase analysis, replacing the first 
        // with new tag.
	bool done=false;
	for (a=wb; a!=we; a++) {
	  bool found=false;
	  for (tr=r->begin(), tr++; tr!=r->end() and not found; tr++)
	    found = ((*tr)==a->get_parole() and (*lem)==a->get_lemma());
	  
	  if (found) {  // tag and lemma match. Delete analysis
	    if (not done) {
	      // first matching analysis. just replace tag.
	      a->set_parole(*tagout);
	      done=true;
	    }
	    else {
	      // not the first, delete.
	      word::iterator a2=a; a2++;
	      w.erase(a);	      
	      a2--; a=a2;  // fix iteration control
	    }
	  }
	}
      }	  
    }
  }
}


//---------------------------------------------
// print obtained analysis.
//---------------------------------------------
void PrintResults(list<sentence> &ls) {
  word::const_iterator ait;
  sentence::iterator w;
  sentence::iterator nxt;
  list<sentence>::iterator is;
  parse_tree tr;  
  dep_tree dep;
  int nsentence=1;
  bool prevde=false; 
 
  for (is=ls.begin(); is!=ls.end(); is++,++nsentence) {
   
    say("<segment>");
    for (w=is->begin(); w!=is->end(); w++) {      

      string wform=w->get_form();
      toXML(wform); 
      string lcform=util::lowercase(wform);
      
      say("  <token form=\""+wform+"\" from=\""+util::int2string(w->get_span_start())+"\" to=\""+util::int2string(w->get_span_finish())+"\" >");

      // if the word is in the 'replace' list, replace all its analysis.
      map<string,list<analysis> >::iterator p=replaces.find(lcform);
      if (p!=replaces.end()) w->set_analysis(p->second);

      // Assume OutputFormat=TAGGED. Output only selected analysis.
      bool tagged=true;
      word::iterator wb = w->selected_begin();
      word::iterator we = w->selected_end();
      
      // ..but if output is MORFO or word was in NoDisambiguate list, output all analysis.
      if (cfg->OutputFormat==MORFO or noTag.find(lcform)!=noTag.end()) {
	wb = w->analysis_begin();
	we = w->analysis_end();
	tagged=false;
      }

      CheckFusion(*w,tagged);
      for (ait=wb; ait!=we; ait++) print_analysis(*ait,wform);
      
      say("  </token>");
    }
    say("</segment>");
  }
  }



//---------------------------------------------
// Plain text, start with tokenizer.
//---------------------------------------------
void ProcessPlain() {
  string text;
  list<word> av;
  list<word>::const_iterator i;
  list<sentence> ls;

    bool head=false;
    while (std::getline(std::cin,text)) {

      cerr<<"   ## Read line ("<<text<<")"<<endl;
      if (!head) {
	string::size_type p=text.find("<?xml version='1.0' encoding='iso-8859-1'?>"); 
	if (p!=string::npos) {
	  text.erase(p,43);
	  say("<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>");
          head=true;
	}
	else ERROR_CRASH("ERROR - <?xml?> header expected");
      }

      if (text[0]==FORMFEED) {
	// process last sentence in buffer (if any)
	ls=sp->split(av, true);  //flush splitter buffer
	morfo->analyze(ls);
	if (cfg->OutputFormat>=TAGGED) tagger->analyze(ls);
	if (cfg->OutputFormat>=TAGGED && cfg->NEC_NEClassification) neclass->analyze(ls);
	
	PrintResults(ls);
        cout<<FORMFEED<<endl; 
        cerr<<FORMFEED<<endl; 
        head=false;
      }
      else {
	// clean xml tags from input
	string::size_type p;
	util::find_and_replace(text, "<text>", "");
	util::find_and_replace(text, "</text>", "");

        // translate XML chars to latin1
	fromXML(text);
	
	av=tk->tokenize(text);
	ls=sp->split(av, cfg->AlwaysFlush);
	morfo->analyze(ls);
	if (cfg->OutputFormat>=TAGGED) tagger->analyze(ls);
	if (cfg->OutputFormat>=TAGGED && cfg->NEC_NEClassification) neclass->analyze(ls);

	PrintResults(ls);	
	av.clear(); // clear list of words for next use
	ls.clear(); // clear list of sentences for next use
      }
    }

}


//---------------------------------------------
// Locate file sppp.dat in the same place than 
// the executable, and load the transformation 
// rules.
//---------------------------------------------
void read_SPPP_rules(string fn) {

  boost::filesystem::path p(fn);
  p=system_complete(p);
  p.remove_filename();
  p = p/"sppp.dat";
  
  boost::filesystem::ifstream fitx(p);

  string line;
  int reading=0;
  while (getline(fitx,line)) {
    if (line == "<NoDisambiguate>") reading=1;
    else if (line == "</NoDisambiguate>") reading=0;
    else if (line == "<ReplaceAll>") reading=2;
    else if (line == "</ReplaceAll>") reading=0;
    else if (line == "<Fusion>") reading=3;
    else if (line == "</Fusion>") reading=0;
    else if (line == "<Output>") reading=4;
    else if (line == "</Output>") reading=0;

    else if (reading==1)   // reading NoDisambiguate section
      noTag.insert(line);
    
    else if (reading==2) { // whole analysis list replacements
      istringstream sin(line);

      string form,al,at;
      sin>>form;
      list<analysis> la;
      while (sin>>al>>at) la.push_back(analysis(al,at));
      replaces.insert(make_pair(form,la));
    }
 
    else if (reading==3) {
      istringstream sin(line);
      list<string> rul;

      string tag;
      sin>>tag;
      while (tag!="=>") {
	rul.push_back(tag);
	sin>>tag;
      }
      // store last tag at the first place.
      sin>>tag;
      rul.push_front(tag);

      fusion.push_back(rul);
    }
 
    else if (reading==4) {  // Read output field arrangements
      istringstream sin(line);

      SPPP_rule r;  // new rule.

      string x;
      sin>>x;  /// get form
      if (x!="*") {
	r.any_form=false;
	if (x[0]=='!') { r.pn_form=false; x = x.substr(1); }	  
	r.form1 = RegEx("^"+x+"$");
      }
      sin>>x;  /// get lemma
      if (x!="*") {
	r.any_lemma=false;
	if (x[0]=='!') { r.pn_lemma=false; x = x.substr(1); }
	r.lemma = RegEx("^"+x+"$");
      }
      sin>>x;  /// get tag
      if (x!="*") {
	r.any_tag=false;
	if (x[0]=='!') { r.pn_tag=false; x = x.substr(1); }	  
	r.tag = RegEx("^"+x);
      }

      sin>>x; 
      if (x!="=>") ERROR_CRASH("Expecting '=>' in rule read from sppp.dat");

      sin>>r.stem;
      sin>>r.rule_id;
      sin>>r.form2;

      // Rest of the line (if any) is ignored (comments).

      // add rule to rule list.
      rules.push_back(r);
    }
  }
}
  


//---------------------------------------------
// Sample main program
//---------------------------------------------
int main(int argc, char **argv) {


  /// load transformation file from FreeLing to SPPP
  read_SPPP_rules(argv[0]);

  // read configuration file and command-line options
  cfg = new config(argv);

  // create required analyzers
  tk = new tokenizer(cfg->TOK_TokenizerFile); 
  sp = new splitter(cfg->SPLIT_SplitterFile);

  // the morfo class requires several options at creation time.
  // they are passed packed in a maco_options object.
  maco_options opt(cfg->Lang);
  // boolean options to activate/desactivate modules
  // default: all modules activated (options set to "false")
  opt.set_active_modules (cfg->MACO_AffixAnalysis,    cfg->MACO_MultiwordsDetection,
			  cfg->MACO_NumbersDetection, cfg->MACO_PunctuationDetection,
			  cfg->MACO_DatesDetection,   cfg->MACO_QuantitiesDetection,
			  cfg->MACO_DictionarySearch, cfg->MACO_ProbabilityAssignment,
			  cfg->MACO_NER_which,        false);

  // decimal/thousand separators used by number detection
  opt.set_nummerical_points(cfg->MACO_Decimal, cfg->MACO_Thousand);
  // Minimum probability for a tag for an unkown word
  opt.set_threshold(cfg->MACO_ProbabilityThreshold);
  // Data files for morphological submodules. by default set to ""
  // Only files for active modules have to be specified 
  opt.set_data_files (cfg->MACO_LocutionsFile, cfg->MACO_QuantitiesFile,
		      cfg->MACO_AffixFile, cfg->MACO_ProbabilityFile,
		      cfg->MACO_DictionaryFile, cfg->MACO_NPdataFile,
		      cfg->MACO_PunctuationFile, "");

  // create analyzer with desired options
  morfo = new maco(opt);

  if (cfg->OutputFormat>=TAGGED) {
     if (cfg->TAGGER_which == HMM)
       tagger = new hmm_tagger(cfg->Lang, cfg->TAGGER_HMMFile,
                               cfg->TAGGER_Retokenize, cfg->TAGGER_ForceSelect);
     else if (cfg->TAGGER_which == RELAX)
       tagger = new relax_tagger(cfg->TAGGER_RelaxFile, cfg->TAGGER_RelaxMaxIter,
                                 cfg->TAGGER_RelaxScaleFactor,
                                 cfg->TAGGER_RelaxEpsilon,
                                 cfg->TAGGER_Retokenize,
                                 cfg->TAGGER_ForceSelect);
  }

  if (cfg->OutputFormat>=TAGGED && cfg->NEC_NEClassification) {
    neclass = new nec("NP", cfg->NEC_FilePrefix);
  }


  // Input is plain text.
  ProcessPlain();

  // clean up. Note that deleting a null pointer is a safe (yet useless) operation
  delete cfg;
  delete tk;
  delete sp; 
  delete morfo; 
  delete tagger;
  delete neclass; 
}

