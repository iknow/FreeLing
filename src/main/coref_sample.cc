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


//------------------------------------------------------------------//
//
//                    IMPORTANT NOTICE
//
//  This file contains a sample main program to illustrate 
//  usage of FreeLing analyzers library.
//
//  This sample main program may be used straightforwardly as 
//  a basic front-end for the analyzers (e.g. to analyze corpora)
//
//  Neverthless, if you want embed the FreeLing libraries inside
//  a larger application, or you want to deal with other 
//  input/output formats (e.g. XML,HTML,...), the efficient and elegant 
//  way to do so is consider this file as a mere example, and call 
//  the library from your your own main code.
//
//------------------------------------------------------------------//


#include <iostream>
using namespace std;

#include <freeling.h>


void ProcessResults(const list<sentence> &ls) {
  
  list<sentence>::const_iterator s;
  word::const_iterator a;
  sentence::const_iterator w;
  
  // for each sentence in list
  for (s=ls.begin(); s!=ls.end(); s++) {
    
    // print sentence XML tag
    cout<<"<SENT>"<<endl;
      
    // for each word in sentence
    for (w=s->begin(); w!=s->end(); w++) {
      
      // print word form, with PoS and lemma chosen by the tagger
      cout<<"  <WORD form=\""<<w->get_form();
      cout<<"\" lemma=\""<<w->get_lemma();
      cout<<"\" pos=\""<<w->get_parole();
      cout<<"\">"<<endl;
      
      // for each possible analysis in word, output lemma, parole and probability
      for (a=w->analysis_begin(); a!=w->analysis_end(); ++a) {
	
	// print analysis info
	cout<<"    <ANALYSIS lemma=\""<<a->get_lemma();
	cout<<"\" pos=\""<<a->get_parole();
	cout<<"\" prob=\""<<a->get_prob();
	cout<<"\"/>"<<endl;
      }
      
      // close word XML tag after list of analysis
      cout<<"  </WORD>"<<endl;
    }
    
    // close sentence XML tag
    cout<<"</SENT>"<<endl;
  }
}  



int main() {
	string text;
	list<word> lw;
	list<sentence> ls;
	paragraph par;
	document doc;

	// create analyzers
	tokenizer tk("/usr/local/share/FreeLing/es/tokenizer.dat");
	splitter sp("/usr/local/share/FreeLing/es/splitter.dat");

	// morphological analysis has a lot of options, and for simplicity they are packed up
	// in a maco_options object. First, create the maco_options object with default values.
	maco_options opt("es");
	// then, set required options on/off
	opt.QuantitiesDetection = true;  //deactivate ratio/currency/magnitudes detection
	opt.SuffixAnalysis = true;
	opt.MultiwordsDetection = true;
	opt.NumbersDetection = true;
	opt.PunctuationDetection = true;
	opt.DatesDetection = true;
	opt.QuantitiesDetection = false;
	opt.DictionarySearch = true;
	opt.ProbabilityAssignment = true;
	opt.NERecognition = true;
	// alternatively, you can set active modules in a single call:
	//     opt.set_active_modules(true, true, true, true, true, false, true, true, true);

	// and provide files for morphological submodules. Note that it is not necessary
	// to set opt.QuantitiesFile, since Quantities module was deactivated.
	opt.LocutionsFile="/usr/local/share/FreeLing/es/locucions.dat";
	opt.SuffixFile="/usr/local/share/FreeLing/es/sufixos.dat";
	opt.ProbabilityFile="/usr/local/share/FreeLing/es/probabilitats.dat";
	opt.DictionaryFile="/usr/local/share/FreeLing/es/maco.db";
	opt.NPdataFile="/usr/local/share/FreeLing/es/np.dat";
	opt.PunctuationFile="/usr/local/share/FreeLing/common/punct.dat";
	// alternatively, you can set the files in a single call:
	//  opt.set_data_files("myMultiwordsFile.dat", "", "mySuffixesFile.dat",
	//                     "myProbabilitiesFile.dat", "myDictionaryFile.dat",
	//                     "myNPdatafile.dat", "myPunctuationFile.dat");
  
	// create the analyzer with the just build set of maco_options
	maco morfo(opt);

	// create a hmm tagger for spanish (with retokenization ability, and forced
	// to choose only one tag per word)
	hmm_tagger tagger("es", "/usr/local/share/FreeLing/es/tagger.dat", true, true);
	chart_parser parser("/usr/local/share/FreeLing/es/grammar-dep.dat");

	nec *neclass = new nec("NP", "/usr/local/share/FreeLing/es/nec/nec");
	int vectors = COREFEX_DIST | COREFEX_IPRON | COREFEX_JPRON | COREFEX_IPRONM | COREFEX_JPRONM | COREFEX_STRMATCH | COREFEX_DEFNP | COREFEX_DEMNP | COREFEX_NUMBER | COREFEX_GENDER | COREFEX_SEMCLASS | COREFEX_PROPNAME | COREFEX_ALIAS | COREFEX_APPOS;
	int max_distance = 20;
	coref *corefclass = new coref("/usr/local/share/FreeLing/es/coref", vectors, max_distance);

	// get plain text input lines while not EOF.
	while (getline(cin,text)) {
		// clear temporary lists;
		lw.clear();
		ls.clear();
	
		// tokenize input line into a list of words
		lw=tk.tokenize(text);

		// accumulate list of words in splitter buffer, returning a list of sentences.
		// The resulting list of sentences may be empty if the splitter has still not
		// enough evidence to decide that a complete sentence has been found. The list
		// may contain more than one sentence (since a single input line may consist
		// of several complete sentences).
		ls=sp.split(lw, false);
		par.insert(par.end(), ls.begin(), ls.end());
	}
	lw.clear();

	ls=sp.split(lw, true);
	par.insert(par.end(), ls.begin(), ls.end());
	doc.push_back(par);

//	par = doc.front();

	// analyze all words in all sentences of the list, enriching them with lemma and PoS
	// information. Some of the words may be glued in one (e.g. dates, multiwords, etc.)
	morfo.analyze(doc.front());

	// disambiguate words in each sentence of given sentence list.
	tagger.analyze(doc.front());
	neclass->analyze(doc.front());
	parser.analyze(doc.front());
	corefclass->analyze(doc);
	// Process the enriched/disambiguated objects in the list of sentences
//	ProcessResults(doc.front());

}


