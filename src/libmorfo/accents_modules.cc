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

#include "freeling/accents_modules.h"
#include "fries/util.h"
#include "freeling/traces.h"

#define MOD_TRACENAME "ACCENTS"
#define MOD_TRACECODE SUFF_TRACE

using namespace std;

///////////////////////////////////////////////////////////////
///  Abstract class constructor.
///////////////////////////////////////////////////////////////

accents_module::accents_module()
{}


//---------------------------------------------------------//
//           default (English) accentuation                //
//---------------------------------------------------------//

///////////////////////////////////////////////////////////////
///  Create a default (null) accent handler (eg for English).
///////////////////////////////////////////////////////////////

accents_default::accents_default(): accents_module()
{
    TRACE(3,"created default accent handler ");
}


///////////////////////////////////////////////////////////////
/// default behaviour: Do nothing.
///////////////////////////////////////////////////////////////

void accents_default::fix_accentuation(std::vector<std::string> &candidates, const sufrule &suf) const
{
  TRACE(3,"Default accentuation. Candidates: "+util::int2string(candidates.size()));
}


//---------------------------------------------------------//
//           Spanish-specific accentuation                 //
//---------------------------------------------------------//

///////////////////////////////////////////////////////////////
/// Create an accent handler for Spanish.
///////////////////////////////////////////////////////////////

accents_es::accents_es(): accents_module()
{
    TRACE(3,"created Spanish accent handler ");
}

///////////////////////////////////////////////////////////////
/// specific Spanish behaviour: modify given roots according
/// to Spanish accentuation requirements. Roots are obtanined
/// after suffix removal and may require accent fixing, which
/// is done here.
///////////////////////////////////////////////////////////////

void accents_es::fix_accentuation(std::vector<std::string> &candidates, const sufrule &suf) const
{
  vector<string> roots;
  vector<string>::iterator r;
  string::iterator c;

  TRACE(3,"Number of candidate roots: "+util::int2string(candidates.size()));
  roots.clear();
  for (r=candidates.begin(); r!=candidates.end(); r++) {

    TRACE(3,"We store always the root in roots without any changes ");
    roots.push_back(*r);

    if (suf.enc) {
      TRACE(3,"enclitic suffix. root="+(*r));
      if (llana_acentuada(*r)) {
	TRACE(3,"llana mal acentuada. Remove accents ");
	remove_accent_esp(*r);
      }
      else if (aguda_mal_acentuada(*r)) {
	TRACE(3,"aguda mal acentuada. Remove accents ");
	remove_accent_esp(*r);
      }
      else if (!is_accentued_esp(*r) && !is_monosylabic(*r)) {
	TRACE(3,"No accents, not monosylabic. Add accent to last vowel.");
	c=r->end(); c--;
	if (*c=='n' || *c=='s' || is_vowel_notacc(*c)) {
	  put_accent_esp(*r);
	}
      }
      else if (is_monosylabic(*r) && is_accentued_esp(*r)) {
	TRACE(3,"Monosylabic root, enclitic form accentued. Remove accents");
	remove_accent_esp(*r);
      }
      roots.push_back(*r); // append to roots this element
    }
    else if (suf.last_acc) {
      TRACE(3,"try with an accent in each sylabe ");
      // first remove all accents
      remove_accent_esp(*r);
      // then construct all possible accentued roots
      for (c=r->begin(); c!=r->end(); c++) {
	if (is_vowel(*c)) {
	  put_accent_esp(*c);
	  roots.push_back(*r);
	  remove_accent_esp(*c);
	}
      }
    }

  }

  candidates=roots;
}


////////////////////////////////////////////////////////
// Special Spanish accent handling routines
////////////////////////////////////////////////////////

bool accents_es::is_vowel(char c) {
  return (c=='a' || c=='e' || c=='i' || c=='o' || c=='u' ||
          c=='á' || c=='é' || c=='í' || c=='ó' || c=='ú' ||
          c=='à' || c=='è' || c=='ì' || c=='ò' || c=='ù' ||
          c=='ï' || c=='ü');
}

bool accents_es::is_vowel_notacc(char c) {
  return (c=='a' || c=='e' || c=='i' || c=='o' || c=='u');
}

bool accents_es::is_open(char c) {
  return (c=='a' || c=='e' || c=='o' ||
          c=='á' || c=='é' || c=='ó' ||
	  c=='à' || c=='è' || c=='ò' );
}

bool accents_es::is_accentued_esp(char c) {
  return (c=='á' || c=='é' || c=='í' || c=='ó' || c=='ú');
}

bool accents_es::is_accentued_esp(const std::string &s) {
  string::const_iterator c;

  for (c=s.begin(); c!=s.end(); c++) {
    if (is_accentued_esp(*c)) return true;
  }
  return false;
}

void accents_es::remove_accent_esp(char &c) {
    if (c=='á') c='a';
    else if (c=='é') c='e';
    else if (c=='í') c='i';
    else if (c=='ó') c='o';
    else if (c=='ú') c='u';
}

void accents_es::remove_accent_esp(std::string &s) {
  string::iterator c;
  for(c=s.begin(); c!=s.end(); c++) {
    remove_accent_esp(*c);
  }
}

void accents_es::put_accent_esp(char &c) {
  if (c=='a') c='á';
  else if (c=='e') c='é';
  else if (c=='i') c='í';
  else if (c=='o') c='ó';
  else if (c=='u') c='ú';
}

bool accents_es::put_accent_esp(std::string &s) {
string::iterator c;

  for(c=s.end(),c--; c!=s.begin() && !is_vowel_notacc(*c); c--);

  if (is_vowel_notacc(*c)) {
    put_accent_esp(*c);
    return true;
  }
  else
    // no unaccented vowels found
    return(false);
}

bool accents_es::aguda_mal_acentuada(const std::string &p) {
  string::iterator p1,p2;
  string s;

 // first lowercase string
  s=util::lowercase(p);

  // find last vowel
  for (p1=s.end(), p1--; !is_vowel(*p1) && p1!=s.begin(); p1--);

  // Last vowel found. get following letter, if any
  p2=p1; p2++;
  // accent pattern is wrong if the accented vowel is 
  // not the last letter or it is not followed by a "n" or an "s".
  return (is_accentued_esp(*p1) && p2!=s.end() && (*p2)!='n' && (*p2)!='s' );
}


bool accents_es::llana_acentuada(const std::string &p) {
  string::iterator p1,p2;
  string s;

 // first lowercase string
  s=util::lowercase(p);

  // find last vowel
  for (p1=s.end(), p1--; !is_vowel(*p1) && p1!=s.begin(); p1--);
  // no vowels found, or the only vowel is first letter in word.
  if (p1==s.begin()) return false;

  // Vowel found. get previous letter
  p2=p1; p2--;
  if (is_vowel(*p2)) {
    // Previous is also a vowel. Check for "diptongos"
    if (is_open(*p2) && is_open(*p1))
      // Hiatus (e.g. "cacao") check accent on first vowel of pair
      return (is_accentued_esp(*p2));
    else if (is_accentued_esp(*p2) && !is_open(*p2))
      // broken diptong (e.g. baldío). accent is Ok.
      return false;
    else
      // it is a "diptongo", we have to look for the precedent syllabe
      p2--;
  }
  // else, it is a consonant, check precedent syllabe

  // search precedent syllabe
  for(; !is_vowel(*p2) && p2!=s.begin(); p2--);

  // No more vowels found.
  if (!is_vowel(*p2)) return false;
  // check for "llana acentuada"  (e.g. comído)
  return (is_accentued_esp(*p2));
}

bool accents_es::is_monosylabic(const std::string &p) {
  string::iterator p1,p2;
  string s;

 // first lowercase string
  s=util::lowercase(p);

  // find last vowel
  for (p1=s.end(), p1--; !is_vowel(*p1) && p1!=s.begin(); p1--);
  // no vowels found, or the only vowel is first letter in word.
  if (p1==s.begin()) return false;

  p2=p1; p2--;
  if (is_vowel(*p2)) {
    // Previous is also a vowel. Check for "diptongos"
    if (is_open(*p2) && is_open(*p1))
      // Hiatus (e.g. "cacao"). Polysyllabic
      return false;
    else if (is_accentued_esp(*p2) && !is_open(*p2))
      // broken diptong (e.g. baldío). Polysyllabic
      return false;
    else
      // else, it is a "diptongo", we have to look for the precedent syllabe
      p2--;
  }
  // else, it is a consonant, check precedent syllabe

  // search precedent syllabe
  for(; !is_vowel(*p2) && p2!=s.begin(); p2--);
  // If, no more vowels found, it is monosyllabous
  return (!is_vowel(*p2));
}


