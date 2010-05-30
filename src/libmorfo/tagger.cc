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

#include "freeling/tagger.h"
#include "freeling/traces.h"

using namespace std;

#define MOD_TRACENAME "TAGGER"
#define MOD_TRACECODE TAGGER_TRACE

///////////////////////////////////////////////////////////////
/// Create an instance of the class, initializing options member.
/// Since tagger is an abstract class, this is called always 
/// from child constructors.
///////////////////////////////////////////////////////////////

POS_tagger::POS_tagger(bool r, unsigned int f) : retok(r),force(f) {};


////////////////////////////////////////////////////////////////
/// Look for words with remaining ambiguity and force the selection
/// of only one Pos tag
////////////////////////////////////////////////////////////////

void POS_tagger::force_select(std::list<sentence> &ls) {
  list<sentence>::iterator se;
  sentence::iterator w;
  word::iterator i;

  for (se=ls.begin(); se!=ls.end(); se++) {
    // process each word
    for (w=se->begin(); w!=se->end(); w++) {
      
      TRACE(2,"Forcing select, word: "+w->get_form());
      
      // unselect all analysis in selected list but one      
      i=w->selected_begin();
      w->unselect_all_analysis();
      w->select_analysis(i);
    }
    
    TRACE_SENTENCE(1,(*se));    
  }
}



////////////////////////////////////////////////////////////////
/// Look for words whose selected tag has retokenizing rules, and
/// retokenize them appropriately
////////////////////////////////////////////////////////////////

void POS_tagger::retokenize(std::list<sentence> &ls) {
  list<sentence>::iterator se;
  unsigned int p,f;
  sentence::iterator w,q;
  list<word>::iterator i;

  for (se=ls.begin(); se!=ls.end(); se++) {
    // process each word
    for (w=se->begin(); w!=se->end(); w++) {
      
      TRACE(2,"word: "+w->get_form());
      // we only retokenize if the word has only one analysis
      if (w->get_n_selected()==1 && w->selected_begin()->is_retokenizable()) {
	
	// retokenize word.
	TRACE(2,"Retokenization found, replacing... "+w->get_form()+". span=("+util::int2string(w->get_span_start())+","+util::int2string(w->get_span_finish())+")");
	
	list<word> lw=w->selected_begin()->get_retokenizable();
	p=w->get_span_start();
	for (i=lw.begin(); i!=lw.end(); i++) {
	  f= ( p+i->get_form().length()>w->get_span_finish() ? 
	       w->get_span_finish() :
	       p+i->get_form().length() );
	  i->set_span(p,f);
	  p = f;  
	  i->user=w->user;
	  
	  TRACE(2,"  Inserting "+i->get_form()+". span=("+util::int2string(i->get_span_start())+","+util::int2string(i->get_span_finish())+")");
	  w=se->insert(w,*i); 
	  w++;
	}
	
	TRACE(2,"  Erasing "+w->get_form());
	q=w; q--;     // save position of previous word
	se->erase(w);  // erase contracted word
	w=q;          // fix iteration control
	
      }
    }
    
    TRACE_SENTENCE(1,(*se)); 
  }
}


