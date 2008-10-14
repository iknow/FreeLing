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

#ifndef _NP
#define _NP

#include <set>

#include "fries/language.h"
#include "freeling/automat.h"
#include "regexp-pcre++.h"

#define RE_NA "^(NC|AQ)"
#define RE_DNP  "^[FWZ]"
#define RE_CLO  "^[DSC]"

////////////////////////////////////////////////////////////////
///  The class np implements a dummy proper noun recognizer.
////////////////////////////////////////////////////////////////

class np: public automat {
   private: 
      /// set of function words
      std::set<std::string> func;
      /// set of special punctuation tags
      std::set<std::string> punct;
      /// set of words to be considered possible NPs at sentence beggining
      std::set<std::string> names;
      /// set of words to be ignored as NE parts, even if they are capitalized
      std::set<std::string> ignore;
      /// Tag to assign to detected NEs
      std::string NE_tag;
      /// it is a noun at the beggining of the sentence
      bool initialNoun;
      /// length beyond which a multiword made of all capitialized words ("WRECKAGE: TITANIC 
      /// DISAPPEARS IN NORTHERN SEA") will be considered a title and not a proper noun.
      /// A value of zero deactivates this behaviour.
      unsigned int Title_length;

      bool splitNPs;

      RegEx RE_NounAdj;
      RegEx RE_Closed;
      RegEx RE_DateNumPunct;

      int ComputeToken(int,sentence::iterator &, sentence &);
      void ResetActions();
      void StateActions(int, int, int, sentence::const_iterator);
      void SetMultiwordAnalysis(sentence::iterator, int);
      bool ValidMultiWord(const word &);

   public:
      /// Constructor
      np(const std::string &); 
};

#endif

