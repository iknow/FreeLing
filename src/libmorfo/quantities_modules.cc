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

#include "freeling/quantities_modules.h"
#include "fries/util.h"
#include "freeling/traces.h"

using namespace std;

#define MOD_TRACENAME "QUANTITIES"
#define MOD_TRACECODE QUANT_TRACE



///////////////////////////////////////////////////////////////
///  Abstract class constructor.
///////////////////////////////////////////////////////////////

quantities_module::quantities_module(): automat(), measures("")
{}


//-----------------------------------------------//
//   Default currency and ratio recognizer.      //
//   Only recognize simple stuff such as "20%"   //
//-----------------------------------------------//

// State codes 
// states for a currency amount, percentage or ratio expression
#define A 1 // initial state
#define B 2 // got a number (a quantity)
#define C 3 // got "20 %"  (VALID PERCENTAGE)
// stop state
#define STOP 4

// Token codes
#define TK_number  1 // number
#define TK_pc      2 // sign "%"
#define TK_other   3

///////////////////////////////////////////////////////////////
///  Create a default quantities recognizer.
///////////////////////////////////////////////////////////////

quantities_default::quantities_default(const std::string &CurrFile): quantities_module()
{
  // Initializing tokens hash
  tok.insert(make_pair("%",TK_pc));

  // Initialize special state attributes
  initialState=A; stopState=STOP;

  // Initialize Final state set
  Final.insert(C); 

  // Initialize transitions table. By default, stop state
  int s,t;
  for(s=0;s<MAX_STATES;s++) for(t=0;t<MAX_TOKENS;t++) trans[s][t]=STOP;
  
  // State A
  trans[A][TK_number]=B;
  // State B
  trans[B][TK_pc]=C;  
  // State C
  // nothing else expected from here.

  TRACE(3,"analyzer succesfully created");
}


//-- Implementation of virtual functions from class automat --//

///////////////////////////////////////////////////////////////
///  Compute the right token code for word j from given state.
///////////////////////////////////////////////////////////////

int quantities_default::ComputeToken(int state, sentence::iterator &j, sentence &se)
{
  string form,name,code;
  int token;
  map<string,int>::iterator im;
 
  form = util::lowercase(j->get_form());
  
  token = TK_other;
  im = tok.find(form);
  if (im!=tok.end()) {
    token = (*im).second;
  }
  
  TRACE(3,"Next word form is: ["+form+"] token="+util::int2string(token));     
  // if the token was in the table, we're done
  if (token != TK_other) return (token);

  // Token not found in translation table, let's have a closer look.

  // check to see if it is a number
  if (j->get_n_analysis() && j->get_parole()=="Z") {
    token = TK_number;
  }

  return(token);
}

///////////////////////////////////////////////////////////////
///   Reset acumulators used by state actions:
///   value1, value2 (for ratios).
///////////////////////////////////////////////////////////////

void quantities_default::ResetActions() 
{
  value1=0; value2=0;
}


///////////////////////////////////////////////////////////////
///  Perform necessary actions in "state" reached from state 
///  "origin" via word j interpreted as code "token":
///  Basically, when reaching a state with an informative 
//   token (number, currency name) store the relevant info.
///////////////////////////////////////////////////////////////

void quantities_default::StateActions(int origin, int state, int token, sentence::const_iterator j)
{
  string form,name,code;
  long double value;

  form = util::lowercase(j->get_form());
  TRACE(3,"Reaching state "+util::int2string(state)+" with token "+util::int2string(token)+" for word ["+form+"]");

  // get token numerical value, if any
  value=0;
  if (token==TK_number &&  j->get_n_analysis() && j->get_parole()=="Z") {
    value = util::string2longdouble(j->get_lemma());
  }

  // State actions
  switch (state) {
  // ---------------------------------
  case B:
    // a nummerical value has been found. Store it just in case.
    TRACE(3,"Actions for state B");
    value1=value;
    break;
  // ---------------------------------
  case C:
    // complete ratio/percentage found
    TRACE(3,"Actions for state C");
    if (token==TK_pc)  
      // percent sign "%", second value of ratio is 100
      value2=100;
    break;
  // ---------------------------------
  default: break;
  }
}


///////////////////////////////////////////////////////////////
///   Set the appropriate lemma and parole for the 
///   new multiword.
///////////////////////////////////////////////////////////////

void quantities_default::SetMultiwordAnalysis(sentence::iterator i, int fstate)
{
  string lemma,tag;

  // Setting the lemma 
  lemma=util::longdouble2string(value1)+"/"+util::longdouble2string(value2);
  tag="Zp";

  i->set_analysis(analysis(lemma,tag));   
  TRACE(3,"Analysis set to: "+lemma+" "+tag);
}


// undef literals to prepare for next module definition
#undef A
#undef B
#undef C
#undef STOP
#undef TK_number
#undef TK_pc
#undef TK_other


//-----------------------------------------------------------//
//   Spanish currency and ratio recognizer.      //
//   The same structure is used, different lexical units     //
//   obtained from configuration files, or mixed in the data //
//-----------------------------------------------------------//

// State codes 
// states for a currency amount, percentage or ratio expression
#define A 1 // initial state
#define B 2 // got a number (a quantity)
#define C 3 // got "20 %", "20 por ciento", "2 de cada 3", "4 sobre 5", "2 tercios"  (VALID PERCENTAGE)
#define D 4 // got "por" after a number 
#define E 5 // got "de" after a number
#define F 6 // got "de cada" after a number
#define G 7 // got a measure unit or a currency name after a number
// stop state
#define STOP 8

// Token codes
#define TK_number   1 // number
#define TK_n100     2 // number with value 100 or 1000 
#define TK_pc       3 // sign "%"
#define TK_wde      4 // word "de"
#define TK_wcada    5 // word "cada"
#define TK_wpor     6 // word "por"
#define TK_wsobre   7 // word "sobre" "entre"
#define TK_avo      8 // fraction word (tercio, veinteavo, milesima)
#define TK_part     9 // word "parte" or "partes"
#define TK_unit    10 // measure unit or currency name
#define TK_other   11

///////////////////////////////////////////////////////////////
///  Create a quantities recognizer for Spanish.
///////////////////////////////////////////////////////////////

quantities_es::quantities_es(const std::string &QuantFile): quantities_module()
{  
  // Initializing tokens hash
  tok.insert(make_pair("de",TK_wde));      
  tok.insert(make_pair("cada",TK_wcada));  
  tok.insert(make_pair("por",TK_wpor));    
  tok.insert(make_pair("sobre",TK_wsobre));
  tok.insert(make_pair("entre",TK_wsobre));
  tok.insert(make_pair("%",TK_pc));        
  tok.insert(make_pair("parte",TK_part));  
  tok.insert(make_pair("partes",TK_part)); 

  // es
  fract.insert(make_pair("medio",2));
  fract.insert(make_pair("tercio",3));
  fract.insert(make_pair("tercero",3));
  fract.insert(make_pair("cuarto",4));
  fract.insert(make_pair("quinto",5));
  fract.insert(make_pair("sexto",6));  fract.insert(make_pair("seisavo",6));
  fract.insert(make_pair("séptimo",7));
  fract.insert(make_pair("octavo",8));  fract.insert(make_pair("ochavo",8));
  fract.insert(make_pair("noveno",9));

  fract.insert(make_pair("décimo",10));         fract.insert(make_pair("veinteavo",20));
  fract.insert(make_pair("onceavo",11));        fract.insert(make_pair("veintiunavo",21));
  fract.insert(make_pair("doceavo",12));        fract.insert(make_pair("veintidosavo",22));
  fract.insert(make_pair("treceavo",13));       fract.insert(make_pair("veintitresavo",23));
  fract.insert(make_pair("catorceavo",14));     fract.insert(make_pair("veinticuatroavo",24));
  fract.insert(make_pair("quinceavo",15));      fract.insert(make_pair("veinticincoavo",25));
  fract.insert(make_pair("deciseisavo",16));    fract.insert(make_pair("veintiseisavo",26));
  fract.insert(make_pair("diecisieteavo",17));  fract.insert(make_pair("veintisieteavo",27));
  fract.insert(make_pair("dieciochoavo",18));   fract.insert(make_pair("veintiochoavo",28));
  fract.insert(make_pair("diecinueveavo",19));  fract.insert(make_pair("veintinueveavo",29));

  fract.insert(make_pair("treintavo",30));         fract.insert(make_pair("cuarentavo",40));
  fract.insert(make_pair("treintaiunavo",31));     fract.insert(make_pair("cuarentaiunavo",41));
  fract.insert(make_pair("treintaidosavo",32));    fract.insert(make_pair("cuarentaidosavo",42));
  fract.insert(make_pair("treintaitresavo",33));   fract.insert(make_pair("cuarentaitresavo",43));
  fract.insert(make_pair("treintaicuatroavo",34)); fract.insert(make_pair("cuarentaicuatroavo",44));
  fract.insert(make_pair("treintaicincoavo",35));  fract.insert(make_pair("cuarentaicincoavo",45));
  fract.insert(make_pair("treintaiseisavo",36));   fract.insert(make_pair("cuarentaiseisavo",46));
  fract.insert(make_pair("treintaisieteavo",37));  fract.insert(make_pair("cuarentaisieteavo",47));
  fract.insert(make_pair("treintaiochoavo",38));   fract.insert(make_pair("cuarentaiochoavo",48));
  fract.insert(make_pair("treintainueveavo",39));  fract.insert(make_pair("cuarentainueveavo",49));

  fract.insert(make_pair("cincuentavo",50));         fract.insert(make_pair("sesentavo",60));
  fract.insert(make_pair("cincuentaiunavo",51));     fract.insert(make_pair("sesentaiunavo",61));
  fract.insert(make_pair("cincuentaidosavo",52));    fract.insert(make_pair("sesentaidosavo",62));
  fract.insert(make_pair("cincuentaitresavo",53));   fract.insert(make_pair("sesentaitresavo",63));
  fract.insert(make_pair("cincuentaicuatroavo",54)); fract.insert(make_pair("sesentaicuatroavo",64));
  fract.insert(make_pair("cincuentaicincoavo",55));  fract.insert(make_pair("sesentaicincoavo",65));
  fract.insert(make_pair("cincuentaiseisavo",56));   fract.insert(make_pair("sesentaiseisavo",66));
  fract.insert(make_pair("cincuentaisieteavo",57));  fract.insert(make_pair("sesentaisieteavo",67));
  fract.insert(make_pair("cincuentaiochoavo",58));   fract.insert(make_pair("sesentaiochoavo",68));
  fract.insert(make_pair("cincuentainueveavo",59));  fract.insert(make_pair("sesentainueveavo",69));

  fract.insert(make_pair("setentavo",70));          fract.insert(make_pair("ochentavo",80));
  fract.insert(make_pair("setentaiunavo",71));      fract.insert(make_pair("ochentaiunavo",81));
  fract.insert(make_pair("setentaidosavo",72));     fract.insert(make_pair("ochentaidosavo",82));
  fract.insert(make_pair("setentaitresavo",73));    fract.insert(make_pair("ochentaitresavo",83));
  fract.insert(make_pair("setentaicuatroavo",74));  fract.insert(make_pair("ochentaicuatroavo",84));
  fract.insert(make_pair("setentaicincoavo",75));   fract.insert(make_pair("ochentaicincoavo",85));
  fract.insert(make_pair("setentaiseisavo",76));    fract.insert(make_pair("ochentaiseisavo",86));
  fract.insert(make_pair("setentaisieteavo",77));   fract.insert(make_pair("ochentaisieteavo",87));
  fract.insert(make_pair("setentaiochoavo",78));    fract.insert(make_pair("ochentaiochoavo",88));
  fract.insert(make_pair("setentainueveavo",79));   fract.insert(make_pair("ochentainueveavo",89));

  fract.insert(make_pair("noventavo",90));          fract.insert(make_pair("centésimo",100));
  fract.insert(make_pair("noventaiunavo",91));      fract.insert(make_pair("milésimo",1000));
  fract.insert(make_pair("noventaidosavo",92));     fract.insert(make_pair("diezmilésimo",10000));
  fract.insert(make_pair("noventaitresavo",93));    fract.insert(make_pair("cienmilésimo",100000));
  fract.insert(make_pair("noventaicuatroavo",94));  fract.insert(make_pair("millonésimo",1000000));
  fract.insert(make_pair("noventaicincoavo",95));   fract.insert(make_pair("diezmillonésimo",10000000));
  fract.insert(make_pair("noventaiseisavo",96));    fract.insert(make_pair("cienmillonésimo",100000000));
  fract.insert(make_pair("noventaisieteavo",97));   fract.insert(make_pair("milmillonésimo",1000000000));
  fract.insert(make_pair("noventaiochoavo",98));    
  fract.insert(make_pair("noventainueveavo",99));   
                                                    

  fract.insert(make_pair("doscientosavo",200));
  fract.insert(make_pair("trescientosavo",300));
  fract.insert(make_pair("cuatrocientosavo",400));
  fract.insert(make_pair("quinientosavo",500));
  fract.insert(make_pair("seiscientosavo",600));
  fract.insert(make_pair("sietecientosavo",700));
  fract.insert(make_pair("ochocientosavo",800));
  fract.insert(make_pair("novecientosavo",900));

  // Initialize special state attributes
  initialState=A; stopState=STOP;

  // Initialize Final state set 
  Final.insert(C);  Final.insert(G); 

  // Initialize transitions table. By default, stop state
  int s,t;
  for(s=0;s<MAX_STATES;s++) for(t=0;t<MAX_TOKENS;t++) trans[s][t]=STOP;
  
  // State A
  trans[A][TK_number]=B;
  // State B
  trans[B][TK_pc]=C;      trans[B][TK_wpor]=D;  trans[B][TK_wde]=E;
  trans[B][TK_wsobre]=F;  trans[B][TK_avo]=C;   trans[B][TK_unit]=G;
  // State C
  trans[C][TK_part]=C;
  // State D
  trans[D][TK_n100]=C;
  // State E
  trans[E][TK_wcada]=F;  trans[E][TK_unit]=G;
  // State F
  trans[F][TK_number]=C;
  // State G
  // nothing else expected from here.

  // opening quantities config file
  ifstream fabr (QuantFile.c_str());
  if (!fabr) { 
    ERROR_CRASH("Error opening currency file "+QuantFile);
  }

  // load maps in config file
  string line;
  int reading=0;
  while (getline(fabr,line)) {

    istringstream sin;
    sin.str(line);
    
    if (line == "<Currency>") reading=1;
    else if (line == "</Currency>") reading=0;
    else if (line == "<Measure>") reading=2;
    else if (line == "</Measure>") reading=0;
    else if (line == "<MeasureNames>") reading=3;
    else if (line == "</MeasureNames>") reading=0;
    else if (reading==1) {
      // reading currency lines
      currency_key = line;
    }
    else if (reading==2) {
      // reading Measure units
      string key,unit;
      sin>>unit>>key;
      units.insert(make_pair(key,unit));
    }
    else if (reading==3) {
      // reading Measure units
      measures.add_locution(line);
    }
  }
  fabr.close(); 


  TRACE(3,"analyzer succesfully created");
}


//-- Implementation of virtual functions from class automat --//

///////////////////////////////////////////////////////////////
///  Compute the right token code for word j from given state.
///////////////////////////////////////////////////////////////

int quantities_es::ComputeToken(int state, sentence::iterator &j, sentence &se)
{
  string form,lema,name,code;
  int token,value;
  map<string,int>::const_iterator im;
 
  form = util::lowercase(j->get_form());
  lema = j->get_lemma();
  
  token = TK_other;
  im = tok.find(form);
  if (im!=tok.end()) {
    token = (*im).second;
  }
  
  TRACE(3,"Next word form is: ["+form+"] token="+util::int2string(token)); 
  // if the token was in the table, we're done
  if (token != TK_other) return (token);

  // Token not found in translation table, let's have a closer look.

  // check to see if it is a number
  if (j->get_n_analysis() && j->get_parole()=="Z") {
    token = TK_number;
  }
  TRACE(3,"checked");

  switch (state) {
  // ---------------------------------
  case B:
    // in state B a currency name, a measure unit, or a fraction may come
    if (measures.annotate(se,j))
      token=TK_unit;
    else {
      // look if any lemma is found in "fract" map
      word::iterator a;
      for (a=j->begin(); a!=j->end() && fract.find(a->get_lemma())==fract.end(); a++);
      // if any, consider this a fraction word (fifth, eleventh, ...) 
      if (a!=j->end()) {
	j->unselect_all_analysis();
	j->select_analysis(a);
	token=TK_avo;
      }
    }
    break;
  case E: 
    // in state E a currency name or a measure unit may come
    if (measures.annotate(se,j))
      token=TK_unit;
    break;
  // ---------------------------------
  case D:
    // in state D only 100 and 1000 are allowed
    if (token==TK_number) {
      value = util::string2int(lema);
      if (value==100 || value==1000)
	token=TK_n100;
    }
    break;
  // ---------------------------------
  default: break;
  }

  return(token);
}


///////////////////////////////////////////////////////////////
///   Reset acumulators used by state actions:
///   value1, value2 (for ratios).
///////////////////////////////////////////////////////////////

void quantities_es::ResetActions() 
{
  value1=0; value2=0;
  unitCode="";
  unitType="";
}


///////////////////////////////////////////////////////////////
///  Perform necessary actions in "state" reached from state 
///  "origin" via word j interpreted as code "token":
///  Basically, when reaching a state with an informative 
//   token (number, currency name) store the relevant info.
///////////////////////////////////////////////////////////////

void quantities_es::StateActions(int origin, int state, int token, sentence::const_iterator j)
{
  string form,lema,name,code;
  long double value;

  form = util::lowercase(j->get_form());
  lema = j->get_lemma();
  TRACE(3,"Reaching state "+util::int2string(state)+" with token "+util::int2string(token)+" for word ("+form+","+lema+")");

  // get token numerical value, if any
  value=0;
  if ((token==TK_number || token==TK_n100) && 
       j->get_n_analysis() && j->get_parole()=="Z") {
    value = util::string2longdouble(lema);
  }

  // State actions
  switch (state) {
  // ---------------------------------
  case B:
    // a nummerical value has been found. Store it just in case.
    TRACE(3,"Actions for state B");
    value1=value;
    break;
  // ---------------------------------
  case C:
    // complete ratio/percentage found
    TRACE(3,"Actions for state C");
    if (token==TK_pc)
      // percent sign "%", second value of ratio is 100
      value2=100;
    else if (token==TK_avo)
      // fraction word (e.g. "two thirds", "dos tercios"), second value is value of second word
      value2=fract[lema];
    else if (token!=TK_part)
      // normal ratio "3 de cada 4" "5 por mil", store second value.
      value2=value;
    break;
  // ---------------------------------
  case G:
    // number + measure unit (or currency name) found, store magnitude and unit
    TRACE(3,"Actions for state I");
    unitCode=units[lema]+"_"+lema;
    unitType=units[lema];
    break;
  // ---------------------------------
  default: break;
  }
}


///////////////////////////////////////////////////////////////
///   Set the appropriate lemma and parole for the 
///   new multiword.
///////////////////////////////////////////////////////////////

void quantities_es::SetMultiwordAnalysis(sentence::iterator i, int fstate)
{
  string lemma,tag,name,code;

  // Setting the lemma 
  if (unitType==currency_key) {
    lemma=unitCode+":"+util::longdouble2string(value1);
    tag="Zm";
  }
  else if (unitCode!="") {
    // it's a quantity + measure unit
    lemma=unitCode+":"+util::longdouble2string(value1);
    tag="Zu";
  }
  else {
    // it's a ratio
    lemma=util::longdouble2string(value1)+"/"+util::longdouble2string(value2);
    tag="Zp";
  }

  i->set_analysis(analysis(lemma,tag));   
  TRACE(3,"Analysis set to: "+lemma+" "+tag);
}


// undef literals to prepare for next module definition

#undef A 
#undef B 
#undef C 
#undef D 
#undef E
#undef F
#undef G
#undef STOP
#undef TK_number
#undef TK_n100
#undef TK_pc
#undef TK_wde
#undef TK_wcada
#undef TK_wpor
#undef TK_wsobre
#undef TK_avo
#undef TK_part
#undef TK_unit
#undef TK_other



//-----------------------------------------------------------//
//   Catalan currency and ratio recognizer.      //
//   The same structure is used, different lexical units     //
//   obtained from configuration files, or mixed in the data //
//-----------------------------------------------------------//

// State codes 
// states for a currency amount, percentage or ratio expression
#define A 1 // initial state
#define B 2 // got a number (a quantity)
#define C 3 // got "20 %", "20 por ciento", "2 de cada 3", "4 sobre 5", "2 tercios"  (VALID PERCENTAGE)
#define D 4 // got "por" after a number 
#define E 5 // got "de" after a number
#define F 6 // got "de cada" after a number
#define G 7 // got a measure unit or a currency name after a number
// stop state
#define STOP 8

// Token codes
#define TK_number   1 // number
#define TK_n100     2 // number with value 100 or 1000 
#define TK_pc       3 // sign "%"
#define TK_wde      4 // word "de"
#define TK_wcada    5 // word "cada"
#define TK_wpor     6 // word "por"
#define TK_wsobre   7 // word "sobre" "entre"
#define TK_avo      8 // fraction word (tercio, veinteavo, milesima)
#define TK_part     9 // word "parte" or "partes"
#define TK_unit    10 // measure unit or currency name
#define TK_other   11

///////////////////////////////////////////////////////////////
///  Create a quantities recognizer for Catalan.
///////////////////////////////////////////////////////////////

quantities_ca::quantities_ca(const std::string &QuantFile): quantities_module()
{  
  // Initializing tokens hash
  tok.insert(make_pair("de",TK_wde));     
  tok.insert(make_pair("cada",TK_wcada)); 
  tok.insert(make_pair("per",TK_wpor));   
  tok.insert(make_pair("sobre",TK_wsobre));
  tok.insert(make_pair("entre",TK_wsobre));
  tok.insert(make_pair("%",TK_pc));        
  tok.insert(make_pair("part",TK_part));   
  tok.insert(make_pair("parts",TK_part));  

  // ca
  fract.insert(make_pair("mig",2));
  fract.insert(make_pair("terç",3));
  fract.insert(make_pair("quart",4));
  fract.insert(make_pair("cinquè",5));
  fract.insert(make_pair("sisè",6));
  fract.insert(make_pair("setè",7));
  fract.insert(make_pair("vuitè",8));
  fract.insert(make_pair("novè",9));

  fract.insert(make_pair("desè",10));      fract.insert(make_pair("vintè",20));
  fract.insert(make_pair("onzè",11));      fract.insert(make_pair("vint-i-unè",21));
  fract.insert(make_pair("dotzè",12));     fract.insert(make_pair("vint-i-dosè",22));
  fract.insert(make_pair("tretzè",13));    fract.insert(make_pair("vint-i-tresè",23));
  fract.insert(make_pair("catorzè",14));   fract.insert(make_pair("vint-i-quatrè",24));
  fract.insert(make_pair("quinzè",15));    fract.insert(make_pair("vint-i-cinquè",25));
  fract.insert(make_pair("setzè",15));     fract.insert(make_pair("vint-i-sisè",26));
  fract.insert(make_pair("dissetè",17));   fract.insert(make_pair("vint-i-setè",27));
  fract.insert(make_pair("divuitè",18));   fract.insert(make_pair("vint-i-vuitè",28));
  fract.insert(make_pair("dinovè",19));    fract.insert(make_pair("vint-i-novè",29));

  fract.insert(make_pair("trentè",30));         fract.insert(make_pair("quarantè",40));
  fract.insert(make_pair("trenta-unè",31));     fract.insert(make_pair("quaranta-unè",41));
  fract.insert(make_pair("trenta-dosè",32));    fract.insert(make_pair("quaranta-dosè",42));
  fract.insert(make_pair("trenta-tresè",33));   fract.insert(make_pair("quaranta-tresè",43));
  fract.insert(make_pair("trenta-cuatrè",34));  fract.insert(make_pair("quaranta-quatrè",44));
  fract.insert(make_pair("trenta-cinquèo",35)); fract.insert(make_pair("quaranta-cinquè",45));
  fract.insert(make_pair("trenta-sisè",36));    fract.insert(make_pair("quaranta-sisè",46));
  fract.insert(make_pair("trenta-setè",37));    fract.insert(make_pair("quaranta-setè",47));
  fract.insert(make_pair("trenta-vuitè",38));   fract.insert(make_pair("quaranta-vuitè",48));
  fract.insert(make_pair("trenta-novè",39));    fract.insert(make_pair("quaranta-novè",49));

  fract.insert(make_pair("cinquantè",50));         fract.insert(make_pair("seixantè",60));
  fract.insert(make_pair("cinquanta-unè",51));     fract.insert(make_pair("seixanta-unè",61));
  fract.insert(make_pair("cinquanta-dosè",52));    fract.insert(make_pair("seixanta-dosè",62));
  fract.insert(make_pair("cinquanta-tresè",53));   fract.insert(make_pair("seixanta-tresè",63));
  fract.insert(make_pair("cinquanta-cuatrè",54));  fract.insert(make_pair("seixanta-quatrè",64));
  fract.insert(make_pair("cinquanta-cinquèo",55)); fract.insert(make_pair("seixanta-cinquè",65));
  fract.insert(make_pair("cinquanta-sisè",56));    fract.insert(make_pair("seixanta-sisè",66));
  fract.insert(make_pair("cinquanta-setè",57));    fract.insert(make_pair("seixanta-setè",67));
  fract.insert(make_pair("cinquanta-vuitè",58));   fract.insert(make_pair("seixanta-vuitè",68));
  fract.insert(make_pair("cinquanta-novè",59));    fract.insert(make_pair("seixanta-novè",69));
 
  fract.insert(make_pair("setantè",70));         fract.insert(make_pair("vuitantè",80));
  fract.insert(make_pair("setanta-unè",71));     fract.insert(make_pair("vuitanta-unè",81));
  fract.insert(make_pair("setanta-dosè",72));    fract.insert(make_pair("vuitanta-dosè",82));
  fract.insert(make_pair("setanta-tresè",73));   fract.insert(make_pair("vuitanta-tresè",83));
  fract.insert(make_pair("setanta-cuatrè",74));  fract.insert(make_pair("vuitanta-quatrè",84));
  fract.insert(make_pair("setanta-cinquèo",75)); fract.insert(make_pair("vuitanta-cinquè",85));
  fract.insert(make_pair("setanta-sisè",76));    fract.insert(make_pair("vuitanta-sisè",86));
  fract.insert(make_pair("setanta-setè",77));    fract.insert(make_pair("vuitanta-setè",87));
  fract.insert(make_pair("setanta-vuitè",78));   fract.insert(make_pair("vuitanta-vuitè",88));
  fract.insert(make_pair("setanta-novè",79));    fract.insert(make_pair("vuitanta-novè",89));

  fract.insert(make_pair("norantè",90));          fract.insert(make_pair("centèssim",100));
  fract.insert(make_pair("noranta-unè",91));      fract.insert(make_pair("milèssim",1000));
  fract.insert(make_pair("noranta-dosè",92));     fract.insert(make_pair("deumilèssim",10000));
  fract.insert(make_pair("noranta-tresè",93));    fract.insert(make_pair("centmilèssim",100000));
  fract.insert(make_pair("noranta-quatrè",94));  fract.insert(make_pair("mil·lionèssim",1000000));
  fract.insert(make_pair("noranta-cinquè",95));   fract.insert(make_pair("deumil·lionèssim",10000000));
  fract.insert(make_pair("noranta-sisè",96));    fract.insert(make_pair("centmil·lionèssim",100000000));
  fract.insert(make_pair("noranta-setè",97));   fract.insert(make_pair("milmil·lionèssim",1000000000));
  fract.insert(make_pair("noranta-vuitè",98));
  fract.insert(make_pair("noranta-novè",99));

  // Initialize special state attributes
  initialState=A; stopState=STOP;

  // Initialize Final state set 
  Final.insert(C);  Final.insert(G); 

  // Initialize transitions table. By default, stop state
  int s,t;
  for(s=0;s<MAX_STATES;s++) for(t=0;t<MAX_TOKENS;t++) trans[s][t]=STOP;
  
  // State A
  trans[A][TK_number]=B;
  // State B
  trans[B][TK_pc]=C;      trans[B][TK_wpor]=D;  trans[B][TK_wde]=E;
  trans[B][TK_wsobre]=F;  trans[B][TK_avo]=C;   trans[B][TK_unit]=G;
  // State C
  trans[C][TK_part]=C;
  // State D
  trans[D][TK_n100]=C;
  // State E
  trans[E][TK_wcada]=F;  trans[E][TK_unit]=G;
  // State F
  trans[F][TK_number]=C;
  // State G
  // nothing else expected from here.

  // opening quantities config file
  ifstream fabr (QuantFile.c_str());
  if (!fabr) { 
    ERROR_CRASH("Error opening currency file "+QuantFile);
  }

  // load maps in config file
  string line;
  int reading=0;
  while (getline(fabr,line)) {

    istringstream sin;
    sin.str(line);
    
    if (line == "<Currency>") reading=1;
    else if (line == "</Currency>") reading=0;
    else if (line == "<Measure>") reading=2;
    else if (line == "</Measure>") reading=0;
    else if (line == "<MeasureNames>") reading=3;
    else if (line == "</MeasureNames>") reading=0;
    else if (reading==1) {
      // reading currency lines
      currency_key = line;
    }
    else if (reading==2) {
      // reading Measure units
      string key,unit;
      sin>>unit>>key;
      units.insert(make_pair(key,unit));
    }
    else if (reading==3) {
      // reading Measure units
      measures.add_locution(line);
    }
  }
  fabr.close(); 

  TRACE(3,"analyzer succesfully created");
}


//-- Implementation of virtual functions from class automat --//

///////////////////////////////////////////////////////////////
///  Compute the right token code for word j from given state.
///////////////////////////////////////////////////////////////

int quantities_ca::ComputeToken(int state, sentence::iterator &j, sentence &se)
{
  string form,lema,name,code;
  int token,value;
  map<string,int>::const_iterator im;
 
  form = util::lowercase(j->get_form());
  lema = j->get_lemma();
  
  token = TK_other;
  im = tok.find(form);
  if (im!=tok.end()) {
    token = (*im).second;
  }
  
  TRACE(3,"Next word form is: ["+form+"] token="+util::int2string(token)); 
  // if the token was in the table, we're done
  if (token != TK_other) return (token);

  // Token not found in translation table, let's have a closer look.

  // check to see if it is a number
  if (j->get_n_analysis() && j->get_parole()=="Z") {
    token = TK_number;
  }
  TRACE(3,"checked");

  switch (state) {
  // ---------------------------------
  case B:
    // in state B a currency name, a measure unit, or a fraction may come
    if (measures.annotate(se,j))
      token=TK_unit;
    else {
      // look if any lemma is found in "fract" map
      word::iterator a;
      for (a=j->begin(); a!=j->end() && fract.find(a->get_lemma())==fract.end(); a++);
      // if any, consider this a fraction word (fifth, eleventh, ...) 
      if (a!=j->end()) {
	j->unselect_all_analysis();
	j->select_analysis(a);
	token=TK_avo;
      }
    }
    break;
  case E: 
    // in state E a currency name or a measure unit may come
    if (measures.annotate(se,j))
      token=TK_unit;
    break;
  // ---------------------------------
  case D:
    // in state D only 100 and 1000 are allowed
    if (token==TK_number) {
      value = util::string2int(j->get_lemma());
      if (value==100 || value==1000)
	token=TK_n100;
    }
    break;
  // ---------------------------------
  default: break;
  }

  return(token);
}


///////////////////////////////////////////////////////////////
///   Reset acumulators used by state actions:
///   value1, value2 (for ratios).
///////////////////////////////////////////////////////////////

void quantities_ca::ResetActions() 
{
  value1=0; value2=0;
  unitCode="";
  unitType="";
}


///////////////////////////////////////////////////////////////
///  Perform necessary actions in "state" reached from state 
///  "origin" via word j interpreted as code "token":
///  Basically, when reaching a state with an informative 
//   token (number, currency name) store the relevant info.
///////////////////////////////////////////////////////////////

void quantities_ca::StateActions(int origin, int state, int token, sentence::const_iterator j)
{
  string form,lema,name,code;
  long double value;

  form = util::lowercase(j->get_form());
  lema = j->get_lemma();
  TRACE(3,"Reaching state "+util::int2string(state)+" with token "+util::int2string(token)+" for word ["+form+"]");

  // get token numerical value, if any
  value=0;
  if ((token==TK_number || token==TK_n100) && 
       j->get_n_analysis() && j->get_parole()=="Z") {
    value = util::string2longdouble(j->get_lemma());
  }

  // State actions
  switch (state) {
  // ---------------------------------
  case B:
    // a nummerical value has been found. Store it just in case.
    TRACE(3,"Actions for state B");
    value1=value;
    break;
  // ---------------------------------
  case C:
    // complete ratio/percentage found
    TRACE(3,"Actions for state C");
    if (token==TK_pc)
      // percent sign "%", second value of ratio is 100
      value2=100;
    else if (token==TK_avo)
      // fraction word (e.g. "two thirds", "dos tercios"), second value is value of second word
      value2=fract[lema];
    else if (token!=TK_part)
      // normal ratio "3 de cada 4" "5 por mil", store second value.
      value2=value;
    break;
  // ---------------------------------
  case G:
    // number + measure unit (or currency name) found, store magnitude and unit
    TRACE(3,"Actions for state I");
    unitCode=units[lema]+"_"+lema;
    unitType=units[lema];
    break;
  // ---------------------------------
  default: break;
  }
}


///////////////////////////////////////////////////////////////
///   Set the appropriate lemma and parole for the 
///   new multiword.
///////////////////////////////////////////////////////////////

void quantities_ca::SetMultiwordAnalysis(sentence::iterator i, int fstate)
{
  string lemma,tag,name,code;

  // Setting the lemma 
  if (unitType==currency_key) {
    lemma=unitCode+":"+util::longdouble2string(value1);
    tag="Zm";
  }
  else if (unitCode!="") {
    // it's a quantity + measure unit
    lemma=unitCode+":"+util::longdouble2string(value1);
    tag="Zu";
  }
  else {
    // it's a ratio
    lemma=util::longdouble2string(value1)+"/"+util::longdouble2string(value2);
    tag="Zp";
  }

  i->set_analysis(analysis(lemma,tag));   
  TRACE(3,"Analysis set to: "+lemma+" "+tag);
}


// undef literals to prepare for next module definition

#undef A 
#undef B 
#undef C 
#undef D 
#undef E
#undef F
#undef G
#undef STOP
#undef TK_number
#undef TK_n100
#undef TK_pc
#undef TK_wde
#undef TK_wcada
#undef TK_wpor
#undef TK_wsobre
#undef TK_avo
#undef TK_part
#undef TK_unit
#undef TK_other



//-----------------------------------------------------------//
//   Galician currency and ratio recognizer.      //
//   The same structure is used, different lexical units     //
//   obtained from configuration files, or mixed in the data //
//-----------------------------------------------------------//

// State codes 
// states for a currency amount, percentage or ratio expression
#define A 1 // initial state
#define B 2 // got a number (a quantity)
#define C 3 // got "20 %", "20 por ciento", "2 de cada 3", "4 sobre 5", "2 tercios"  (VALID PERCENTAGE)
#define D 4 // got "por" after a number 
#define E 5 // got "de" after a number
#define F 6 // got "de cada" after a number
#define G 7 // got a measure unit or a currency name after a number
// stop state
#define STOP 8

// Token codes
#define TK_number   1 // number
#define TK_n100     2 // number with value 100 or 1000 
#define TK_pc       3 // sign "%"
#define TK_wde      4 // word "de"
#define TK_wcada    5 // word "cada"
#define TK_wpor     6 // word "por"
#define TK_wsobre   7 // word "sobre" "entre"
#define TK_avo      8 // fraction word (tercio, veinteavo, milesima)
#define TK_part     9 // word "parte" or "partes"
#define TK_unit    10 // measure unit or currency name
#define TK_other   11


///////////////////////////////////////////////////////////////
///  Create a quantities recognizer for Galician.
///////////////////////////////////////////////////////////////

quantities_gl::quantities_gl(const std::string &QuantFile): quantities_module()
{  
  // Initializing tokens hash
  tok.insert(make_pair("de",TK_wde));
  tok.insert(make_pair("cada",TK_wcada));
  tok.insert(make_pair("por",TK_wpor));
  tok.insert(make_pair("sobre",TK_wsobre));
  tok.insert(make_pair("entre",TK_wsobre));
  tok.insert(make_pair("%",TK_pc));
  tok.insert(make_pair("parte",TK_part));
  tok.insert(make_pair("partes",TK_part));

  // gl
  fract.insert(make_pair("medio",2));
  fract.insert(make_pair("terzo",3));
  fract.insert(make_pair("cuarto",4));
  fract.insert(make_pair("quinto",5));
  fract.insert(make_pair("sexto",6));
  fract.insert(make_pair("sétimo",7));
  fract.insert(make_pair("oitavo",8));
  fract.insert(make_pair("noveno",9));

  fract.insert(make_pair("décimo",10));      fract.insert(make_pair("vinteavo",20));
  fract.insert(make_pair("onceavo",11));      fract.insert(make_pair("vinte-e-unavo",21));
  fract.insert(make_pair("doceavo",12));     fract.insert(make_pair("vinte-e-dousavo",22));
  fract.insert(make_pair("treceavo",13));    fract.insert(make_pair("vinte-e-tresavo",23));
  fract.insert(make_pair("catorceavo",14));   fract.insert(make_pair("vinte-e-catroavo",24));
  fract.insert(make_pair("quinceavo",15));    fract.insert(make_pair("vinte-e-cincoavo",25));
  fract.insert(make_pair("dezaseisavo",16));     fract.insert(make_pair("vinte-e-seisavo",26));
  fract.insert(make_pair("dezaseteavo",17));   fract.insert(make_pair("vinte-e-seteavo",27));
  fract.insert(make_pair("dezaoitoavo",18));   fract.insert(make_pair("vinte-e-oitoavo",28));
  fract.insert(make_pair("dezanoveavo",19));    fract.insert(make_pair("vinte-e-noveavo",29));

  fract.insert(make_pair("trintaavo",30));         fract.insert(make_pair("corentaavo",40));
  fract.insert(make_pair("trinta-e-unavo",31));     fract.insert(make_pair("corenta-e-unavo",41));
  fract.insert(make_pair("trinta-e-dousavo",32));    fract.insert(make_pair("corenta-e-dousavo",42));
  fract.insert(make_pair("trinta-e-tresavo",33));   fract.insert(make_pair("corenta-e-tresavo",43));
  fract.insert(make_pair("trinta-e-catroavo",34));  fract.insert(make_pair("corenta-e-catroavo",44));
  fract.insert(make_pair("trinta-e-cincoavo",35)); fract.insert(make_pair("corenta-e-cincoavo",45));
  fract.insert(make_pair("trinta-e-seisavo",36));    fract.insert(make_pair("corenta-e-seisavo",46));
  fract.insert(make_pair("trinta-e-seteavo",37));    fract.insert(make_pair("corenta-e-seteavo",47));
  fract.insert(make_pair("trinta-e-oitoavo",38));   fract.insert(make_pair("corenta-e-oitoavo",48));
  fract.insert(make_pair("trinta-e-noveavo",39));    fract.insert(make_pair("corenta-e-noveavo",49));

  fract.insert(make_pair("cincuentaavo",50));         fract.insert(make_pair("sesentaavo",60));
  fract.insert(make_pair("cincuenta-e-unavo",51));     fract.insert(make_pair("sesenta-e-unavo",61));
  fract.insert(make_pair("cincuenta-e-dousavo",52));    fract.insert(make_pair("sesenta-e-dousavo",62));
  fract.insert(make_pair("cincuenta-e-tresavo",53));   fract.insert(make_pair("sesenta-e-tresavo",63));
  fract.insert(make_pair("cincuenta-e-catroavo",54));  fract.insert(make_pair("sesenta-e-catroavo",64));
  fract.insert(make_pair("cincuenta-e-cincoavo",55)); fract.insert(make_pair("sesenta-e-cincoavo",65));
  fract.insert(make_pair("cincuenta-e-seisavo",56));    fract.insert(make_pair("sesenta-e-seisavo",66));
  fract.insert(make_pair("cincuenta-e-seteavo",57));    fract.insert(make_pair("sesenta-e-seteavo",67));
  fract.insert(make_pair("cincuenta-e-oitoavo",58));   fract.insert(make_pair("sesenta-e-oitoavo",68));
  fract.insert(make_pair("cincuenta-e-noveavo",59));    fract.insert(make_pair("sesenta-e-noveavo",69));
 
  fract.insert(make_pair("setentaavo",70));         fract.insert(make_pair("oitentaavo",80));
  fract.insert(make_pair("setenta-e-unavo",71));     fract.insert(make_pair("oitenta-e-unavo",81));
  fract.insert(make_pair("setenta-e-dousavo",72));    fract.insert(make_pair("oitenta-e-dousavo",82));
  fract.insert(make_pair("setenta-e-tresavo",73));   fract.insert(make_pair("oitenta-e-tresavo",83));
  fract.insert(make_pair("setenta-e-catroavo",74));  fract.insert(make_pair("oitenta-e-catroavo",84));
  fract.insert(make_pair("setenta-e-cincoavo",75)); fract.insert(make_pair("oitenta-e-cincoavo",85));
  fract.insert(make_pair("setenta-e-seisavo",76));    fract.insert(make_pair("oitenta-e-seisavo",86));
  fract.insert(make_pair("setenta-e-seteavo",77));    fract.insert(make_pair("oitenta-e-seteavo",87));
  fract.insert(make_pair("setenta-e-oitoavo",78));   fract.insert(make_pair("oitenta-e-oitoavo",88));
  fract.insert(make_pair("setenta-e-noveavo",79));    fract.insert(make_pair("oitenta-e-noveavo",89));

  fract.insert(make_pair("noventaavo",90));          fract.insert(make_pair("centésimo",100));
  fract.insert(make_pair("noventa-e-unavo",91));      fract.insert(make_pair("milésimo",1000));
  fract.insert(make_pair("noventa-e-dousavo",92));     fract.insert(make_pair("dezmilésimo",10000));
  fract.insert(make_pair("noventa-e-tresavo",93));    fract.insert(make_pair("cenmilésimo",100000));
  fract.insert(make_pair("noventa-e-catroavo",94));  fract.insert(make_pair("millonésimo",1000000));
  fract.insert(make_pair("noventa-e-cincoavo",95));   fract.insert(make_pair("dezmillonésimo",10000000));
  fract.insert(make_pair("noventa-e-seisavo",96));    fract.insert(make_pair("cenmillonésimo",100000000));
  fract.insert(make_pair("noventa-e-seteavo",97));   fract.insert(make_pair("milmillonésimo",1000000000));
  fract.insert(make_pair("noventa-e-oitoavo",98));
  fract.insert(make_pair("noventa-e-noveavo",99));

  // Initialize special state attributes
  initialState=A; stopState=STOP;

  // Initialize Final state set 
  Final.insert(C);  Final.insert(G); 

  // Initialize transitions table. By default, stop state
  int s,t;
  for(s=0;s<MAX_STATES;s++) for(t=0;t<MAX_TOKENS;t++) trans[s][t]=STOP;
  
  // State A
  trans[A][TK_number]=B;
  // State B
  trans[B][TK_pc]=C;      trans[B][TK_wpor]=D;  trans[B][TK_wde]=E;
  trans[B][TK_wsobre]=F;  trans[B][TK_avo]=C;   trans[B][TK_unit]=G;
  // State C
  trans[C][TK_part]=C;
  // State D
  trans[D][TK_n100]=C;
  // State E
  trans[E][TK_wcada]=F;  trans[E][TK_unit]=G;
  // State F
  trans[F][TK_number]=C;
  // State G
  // nothing else expected from here.

  // opening quantities config file
  ifstream fabr (QuantFile.c_str());
  if (!fabr) { 
    ERROR_CRASH("Error opening currency file "+QuantFile);
  }

  // load maps in config file
  string line;
  int reading=0;
  while (getline(fabr,line)) {

    istringstream sin;
    sin.str(line);
    
    if (line == "<Currency>") reading=1;
    else if (line == "</Currency>") reading=0;
    else if (line == "<Measure>") reading=2;
    else if (line == "</Measure>") reading=0;
    else if (line == "<MeasureNames>") reading=3;
    else if (line == "</MeasureNames>") reading=0;
    else if (reading==1) {
      // reading currency lines
      currency_key = line;
    }
    else if (reading==2) {
      // reading Measure units
      string key,unit;
      sin>>unit>>key;
      units.insert(make_pair(key,unit));
    }
    else if (reading==3) {
      // reading Measure units
      measures.add_locution(line);
    }
  }
  fabr.close(); 


  TRACE(3,"analyzer succesfully created");
}


//-- Implementation of virtual functions from class automat --//


///////////////////////////////////////////////////////////////
///  Compute the right token code for word j from given state.
///////////////////////////////////////////////////////////////

int quantities_gl::ComputeToken(int state, sentence::iterator &j, sentence &se)
{
  string form,lema,name,code;
  int token,value;
  map<string,int>::const_iterator im;
 
  form = util::lowercase(j->get_form());
  lema = j->get_lemma();
  
  token = TK_other;
  im = tok.find(form);
  if (im!=tok.end()) {
    token = (*im).second;
  }
  
  TRACE(3,"Next word form is: ["+form+"] token="+util::int2string(token)); 
  // if the token was in the table, we're done
  if (token != TK_other) return (token);

  // Token not found in translation table, let's have a closer look.

  // check to see if it is a number
  if (j->get_n_analysis() && j->get_parole()=="Z") {
    token = TK_number;
  }
  TRACE(3,"checked");

  switch (state) {
  // ---------------------------------
  case B:
    // in state B a currency name, a measure unit, or a fraction may come
    if (measures.annotate(se,j))
      token=TK_unit;
    else {
      // look if any lemma is found in "fract" map
      word::iterator a;
      for (a=j->begin(); a!=j->end() && fract.find(a->get_lemma())==fract.end(); a++);
      // if any, consider this a fraction word (fifth, eleventh, ...) 
      if (a!=j->end()) {
	j->unselect_all_analysis();
	j->select_analysis(a);
	token=TK_avo;
      }
    }
    break;
  case E: 
    // in state E a currency name or a measure unit may come
    if (measures.annotate(se,j))
      token=TK_unit;
    break;
  // ---------------------------------
  case D:
    // in state D only 100 and 1000 are allowed
    if (token==TK_number) {
      value = util::string2int(j->get_lemma());
      if (value==100 || value==1000)
	token=TK_n100;
    }
    break;
  // ---------------------------------
  default: break;
  }

  return(token);
}


///////////////////////////////////////////////////////////////
///   Reset acumulators used by state actions:
///   value1, value2 (for ratios).
///////////////////////////////////////////////////////////////

void quantities_gl::ResetActions() 
{
  value1=0; value2=0;
  unitCode="";
  unitType="";
}


///////////////////////////////////////////////////////////////
///  Perform necessary actions in "state" reached from state 
///  "origin" via word j interpreted as code "token":
///  Basically, when reaching a state with an informative 
//   token (number, currency name) store the relevant info.
///////////////////////////////////////////////////////////////

void quantities_gl::StateActions(int origin, int state, int token, sentence::const_iterator j)
{
  string form,lema,name,code;
  long double value;

  form = util::lowercase(j->get_form());
  lema = j->get_lemma();
  TRACE(3,"Reaching state "+util::int2string(state)+" with token "+util::int2string(token)+" for word ["+form+"]");

  // get token numerical value, if any
  value=0;
  if ((token==TK_number || token==TK_n100) && 
       j->get_n_analysis() && j->get_parole()=="Z") {
    value = util::string2longdouble(j->get_lemma());
  }

  // State actions
  switch (state) {
  // ---------------------------------
  case B:
    // a nummerical value has been found. Store it just in case.
    TRACE(3,"Actions for state B");
    value1=value;
    break;
  // ---------------------------------
  case C:
    // complete ratio/percentage found
    TRACE(3,"Actions for state C");
    if (token==TK_pc)
      // percent sign "%", second value of ratio is 100
      value2=100;
    if (token==TK_avo)
      // fraction word (e.g. "two thirds", "dos tercios"), second value is value of second word
      value2=fract[lema];
    else if (token!=TK_part)
      // normal ratio "3 de cada 4" "5 por mil", store second value.
      value2=value;
    break;
  // ---------------------------------
  case G:
    // number + measure unit (or currency name) found, store magnitude and unit
    TRACE(3,"Actions for state I");
    unitCode=units[lema]+"_"+lema;
    unitType=units[lema];
    break;
  // ---------------------------------
  default: break;
  }
}


///////////////////////////////////////////////////////////////
///   Set the appropriate lemma and parole for the 
///   new multiword.
///////////////////////////////////////////////////////////////

void quantities_gl::SetMultiwordAnalysis(sentence::iterator i, int fstate)
{
  string lemma,tag,name,code;

  // Setting the lemma 
  if (unitType==currency_key) {
    lemma=unitCode+":"+util::longdouble2string(value1);
    tag="Zm";
  }
  else if (unitCode!="") {
    // it's a quantity + measure unit
    lemma=unitCode+":"+util::longdouble2string(value1);
    tag="Zu";
  }
  else {
    // it's a ratio
    lemma=util::longdouble2string(value1)+"/"+util::longdouble2string(value2);
    tag="Zp";
  }

  i->set_analysis(analysis(lemma,tag));   
  TRACE(3,"Analysis set to: "+lemma+" "+tag);
}


// undef literals to prepare for next module definition

#undef A 
#undef B 
#undef C 
#undef D 
#undef E
#undef F
#undef G
#undef STOP
#undef TK_number
#undef TK_n100
#undef TK_pc
#undef TK_wde
#undef TK_wcada
#undef TK_wpor
#undef TK_wsobre
#undef TK_avo
#undef TK_part
#undef TK_unit
#undef TK_other


//-----------------------------------------------------------//
//   English currency and ratio recognizer.      //
//   The same structure is used, different lexical units     //
//   obtained from configuration files, or mixed in the data //
//-----------------------------------------------------------//

// State codes 
// states for a currency amount, percentage or ratio expression
#define A 1 // initial state
#define B 2 // got a number (a quantity)
#define C 3 // got "20 %", "20 por ciento", "2 de cada 3", "4 sobre 5", "2 tercios"  (VALID PERCENTAGE)
#define D 4 // got "por" after a number 
#define E 5 // got "de" after a number
#define F 6 // got "of each" or "out of" after a number
#define G 7 // got a measure unit or a currency name after a number
#define H 8 // got word "out" after a number
// stop state
#define STOP 9

// Token codes
#define TK_number   1 // number
#define TK_n100     2 // number with value 100 or 1000 
#define TK_pc       3 // sign "%"
#define TK_wout     4 // word "out"
#define TK_wde      5 // word "of"
#define TK_wcada    6 // word "each"
#define TK_wpor     7 // word "per"
#define TK_wsobre   8 // word "between"
#define TK_avo      9 // fraction word (tenth, half)
#define TK_part    10 // word "part" or "parts"
#define TK_unit    11 // measure unit or currency name
#define TK_other   12

///////////////////////////////////////////////////////////////
///  Create a quantities recognizer for Spanish.
///////////////////////////////////////////////////////////////

quantities_en::quantities_en(const std::string &QuantFile): quantities_module()
{  
  // Initializing tokens hash
  tok.insert(make_pair("out",TK_wout));      
  tok.insert(make_pair("of",TK_wde));      
  tok.insert(make_pair("each",TK_wcada));  
  tok.insert(make_pair("per",TK_wpor));    
  tok.insert(make_pair("between",TK_wsobre));
  tok.insert(make_pair("%",TK_pc));        
  tok.insert(make_pair("part",TK_part));  
  tok.insert(make_pair("parts",TK_part)); 

  // es
  fract.insert(make_pair("half",2));
  fract.insert(make_pair("third",3));
  fract.insert(make_pair("fourth",4));
  fract.insert(make_pair("fifth",5));
  fract.insert(make_pair("sixth",6));
  fract.insert(make_pair("seventh",7));
  fract.insert(make_pair("eighth",8));
  fract.insert(make_pair("ninth",9));

  fract.insert(make_pair("tenth",10));       fract.insert(make_pair("twentieth",20));
  fract.insert(make_pair("eleventh",11));    fract.insert(make_pair("twenty-first",21));
  fract.insert(make_pair("twelveth",12));    fract.insert(make_pair("twenty-second",22));
  fract.insert(make_pair("thirteenth",13));  fract.insert(make_pair("twenty-third",23));
  fract.insert(make_pair("fourteenth",14));  fract.insert(make_pair("twenty-fourth",24));
  fract.insert(make_pair("fifteenth",15));   fract.insert(make_pair("twenty-fifth",25));
  fract.insert(make_pair("sixteenth",16));   fract.insert(make_pair("twenty-sixth",26));
  fract.insert(make_pair("seventeenth",17)); fract.insert(make_pair("twenty-seventh",27));
  fract.insert(make_pair("eighteenth",18));  fract.insert(make_pair("twenty-eighth",28));
  fract.insert(make_pair("nineteenth",19));  fract.insert(make_pair("twenty-nineth",29));

  fract.insert(make_pair("thirtieth",30));      fract.insert(make_pair("fortieth",40));
  fract.insert(make_pair("thirty-first",31));   fract.insert(make_pair("forty-first",41));
  fract.insert(make_pair("thirty-second",32));  fract.insert(make_pair("forty-second",42));
  fract.insert(make_pair("thirty-third",33));   fract.insert(make_pair("forty-third",43));
  fract.insert(make_pair("thirty-fourth",34));  fract.insert(make_pair("forty-fourth",44));
  fract.insert(make_pair("thirty-fifth",35));   fract.insert(make_pair("forty-fifth",45));
  fract.insert(make_pair("thirty-sixth",36));   fract.insert(make_pair("forty-sixth",46));
  fract.insert(make_pair("thirty-seventh",37)); fract.insert(make_pair("forty-seventh",47));
  fract.insert(make_pair("thirty-eighth",38));  fract.insert(make_pair("forty-eighth",48));
  fract.insert(make_pair("thirty-nineth",39));  fract.insert(make_pair("forty-nineth",49));

  fract.insert(make_pair("fiftieth",50));      fract.insert(make_pair("sixtieth",60));
  fract.insert(make_pair("fifty-first",51));   fract.insert(make_pair("sixty-first",61));
  fract.insert(make_pair("fifty-second",52));  fract.insert(make_pair("sixty-second",62));
  fract.insert(make_pair("fifty-third",53));   fract.insert(make_pair("sixty-third",63));
  fract.insert(make_pair("fifty-fourth",54));  fract.insert(make_pair("sixty-fourth",64));
  fract.insert(make_pair("fifty-fifth",55));   fract.insert(make_pair("sixty-fifth",65));
  fract.insert(make_pair("fifty-sixth",56));   fract.insert(make_pair("sixty-sixth",66));
  fract.insert(make_pair("fifty-seventh",57)); fract.insert(make_pair("sixty-seventh",67));
  fract.insert(make_pair("fifty-eighth",58));  fract.insert(make_pair("sixty-eighth",68));
  fract.insert(make_pair("fifty-nineth",59));  fract.insert(make_pair("sixty-nineth",69));

  fract.insert(make_pair("seventieth",70));      fract.insert(make_pair("eightieth",80));
  fract.insert(make_pair("seventy-first",71));   fract.insert(make_pair("eighty-first",81));
  fract.insert(make_pair("seventy-second",72));  fract.insert(make_pair("eighty-second",82));
  fract.insert(make_pair("seventy-third",73));   fract.insert(make_pair("eighty-third",83));
  fract.insert(make_pair("seventy-fourth",74));  fract.insert(make_pair("eighty-fourth",84));
  fract.insert(make_pair("seventy-fifth",75));   fract.insert(make_pair("eighty-fifth",85));
  fract.insert(make_pair("seventy-sixth",76));   fract.insert(make_pair("eighty-sixth",86));
  fract.insert(make_pair("seventy-seventh",77)); fract.insert(make_pair("eighty-seventh",87));
  fract.insert(make_pair("seventy-eighth",78));  fract.insert(make_pair("eighty-eighth",88));
  fract.insert(make_pair("seventy-nineth",79));  fract.insert(make_pair("eighty-nineth",89));

  fract.insert(make_pair("ninetieth",90));       fract.insert(make_pair("hundredth",100));
  fract.insert(make_pair("ninety-first",91));    fract.insert(make_pair("thousandth",1000));
  fract.insert(make_pair("ninety-second",92));   fract.insert(make_pair("ten-thousandth",10000));
  fract.insert(make_pair("ninety-third",93));    fract.insert(make_pair("one-hundred-thousandth",100000));
  fract.insert(make_pair("ninety-fourth",94));   fract.insert(make_pair("one-millionth",1000000));
  fract.insert(make_pair("ninety-fifth",95));    fract.insert(make_pair("ten-millionth",10000000));
  fract.insert(make_pair("ninety-sixth",96));    fract.insert(make_pair("one-hundred-millionth",100000000));
  fract.insert(make_pair("ninety-seventh",97));  fract.insert(make_pair("one-billionth",1000000000));
  fract.insert(make_pair("ninety-eighth",98));   
  fract.insert(make_pair("ninety-nineth",99));   

  // Initialize special state attributes
  initialState=A; stopState=STOP;

  // Initialize Final state set 
  Final.insert(C);  Final.insert(G); 

  // Initialize transitions table. By default, stop state
  int s,t;
  for(s=0;s<MAX_STATES;s++) for(t=0;t<MAX_TOKENS;t++) trans[s][t]=STOP;
  
  // State A
  trans[A][TK_number]=B;
  // State B
  trans[B][TK_pc]=C;      trans[B][TK_wpor]=D;  trans[B][TK_wde]=E;
  trans[B][TK_wsobre]=F;  trans[B][TK_avo]=C;   trans[B][TK_unit]=G;
  trans[B][TK_wout]=H;
  // State C
  trans[C][TK_part]=C;
  // State D
  trans[D][TK_n100]=C;
  // State E
  trans[E][TK_wcada]=F;  trans[E][TK_unit]=G;
  // State F
  trans[F][TK_number]=C;
  // State G
  // nothing else expected from here.
  // State H
  trans[H][TK_wde]=F;

  // opening quantities config file
  ifstream fabr (QuantFile.c_str());
  if (!fabr) { 
    ERROR_CRASH("Error opening currency file "+QuantFile);
  }

  // load maps in config file
  string line;
  int reading=0;
  while (getline(fabr,line)) {

    istringstream sin;
    sin.str(line);
    
    if (line == "<Currency>") reading=1;
    else if (line == "</Currency>") reading=0;
    else if (line == "<Measure>") reading=2;
    else if (line == "</Measure>") reading=0;
    else if (line == "<MeasureNames>") reading=3;
    else if (line == "</MeasureNames>") reading=0;
    else if (reading==1) {
      // reading currency lines
      currency_key = line;
    }
    else if (reading==2) {
      // reading Measure units
      string key,unit;
      sin>>unit>>key;
      units.insert(make_pair(key,unit));
    }
    else if (reading==3) {
      // reading Measure units
      TRACE(3,"adding locution: "+line);
      measures.add_locution(line);
    }
  }
  fabr.close(); 


  TRACE(3,"analyzer succesfully created");
}


//-- Implementation of virtual functions from class automat --//

///////////////////////////////////////////////////////////////
///  Compute the right token code for word j from given state.
///////////////////////////////////////////////////////////////

int quantities_en::ComputeToken(int state, sentence::iterator &j, sentence &se)
{
  string form,lema,name,code;
  int token,value;
  map<string,int>::const_iterator im;
 
  form = util::lowercase(j->get_form());
  lema = j->get_lemma();
  
  token = TK_other;
  im = tok.find(form);
  if (im!=tok.end()) {
    token = (*im).second;
  }
  
  TRACE(3,"Next word form is: ["+form+"] token="+util::int2string(token)); 
  // if the token was in the table, we're done
  if (token != TK_other) return (token);

  // Token not found in translation table, let's have a closer look.

  // check to see if it is a number
  if (j->get_n_analysis() && j->get_parole()=="Z") {
    token = TK_number;
  }
  TRACE(3,"checked");

  switch (state) {
  // ---------------------------------
  case B:
    // in state B a currency name, a measure unit, or a fraction may come
    if (measures.annotate(se,j))
      token=TK_unit;
    else {
      // look if any lemma is found in "fract" map
      word::iterator a;
      for (a=j->begin(); a!=j->end() && fract.find(a->get_lemma())==fract.end(); a++);
      // if any, consider this a fraction word (fifth, eleventh, ...) 
      if (a!=j->end()) {
	j->unselect_all_analysis();
	j->select_analysis(a);
	token=TK_avo;
      }
    }
    break;
  case E: 
    // in state E a currency name or a measure unit may come
    if (measures.annotate(se,j))
      token=TK_unit;
    break;
  // ---------------------------------
  case D:
    // in state D only 100 and 1000 are allowed
    if (token==TK_number) {
      value = util::string2int(j->get_lemma());
      if (value==100 || value==1000)
	token=TK_n100;
    }
    break;
  // ---------------------------------
  default: break;
  }

  return(token);
}


///////////////////////////////////////////////////////////////
///   Reset acumulators used by state actions:
///   value1, value2 (for ratios).
///////////////////////////////////////////////////////////////

void quantities_en::ResetActions() 
{
  value1=0; value2=0;
  unitCode="";
  unitType="";
}


///////////////////////////////////////////////////////////////
///  Perform necessary actions in "state" reached from state 
///  "origin" via word j interpreted as code "token":
///  Basically, when reaching a state with an informative 
//   token (number, currency name) store the relevant info.
///////////////////////////////////////////////////////////////

void quantities_en::StateActions(int origin, int state, int token, sentence::const_iterator j)
{
  string form,lema,name,code;
  long double value;

  form = util::lowercase(j->get_form());
  lema = j->get_lemma();
  TRACE(3,"Reaching state "+util::int2string(state)+" with token "+util::int2string(token)+" for word ["+form+"]");

  // get token numerical value, if any
  value=0;
  if ((token==TK_number || token==TK_n100) && 
       j->get_n_analysis() && j->get_parole()=="Z") {
    value = util::string2longdouble(j->get_lemma());
  }

  // State actions
  switch (state) {
  // ---------------------------------
  case B:
    // a nummerical value has been found. Store it just in case.
    TRACE(3,"Actions for state B");
    value1=value;
    break;
  // ---------------------------------
  case C:
    // complete ratio/percentage found
    TRACE(3,"Actions for state C");
    if (token==TK_pc)
      // percent sign "%", second value of ratio is 100
      value2=100;
    if (token==TK_avo)
      // fraction word (e.g. "two thirds", "dos tercios"), second value is value of second word
      value2=fract[lema];
    else if (token!=TK_part)
      // normal ratio "3 de cada 4" "5 por mil", store second value.
      value2=value;
    break;
  // ---------------------------------
  case G:
    // number + measure unit (or currency name) found, store magnitude and unit
    TRACE(3,"Actions for state I");
    unitCode=units[lema]+"_"+lema;
    unitType=units[lema];
    break;
  // ---------------------------------
  default: break;
  }
}


///////////////////////////////////////////////////////////////
///   Set the appropriate lemma and parole for the 
///   new multiword.
///////////////////////////////////////////////////////////////

void quantities_en::SetMultiwordAnalysis(sentence::iterator i, int fstate)
{
  string lemma,tag,name,code;

  // Setting the lemma 
  if (unitType==currency_key) {
    lemma=unitCode+":"+util::longdouble2string(value1);
    tag="Zm";
  }
  else if (unitCode!="") {
    // it's a quantity + measure unit
    lemma=unitCode+":"+util::longdouble2string(value1);
    tag="Zu";
  }
  else {
    // it's a ratio
    lemma=util::longdouble2string(value1)+"/"+util::longdouble2string(value2);
    tag="Zp";
  }

  i->set_analysis(analysis(lemma,tag));   
  TRACE(3,"Analysis set to: "+lemma+" "+tag);
}


// undef literals to prepare for next module definition

#undef A 
#undef B 
#undef C 
#undef D 
#undef E
#undef F
#undef G
#undef STOP
#undef TK_number
#undef TK_n100
#undef TK_pc
#undef TK_wde
#undef TK_wcada
#undef TK_wpor
#undef TK_wsobre
#undef TK_avo
#undef TK_part
#undef TK_unit
#undef TK_other


