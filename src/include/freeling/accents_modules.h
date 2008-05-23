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

#ifndef _ACCENTS_MOD
#define _ACCENTS_MOD

#include <string>
#include <vector>

#include "freeling/sufrule.h"

////////////////////////////////////////////////////////////////
///
///  The abstract class accents_module generalizes accentuation
/// rules for different languages.
///
////////////////////////////////////////////////////////////////

class accents_module {

   public:
      /// Constructor
      accents_module();
      /// Specific accentuation patterns
      virtual void fix_accentuation(std::vector<std::string> &, const sufrule &) const =0;
      /// Destructor
      virtual ~accents_module() {};

};


////////////////////////////////////////////////////////////////
///
///  Derived accents_module for null accentuation (eg english).
///
////////////////////////////////////////////////////////////////

class accents_default: public accents_module {

   public:
      /// Constructor
      accents_default();

      /// default accentuation patterns
      void fix_accentuation(std::vector<std::string> &, const sufrule &) const;
};


////////////////////////////////////////////////////////////////
///
///  Derived accents_module for Spanish accentuation.
///
////////////////////////////////////////////////////////////////

class accents_es: public accents_module {

   private:
      /// check for a vowel (maybe with latin accents)
      static bool is_vowel(char);
      /// check for a vowel (strictly)
      static bool is_vowel_notacc(char);
      /// check monosyllabous word
      static bool is_monosylabic(const std::string &);
      /// check for weak spanish vowels
      static bool is_open(char);
      /// check for spanish accents
      static bool is_accentued_esp(char);
      /// check for spanish accents
      static bool is_accentued_esp(const std::string &);
      /// remove spanish accents
      static void remove_accent_esp(std::string &);
      /// remove spanish accents
      static void remove_accent_esp(char &);
      /// set spanish accents
      static void put_accent_esp(char &);
      /// set spanish accents
      static bool put_accent_esp(std::string &);
      /// check for spanish accent pattern: "palabras llanas acentuadas"
      static bool llana_acentuada(const std::string &);
      /// check for spanish accent pattern: "palabras agudas mal acentuadas"
      static bool aguda_mal_acentuada(const std::string &);

   public:
      /// Constructor
      accents_es();

      /// Specific accentuation patterns for Spanish
      void fix_accentuation(std::vector<std::string> &, const sufrule &) const;
};


#endif
