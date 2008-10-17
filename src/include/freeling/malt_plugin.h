//////////////////////////////////////////////////////////////////
////
////    FreeLing - Open Source Language Analyzers
////
////    Copyright (C) 2004   TALP Research Center
////                         Universitat Politecnica de Catalunya
////
////    This library is free software; you can redistribute it and/or
////    modify it under the terms of the GNU General Public
////    License as published by the Free Software Foundation; either
////    version 2.1 of the License, or (at your option) any later version.
////
////    This library is distributed in the hope that it will be useful,
////    but WITHOUT ANY WARRANTY; without even the implied warranty of
////    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
////    General Public License for more details.
////
////    You should have received a copy of the GNU General Public
////    License along with this library; if not, write to the Free Software
////    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
////
////    contact: Lluis Padro (padro@lsi.upc.es)
////             TALP Research Center
////             despatx C6.212 - Campus Nord UPC
////             08034 Barcelona.  SPAIN
////
////	author: Xavier Dolcet (xdolcet@lsi.upc.edu)
////
//////////////////////////////////////////////////////////////////


#ifdef ENABLE_MALT

#ifndef _MALT_PLUGIN
#define _MALT_PLUGIN

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <jni.h>
#include "freeling.h"

using namespace std;

class malt_parser : public dependency_parser {
 private:
	// Java environment
	JNIEnv *env;
	// Java VM
	JavaVM *jvm;
	// MaltConsoleEngine class
	jclass EngineClass;
	// MaltConsoleEngine object
	jobject parser;
	jobject config;
	jobject configSentence;
	// Some settings
	string language;
	
	void analyze_sentence(sentence &);
	void parse_sentence(string);
	string int2str(int num);
	string create_line(string, string, string, string, string, string);
	void retrieve_nodes_words(parse_tree::iterator, vector<parse_tree::iterator> &, int &);
	void retrieve_words(vector<word>);
	void load_sentence(string);
	jobject parse_sentence();
	void printJString (jstring);
	void print_deptree(dep_tree::iterator, int);

	void check_exception();

 public:
	malt_parser (string,string);
	void analyze(std::list<sentence> &);
	std::list<sentence> analyze(const std::list<sentence> &);

};

#endif
#endif
