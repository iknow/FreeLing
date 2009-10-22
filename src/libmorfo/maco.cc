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

#include "freeling/maco.h"
#include "freeling/traces.h"

using namespace std;

#define MOD_TRACENAME "MACO"
#define MOD_TRACECODE MACO_TRACE

///////////////////////////////////////////////////////////////
///  Create the morphological analyzer, and all required 
/// recognizers and modules.
///////////////////////////////////////////////////////////////

maco::maco(const maco_options &opts): defaultOpt(opts) {

  numb = (opts.NumbersDetection      ? new numbers(opts.Lang,opts.Decimal,opts.Thousand) 
                                     : NULL);
  punt = (opts.PunctuationDetection  ? new punts(opts.PunctuationFile) 
                                     : NULL);
  date = (opts.DatesDetection        ? new dates(opts.Lang) 
                                     : NULL);
  dico = (opts.DictionarySearch      ? new dictionary(opts.Lang, opts.DictionaryFile, opts.AffixAnalysis, opts.AffixFile) 
                                     : NULL);
  loc = (opts.MultiwordsDetection    ? new locutions(opts.LocutionsFile) 
	                             : NULL);

  if (opts.NERecognition==NER_BASIC) npm = new np(opts.NPdataFile);
  else if (opts.NERecognition==NER_BIO) npm= new bioner(opts.NPdataFile);
  else npm=NULL;

  quant = (opts.QuantitiesDetection  ? new quantities(opts.Lang, opts.QuantitiesFile)
                                     : NULL);
  prob = (opts.ProbabilityAssignment ? new probabilities(opts.Lang, opts.ProbabilityFile, opts.ProbabilityThreshold)
                                     : NULL);

  correct = (opts.OrthographicCorrection ? new corrector(opts.CorrectorFile, *dico) : NULL); 
}

///////////////////////////////////////////////////////////////
///  Destroy morphological analyzer, and all required 
/// recognizers and modules.
///////////////////////////////////////////////////////////////

maco::~maco() {
  delete numb;
  delete punt;
  delete date;
  delete dico;
  delete loc;
  delete npm;
  delete quant;
  delete prob;
  delete correct;
}


///////////////////////////////////////////////////////////////
///  Analyze given sentences.
///////////////////////////////////////////////////////////////  

void maco::analyze(std::list<sentence> &ls) {
  list<sentence>::iterator is;
  
     if (defaultOpt.NumbersDetection){ 
       // (Skipping number detection will affect dates and quantities modules)
       for (is=ls.begin(); is!=ls.end(); is++) {
	 numb->annotate(*is);    
       }    
       TRACE(2,"Sentences annotated by the numbers module.");
     }

     if(defaultOpt.PunctuationDetection){ 
       for (is=ls.begin(); is!=ls.end(); is++) {
	 punt->annotate(*is);    
       }
     }
     TRACE(2,"Sentences annotated by the punts module.");
     
     if(defaultOpt.DatesDetection) { 
       for (is=ls.begin(); is!=ls.end(); is++) {
	 date->annotate(*is);    
       }    
       TRACE(2,"Sentences annotated by the dates module.");
     }

     if(defaultOpt.DictionarySearch) {
       // (Skipping dictionary search will also skip suffix analysis)
       for (is=ls.begin(); is!=ls.end(); is++) {
	 dico->annotate(*is);
       }   
       TRACE(2,"Sentence annotated by the dictionary searcher.");
     }

     // annotate list of sentences with required modules
     if (defaultOpt.MultiwordsDetection) { 
       for (is=ls.begin(); is!=ls.end(); is++) {
	 loc->annotate(*is);
       }
       TRACE(2,"Sentences annotated by the locutions module.");
     }

     if(defaultOpt.NERecognition!=NER_NONE){ 
       for (is=ls.begin(); is!=ls.end(); is++) {
	 npm->annotate(*is);
       }    
       TRACE(2,"Sentences annotated by the np module.");
     }

     if(defaultOpt.QuantitiesDetection){
       for (is=ls.begin(); is!=ls.end(); is++) {
	 quant->annotate(*is);    
       }    
       TRACE(2,"Sentences annotated by the quantities module.");
     }

     if (defaultOpt.OrthographicCorrection){
       for (is=ls.begin(); is!=ls.end(); is++) {
	 correct->annotate(*is);
       }
       TRACE(2,"Orthographic correction of the sentence.");
     }
     

     if (defaultOpt.ProbabilityAssignment) {
       for (is=ls.begin(); is!=ls.end(); is++) {
	 prob->annotate(*is);    
       }    
       TRACE(2,"Sentences annotated by the probabilities module.");
     }

     // mark all analysis of each word as selected (taggers assume it's this way)
     for (is=ls.begin(); is!=ls.end(); is++)
       for (sentence::iterator w=is->begin(); w!=is->end(); w++)
	 w->select_all_analysis();
}

///////////////////////////////////////////////////////////////
///  Analyze given sentences, return analyzed copy
///////////////////////////////////////////////////////////////  

std::list<sentence> maco::analyze(const std::list<sentence> &ls) {
list<sentence> al;

  al=ls;
  analyze(al);
  return(al);
}
