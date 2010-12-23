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
#define MOD_TRACECODE AFF_TRACE

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

void accents_default::fix_accentuation(std::set<std::string> &candidates, const sufrule &suf) const
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

void accents_es::fix_accentuation(set<string> &candidates, const sufrule &suf) const
{
  set<string> roots;
  set<string>::iterator r;
  string::iterator c;
  unsigned int i;

  TRACE(3,"Number of candidate roots: "+util::int2string(candidates.size()));
  roots.clear();
  for (r=candidates.begin(); r!=candidates.end(); r++) {

    string s=(*r);

    TRACE(3,"We store always the root in roots without any changes ");
    roots.insert(s);

    if (suf.enc) {
      TRACE(3,"enclitic suffix. root="+s);
      if (llana_acentuada(s)) {
	TRACE(3,"llana mal acentuada. Remove accents ");
	remove_accent_esp(s);
      }
      else if (aguda_mal_acentuada(s)) {
	TRACE(3,"aguda mal acentuada. Remove accents ");
	remove_accent_esp(s);
      }
      else if (!is_accentued_esp(s) && !is_monosylabic(s)) {
	TRACE(3,"No accents, not monosylabic. Add accent to last vowel.");
	i=s.length()-1;
	if (s[i]=='n' || s[i]=='s' || is_vowel_notacc(s[i])) {
	  put_accent_esp(s);
	}
      }
      else if (is_monosylabic(s) && is_accentued_esp(s)) {
	TRACE(3,"Monosylabic root, enclitic form accentued. Remove accents");
	remove_accent_esp(s);
      }
      roots.insert(s); // append to roots this element
    }
    else if (suf.acc) {
      TRACE(3,"try with an accent in each sylabe ");
      // first remove all accents
      remove_accent_esp(s);
      // then construct all possible accentued roots
      for (i=0; i<s.length(); i++) {
	if (is_vowel(s[i])) {
	  put_accent_esp(s[i]);
	  roots.insert(s);
	  remove_accent_esp(s[i]);
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



//---------------------------------------------------------//
//      Galician-specific accentuation                     //
//---------------------------------------------------------//

///////////////////////////////////////////////////////////////
/// Create an accent handler for Galician.
///////////////////////////////////////////////////////////////

accents_gl::accents_gl(): accents_module()
{
    TRACE(3,"created Galician accent handler ");
}

///////////////////////////////////////////////////////////////
/// specific Galician behaviour: modify given roots according
/// to Galician accentuation requirements. Roots are obtanined
/// after suffix removal and may require accent fixing, which
/// is done here.
///////////////////////////////////////////////////////////////


void accents_gl::fix_accentuation(set<string> &candidates, const sufrule &suf) const
{
  set<string> roots;
  set<string>::iterator r;
  unsigned int i;
  
  // Disambiguates phonological accent for roots with accusative allomorph before cheking if accent is right or wrong in roots (e. g. comelos -> comer + o / cómelos -> comes + o)
  fix_accusative_allomorph_gl(candidates);
   
  TRACE(3,"Number of candidate roots: "+util::int2string(candidates.size()));
  roots.clear();

  for (r=candidates.begin(); r!=candidates.end(); r++) {
   // first lowercase string (is needed here to consider accentued capitalization)
   string s=util::lowercase(*r);
   
   if (suf.enc) { // afixos.dat sixth field	is on
     TRACE(3,"enclitic suffix root="+s);
     if (is_accentued_gl(s)==false) { // not accentued roots
      if (check_add_gl(s)==true){ 
        TRACE(3,"Add accent to second singular and third persons of future indicative ");
        put_accent_gl(s);
      }
     }
     else { // accentued roots
      TRACE(3,"Check if accent is wrong ");
      if (check_accent_gl(s)==false) {
       TRACE(3,"Remove wrong accents ");
       remove_accent_gl(s);
      }
     }
     roots.insert(s); // append to roots this element
   }
   else if (suf.acc) { // afixos.dat fifth field is on
    TRACE(3,"We store always the root in roots without any changes ");
    roots.insert(s);
    TRACE(3,"try with an accent in each syllabe ");
    // first remove all accents
    remove_accent_gl(s);
    // then construct all possible accentued roots
    for (i=0; i<s.length(); i++) {
     if (is_vowel_notacc(s[i])) {
      put_accent_gl(s[i]);
      roots.insert(s);
      remove_accent_gl(s[i]);
     }
    }
   }
  }
  candidates=roots;
}


////////////////////////////////////////////////////////
// Special Galician accent handling routines
////////////////////////////////////////////////////////

bool accents_gl::is_vowel(char c) {
  return (c=='a' || c=='e' || c=='i' || c=='o' || c=='u' ||
          c=='á' || c=='é' || c=='í' || c=='ó' || c=='ú' ||
          c=='à' || c=='è' || c=='ì' || c=='ò' || c=='ù' ||
          c=='ï' || c=='ü');
}

bool accents_gl::is_vowel_notacc(char c) {
  return (c=='a' || c=='e' || c=='i' || c=='o' || c=='u');
}

bool accents_gl::is_open(char c) {
  return (c=='a' || c=='e' || c=='o' ||
          c=='á' || c=='é' || c=='ó' ||
          c=='à' || c=='è' || c=='ò' );
}

bool accents_gl::is_accentued_gl(char c) {
  return (c=='á' || c=='é' || c=='í' || c=='ó' || c=='ú');
}

bool accents_gl::is_accentued_gl(const std::string &s) {
  string::const_iterator c;

  for (c=s.begin(); c!=s.end(); c++) {
    if (is_accentued_gl(*c)) return true;
  }
  return false;
}

void accents_gl::remove_accent_gl(char &c) {
    if (c=='á') c='a';
    else if (c=='é') c='e';
    else if (c=='í') c='i';
    else if (c=='ó') c='o';
    else if (c=='ú') c='u';
}

void accents_gl::remove_accent_gl(std::string &s) {
  string::iterator c;
  for(c=s.begin(); c!=s.end(); c++) {
    remove_accent_gl(*c);
  }
}

void accents_gl::put_accent_gl(char &c) {
  if (c=='a') c='á';
  else if (c=='e') c='é';
  else if (c=='i') c='í';
  else if (c=='o') c='ó';
  else if (c=='u') c='ú';
}

void accents_gl::put_accent_gl(std::string &s) {
  string::iterator p1,p2;

  p2=s.end(); p2--; 
  p1=p2; p1--;

  if ( is_monosyllabic(s)==false && ( is_vowel_notacc(*p2) && (*p2)!='u') ) {
    put_accent_gl(*p2);   
  }
  else if ( is_monosyllabic(s)==false && ( is_vowel_notacc(*p1) && (*p1)!='u' ) && ( (*p2) =='s' || (*p2) =='n' ) ) {
    put_accent_gl(*p1);   
  }
}


bool accents_gl::check_add_gl(const std::string &p) {
  string::iterator p1,p2;
  string s;

  // first lowercase string
  s=util::lowercase(p);

  //Checks second person singular and third person of future forms (accentued when alone)
  p2=s.end(); p2--; 
  p1=p2; p1--;
 
  //Third singular person of indicative future	and some presents (e. g. darache -> dará)
  if ( is_monosyllabic(s)==false && ( is_vowel_notacc(*p2) && (*p2)!='u' ) ) {
    return true;
  }
  //Second singular person and third plural person of future indicative and some presents (e. g. darasme -> darás)
  else if ( is_monosyllabic(s)==false && ( is_vowel_notacc(*p1) && (*p1)!='u' ) && ( (*p2) =='s' || (*p2) =='n' ) ) {
    return true;
  }
  return false;
}

bool accents_gl::check_accent_gl(const std::string &p) {
  string::iterator p0,p1,p2,p3,q0,q1,q2,q3;
  string s;

  // first lowercase string
  s=util::lowercase(p);
 
  // Diacritic accent
  if (s=="cómpre" || s=="cómpren" || s=="dá" || s=="dás" || s=="é" || s=="pór" || s=="sé" || s=="vén" || s=="vés")
    return true;


  //Checks second singular person and third singular and plural persons of future indicative forms for adding accent
  p2=s.end(); p2--; 
  p1=p2; p1--;

  //Third singular person of future and some presents (e. g. daráchemas)
  if ( is_monosyllabic(s)==false && ( (*p2)=='á' || (*p2)=='é' || (*p2)=='í' || (*p2)=='ó' ) )
    return true;
  //Second singular and third plural persons of future and some presents (e. g. darásllela)
  else if ( is_monosyllabic(s)==false && ( (*p1)=='á' || (*p1)=='é' || (*p1)=='í' || (*p1)=='ó' ) && ( (*p2)=='s' || (*p2)=='n' ) ) 
    return true;
  
  //First and second persons plural of past subjunctive (e. g. cantásemola) 
  p1--;
  p3=p1;
  p2=p3; p2--;
  p1=p2; p1--;
  p0=p1; p0--;
      
  if (((*p0)=='á' || (*p0)=='é'  || (*p0)=='í' )  && (*p1)=='s' && (*p2)=='e' && ((*p3)=='m' ||(*p3)=='d')) 
    return true;
    
  //Checks accentued closed vowels 
  for (q2=s.end(); q2!=s.begin(); q2--){
    if (is_accentued_gl(*q2) && !is_open(*q2)) {
    // Accentued closed vowel found. Get previous and next letter
    q3=q2; q3++;
    q1=q2; q1--;
    q0=q1; q0--;
 
      // Next is also a vowel. 
      if (is_accentued_gl(*q2) && !is_open(*q2) && is_vowel_notacc(*q3))
        // Hiatus (e.g. comíalle, perseguíao). Accent is right.
        return true;
      // Previous is also a vowel.
      else if (is_vowel_notacc(*q1) && !is_open(*q2) && is_accentued_gl(*q2)){
        // Prevents digraphs: gu qu +í (e.g. quíxeno perseguíndoas).  Accent is wrong.
        if (((*q0)=='g' || (*q0)=='q' ) && (*q1)=='u' && (*q2)=='í') {
          return false;
        }
        // Hiatus (e.g. roínlle). Accent is right. 
        else return true;
      }
    }     
  }
  // Accent is wrong (e. g. cáelle caéralle tróuxolle atéivolas cantáballe)  
  return false;
}

bool accents_gl::is_monosyllabic(const std::string &p) {
  string::iterator p0,p1,p2,p3;
  string s;
  
 // first lowercase string
  s=util::lowercase(p);

  // find last vowel
  for (p2=s.end(), p2--; !is_vowel(*p2) && p2!=s.begin(); p2--);
  // no vowels found, or the only vowel is first letter in word.
  if (p2==s.begin()) return false;

  p3=p2;p3++;
  p1=p2; p1--;
  p0=p1; p0--;

  if (is_vowel(*p2)) {
    
    // Previous is also a vowel. Check for diphthongs
    if (is_open(*p1) && is_open(*p2)) 
      // Hiatus (e.g. "cacao"). Polysyllabic.
      return false;
    else if (is_accentued_gl(*p2) && !is_open(*p2) && is_vowel_notacc(*p3))
      // Hiatus (e.g. comíalle). Polysyllabic.
      return false;
    else if (is_vowel_notacc(*p1) && !is_open(*p2) && is_accentued_gl(*p2))
      // Hiatus (e.g. roínlle). Polysyllabic. 
      return false;
    else if ( ( ( (*p0)=='q' || (*p0)=='g' )  && (*p1)=='u' && ( (*p2)=='e' || (*p2)=='i' ||(*p2)=='é' ||(*p2)=='í' ) ) || ( is_vowel(*p1) && is_vowel(*p2) && ( ( !is_accentued_gl(*p1) && !is_open(*p1) ) || ( !is_accentued_gl(*p2) && !is_open(*p2) ) ) ) ) {
      for(; !is_vowel(*p0) && p0!=s.begin(); p0--);
        // If, no more vowels found, it is monosyllabous
        return (!is_vowel(*p0));
    }
  }
  for(; !is_vowel(*p1) && p1!=s.begin(); p1--);
    // If, no more vowels found, it is monosyllabous
    return (!is_vowel(*p1));
}

void accents_gl::fix_accusative_allomorph_gl(set<string> &s) {
  set<string>::iterator r1, r2;
  for (r2=s.end(),r2--; r2!=s.begin()++; r2--) {
    r1=r2; r1--;
    string root1=util::lowercase(*r1),root2=util::lowercase(*r2);
    string::iterator k0,k1,l0,l1;
    k1=root1.end(); k1--;
    k0=k1; k0--;
    l1=root2.end(); l1--;
    l0=l1; l0--;
    // Compares actual candidate and next candidate to disambiguate comelo / cómelo
    if ((*k1)== 'r' && (*l1)=='s' && ((*k0)=='a' || (*k0)=='e' || (*k0)=='o') && (*k0)==(*l0) && root1.length()==root2.length()){
      if (is_accentued_gl(root1)==true  && is_accentued_gl(root2)==true){
        // It removes -r candidate (infinitive or future subjunctive: '*cómelo' is not 'comer' + 'o' ['cómelo' = 'comes' + 'o']) 
        s.erase (r1);          
      }
      if (is_accentued_gl(root1)==false && is_accentued_gl(root2)==false) {
        if (check_add_gl(root2)==true) {
          // It changes -s candidate to oxytone one: 'prevelo' = 'prevés' (second singular person  of present indicative) + 'o' || 'prever' (infinitive) + 'o' && '*comelo' is not 'comes' {'*comés' is not into de dictionary} + 'o' ['comelo' = 'comer' + 'o'])
          put_accent_gl(root2);
          s.insert(r2, root2);
        }
      }
    }
  }
}

