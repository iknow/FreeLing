//////////////////////////////////////////////////////////////////
//
//    FreeLing - Open Source Language Analyzers
//
//    Copyright (C) 2008   TALP Research Center
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
//    author:  Xavier Dolcet (xdolcet@lsi.upc.edu)
//
////////////////////////////////////////////////////////////////


#ifdef ENABLE_MALT

#include <cstdlib>
#include "freeling/traces.h"
#include "freeling/malt_plugin.h"

using namespace std;

#define MOD_TRACENAME "DEP_MALT"
#define MOD_TRACECODE DEP_TRACE


//----------------------------------------------
// Setups the MaltParser: JVM, class, methods
//
// Input parameters:
// - lang: the language
//----------------------------------------------
malt_parser::malt_parser (string file, string lang) {
	JavaVMInitArgs vm_args;
	JavaVMOption options[1];
	jint res;
	
	// Create the classpath char
        string path="-Djava.class.path=";
        if (getenv("CLASSPATH")==NULL) 
          WARNING("CLASSPATH not defined, I might fail to find malt.jar...");
        else 
          path = path+string(getenv("CLASSPATH"));

	//	string path("-Djava.class.path=malt.jar:log4j.jar:libsvm.jar:libsvm28.jar");
	options[0].optionString = const_cast<char *> (path.c_str());
	// Debug options
	//options[1].optionString = "-Xdebug";
	//options[2].optionString = "-Xnoagent";
	//options[3].optionString = "-verbose:jni";
	//options[4].optionString = "-Xcheck:jni";
	
	// Sets the language
	language = lang;
	
	// Create the JVM arguments
	res = JNI_GetDefaultJavaVMInitArgs(&vm_args);
	
	vm_args.version = JNI_VERSION_1_4;
	vm_args.options = options;
	vm_args.nOptions = 1;
	vm_args.ignoreUnrecognized = JNI_TRUE;

	// Create the JAVA VM
	res = JNI_CreateJavaVM(&jvm,(void **)&env,&vm_args);	
	if (res < 0) ERROR_CRASH("Can't create Java VM");
	
	// Get the class we want to execute
	EngineClass = env->FindClass("org/maltparser/MaltConsoleEngine");	
	if (EngineClass == NULL) {
	  check_exception();
	  jvm->DestroyJavaVM();
	  ERROR_CRASH("Class MaltConsoleEngine not found - Killing the JVM");
	}

	// Get the main method...
	//	jmethodID main = env->GetMethodID(EngineClass, "<init>","([Ljava/lang/String;)V");
	jmethodID main = env->GetMethodID(EngineClass, "<init>","()V");
	if (main == NULL) {
	  check_exception();
	  jvm->DestroyJavaVM();
	  ERROR_CRASH("main method not found - Killing the JVM");
	}
	
	// and the setupEngine method
	//	jmethodID setup = env->GetMethodID(EngineClass, "setupEngine","([Ljava/lang/String;)Lorg/maltparser/core/config/SingleMaltConfiguration;");
	jmethodID setup = env->GetMethodID(EngineClass, "startEngine","([Ljava/lang/String;)V");
	if (setup == NULL) {
	  check_exception();
	  jvm->DestroyJavaVM();
	  ERROR_CRASH("setupEngine method not found - Killing the JVM");
	}
	
	int pos = file.find_last_of('.');
	string filename = file.substr(0,pos);
	TRACE(3, "Model file: "+filename);
	
	// Create the needed objects: parser, config and sentence
	// Create the parser's parameters (String[] args object)	
	jstring arg0 = env->NewStringUTF("-c");
	jstring arg1 = env->NewStringUTF(filename.c_str());
	jstring arg2 = env->NewStringUTF("-i");
	jstring arg3 = env->NewStringUTF("input");
	jstring arg4 = env->NewStringUTF("-o");
	jstring arg5 = env->NewStringUTF("output");
	jstring arg6 = env->NewStringUTF("-m");
	jstring arg7 = env->NewStringUTF("parse");
	
	if (arg2 == NULL || arg3 == NULL || arg4 == NULL || arg5 == NULL) {
	  check_exception();
	  jvm->DestroyJavaVM();
	  ERROR_CRASH("Error creating arguments for Malt Parser");
	}
	
	jclass StringClass = env->FindClass("java/lang/String");
	
	jobjectArray args = env->NewObjectArray(8, StringClass, arg0);
	env->SetObjectArrayElement(args,1,arg1);
	env->SetObjectArrayElement(args,2,arg2);
	env->SetObjectArrayElement(args,3,arg3);
	env->SetObjectArrayElement(args,4,arg4);
	env->SetObjectArrayElement(args,5,arg5);
	env->SetObjectArrayElement(args,6,arg6);
	env->SetObjectArrayElement(args,7,arg7);
	
	if (args == NULL) {
	  check_exception();
	  jvm->DestroyJavaVM();
	  ERROR_CRASH("Error creating args array");
	}
	
	TRACE(2,"Creating parser, configuration and sentence");
	// Creates a parser object
	parser = env->NewObject(EngineClass,main,args); 
	check_exception();

	// Gets a configuration object
	config = env->CallObjectMethod(parser,setup,args);
	check_exception();

	// Initializes the configuration & gets its sentence object
	jclass ConfigClass = env->GetObjectClass(config);
	
	jmethodID initialize = env->GetMethodID(ConfigClass,"initialize","(Ljava/lang/Integer;)V");
	if (initialize == NULL) {
	  check_exception();
	  jvm->DestroyJavaVM();
	  ERROR_CRASH("initialize method not found - Killing the JVM");
	}
	
	jclass IntegerClass = env->FindClass("java/lang/Integer");
	jint initInt = 0;
	jmethodID constrInt = env->GetMethodID(IntegerClass,"<init>","(I)V");
	if (constrInt == NULL) {
	  check_exception();
	  jvm->DestroyJavaVM();
	  ERROR_CRASH("Constructor method of integer not found");
	}
	jobject initArg = env->NewObject(IntegerClass,constrInt,initInt);
	env->CallVoidMethod(config,initialize,initArg);
	check_exception();
	
	ConfigClass = env->FindClass("org/maltparser/core/config/SingleMaltConfiguration");
	jmethodID getSentence = env->GetMethodID(ConfigClass,"getSentence","()Lorg/maltparser/core/sentence/Sentence;");
	if (getSentence == NULL) {
	  check_exception();
	  jvm->DestroyJavaVM();
	  ERROR_CRASH("getSentence method not found - Killing the JVM");
	}

	configSentence = env->CallObjectMethod(config,getSentence);
	check_exception();

	//Delete local references
	env->DeleteLocalRef(arg0);
	env->DeleteLocalRef(arg1);
	env->DeleteLocalRef(arg2);
	env->DeleteLocalRef(arg3);
	env->DeleteLocalRef(args);
	env->DeleteLocalRef(StringClass);
	env->DeleteLocalRef(ConfigClass);

	TRACE(1,"MaltParser plugin successfully created");
}


void malt_parser::analyze (std::list<sentence> &ls) {	
  for (list<sentence>::iterator iter=ls.begin(); iter!=ls.end(); iter++)
    analyze_sentence(*iter);
}


std::list<sentence> malt_parser::analyze(const std::list<sentence> &ls) {
  list<sentence> myls=ls;
  analyze(myls);
  return myls;
}


void malt_parser::analyze_sentence (sentence &sent) {
	
	int id;
	vector<parse_tree::iterator> nodes;
	vector<word> words;
	int size;
	bool parsed = sent.is_parsed();
	if (parsed){
	  parse_tree &tre = sent.get_parse_tree();
	  
	  // This call retrieves the words from the parser_tree, creates a CoNLL format string and loads it
	  // into the configSentence object
	  id = 1;
	  TRACE(3, "Read the tree and fill the Sentence object");
	  retrieve_nodes_words(tre.begin(), nodes, id);
	  size = nodes.size();
	} 
	else {
	  TRACE(3, "Read the sentence and fill the Sentence object");
	  words = sent.get_words();
	  size = words.size();
	  retrieve_words(words);
	}
	
	vector< pair<int,int> > relations;
	
	// Once the configSentence has all the lines, we parse it and
	// retrieve the DependencyGraph object
	TRACE(3, "Parse the sentence and get the DependencyGraph object");
	jobject deps = parse_sentence();
	
	TRACE(3, "Transforming the DependencyGraph object");
	if (deps == NULL) WARNING ("Deps is NULL");
	
	// Get the getNode, getOutputColumnMap and toTab methods
	jclass DepGraphClass = env->GetObjectClass(deps);
	jmethodID getNode = env->GetMethodID(DepGraphClass,"getNode","(I)Lorg/maltparser/core/graph/Node;");
	if (getNode == NULL) {
	  jvm->DestroyJavaVM();
	  ERROR_CRASH("getNode method not found");
	}
	
	jmethodID getOutputColumnMap = env->GetMethodID(DepGraphClass,"getOutputColumnMap","()Ljava/util/HashMap;");
	if (getOutputColumnMap == NULL) {
	  jvm->DestroyJavaVM();
	  ERROR_CRASH("getOutputColumnMap method not found");
	}
	
	jmethodID toTab = env->GetMethodID(DepGraphClass,"toTab","()Ljava/lang/String;");
	if (toTab == NULL) {
	  jvm->DestroyJavaVM();
	  ERROR_CRASH("toTab method not found");
	}
	
	// Get the getToken, getLabelCode and getIndex methods
	jclass NodeClass = env->FindClass("org/maltparser/core/graph/Node");
	jmethodID getLabelCode = 
	  env->GetMethodID(NodeClass,"getLabelCode","(Lorg/maltparser/core/graph/Node;Lorg/maltparser/core/symbol/SymbolTable;)Ljava/lang/Integer;");
	if (getLabelCode == NULL) {
	  jvm->DestroyJavaVM();
	  ERROR_CRASH("getLabelCode method not found");
	}
	
	jmethodID getIndex = env->GetMethodID(NodeClass,"getIndex","()Ljava/lang/Integer;");
	if (getIndex == NULL) {
	  jvm->DestroyJavaVM();
	  ERROR_CRASH("getIndex method not found");
	}
	
	jmethodID getHead = env->GetMethodID(NodeClass,"getHead","()Lorg/maltparser/core/graph/Node;");
	if (getHead == NULL) {
	  jvm->DestroyJavaVM();
	  ERROR_CRASH("getHead method not found");
	}
	
	//Get the HashMap get method
	jclass HashmapClass = env->FindClass("java/util/HashMap");
	jmethodID get = env->GetMethodID(HashmapClass,"get","(Ljava/lang/Object;)Ljava/lang/Object;");
	if (get == NULL) {
	  jvm->DestroyJavaVM();
	  ERROR_CRASH("get method not found");
	}
	
	// Get the SymbolTable getSymbol methods
	jclass SymbolTableClass = env->FindClass("org/maltparser/core/symbol/SymbolTable");
	jmethodID getSymbol = env->GetMethodID(SymbolTableClass,"getSymbolCodeToString","(Ljava/lang/Integer;)Ljava/lang/String;");
	if (getSymbol == NULL) {
	  jvm->DestroyJavaVM();
	  ERROR_CRASH("getSymbolCodeToString method not found");
	}
	
	// Get the dataFormat of the sentence so we can get the columns that we want
	jclass SentenceClass = env->GetObjectClass(configSentence);
	jmethodID getDataFormat = env->GetMethodID(SentenceClass,"getDataFormatInstance","()Lorg/maltparser/core/dataformat/DataFormatInstance;");
	if (getDataFormat == NULL) {
	  jvm->DestroyJavaVM();
	  ERROR_CRASH("getDataFormat method not found");
	}
	jobject dataFormat = env->CallObjectMethod(configSentence,getDataFormat);
	
	// Get the getColumnDescriptionByName method
	jclass DataFormatClass = env->GetObjectClass(dataFormat);
	string getColumnDescriptionParam = "(Ljava/lang/String;)Lorg/maltparser/core/dataformat/ColumnDescription;";
	jmethodID getColumnDescription = env->GetMethodID(DataFormatClass, "getColumnDescriptionByName", getColumnDescriptionParam.c_str());
	if (getColumnDescription == NULL) {
	  jvm->DestroyJavaVM();
	  ERROR_CRASH("getColumnDescriptionByName method not found");
	}
	
	// Get the getSymbolTable method from ColumnDescription class
	jclass ColumnDescriptionClass = env->FindClass("org/maltparser/core/dataformat/ColumnDescription");
	jmethodID getSymbolTable = env->GetMethodID(ColumnDescriptionClass,"getSymbolTable","()Lorg/maltparser/core/symbol/SymbolTable;");
	if (getSymbolTable == NULL) {
	  jvm->DestroyJavaVM();
	  ERROR_CRASH("getSymbolTable method not found");
	}
	
	jclass IntegerClass = env->FindClass("java/lang/Integer");
	jmethodID intValue = env->GetMethodID(IntegerClass,"intValue","()I");
	if (intValue == NULL) {
	  jvm->DestroyJavaVM();
	  ERROR_CRASH("intValue method not found");
	}
	
	jstring dep_out = (jstring) env->CallObjectMethod(deps,toTab);
	printJString(dep_out);
	
	jstring col_name = env->NewStringUTF("DEPREL");
	jobject columnDesc = env->CallObjectMethod(dataFormat,getColumnDescription,col_name);
	check_exception();
	if (columnDesc == NULL) WARNING("columndesc is NULL");
	
	jobject outputMap = env->CallObjectMethod(deps,getOutputColumnMap);
	check_exception();

	jobject aux_table = env->CallObjectMethod(columnDesc,getSymbolTable);
	check_exception();

	jobject table = env->CallObjectMethod(outputMap,get,aux_table);
	check_exception();
	
	vector<dep_tree *> trees;
	int top=0;
	
	jobject node, node_head,head_index,label_code;
	jstring dep_label;
	
	// Bucle
	for (jint i = 1; i <= size; i++) {
	  
	  id = (int) i;
	  
	  node = env->CallObjectMethod(deps,getNode,i);
	  check_exception();

	  // Retrieve info from the JAVA Dependency Tree
	  node_head = env->CallObjectMethod(node,getHead);
	  check_exception();

	  head_index = env->CallObjectMethod(node_head,getIndex);
	  check_exception();

	  label_code = env->CallObjectMethod(node,getLabelCode,node_head,table);
	  check_exception();

	  dep_label = (jstring) env->CallObjectMethod(table,getSymbol,label_code);
	  check_exception();

	  jint jint_head_index = env->CallIntMethod(head_index,intValue);
	  
	  // Prepare all the info needed for a depnode
	  const char* char_dep_label = env->GetStringUTFChars(dep_label,0);
	  check_exception();

	  string str_dep_label(char_dep_label);
	  env->ReleaseStringUTFChars(dep_label, char_dep_label);
	  check_exception();
	  
	  int int_head_index = (int) jint_head_index;
	  
	  // Create the depnode and a dep_tree
	  dep_tree *tree;
	  if (parsed) {
	    depnode d(nodes[id-1]->info);
	    d.set_link(nodes[id-1]);
	    d.set_label(str_dep_label);
	    tree = new dep_tree(d);
	  } 
	  else {
	    depnode d(str_dep_label);
	    d.set_word(words[id-1]);
	    tree = new dep_tree(d);
	  }
	  // Push the dep tree into the trees vector and create a relationship pair
	  trees.push_back(tree);
	  
	  if (int_head_index != 0) {
	    relations.push_back(make_pair(int_head_index-1,id-1));
	  } else {
	    top = id-1;
	  }
	  
	  env->DeleteLocalRef(node);
	  env->DeleteLocalRef(node_head);
	  env->DeleteLocalRef(head_index);
	  env->DeleteLocalRef(label_code);
	  env->DeleteLocalRef(dep_label);
	  
	}
	
	env->DeleteLocalRef(dataFormat);
	env->DeleteLocalRef(dep_out);
	env->DeleteLocalRef(col_name);
	env->DeleteLocalRef(columnDesc);
	env->DeleteLocalRef(outputMap);
	env->DeleteLocalRef(aux_table);
	env->DeleteLocalRef(table);
	
	for (vector< pair<int,int> >::iterator i = relations.begin(); i != relations.end(); i++) {
	  trees[i->first]->hang_child(*trees[i->second]);
	}
	
	dep_tree *tree = trees[top];
	sent.set_dep_tree(*tree);
	// print_deptree(tree->begin(), 0);

}

void malt_parser::retrieve_nodes_words(parse_tree::iterator n, vector<parse_tree::iterator> &nodes, int &id) {
  parse_tree::sibling_iterator child;
  
  if (n->num_children() == 0) {
    nodes.push_back(n);
		// Get the word and construct the string we're going to load into the MaltParser sentence object
    word w = n->info.get_word();
    string feats("_");
    // Get all the fields needed
    string line = create_line(int2str(id), w.get_form(), w.get_lemma(), w.get_parole(), w.get_short_parole(language), feats);
    // Load line into sentence
    load_sentence(line);
    id++;
  } 
  else {
    for (child = n->sibling_begin(); child != n->sibling_end(); ++child) {
      retrieve_nodes_words(child,nodes,id);
    }
  }
}

void malt_parser::retrieve_words(vector<word> words) {
  vector<word>::iterator iter;
  int id = 0;
  for (iter = words.begin(); iter != words.end(); iter++){
    string feats("_");
    // Get all the fields needed
    string line = create_line(int2str(id), iter->get_form(), iter->get_lemma(), iter->get_parole(), iter->get_short_parole(language), feats);
    load_sentence(line);
    id++;
  }
}

string malt_parser::int2str (int num) {
  stringstream out;
  out << num;
  return out.str();
}

string malt_parser::create_line(string id_token, string form, string lemma, string pos, string short_pos, string feats) {
  // Separator & Unknown characters
  string sep("\t");
  string unk("_");
  // Create the line with the given info
  string line = id_token + sep + form + sep + lemma + sep + pos.substr(0,1) + sep + short_pos.substr(0,2) + sep + feats;
  // Plus the unknown info
  // This crashes the execution -> if you have a .mco is not needed
  // line = line + sep + unk + sep + unk + sep + unk + sep + unk;
  return line;
}

void malt_parser::load_sentence(string line) {
  // Get the MaltParser Sentence class and its method setToken
  jclass SentenceClass = env->GetObjectClass(configSentence);
  jmethodID setToken = env->GetMethodID(SentenceClass,"setToken","(Ljava/lang/String;)V");
  if (setToken == NULL ) {
    jvm->DestroyJavaVM();
    ERROR_CRASH("setToken method not found");
  }
  
  TRACE(3, "Loading sentence: "+line);
 
  // Create the String argument
  jstring args = env->NewStringUTF(line.c_str());
  
  // Call the setToken method
  env->CallVoidMethod(configSentence,setToken,args);
  check_exception();
}

jobject malt_parser::parse_sentence() {
  // Get the SingleMaltConfiguration method parseSentence
  jclass ConfigClass = env->GetObjectClass(config);
  jmethodID parseSentence = env->GetMethodID(ConfigClass,"parseSentence","()Lorg/maltparser/core/graph/DependencyGraph;");
  
  if (parseSentence == NULL) {
    jvm->DestroyJavaVM();
    ERROR_CRASH("parseSentence == NULL");
  }
  jobject depGraph = env->CallObjectMethod(config,parseSentence);
  check_exception();
  
  // depGraph contains a MaltParser DependencyGraph object
  return depGraph;
}

void malt_parser::printJString (jstring str) {
  const char* aux;
  jboolean is_copy;
  aux = env->GetStringUTFChars(str,&is_copy);
  cout<<"JString: "<<aux<<endl;
  if (is_copy == JNI_TRUE) env->ReleaseStringUTFChars(str,aux);
}


// debugging purposes
void malt_parser::print_deptree(dep_tree::iterator n, int depth) {
  dep_tree::sibling_iterator d;
  
  cout << string(depth*2,' ');
  cout << n->info.get_link()->info.get_label() << "/" << n->info.get_label() << "/";
  word w = n->info.get_word();
  cout << "(" << w.get_form() << " " << w.get_lemma() << " " << w.get_parole() << ")";
  
  if (n->num_children() > 0) {
    cout << " [" << endl;
    
    // Print nodes
    for (d = n->sibling_begin(); d != n->sibling_end(); ++d)
      print_deptree(d, depth + 1);
    
    cout << string(depth*2,' ') << "]";
  }
  
  cout << endl;
}


void malt_parser::check_exception() {
  if (env->ExceptionOccurred()) 
    env->ExceptionDescribe();
}


/*
int main () {
	cout << "MaltParser test begin..." << endl;
	
	string text = "El gat menja peix al terrat.";
	
	// Sample.cc code begins here...
	list<word> lw;
	list<sentence> ls;
	
	// create analyzers
	tokenizer tk("tokenizer.dat"); 
	splitter sp("splitter.dat");
	
	// morphological analysis has a lot of options, and for simplicity they are packed up
	// in a maco_options object. First, create the maco_options object with default values.
	maco_options opt("ca");  
	// then, set required options on/off  
	opt.QuantitiesDetection = false;  //deactivate ratio/currency/magnitudes detection 
	opt.SuffixAnalysis = true;       opt.MultiwordsDetection = true;   opt.NumbersDetection = true; 
	opt.PunctuationDetection = true; opt.DatesDetection = true;        opt.QuantitiesDetection = false; 
	opt.DictionarySearch = true;     opt.ProbabilityAssignment = true; opt.NERecognition = true;   
	// alternatively, you can set active modules in a single call:
	//     opt.set_active_modules(true, true, true, true, true, false, true, true, true);
	
	// and provide files for morphological submodules. Note that it is not necessary
	// to set opt.QuantitiesFile, since Quantities module was deactivated.
	opt.LocutionsFile="locucions.dat";       opt.SuffixFile="sufixos.dat";
	opt.ProbabilityFile="probabilitats.dat";  opt.DictionaryFile="maco.db";
	opt.NPdataFile="np.dat";              opt.PunctuationFile="../common/punct.dat"; 
	// alternatively, you can set the files in a single call:
	//  opt.set_data_files("myMultiwordsFile.dat", "", "mySuffixesFile.dat", 
	//                     "myProbabilitiesFile.dat", "myDictionaryFile.dat", 
	//                     "myNPdatafile.dat", "myPunctuationFile.dat");
	
	// create the analyzer with the just build set of maco_options
	maco morfo(opt); 
	// create a hmm tagger for spanish (with retokenization ability, and forced 
	// to choose only one tag per word)
	hmm_tagger tagger("ca", "tagger.dat", true, true); 
	// create chunker
	chart_parser parser("grammar-dep.dat");
	// create dependency parser 
	dependencyMaker dep("dep/dependences.dat", parser.get_start_symbol());
	
	// tokenize the string text
	lw=tk.tokenize(text);
	ls=sp.split(lw, true);
	
	// perform and output morphosyntactic analysis and disambiguation
	morfo.analyze(ls);
	tagger.analyze(ls);
	
	// perform and output Chunking
	parser.analyze(ls);
	
	malt_parser dep_parse("test","ca");
	
	dep_parse.analyze(ls);
	
	// clear temporary lists;
	lw.clear(); ls.clear();    
	
}*/

#endif
