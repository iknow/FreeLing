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


#ifndef _SUFFRULE
#define _SUFFRULE

#include "regexp-pcre++.h"


/////////////////////////////////////////////
/// Class sufrule contains a suffixation rule,
/// and is used by class suffixes.
/////////////////////////////////////////////

class sufrule {
  public:
    std::string term,output,retok;
    RegEx cond;
    int last_acc,not_acc,lem_form,enc,always;

    sufrule() : cond("") {}
    sufrule(const std::string & c) : cond(c) {}
    sufrule(const sufrule & s) : cond(s.cond) {
      term=s.term; output=s.output; retok=s.retok;
      last_acc=s.last_acc; not_acc=s.not_acc; lem_form=s.lem_form;
      enc=s.enc; always=s.always;
    }
};


#endif
