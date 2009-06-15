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


#include <iostream>
using namespace std;

#include <freeling.h>

void OutputMySenses(const analysis &a) {
  list<string> ls=a.get_senses();
  if (ls.size()>0) {
     cout<<" "<<util::list2string(ls,":");
  }
  else cout<<" -";
}
//---------------------------------------------
// print obtained analysis.
//---------------------------------------------
void PrintMyTree(document & doc, parse_tree::iterator n, int depth) {
	parse_tree::sibling_iterator d;

	cout<<string(depth*2,' ');
	if (n->num_children()==0) {
		if (n->info.is_head()) { cout<<"+";}
		word w=n->info.get_word();
		cout<<"("<<w.get_form()<<" "<<w.get_lemma()<<" "<<w.get_parole();
		OutputMySenses((*w.selected_begin()));
		cout<<")"<<endl;
	} else {
		if (n->info.is_head()) { cout<<"+";}
		//Get the class of coreference and print.
		int ref = doc.get_coref_group(n->info);
		if(n->info.get_label() == "sn" && ref != -1){
			cout<<n->info.get_label()<<"(REF " << ref <<")_["<<endl;
		} else {
			cout<<n->info.get_label()<<"_["<<endl;
		}
		for (d=n->sibling_begin(); d!=n->sibling_end(); ++d)
			PrintMyTree(doc, d, depth+1);
		cout<<string(depth*2,' ')<<"]"<<endl;
	}
}

int main() {
	string text;
	list<word> lw;
	list<sentence> ls;
	paragraph par;
	document doc;

        // adjust the path to your installation
	string path = "/usr/local/share/FreeLing/es/";

	// create analyzers
	tokenizer tk(path+"tokenizer.dat");
	splitter sp(path+"splitter.dat");

	// morphological analysis has a lot of options, and for simplicity they are packed up
	// in a maco_options object. First, create the maco_options object with default values.
	maco_options opt("es");
        // enable/disable desired modules
	opt.set_active_modules(true, true, true, true, true, true, true, true, true, NER_BASIC);
        // provide config files for each module
 	opt.set_data_files(path+"locucions.dat",path+"afixos.dat",path+"probabilitats.dat",
			   path+"maco.db",path+"np.dat",path+"/../common/punct.dat");  
	// create the analyzer with the just build set of maco_options
	maco morfo(opt);

	// create a hmm tagger for spanish (with retokenization ability, and forced
	// to choose only one tag per word)
	hmm_tagger tagger("es", path+"tagger.dat", true, true);
        // create a shallow parser
	chart_parser parser(path+"grammar-dep.dat");

	// create a NE classifier
	nec *neclass = new nec("NP", path+"nec/nec");

	// create a correference solver
	int vectors = COREFEX_DIST | COREFEX_IPRON | COREFEX_JPRON | COREFEX_IPRONM | COREFEX_JPRONM | COREFEX_STRMATCH | COREFEX_DEFNP | COREFEX_DEMNP | COREFEX_NUMBER | COREFEX_GENDER | COREFEX_SEMCLASS | COREFEX_PROPNAME | COREFEX_ALIAS | COREFEX_APPOS;
	int max_distance = 20;
	coref *corefclass = new coref(path+"coref", vectors, max_distance);

	// get plain text input lines while not EOF.
	while (getline(cin,text)) {
	  // clear temporary lists;
	  lw.clear();
	  ls.clear();
	  
	  // tokenize input line into a list of words
	  lw=tk.tokenize(text);
	  // split on sentece boundaries
	  ls=sp.split(lw, false);
	  // build paragraphs from sentences
	  par.insert(par.end(), ls.begin(), ls.end());
	}
	lw.clear();

	ls=sp.split(lw, true);
	par.insert(par.end(), ls.begin(), ls.end());

	// build document from paragraphs
	doc.push_back(par);

        // IMPORTANT : In this simple example, each document is assumed to consist of only one paragraph.
        // if your document has more than one, you'll have to loop to process them all. 

	// Process text in the document up to shallow parsing
	morfo.analyze(doc.front());
	tagger.analyze(doc.front());
	neclass->analyze(doc.front());
	parser.analyze(doc.front());

        // HERE you would end the loop, to get all the document parsed

        // then, try to solve correferences in the document
	corefclass->analyze(doc);

        // for each sentence in each paragraph, output the tree and coreference information.
	list<paragraph>::iterator parIt;
	list<sentence>::iterator seIt;
	for (parIt = doc.begin(); parIt != doc.end(); ++parIt){
		for(seIt = (*parIt).begin(); seIt != (*parIt).end(); ++seIt){
			PrintMyTree(doc, (*seIt).get_parse_tree().begin(), 0);
		}
	}


}


