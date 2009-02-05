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

#include "freeling/coref.h"
#include "fries/util.h"
#include "freeling/traces.h"

using namespace std;

#define MOD_TRACENAME "COREF"
#define MOD_TRACECODE COREF_TRACE

//////////////////////////////////////////////////////////////////
///    Outputs a sample. Only for debug purposes
//////////////////////////////////////////////////////////////////

void outSample(struct SAMPLE &sa){
	cout << "SAMPLE" << endl;
	cout << "	Sent: " << sa.sent << endl;
	cout << "	Beg: " << sa.posbegin << endl;
	cout << "	End: " << sa.posend << endl;
	cout << "	Text: " << sa.text << endl;
	cout << "	Tok: ";
	for(vector<string>::iterator it = sa.texttok.begin(); it!= sa.texttok.end();++it)
		cout << (*it) << " ";
	cout << endl;
	cout << "	Tag: ";
	for(vector<string>::iterator it = sa.tags.begin(); it!= sa.tags.end();++it)
		cout << (*it) << " ";
	cout << endl;
}

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

///////////////////////////////////////////////////////////////
/// Create a coreference module, loading
/// appropriate files.
///////////////////////////////////////////////////////////////

coref::coref(const std::string &filepref){
	int vectors;
	string feat,file,line;

	// create feature extractor
	extractor = new coref_fex();
	extractor->typeVector = TYPE_TWO;
	vectors = DIST | IPRON | JPRON | STRMATCH | DEFNP | DEMNP | GENDER | SEMCLASS | PROPNAME | ALIAS | APPOS;
	extractor->setVectors(vectors);

	// create AdaBoost classifier
	TRACE(3," Loading adaboost model "+filepref+".abm");
	classifier = new adaboost(filepref+".abm");

	TRACE(3,"analyzer succesfully created");
}

///////////////////////////////////////////////////////////////
/// Fills a sample from a node info
///////////////////////////////////////////////////////////////

void coref::set_sample(parse_tree::iterator pt, struct SAMPLE & sample) const{
	parse_tree::sibling_iterator d;

	if (pt->num_children()==0) {
		word wo=pt->info.get_word();
		string w = wo.get_form();
		string t = wo.get_parole();
		std::transform(w.begin(), w.end(), w.begin(), (int (*)(int))std::tolower);
		std::transform(t.begin(), t.end(), t.begin(), (int (*)(int))std::tolower);

		sample.posend++;
		sample.text += " " + w;
		sample.texttok.push_back(w);
		sample.tags.push_back(t);
	} else {
		for (d=pt->sibling_begin(); d!=pt->sibling_end(); ++d)
			set_sample(d, sample);
	}
}

///////////////////////////////////////////////////////////////
/// Finds recursively all the SN and put in a list of samples
///////////////////////////////////////////////////////////////

void coref::add_candidates(int sent, int & word, parse_tree::iterator pt, list<struct SAMPLE> & candidates) const{
	parse_tree::sibling_iterator d;

	if (pt->num_children()==0) {
		word++;
	} else {
		if(pt->info.get_label() == "sn"){
			struct SAMPLE candidate;
			candidate.sent = sent;
			candidate.posbegin = word;
			candidate.posend = word;
			set_sample(pt, candidate);
			word = candidate.posend;
			candidate.node1 = &(pt->info);
			candidates.push_back(candidate);
		} else {
			for (d=pt->sibling_begin(); d!=pt->sibling_end(); ++d)
				add_candidates(sent, word, d, candidates);
		}
	}
}

///////////////////////////////////////////////////////////////
/// Check if the two samples are coreferents. Uses the classifier.
///////////////////////////////////////////////////////////////

bool coref::check_coref(struct SAMPLE & sa1, struct SAMPLE & sa2) const{
	int j;
	double pred[classifier->get_nlabels()];
	std::vector<int> *encoded = new std::vector<int>;
	struct EXAMPLE ex;
	bool ret = false;

	ex.sample1 = sa1;
	ex.sample2 = sa2;
	ex.sent = ex.sample2.sent - ex.sample1.sent;
	//outSample(ex.sample1);
	//outSample(ex.sample2);
	extractor->extract(ex, encoded);
	example exampl(classifier->get_nlabels());
	for(std::vector<int>::iterator it = encoded->begin(); it!= encoded->end(); ++it){
		exampl.add_feature((*it));
	}

	// classify current example
	classifier->classify(exampl,pred);

	// find out which class has highest weight (alternatively, we
	// could select all classes with positive weight, and propose
	// more than one class per example)
	double max=pred[0];
	string tag=classifier->get_label(0);
	for (j=1; j<classifier->get_nlabels(); j++) {
		if (pred[j]>max) {
			max=pred[j];
			tag=classifier->get_label(j);
		}
	}
	// if no label has a positive prediction and <others> is defined, select <others> label.
	string def = classifier->default_class();
	if (max<0 && def!="") tag = def;

	if(tag == "Positive"){
		ret = true;
	}
	return (ret);
}
/////////////////////////////////////////////////////////////////////////////
/// Classify the SN is our group of coreference
/////////////////////////////////////////////////////////////////////////////

void coref::analyze(document & doc) const {
	vector<word>::iterator wr1, wr2;
	list<sentence>::iterator se;
	list<paragraph>::iterator par;
	sentence::iterator w;
	word::iterator a;
	set<int>::iterator f;
	vector<set<int> > features;
	map<string,int>::const_iterator p;

	string tag,def;

	list<struct SAMPLE> candidates;
	list<struct SAMPLE>::iterator it1, it2;

	int sent1;
	int word1;
	bool found;
	int count;
	sent1 = 0;
	word1 = 0;
	for (par = doc.begin(); par != doc.end(); ++par){
		for(se = (*par).begin(); se != (*par).end(); ++se){
			add_candidates(sent1, word1, (*se).get_parse_tree().begin(), candidates);
			sent1++;
		}
	}
	it2 = candidates.begin();
	++it2;
	while(it2 != candidates.end()){
		it1 = it2;
		--it1;
		found = false;
		count = 20;
		while(it1 != candidates.begin() && !found && count > 0){
			found = check_coref(*it1, *it2);
			if(found)
				doc.add_positive(*(*it1).node1, *(*it2).node1);
			--it1;
			count--;
		}
		++it2;
	}
	for (par = doc.begin(); par != doc.end(); ++par){
		for(se = (*par).begin(); se != (*par).end(); ++se){
			PrintMyTree(doc, (*se).get_parse_tree().begin(), 0);
		}
	}
}


