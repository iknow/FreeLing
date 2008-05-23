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

#include "freeling/dates.h"
#include "fries/util.h"
#include "freeling/traces.h"

using namespace std;

#define MOD_TRACENAME "DATES"
#define MOD_TRACECODE DATES_TRACE

///////////////////////////////////////////////////////////////
///  Create the appropriate dates_module (according to
/// received options), and create a wrapper to access it.
//////////////////////////////////////////////////////////////

dates::dates(const std::string &Lang)
{            

  if (string(Lang)=="es") 
    // Spanish dates handler
    who = new dates_es();
  else if (string(Lang)=="ca") 
    // Catalan dates handler 
    who = new dates_ca();
  else if (string(Lang)=="en")
    // English dates handler
    who = new dates_en();
  else       
    // Default dates handler.
    who = new dates_default();
}            
             
             
///////////////////////////////////////////////////////////////
///  Destroy the wrapper and the wrapped dates_module.
///////////////////////////////////////////////////////////////

dates::~dates()
{            
  delete(who); 
}           

             
///////////////////////////////////////////////////////////////
/// wrapper methods: just call the wrapped dates_module.
///////////////////////////////////////////////////////////////

void dates::annotate(sentence &s) {
  who->annotate(s);
}            
