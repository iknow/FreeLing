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

#ifndef _SUFFIXES
#define _SUFFIXES

#include <string>
#include <set>
#include <map>
#include <vector>

#include "fries/language.h"
#include "freeling/sufrule.h"
#include "freeling/accents.h"

#define SUF 0
#define PREF 1

// just declaring
class dictionary;

////////////////////////////////////////////////////////////////
///  Class suffixes implements suffixation rules and
///  dictionary search for suffixed word forms.
////////////////////////////////////////////////////////////////

class affixes {

   private:
      /// Language-specific accent handler
      accents accen;

      /// all suffixation/prefixation rules
      std::multimap<std::string,sufrule> affix[2];
      /// suffixation/prefixation rules applied unconditionally
      std::multimap<std::string,sufrule> affix_always[2];

      /// index of existing suffix/prefixs lengths.
      std::set<unsigned int> ExistingLength[2];
      /// Length of longest suffix/prefix.
      unsigned int Longest[2];

      /// auxiliary methods to deal with suffixing
      void look_for_affixes_in_list (int, std::multimap<std::string,sufrule> &, word &, dictionary &) const;
      /// auxiliary methods to deal with suffixing
      std::vector<std::string> GenerateRoots(int, const sufrule &, const std::string &) const;
      /// auxiliary methods to deal with suffixing
      void SearchRootsList(const std::vector<std::string> &, const std::string &, sufrule &, word &, dictionary &) const;
      /// auxiliary method to deal with retokenization
      void CheckRetokenizable(const sufrule &, const std::string &, const std::string &, const std::string &, dictionary &, std::list<word> &) const;

   public:
      /// Constructor
      affixes(const std::string &, const std::string &);

      /// look up possible roots of a suffixed/prefixed form
      void look_for_affixes(word &, dictionary &);
};

#endif
