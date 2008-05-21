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

////////////////////////////////////////////////////////////////
//
//  libmorfo.i
//  This is the SWIG input file, used to generate perl/pyhton/java APIs.
//
////////////////////////////////////////////////////////////////

%module libmorfo_python

%{
 #include "traces.h"
 #include "freeling.h"
%}

%include std_string.i
%include std_list.i
%include std_vector.i

%template(VectorWord) std::vector<word>;
%template(ListWord) std::list<word>;
%template(ListAnalysis) std::list<analysis>;
%template(ListSentence) std::list<sentence>;
%template(ListParagraph) std::list<paragraph>;

template<class T> class tree {};
%template(TreeNode) tree<node>;
%template(TreeDepNode) tree<depnode>;
%rename(operator_assignment) operator=;

class traces {
 public:
    // current trace level
    static int TraceLevel;
    // modules to trace
    static unsigned long TraceModule;
};


class analysis {
   public:
      /// constructor
      analysis();
      /// constructor
      analysis(const std::string &, const std::string &);
       
      void set_lemma(const std::string &);
      void set_parole(const std::string &);
      void set_prob(double);
      void set_retokenizable(const std::list<word> &);

      bool has_prob(void) const;  
      std::string get_lemma(void) const;
      std::string get_parole(void) const;
      std::string get_short_parole(const std::string &) const;
      double get_prob(void) const;
      bool is_retokenizable(void) const;
      std::list<word> get_retokenizable(void) const;
      std::list<std::string> get_senses(void) const;
      void set_senses(const std::list<std::string> &);
};


////////////////////////////////////////////////////////////////
///   Class word stores all info related to a word: 
///  form, list of analysis, list of tokens (if multiword).
////////////////////////////////////////////////////////////////

class word : public std::list<analysis> {
   public:
      /// constructor
      word();
      /// constructor
      word(const std::string &);
      /// constructor
      word(const std::string &, const std::list<word> &);
      /// constructor
      word(const std::string &, const std::list<analysis> &, const std::list<word> &);
      /// Copy constructor
      word(const word &);
      /// assignment
      word& operator=(const word&);

      /// true iff the word has more than one analysis
      bool is_ambiguous(void) const;
      /// true iff the word is a multiword compound
      bool is_multiword(void) const;
      /// get number of words in compound
      int get_n_words_mw(void) const;
      /// get word objects that compound the multiword
      std::list<word> get_words_mw(void) const;
      /// get word form
      std::string get_form(void) const;
      /// Get the selected analysis
      analysis get_selected_analysis(void) const;
      /// Get an iterator to the selected analysis
      word::iterator selected_analysis(void) const;
      /// get lemma for the selected analysis in list
      std::string get_lemma(void) const;
      /// get parole for the selected analysis  
      std::string get_parole(void) const;
      /// get parole (short version) for the selected analysis  
      std::string get_short_parole(const std::string &) const;

      /// get sense list for the selected analysis  
      std::list<std::string> get_senses(void) const;
      /// set sense list for the selected analysis  
      void set_senses(const std::list<std::string> &);
      /// get token span.
      int get_span_start(void) const;
      int get_span_finish(void) const;
      /// get in_dict
      bool found_in_dict(void) const;
      /// set in_dict
      void set_found_in_dict(bool);

      /// add one analysis to current analysis list  (no duplicate check!)
      void add_analysis(const analysis &);
      /// set analysis list to one single analysis, overwriting current values
      void set_analysis(const analysis &);
      /// set analysis list, overwriting current values
      void set_analysis(const std::list<analysis> &);
      /// set word form
      void set_form(const std::string &);
      /// set token span
      void set_span(int,int);
      /// set user data
      void set_user_data(void *);
      
      /// get number of analysis in current list
      int get_n_analysis(void) const; 
      /// copy analysis list
      void copy_analysis(const word &);
      /// mark the given analysis as the selected one.
      void select_analysis(word::iterator);
      /// get list of analysis (only useful for perl API)
      std::list<analysis> get_analysis(void) const; 
      /// get begin iterator to analysis list.
      word::iterator analysis_begin(void);
      word::const_iterator analysis_begin(void) const;
      /// get end iterator to analysis list.
      word::iterator analysis_end(void);
      word::const_iterator analysis_end(void) const;
};

////////////////////////////////////////////////////////////////
///   Class parse tree is used to store the results of parsing
///  Each node in the tree is either a label (intermediate node)
///  or a word (leaf node)
////////////////////////////////////////////////////////////////

class node {
  public:
    node();
    node(const std::string &);
    std::string get_label() const;
    word get_word() const;
    void set_word(word &);
    void set_label(const std::string &);
    bool is_head(void) const;
    void set_head(const bool);
    bool is_chunk(void) const;
    void set_chunk(const int);
    int  get_chunk_ord(void) const;

};

class parse_tree : public tree<node> {
  public:
    parse_tree();
    parse_tree(const node &);
};


////////////////////////////////////////////////////////////////
/// class denode stores nodes of a dependency tree and
///  parse tree <-> deptree relations
////////////////////////////////////////////////////////////////

class depnode : public node {
  public:
    depnode();
    depnode(const std::string &);
    depnode(const node &);
    void set_link(const parse_tree::iterator);
    parse_tree::iterator get_link(void);
    void set_label(const std::string &);
    void set_dep_source(const std::string &);
    void set_dep_target(const std::string &); 
    void set_dep_result(const std::string &); 
    std::string get_dep_source(void) const; 
    std::string get_dep_target(void) const;
    std::string get_dep_result(void) const;
};

////////////////////////////////////////////////////////////////
/// class dep_tree stores a dependency tree
////////////////////////////////////////////////////////////////

class dep_tree :  public tree<depnode> {
  public:
    dep_tree();
    dep_tree(const depnode &);
};


////////////////////////////////////////////////////////////////
///   Class sentence is just a list of words that someone
/// (the splitter) has validated it as a complete sentence.
/// It may include a parse tree.
////////////////////////////////////////////////////////////////

class sentence : public std::vector<word> {
 public:
  sentence();
  void set_parse_tree(const parse_tree &);
  parse_tree get_parse_tree();
  bool is_parsed() const;
  std::vector<word> get_words() const;
  dep_tree get_dep_tree();
  void set_dep_tree(const dep_tree &);
  bool is_dep_parsed() const;
  sentence::iterator words_begin();
  sentence::const_iterator words_begin() const;
  sentence::iterator words_end();
  sentence::const_iterator words_end() const;
};

////////////////////////////////////////////////////////////////
///   Class paragraph is just a list of sentences that someone
///  has validated it as a paragraph.
////////////////////////////////////////////////////////////////

class paragraph : public std::list<sentence> {};

////////////////////////////////////////////////////////////////
///   Class document is a list of paragraphs. It may have additional 
///  information (such as title)
////////////////////////////////////////////////////////////////

class document : public std::list<paragraph> {
  paragraph title;
};

/*------------------------------------------------------------------------*/
class tokenizer {
   public:
       /// Constructor
       tokenizer(const std::string &);

       /// tokenize string with default options
       std::list<word> tokenize(const std::string &);
       /// tokenize string with default options, tracking offset in given int param.
       std::list<word> tokenize(const std::string &, unsigned long &);
};

/*------------------------------------------------------------------------*/
class splitter {
   public:
      /// Constructor
      splitter(const std::string &);

      /// split sentences with default options
      std::list<sentence> split(const std::list<word> &, bool);
};


/*------------------------------------------------------------------------*/
class maco_options {
 public:
    // Language analyzed
    std::string Lang;
    /// Morphological analyzer options
    bool SuffixAnalysis, MultiwordsDetection,
        NumbersDetection, PunctuationDetection,
        DatesDetection, QuantitiesDetection,
        DictionarySearch, ProbabilityAssignment,
        NERecognition;
    /// Morphological analyzer options
    std::string Decimal, Thousand;
    /// Morphological analyzer options
    std::string LocutionsFile, QuantitiesFile, SuffixFile,
           ProbabilityFile, DictionaryFile,
           NPdataFile, PunctuationFile;
    double ProbabilityThreshold;

    /// constructor
    maco_options(const std::string &);

    /// Option setting methods provided to ease perl interface generation.
    /// Since option data members are public and can be accessed directly
    /// from C++, the following methods are not necessary, but may become
    /// convenient sometimes.
    void set_active_modules(bool,bool,bool,bool,bool,bool,bool,bool,bool);
    void set_nummerical_points(const std::string &,const std::string &);
    void set_data_files(const std::string &,const std::string &,const std::string &,const std::string &,
                        const std::string &,const std::string &,const std::string &);
    void set_threshold(double);
};

/*------------------------------------------------------------------------*/
class maco {
   public:
      /// Constructor
      maco(const maco_options &);

      /// analyze sentences
      void analyze(std::list<sentence> &);
      /// analyze sentences, return analyzed copy
      std::list<sentence> analyze(const std::list<sentence> &);
};

/*------------------------------------------------------------------------*/
class hmm_tagger {
   public:
       /// Constructor
       hmm_tagger(const std::string &, const std::string &, bool);

       /// analyze sentences with default options
       void analyze(std::list<sentence> &);
       /// analyze sentences, return analyzed copy
       std::list<sentence> analyze(const std::list<sentence> &);
};


/*------------------------------------------------------------------------*/
class relax_tagger {
   public:
       /// Constructor, given the constraints file and config parameters
       relax_tagger(const std::string &, int, double, double, bool);

       /// analyze sentences with default options
       void analyze(std::list<sentence> &);
       /// analyze sentences, return analyzed copy
       std::list<sentence> analyze(const std::list<sentence> &);
};

/*------------------------------------------------------------------------*/
class chart_parser {
 public:
   /// Constructors
   chart_parser(const std::string&);
   /// Get the start symbol of the grammar
   std::string get_start_symbol(void) const;
   /// parse sentences in list
   void analyze(std::list<sentence> &);
   /// parse sentences, return analyzed copy
   std::list<sentence> analyze(const std::list<sentence> &);
};


/*------------------------------------------------------------------------*/
class dependencyMaker {
 public:   
   /// constructor
   dependencyMaker(const std::string &, const std::string &);
   /// Enrich all sentences in given list with a depenceny tree.
   void analyze(std::list<sentence> &);
   /// Enrich all sentences, return copy
   std::list<sentence> analyze(const std::list<sentence> &);
};
