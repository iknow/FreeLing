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

#include "freeling/numbers.h"
#include "fries/util.h"
#include "freeling/traces.h"

using namespace std;

#define MOD_TRACENAME "NUMBERS"
#define MOD_TRACECODE NUMBERS_TRACE

///////////////////////////////////////////////////////////////
///  Create the appropriate numbers_module (according to
/// received options), and create a wrapper to access it.
//////////////////////////////////////////////////////////////

numbers::numbers(const std::string &Lang, const std::string &Decimal, const std::string &Thousand)
{

  if (string(Lang)=="es") 
    // Spanish numbers handler
    who = new numbers_es(Decimal, Thousand);
  else if (string(Lang)=="ca") 
    // Catalan numbers handler 
    who = new numbers_ca(Decimal, Thousand);
  else if (string(Lang)=="gl") 
    // Galician numbers handler 
    who = new numbers_gl(Decimal, Thousand);
  else if (string(Lang)=="it") 
    // Italian numbers handler 
    who = new numbers_it(Decimal, Thousand);
  else if (string(Lang)=="en") 
    // English numbers handler 
    who = new numbers_en(Decimal, Thousand);
  else
    // Default numbers handler.
    who = new numbers_default(Decimal, Thousand);
  
}

///////////////////////////////////////////////////////////////
///  Destroy the wrapper and the wrapped numbers_module.
///////////////////////////////////////////////////////////////

numbers::~numbers()
{
  delete(who);
}

             
///////////////////////////////////////////////////////////////
/// wrapper methods: just call the wrapped numbers_module.
///////////////////////////////////////////////////////////////

void numbers::annotate(sentence &s) {
  who->annotate(s);
}

