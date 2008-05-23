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

#include <fstream>
#include <sstream>

#include "freeling/quantities.h"
#include "fries/util.h"
#include "freeling/traces.h"

using namespace std;

#define MOD_TRACENAME "QUANTITIES"
#define MOD_TRACECODE QUANT_TRACE


///////////////////////////////////////////////////////////////
///  Create the appropriate quantities_module (according to
/// received options), and create a wrapper to access it.
//////////////////////////////////////////////////////////////

quantities::quantities(const std::string &Lang, const std::string &currFile)
{

  if (string(Lang)=="es") 
    // Spanish quantities handler
    who = new quantities_es(currFile);
  else if (string(Lang)=="ca") 
    // Catalan quantities handler 
    who = new quantities_ca(currFile);
  else if (string(Lang)=="gl") 
    // Galician quantities handler 
    who = new quantities_gl(currFile);
  else if (string(Lang)=="en") 
    // Galician quantities handler 
    who = new quantities_en(currFile);
  else 
    // Default quantities handler.
    who = new quantities_default(currFile);  
}


///////////////////////////////////////////////////////////////
///  Destroy the wrapper and the wrapped quantities_module.
///////////////////////////////////////////////////////////////

quantities::~quantities()
{
  delete(who);
}

///////////////////////////////////////////////////////////////
/// wrapper methods: just call the wrapped quantities_module.
///////////////////////////////////////////////////////////////

void quantities::annotate(sentence &s) {
  who->annotate(s);
}
