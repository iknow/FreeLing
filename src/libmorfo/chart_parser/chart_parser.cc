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

#include "freeling/chart_parser.h"
#include "fries/util.h"
#include "freeling/traces.h"

using namespace std;

#define MOD_TRACENAME "CHART_PARSER"
#define MOD_TRACECODE CHART_TRACE


////////////////////////////////////////////////////////////////
/// Constructor.
////////////////////////////////////////////////////////////////

chart_parser::chart_parser(const string &gramFile): ch(), gram(gramFile) {

  /// set given grammar as the active grammar for the chart.
  ch.set_grammar(gram);

  TRACE(3,"Analyzer successfully created.");
}


////////////////////////////////////////////////////////////////
/// query parser about start symbol of current grammar
////////////////////////////////////////////////////////////////

string chart_parser::get_start_symbol(void) const {
 return gram.get_start_symbol();
}


////////////////////////////////////////////////////////////////
/// analyze sentences with specific options.
////////////////////////////////////////////////////////////////

void chart_parser::analyze(list<sentence> &ls) {
  list<sentence>::iterator s; 
  sentence::iterator w;
  parse_tree tr;
  parse_tree::preorder_iterator n;

  for (s=ls.begin(); s!=ls.end(); s++) {
    ch.load_sentence(*s);
    ch.parse();
    TRACE(3," Sentence parsed.");

    // navigate through the chart and obtain a parse tree for the sentence
    tr=ch.get_tree(ch.get_size()-1,0);
    // associate leaf nodes in the tree with the right word in the sentence:
    w=s->begin();
    for (n=tr.begin(); n!=tr.end(); ++n) {
      TRACE(3," Completing tree: "+n->info.get_label()+" children:"+util::int2string(n->num_children()));
      if (n->num_children()==0) {
	n->info.set_word(*w);
	n->info.set_label(w->get_parole());
	w++;
      }
    }
    // include the tree in the sentence object
    s->set_parse_tree(tr);
  }
}


////////////////////////////////////////////////////////////////
/// analyze sentences, return analyzed copy (useful for Perl API).
////////////////////////////////////////////////////////////////

list<sentence> chart_parser::analyze(const list<sentence> &ls) {
  list<sentence> x;

  x = ls;
  analyze(x);
  return(x);
}

