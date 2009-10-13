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
 #include "freeling/traces.h"
 #include "freeling.h"
 #include "fries.h"
 #include "omlet.h"
 using namespace std;
%}

%include std_string.i
%include std_list.i
%include std_vector.i
%include std_map.i

%template(VectorWord) std::vector<word>;
%template(ListWord) std::list<word>;
%template(ListAnalysis) std::list<analysis>;
%template(ListSentence) std::list<sentence>;
%template(ListParagraph) std::list<paragraph>;

%template(ListString) std::list<std::string>;
%template(ListInt) std::list<int>;
%template(VectorListInt) std::vector<std::list<int> >;
%template(VectorListString) std::vector<std::list<std::string> >;


###############  FRIES #####################

template <class T> class tree { 
 public:
  T info;
  class generic_iterator;
  class preorder_iterator;
  class sibling_iterator;
  typedef preorder_iterator iterator;
 
  tree();
  tree(const T&);
  tree(const tree<T>&);
  tree(const preorder_iterator&);
  ~tree();
  ##tree<T>& operator=(const tree<T>&);

  unsigned int num_children() const;
  sibling_iterator nth_child(unsigned int) const;
  tree<T>& nth_child_ref(unsigned int) const;
  T& get_info();
  void append_child(const tree<T> &);
  void hang_child(tree<T> &);
  void clear();
  bool empty() const;

  sibling_iterator sibling_begin();
  sibling_iterator sibling_end() const;

  preorder_iterator begin();
  preorder_iterator end() const;

  class generic_iterator  { 
    protected:
       tree *pnode;
    public:
       generic_iterator();
       generic_iterator(tree *);
       tree<T>& operator*() const;
       tree<T>* operator->() const;
       bool operator==(const generic_iterator &) const;
       bool operator!=(const generic_iterator &) const;
  };

  /// traverse the tree in preorder (parent first, then children)
  class preorder_iterator : public generic_iterator { 
    public:
       preorder_iterator();
       preorder_iterator(tree *);
       preorder_iterator(sibling_iterator&);

       preorder_iterator& operator++();
       preorder_iterator& operator--();
       preorder_iterator& operator+=(unsigned int);
       preorder_iterator& operator-=(unsigned int);
  };

 /// traverse all children of the same node
 class sibling_iterator : public generic_iterator {
    friend class preorder_iterator;
    public:
       sibling_iterator();
       sibling_iterator(tree *);

       sibling_iterator& operator++();
       sibling_iterator& operator--();
       sibling_iterator& operator+=(unsigned int);
       sibling_iterator& operator-=(unsigned int);
  };
};

%template(TreeNode) tree<node>;
%template(TreeDepnode) tree<depnode>;
%rename(operator_assignment) operator=;


class analysis {
   public:
      /// user-managed data, we just store it.
      std::vector<std::string> user;

      /// constructor
      analysis();
      /// constructor
      analysis(const std::string &, const std::string &);
      /// assignment
      analysis& operator=(const analysis&);

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

      //void set_senses(const std::list<std::pair<std::string,double> > &);
      //std::list<std::pair<std::string,double> > get_senses(void) const;
      std::string get_senses_string(void) const;

};


////////////////////////////////////////////////////////////////
///   Class word stores all info related to a word: 
///  form, list of analysis, list of tokens (if multiword).
////////////////////////////////////////////////////////////////

class word : public std::list<analysis> {
   public:
      /// user-managed data, we just store it.
      std::vector<std::string> user;

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

      /// Get the number of selected analysis
      int get_n_selected(void) const;
      /// get the number of unselected analysis
      int get_n_unselected(void) const;
      /// true iff the word is a multiword compound
      bool is_multiword(void) const;
      /// get number of words in compound
      int get_n_words_mw(void) const;
      /// get word objects that compound the multiword
      std::list<word> get_words_mw(void) const;
      /// get word form
      std::string get_form(void) const;
      /// Get an iterator to the first selected analysis
      word::iterator selected_begin(void);
      /// Get an iterator to the first selected analysis
      word::const_iterator selected_begin(void) const;
      /// Get an iterator to the end of selected analysis list
      word::iterator selected_end(void);
      /// Get an iterator to the end of selected analysis list
      word::const_iterator selected_end(void) const;
      /// Get an iterator to the first unselected analysis
      word::iterator unselected_begin(void);
      /// Get an iterator to the first unselected analysis
      word::const_iterator unselected_begin(void) const;
      /// Get an iterator to the end of unselected analysis list
      word::iterator unselected_end(void);
      /// Get an iterator to the end of unselected analysis list
      word::const_iterator unselected_end(void) const;
      /// get lemma for the selected analysis in list
      std::string get_lemma(void) const;
      /// get parole for the selected analysis  
      std::string get_parole(void) const;
      /// get parole (short version) for the selected analysis  
      std::string get_short_parole(const std::string &) const;

      /// set sense list for the selected analysis  
      //void set_senses(const std::list<std::pair<std::string,double> > &);
      /// get sense list for the selected analysis  
      //std::list<std::pair<std::string,double> > get_senses(void) const;
      std::string get_senses_string(void) const;
   
      /// get token span.
      unsigned long get_span_start(void) const;
      unsigned long get_span_finish(void) const;

      /// get in_dict
      bool found_in_dict(void) const;
      /// set in_dict
      void set_found_in_dict(bool);
      /// check if there is any retokenizable analysis
      bool has_retokenizable(void) const;

      /// add one analysis to current analysis list  (no duplicate check!)
      void add_analysis(const analysis &);
      /// set analysis list to one single analysis, overwriting current values
      void set_analysis(const analysis &);
      /// set analysis list, overwriting current values
      void set_analysis(const std::list<analysis> &);
      /// set word form
      void set_form(const std::string &);
      /// set token span
      void set_span(unsigned long, unsigned long);
      
      /// look for an analysis with a parole matching given regexp
      bool find_tag_match(RegEx &);

      /// get number of analysis in current list
      int get_n_analysis(void) const; 
      /// copy analysis list
      void copy_analysis(const word &);
      /// empty the list of selected analysis
      void unselect_all_analysis();
      /// mark all analysisi as selected
      void select_all_analysis();
      /// add the given analysis to selected list.
      void select_analysis(word::iterator);
      /// remove the given analysis from selected list.
      void unselect_analysis(word::iterator);
      /// get list of analysis (useful for perl API)
      std::list<analysis> get_analysis(void) const; 
      /// get begin iterator to analysis list (useful for perl/java API)
      word::iterator analysis_begin(void);
      word::const_iterator analysis_begin(void) const;
      /// get end iterator to analysis list (useful for perl/java API)
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
    tree<node>& get_link_ref(void);
    void set_label(const std::string &);
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
  parse_tree & get_parse_tree(void);
  bool is_parsed() const;  
  dep_tree & get_dep_tree();
  void set_dep_tree(const dep_tree &);
  bool is_dep_parsed() const;
  /// get word list (useful for perl API)
  std::vector<word> get_words() const;
  /// get iterators to word list (useful for perl/java API)
  sentence::iterator words_begin(void);
  sentence::const_iterator words_begin(void) const;
  sentence::iterator words_end(void);
  sentence::const_iterator words_end(void) const;
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
 public:
    document();
    void add_positive(std::string, std::string);
    int get_coref_group(std::string) const;
    std::list<std::string> get_coref_nodes(int) const;
    bool is_coref(std::string, std::string) const;
};


/*----------------------------- fex.h ----------------------------------*/

class fex {
 
 public:
      /// Constructor
      fex(const std::string &, const std::string &lex=""); 

      /// For Perl/java APIs, encode given sentence in feature names, return result as vector
      std::vector<std::list<std::string> > encode_name(const sentence &, bool);
      /// For Perl/java APIs, encode given sentence in feature codes, return result as vector
      std::vector<std::list<int> > encode_int(const sentence &);

      /// empty lexicon
      void clear_lexicon(); 
      /// add feature occurrence to lexicon
      void add_lexicon(const std::string &);
      /// save lexicon to a file, filtering features with low occurrence rate
      void save_lexicon(const std::string &, double min=0.0) const;
      /// load lexicon from a file
      void load_lexicon(const std::string &);
};


###############  FREELING  #####################

class traces {
 public:
    // current trace level
    static int TraceLevel;
    // modules to trace
    static unsigned long TraceModule;
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
    bool AffixAnalysis, MultiwordsDetection,
        NumbersDetection, PunctuationDetection,
        DatesDetection, QuantitiesDetection,
        DictionarySearch, ProbabilityAssignment,
        NERecognition;
    /// Morphological analyzer options
    std::string Decimal, Thousand;
    /// Morphological analyzer options
    std::string LocutionsFile, QuantitiesFile, AffixFile,
           ProbabilityFile, DictionaryFile,
           NPdataFile, PunctuationFile;
    double ProbabilityThreshold;

    /// constructor
    maco_options(const std::string &);

    /// Option setting methods provided to ease perl interface generation.
    /// Since option data members are public and can be accessed directly
    /// from C++, the following methods are not necessary, but may become
    /// convenient sometimes.
    void set_active_modules(bool,bool,bool,bool,bool,bool,bool,bool,int,bool);
    void set_nummerical_points(const std::string &,const std::string &);
    void set_data_files(const std::string &,const std::string &,const std::string &,const std::string &,
                        const std::string &,const std::string &,const std::string &, const std::string &,
			const std::string &);
    void set_threshold(double);
};

/*------------------------------------------------------------------------*/
class maco {
   public:
      /// Constructor
      maco(const maco_options &);

      /// analyze sentences
      //void analyze(std::list<sentence> &);
      /// analyze sentences, return analyzed copy
      std::list<sentence> analyze(const std::list<sentence> &);
};


/*------------------------------------------------------------------------*/
class hmm_tagger {
   public:
       /// Constructor
       hmm_tagger(const std::string &, const std::string &, bool, unsigned int);

       /// analyze sentences with default options
       //void analyze(std::list<sentence> &);
       std::list<sentence> analyze(const std::list<sentence> &);
};


/*------------------------------------------------------------------------*/
class relax_tagger {
   public:
       /// Constructor, given the constraints file and config parameters
       relax_tagger(const std::string &, int, double, double, bool, unsigned int);

       /// analyze sentences with default options
       //void analyze(std::list<sentence> &);
       std::list<sentence> analyze(const std::list<sentence> &);
};


/*------------------------------------------------------------------------*/
class nec {
   public:
      /// Constructor
      nec(const std::string &, const std::string &); 
      /// Destructor
      ~nec();

      /// Classify NEs in given sentence
      void analyze(std::list<sentence> &) const;
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
};

/*------------------------------------------------------------------------*/
class dependency_parser {
  public: 
   dependency_parser();
   virtual ~dependency_parser() {};
   virtual void analyze(std::list<sentence> &)=0;
};


/*------------------------------------------------------------------------*/
class dep_txala : public dependency_parser {
 public:   
   dep_txala(const std::string &, const std::string &);
   void analyze(std::list<sentence> &);
};



/*------------------------------------------------------------------------*/
class senses {
   public:
      /// Constructor
      senses(const std::string &, bool); 
      /// Destructor
      ~senses(); 
 
      /// sense annotate selected analysis for each word in given sentences
      // void analyze(std::list<sentence> &);
      std::list<sentence> analyze(const std::list<sentence> &);
};


/*------------------------------------------------------------------------*/
class disambiguator {
   public:
      /// Constructor
       disambiguator(const std::string &, const std::string &, double, int);
      /// Destructor
      ~disambiguator();

      /// word sense disambiguation for each word in given sentences
      void analyze(std::list<sentence> &);
};


/*------------------------------------------------------------------------*/
class sense_info {
 public:
  /// sense code
  std::string sense;
  /// Part-of-speech
  std::string pos;
  /// hyperonyms
  std::list<std::string> parents;
  /// WN semantic file code
  std::string semfile;
  /// list of synonyms (words in the synset)
  std::list<std::string> words;
  /// list of EWN top ontology properties
  std::list<std::string> tonto;

  /// constructor
  sense_info(const std::string &,const std::string &,const std::string &);
  std::string get_parents_string() const;
};


////////////////////////////////////////////////////////////////
/// Class semanticDB implements a semantic DB interface
////////////////////////////////////////////////////////////////

class semanticDB {
   private:
      /// C++ Interface to BerkeleyDB C API
      Db sensesdb;
      Db wndb;
      /// acces indexed file to find a key
      std::string access_semantic_db(Db &,const std::string &);

   public:
      /// Constructor
      semanticDB(const std::string &, const std::string &);
      /// Destructor
      ~semanticDB();

      /// get list of words for a sense+pos
      std::list<std::string> get_sense_words(const std::string &, const std::string &);
      /// get list of senses for a lemma+pos
      std::list<std::string> get_word_senses(const std::string &, const std::string &);
      /// get sense info for a sensecode+pos
      sense_info get_sense_info(const std::string &, const std::string &);
};

