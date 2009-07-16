#ifndef _DISAMBIG
#define _DISAMBIG

#include <string>
#include <list>

#include "fries/language.h"
#include "freeling/semdb.h"

class disambiguator {
   public:
      /// Constructor
       disambiguator(const std::string &, const std::string &);
      /// Destructor
      ~disambiguator();
      ///removal of trailing -a, -n, -v, or -r from synset code
      std::string convert_synset(std::string);

      /// word sense disambiguation for each word in given sentences
      void analyze(std::list<sentence> &);
};

#endif



