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

// just declaring
class dictionary;

////////////////////////////////////////////////////////////////
///  Class suffixes implements suffixation rules and
///  dictionary search for suffixed word forms.
////////////////////////////////////////////////////////////////

class suffixes {

   private:
      /// Language-specific accent handler
      accents accen;

      /// all suffixation rules
      std::multimap<std::string,sufrule> suffix;
      /// suffixation rules applied unconditionally
      std::multimap<std::string,sufrule> suffix_always;

      /// array of existing suffix lengths.
      std::set<unsigned int> ExistingLength;
      /// Length of longest suffix.
      unsigned int LongestSuf;

      /// auxiliary methods to deal with suffixing
      void look_for_suffixes_in_list (std::multimap<std::string,sufrule> &, word &, dictionary &) const;
      /// auxiliary methods to deal with suffixing
      std::vector<std::string> GenerateRoots(const sufrule &, const std::string &) const;
      /// auxiliary methods to deal with suffixing
      void SearchRootsList(const std::vector<std::string> &, sufrule &, word &, dictionary &) const;
      /// auxiliary method to deal with retokenization
      void CheckRetokenizable(const sufrule &, const std::string &, const std::string &, const std::string &, dictionary &, std::list<word> &) const;

   public:
      /// Constructor
      suffixes(const std::string &, const std::string &);

      /// look up possible roots of a suffixed form
      void look_for_suffixes(word &, dictionary &);
};

#endif
