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

#include "freeling/numbers_modules.h"
#include "fries/util.h"
#include "freeling/traces.h"

using namespace std;

#define MOD_TRACENAME "NUMBERS"
#define MOD_TRACECODE NUMBERS_TRACE

///////////////////////////////////////////////////////////////
///  Abstract class constructor.
///////////////////////////////////////////////////////////////

numbers_module::numbers_module(const std::string &dec, const std::string &thou): automat(), MACO_Decimal(dec), 
                                                                       MACO_Thousand(thou), RE_number(RE_NUM), 
                                                                       RE_code(RE_CODE)
{}


//-----------------------------------------------//
//        Default number recognizer              //
//        Only recognize digits and codes        //
//-----------------------------------------------//

// State codes
#define A 1
#define NUM 2 // got number from initial state
#define COD 3 // got pseudo-numerical code from initial state
// stop state
#define STOP 4

// Token codes
#define TK_num   1  // a number (in digits)
#define TK_code  2  // a code (ex. LX-345-2)
#define TK_other 3

///////////////////////////////////////////////////////////////
///  Create a default numbers recognizer.
///////////////////////////////////////////////////////////////

numbers_default::numbers_default(const std::string &dec, const std::string &thou): numbers_module(dec,thou)
{  
  // Initializing value map
  // Initialize special state attributes
  initialState=A; stopState=STOP;

  // Initialize Final state set 
  Final.insert(NUM);  Final.insert(COD);

  // Initialize transitions table. By default, stop state
  int s,t;
  for(s=0;s<MAX_STATES;s++) for(t=0;t<MAX_TOKENS;t++) trans[s][t]=STOP;
  
  // Initializing transitions table
  // State A
  trans[A][TK_num]=NUM;  trans[A][TK_code]=COD;
  // State NUM
  // nothing else is expected
  // State COD
  // nothing else is expected
 
  TRACE(3,"analyzer succesfully created");
}


//-- Implementation of virtual functions from class automat --//

///////////////////////////////////////////////////////////////
///  Compute the right token code for word j from given state.
///////////////////////////////////////////////////////////////

int numbers_default::ComputeToken(int state, sentence::iterator &j, sentence &se)
{
  string form;
  int token;
 
  form = util::lowercase(j->get_form());
  
  // check to see if it is a number
  if (RE_number.Search(form)) token = TK_num;
  else if (RE_code.Search(form)) token = TK_code;
  else token = TK_other;

  TRACE(3,"Next word form is: ["+form+"] token="+util::int2string(token));     

  TRACE(3,"Leaving state "+util::int2string(state)+" with token "+util::int2string(token)); 
  return (token);
}


///////////////////////////////////////////////////////////////
///   Reset acumulators used by state actions:
///   bilion, milion, units.
///////////////////////////////////////////////////////////////

void numbers_default::ResetActions() 
{
 iscode=0;
 milion=0; bilion=0; units=0;
}


///////////////////////////////////////////////////////////////
///  Perform necessary actions in "state" reached from state 
///  "origin" via word j interpreted as code "token":
///  Basically, when reaching a state, update current 
///  nummerical value.
///////////////////////////////////////////////////////////////

void numbers_default::StateActions(int origin, int state, int token, sentence::const_iterator j)
{
  string form;
  size_t i;
  long double num;

  form = util::lowercase(j->get_form());
  TRACE(3,"Reaching state "+util::int2string(state)+" with token "+util::int2string(token)+" for word ["+form+"]");

  // get token numerical value, if any
  num=0;
  if (token==TK_num) {
    // erase thousand points
    while ((i=form.find_first_of(MACO_Thousand))!=string::npos) {
      form.erase(i,1);
    }
    TRACE(3,"after erasing thousand "+form);
    // make sure decimal point is "."
    if ((i=form.find_last_of(MACO_Decimal))!=string::npos) {
      form[i]='.';
      TRACE(3,"after replacing decimal "+form);
    }

    num = util::string2longdouble(form);
  }

  // State actions
  switch (state) {
  // ---------------------------------
  case NUM:
    TRACE(3,"Actions for NUM state");
    if (token==TK_num)
      units += num;
    break;
  // ---------------------------------
  case COD:
    TRACE(3,"Actions for COD state");
    iscode = CODE;
    break;
  // ---------------------------------
  default: break;
  }
  
  TRACE(3,"State actions completed. units="+util::longdouble2string(units));
}


///////////////////////////////////////////////////////////////
///   Set the appropriate lemma and parole for the 
///   new multiword.
///////////////////////////////////////////////////////////////

void numbers_default::SetMultiwordAnalysis(sentence::iterator i, int fstate)
{
  string lemma;

  // Setting the analysis for the nummerical expression
  if (iscode==CODE) {
    lemma=i->get_form();
  }
  else {
    // compute nummerical value as lemma
    lemma=util::longdouble2string(units);
  }

  i->set_analysis(analysis(lemma,"Z"));
  TRACE(3,"Analysis set to: "+lemma+" Z");
}


// undef codes to prepare for next module definition
#undef A 
#undef NUM
#undef COD
#undef STOP
#undef TK_num
#undef TK_code
#undef TK_other


//---------------------------------------------------------------------------
//        Spanish number recognizer
//---------------------------------------------------------------------------

//// State codes 
#define B1 1  // initial state
#define B2 2  // got hundreds "doscientos"  (VALID NUMBER)
#define B3 3  // got tens "treinta" "docientos treinta"   (VALID NUMBER)
#define B4 4  // got "y" after tens
#define Bu 5  // got units after "y": "doscientos treinta y cuatro"   (VALID NUMBER)
#define B5 6  // got "mil" after unit "doscientos treinta y cuatro mil"   (VALID NUMBER)
#define B6 7  // got hundreds after "mil"   (VALID NUMBER)
#define B7 8  // got tens after "mil"   (VALID NUMBER)
#define B8 9  // got "y" after tens   (VALID NUMBER)
#define Bk 10 // got units after "y"   (VALID NUMBER)
#define M1 11 // got "billones" after a valid number  (VALID NUMBER)
#define M1a 12 // got "y" after "billones"
#define M1b 13 // got "... millones y medio/cuarto" 
#define M2 14 // got hundreds "doscientos" after billions  (VALID NUMBER)
#define M3 15 // got tens "treinta" "docientos treinta"   (VALID NUMBER)
#define M4 16 // got "y" after tens
#define Mu 17 // got units after "y": "doscientos treinta y cuatro"   (VALID NUMBER)
#define M5 18 // got "mil" after unit "doscientos treinta y cuatro mil"   (VALID NUMBER)
#define M6 19 // got hundreds after "mil"   (VALID NUMBER)
#define M7 20 // got tens after "mil"   (VALID NUMBER)
#define M8 21 // got "y" after tens   (VALID NUMBER)
#define Mk 22 // got units after "y"   (VALID NUMBER)
#define S1 23 // got "millones" after a valid number  (VALID NUMBER) 
#define S1a 24 // got "y" after "millones"
#define S1b 25 // got "... millones y medio/cuarto" 
#define S2 26 // got hundreds "doscientos" after millions  (VALID NUMBER)
#define S3 27 // got tens "treinta" "docientos treinta"   (VALID NUMBER)
#define S4 28 // got "y" after tens
#define Su 29 // got units after "y": "doscientos treinta y cuatro"   (VALID NUMBER)
#define S5 30 // got "mil" after unit "doscientos treinta y cuatro mil"   (VALID NUMBER)
#define S6 31 // got hundreds after "mil"   (VALID NUMBER)
#define S7 32 // got tens after "mil"   (VALID NUMBER)
#define S8 33 // got "y" after tens   (VALID NUMBER)
#define Sk 34 // got units after "y"   (VALID NUMBER)
#define COD 35 // got pseudo-numerical code from initial state
#define X1 36 // got "millar", "docenas" "centenares" "decenas" after a number
#define X2 37 // got "y" after "millar/docenas/etc"
#define X3 38 // got "millar y medio/cuarto",  "X docenas y media"
#define X4 39 // got "medio" as 1st word
#define X5 40 // got "medio centenar/millar/etc"
// stop state
#define STOP 41

// Token codes
#define TK_c      1  // hundreds "cien" "doscientos" 
#define TK_d      2  // tens "treinta" "cuarenta"
#define TK_u      3  // units "tres" "cuatro"
#define TK_wy     4  // word "y"
#define TK_wmedio 5  // word "medio"
#define TK_wcuarto 6 // word "cuarto"
#define TK_mil    7  // word "mil"   
#define TK_mill   8  // word "millon" "millones"
#define TK_bill   9  // word "billon" "billones"
#define TK_num   10  // a number (in digits)
#define TK_code  11  // a code (ex. LX-345-2)
#define TK_milr  12  // word "millar/es"
#define TK_cent  13  // word "centenar/es"
#define TK_dec   14  // word "decena/s"
#define TK_doc   15  // word "docena/s
#define TK_other 16
 
 
///////////////////////////////////////////////////////////////
///  Create a numbers recognizer for Spanish.
///////////////////////////////////////////////////////////////

numbers_es::numbers_es(const std::string &dec, const std::string &thou): numbers_module(dec,thou)
{  
  // Initializing value map
  value.insert(make_pair("cien",100));          value.insert(make_pair("ciento",100));
  value.insert(make_pair("doscientos",200));    value.insert(make_pair("doscientas",200));
  value.insert(make_pair("trescientos",300));   value.insert(make_pair("trescientas",300));
  value.insert(make_pair("cuatrocientos",400)); value.insert(make_pair("cuatrocientas",400));
  value.insert(make_pair("quinientos",500));    value.insert(make_pair("quinientas",500));
  value.insert(make_pair("seiscientos",600));   value.insert(make_pair("seiscientas",600));
  value.insert(make_pair("setecientos",700));   value.insert(make_pair("setecientas",700));
  value.insert(make_pair("ochocientos",800));   value.insert(make_pair("ochocientas",800));
  value.insert(make_pair("novecientos",900));   value.insert(make_pair("novecientas",900));
  value.insert(make_pair("treinta",30));     value.insert(make_pair("cuarenta",40));
  value.insert(make_pair("cincuenta",50));   value.insert(make_pair("sesenta",60));
  value.insert(make_pair("setenta",70));     value.insert(make_pair("ochenta",80));
  value.insert(make_pair("noventa",90));     
  value.insert(make_pair("uno",1));     value.insert(make_pair("un",1));
  value.insert(make_pair("una",1));     value.insert(make_pair("dos",2));
  value.insert(make_pair("tres",3));    value.insert(make_pair("cuatro",4));
  value.insert(make_pair("cinco",5));   value.insert(make_pair("seis",6));
  value.insert(make_pair("siete",7));   value.insert(make_pair("ocho",8));
  value.insert(make_pair("nueve",9));   value.insert(make_pair("diez",10));
  value.insert(make_pair("once",11));   value.insert(make_pair("doce",12));
  value.insert(make_pair("trece",13));  value.insert(make_pair("catorce",14));
  value.insert(make_pair("quince",15)); value.insert(make_pair("dieciséis",16));
  value.insert(make_pair("dieciseis",16)); value.insert(make_pair("diecisiete",17)); 
  value.insert(make_pair("dieciocho",18)); value.insert(make_pair("diecinueve",19));
  value.insert(make_pair("veinte", 20));   value.insert(make_pair("veintiún",21));
  value.insert(make_pair("veintiuno",21)); value.insert(make_pair("veintiuna",21));  
  value.insert(make_pair("veintiun",21));  value.insert(make_pair("veintidós",22));
  value.insert(make_pair("veintidos",22));   value.insert(make_pair("veintitrés",23));
  value.insert(make_pair("veintitres",23));  value.insert(make_pair("veinticuatro",24));
  value.insert(make_pair("veinticinco",25)); value.insert(make_pair("veintiséis",26));
  value.insert(make_pair("veintiseis",26));  value.insert(make_pair("veintisiete",27));
  value.insert(make_pair("veintiocho",28));  value.insert(make_pair("veintinueve",29));

  value.insert(make_pair("medio",0.5));    value.insert(make_pair("media",0.5));  
  value.insert(make_pair("cuarto",0.25));

  // Initializing token map
  tok.insert(make_pair("cien",TK_c));        tok.insert(make_pair("ciento",TK_c));
  tok.insert(make_pair("doscientos",TK_c));  tok.insert(make_pair("doscientas",TK_c));
  tok.insert(make_pair("trescientos",TK_c)); tok.insert(make_pair("trescientas",TK_c));
  tok.insert(make_pair("cuatrocientos",TK_c)); tok.insert(make_pair("cuatrocientas",TK_c));
  tok.insert(make_pair("quinientos",TK_c));  tok.insert(make_pair("quinientas",TK_c));
  tok.insert(make_pair("seiscientos",TK_c)); tok.insert(make_pair("seiscientas",TK_c));
  tok.insert(make_pair("setecientos",TK_c)); tok.insert(make_pair("setecientas",TK_c));
  tok.insert(make_pair("ochocientos",TK_c)); tok.insert(make_pair("ochocientas",TK_c));
  tok.insert(make_pair("novecientos",TK_c)); tok.insert(make_pair("novecientas",TK_c));
  tok.insert(make_pair("treinta",TK_d));     tok.insert(make_pair("cuarenta",TK_d));
  tok.insert(make_pair("cincuenta",TK_d));   tok.insert(make_pair("sesenta",TK_d));
  tok.insert(make_pair("setenta",TK_d));     tok.insert(make_pair("ochenta",TK_d));
  tok.insert(make_pair("noventa",TK_d));     tok.insert(make_pair("uno",TK_u));
  tok.insert(make_pair("un",TK_u));       tok.insert(make_pair("una",TK_u));
  tok.insert(make_pair("dos",TK_u));      tok.insert(make_pair("tres",TK_u));
  tok.insert(make_pair("cuatro",TK_u));   tok.insert(make_pair("cinco",TK_u));
  tok.insert(make_pair("seis",TK_u));     tok.insert(make_pair("siete",TK_u));
  tok.insert(make_pair("ocho",TK_u));     tok.insert(make_pair("nueve",TK_u));
  tok.insert(make_pair("diez",TK_u));     tok.insert(make_pair("once",TK_u));
  tok.insert(make_pair("doce",TK_u));     tok.insert(make_pair("trece",TK_u));
  tok.insert(make_pair("catorce",TK_u));  tok.insert(make_pair("quince",TK_u));
  tok.insert(make_pair("dieciséis",TK_u));  tok.insert(make_pair("dieciseis",TK_u));
  tok.insert(make_pair("diecisiete",TK_u)); tok.insert(make_pair("dieciocho",TK_u));
  tok.insert(make_pair("diecinueve",TK_u)); tok.insert(make_pair("veinte",TK_u));
  tok.insert(make_pair("veintiun",TK_u));   tok.insert(make_pair("veintiún",TK_u));
  tok.insert(make_pair("veintiuno",TK_u));  tok.insert(make_pair("veintiuna",TK_u));
  tok.insert(make_pair("veintidós",TK_u));  tok.insert(make_pair("veintidos",TK_u));
  tok.insert(make_pair("veintitrés",TK_u));   tok.insert(make_pair("veintitres",TK_u));
  tok.insert(make_pair("veinticuatro",TK_u)); tok.insert(make_pair("veinticinco",TK_u));
  tok.insert(make_pair("veintiséis",TK_u));   tok.insert(make_pair("veintiseis",TK_u));
  tok.insert(make_pair("veintisiete",TK_u));  tok.insert(make_pair("veintiocho",TK_u));
  tok.insert(make_pair("veintinueve",TK_u));  tok.insert(make_pair("mil",TK_mil));
  tok.insert(make_pair("millon",TK_mill));    tok.insert(make_pair("millón",TK_mill));
  tok.insert(make_pair("millones",TK_mill));  tok.insert(make_pair("billon",TK_bill));
  tok.insert(make_pair("billón",TK_bill));    tok.insert(make_pair("billones",TK_bill));

  tok.insert(make_pair("millar",TK_milr));    tok.insert(make_pair("millares",TK_milr));
  tok.insert(make_pair("centenar",TK_cent));    tok.insert(make_pair("centenares",TK_cent));
  tok.insert(make_pair("decena",TK_dec));    tok.insert(make_pair("decenas",TK_dec));
  tok.insert(make_pair("docena",TK_doc));    tok.insert(make_pair("docenas",TK_doc));

  tok.insert(make_pair("y",TK_wy));
  tok.insert(make_pair("medio",TK_wmedio));
  tok.insert(make_pair("media",TK_wmedio));
  tok.insert(make_pair("cuarto",TK_wcuarto));

  // Initializing power map
  power.insert(make_pair(TK_mil,  1000.0));
  power.insert(make_pair(TK_mill, 1000000.0));
  power.insert(make_pair(TK_bill, 1000000000000.0));
  power.insert(make_pair(TK_milr, 1000.0));
  power.insert(make_pair(TK_cent, 100.0));
  power.insert(make_pair(TK_doc,  12.0));
  power.insert(make_pair(TK_dec,  10.0));

  // Initialize special state attributes
  initialState=B1; stopState=STOP;

  // Initialize Final state set 
  Final.insert(B2);  Final.insert(B3);  Final.insert(Bu);  Final.insert(B5);
  Final.insert(B6);  Final.insert(B7);  Final.insert(Bk); 
  Final.insert(M1);  Final.insert(M2);  Final.insert(M3);  Final.insert(Mu);
  Final.insert(M5);  Final.insert(M6);  Final.insert(M7);  Final.insert(Mk);
  Final.insert(S1);  Final.insert(S2);  Final.insert(S3);  Final.insert(Su);
  Final.insert(S5);  Final.insert(S6);  Final.insert(S7);  Final.insert(Sk);
  Final.insert(M1b); Final.insert(S1b); Final.insert(COD);  
  Final.insert(X1);  Final.insert(X3);  Final.insert(X5);

  // Initialize transitions table. By default, stop state
  int s,t;
  for(s=0;s<MAX_STATES;s++) for(t=0;t<MAX_TOKENS;t++) trans[s][t]=STOP;
  
  // Initializing transitions table
  // State B1
  trans[B1][TK_c]=B2;   trans[B1][TK_d]=B3;   trans[B1][TK_u]=Bu; 
  trans[B1][TK_mil]=B5; trans[B1][TK_num]=Bu; trans[B1][TK_code]=COD;
  trans[B1][TK_wmedio]=X4;
  // State B2
  trans[B2][TK_d]=B3;    trans[B2][TK_u]=Bu;    trans[B2][TK_mil]=B5; 
  trans[B2][TK_bill]=M1; trans[B2][TK_mill]=S1; trans[B2][TK_num]=Bu;
  // State B3
  trans[B3][TK_wy]=B4;   trans[B3][TK_mil]=B5; 
  trans[B3][TK_bill]=M1; trans[B3][TK_mill]=S1;
  // State B4
  trans[B4][TK_u]=Bu; trans[B4][TK_num]=Bu;
  // State Bu
  trans[Bu][TK_mil]=B5; trans[Bu][TK_bill]=M1; trans[Bu][TK_mill]=S1;
  trans[Bu][TK_milr]=X1; trans[Bu][TK_cent]=X1;
  trans[Bu][TK_dec]=X1;  trans[Bu][TK_doc]=X1;
  // State B5
  trans[B5][TK_c]=B6;    trans[B5][TK_d]=B7;    trans[B5][TK_u]=Bk; 
  trans[B5][TK_bill]=M1; trans[B5][TK_mill]=S1; trans[B5][TK_num]=Bk;
  // State B6
  trans[B6][TK_d]=B7;    trans[B6][TK_u]=Bk;    trans[B6][TK_num]=Bk;
  trans[B6][TK_bill]=M1; trans[B6][TK_mill]=S1;
  // State B7
  trans[B7][TK_wy]=B8; trans[B7][TK_bill]=M1; trans[B7][TK_mill]=S1;
  // State B8
  trans[B8][TK_u]=Bk;  trans[B8][TK_num]=Bk; 
  // State Bk
  trans[Bk][TK_bill]=M1; trans[Bk][TK_mill]=S1;

  // State M1
  trans[M1][TK_c]=M2; trans[M1][TK_d]=M3;   trans[M1][TK_num]=Mu; 
  trans[M1][TK_u]=Mu; trans[M1][TK_mil]=M5; trans[M1][TK_wy]=M1a; 
  // State M1a
  trans[M1a][TK_wmedio]=M1b; trans[M1a][TK_wcuarto]=M1b;
  // State M1b
    // nothing else expected
  // State M2
  trans[M2][TK_d]=M3;    trans[M2][TK_u]=Mu;   trans[M2][TK_num]=Mu; 
  trans[M2][TK_mil]=M5;  trans[M2][TK_mill]=S1;
  // State M3
  trans[M3][TK_wy]=M4;   trans[M3][TK_mil]=M5;  trans[M3][TK_mill]=S1;
  // State M4
  trans[M4][TK_u]=Mu;  trans[M4][TK_num]=Mu;
  // State Mu
  trans[Mu][TK_mil]=M5; trans[Mu][TK_mill]=S1;
  // State M5
  trans[M5][TK_c]=M6;   trans[M5][TK_d]=M7;   trans[M5][TK_num]=Mk; 
  trans[M5][TK_u]=Mk;   trans[M5][TK_mill]=S1;
  // State M6
  trans[M6][TK_d]=M7;    trans[M6][TK_u]=Mk;
  trans[M6][TK_mill]=S1; trans[M6][TK_num]=Mk; 
  // State M7
  trans[M7][TK_wy]=M8; trans[M7][TK_mill]=S1;
  // State M8
  trans[M8][TK_u]=Mk;   trans[M8][TK_num]=Mk;
  // State Mk
  trans[Mk][TK_mill]=S1;

  // State S1
  trans[S1][TK_c]=S2; trans[S1][TK_d]=S3;   trans[S1][TK_num]=Su; 
  trans[S1][TK_u]=Su; trans[S1][TK_mil]=S5; trans[S1][TK_wy]=S1a; 
  // State S1a
  trans[S1a][TK_wmedio]=S1b; trans[S1a][TK_wcuarto]=S1b;
  // State S1b
     // nothing else expected
  // State S2
  trans[S2][TK_d]=S3;   trans[S2][TK_u]=Su; 
  trans[S2][TK_mil]=S5; trans[S2][TK_num]=Su; 
  // State S3
  trans[S3][TK_wy]=S4; trans[S3][TK_mil]=S5;
  // State S4
  trans[S4][TK_u]=Su;  trans[S4][TK_num]=Su;
  // State Su
  trans[Su][TK_mil]=S5;
  // State S5
  trans[S5][TK_c]=S6; trans[S5][TK_d]=S7;
  trans[S5][TK_u]=Sk; trans[S5][TK_num]=Sk; 
  // State S6
  trans[S6][TK_d]=S7; trans[S6][TK_u]=Sk; trans[S6][TK_num]=Sk;  
  // State S7
  trans[S7][TK_wy]=S8;
  // State S8
  trans[S8][TK_u]=Sk; trans[S8][TK_num]=Sk; 
  // State Sk
  // nothing else is expected
  // State COD
  // nothing else is expected
  // State X1
  trans[X1][TK_wy]=X2;
  // State X2
  trans[X2][TK_wmedio]=X3;  trans[X2][TK_wcuarto]=X3;
  // State X3
  // nothing else is expected
  // State X4
  trans[X4][TK_bill]=M1;  trans[X4][TK_mill]=S1;   
  trans[X4][TK_milr]=X5;  trans[X4][TK_cent]=X5;
  trans[X4][TK_dec]=X5;   trans[X4][TK_doc]=X5;
  // State X5
  // nothing else is expected
 
  TRACE(3,"analyzer succesfully created");
}


//-- Implementation of virtual functions from class automat --//

///////////////////////////////////////////////////////////////
///  Compute the right token code for word j from given state.
///////////////////////////////////////////////////////////////

int numbers_es::ComputeToken(int state, sentence::iterator &j, sentence &se)
{
  string form;
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
  if (RE_number.Search(form)) token = TK_num;
  else if (RE_code.Search(form)) token = TK_code;

  TRACE(3,"Leaving state "+util::int2string(state)+" with token "+util::int2string(token)); 
  return (token);
}


///////////////////////////////////////////////////////////////
///   Reset acumulators used by state actions:
///   bilion, milion, units.
///////////////////////////////////////////////////////////////

void numbers_es::ResetActions() 
{
 iscode=0;
 milion=0; bilion=0; units=0;
 block=0;
}


///////////////////////////////////////////////////////////////
///  Perform necessary actions in "state" reached from state 
///  "origin" via word j interpreted as code "token":
///  Basically, when reaching a state, update current 
///  nummerical value.
///////////////////////////////////////////////////////////////

void numbers_es::StateActions(int origin, int state, int token, sentence::const_iterator j)
{
  string form;
  size_t i;
  long double num;

  form = util::lowercase(j->get_form());
  TRACE(3,"Reaching state "+util::int2string(state)+" with token "+util::int2string(token)+" for word ["+form+"]");

  // get token numerical value, if any
  num=0;
  if (token==TK_num) {
    // erase thousand points
    while ((i=form.find_first_of(MACO_Thousand))!=string::npos) {
      form.erase(i,1);
    }
    TRACE(3,"after erasing thousand "+form);
    // make sure decimal point is "."
    if ((i=form.find_last_of(MACO_Decimal))!=string::npos) {
      form[i]='.';
      TRACE(3,"after replacing decimal "+form);
    }

    num = util::string2longdouble(form);
  }

  // State actions
  switch (state) {
  // ---------------------------------
  case B2:  case M2:  case S2:
  case B3:  case M3:  case S3:
  case Bu:  case Mu:  case Su:
  case B6:  case M6:  case S6:
  case B7:  case M7:  case S7:
  case Bk:  case Mk:  case Sk:
    TRACE(3,"Actions for normal state");
    if (token==TK_num)
      units += num;
    else
      units += value[form];
    break;
  // ---------------------------------
  case B5:  case M5:  case S5:
    TRACE(3,"Actions for 'thousands' state");
    if (units==0) units=1; // not said: "uno mil ciento treinta y dos"
    units *= power[token];
    break;
  // ---------------------------------
  case M1:
    TRACE(3,"Actions for M1 (bilion) state");
    // store bilion value and get ready in case milions are caming
    bilion = units*power[token];
    units=0;
    break;
  // ---------------------------------
  case M1b:
    TRACE(3,"Actions for M1b state");
    // "dos billones y medio/cuarto"
    bilion += value[form]*power[TK_bill];
    break;
  // ---------------------------------
  case S1:
    TRACE(3,"Actions for S1 (milion) state");
    // store milion value and get ready in case thousands are caming
    milion = units*power[token];
    units=0;
    break;
  // ---------------------------------
  case S1b:
    TRACE(3,"Actions for S1b state");
    // "dos millones y medio/cuarto"
    milion += value[form]*power[TK_mill];
    break;
  // ---------------------------------
  case X1:
    TRACE(3,"Actions for X1 state");
    // "Z millares/decenas/centenares/docenas"
    units *= power[token];
    block = token;
    break;
  // ---------------------------------
  case X3:
    TRACE(3,"Actions for X3 state");
    // "Z millares/decenas/centenares/docenas y medio/cuarto"
    units += value[form]*power[block];
    break;
  // ---------------------------------
  case X4:
    TRACE(3,"Actions for X4 state");
    // "medio" as 1st word
    units = value[form];
    break;
  // ---------------------------------
  case X5:
    TRACE(3,"Actions for X5 state");
    // "medio millar/millon/centenar/docena/etc
    units *= power[token];
    break;
  // ---------------------------------
  case COD:
    TRACE(3,"Actions for COD state");
    iscode = CODE;
    break;
  // ---------------------------------
  default: break;
  }

  TRACE(3,"State actions completed. bilion="+util::longdouble2string(bilion)+" milion="+util::longdouble2string(milion)+" units="+util::longdouble2string(units));
}


///////////////////////////////////////////////////////////////
///   Set the appropriate lemma and parole for the 
///   new multiword.
///////////////////////////////////////////////////////////////

void numbers_es::SetMultiwordAnalysis(sentence::iterator i, int fstate)
{
  string lemma;

  // Setting the analysis for the nummerical expression
  if (iscode==CODE) {
    lemma=i->get_form();
  }
  else {
    // compute nummerical value as lemma
    lemma=util::longdouble2string(bilion+milion+units);
  }

  if (fstate==S1 || fstate==S1b || fstate==M1 ||
      fstate==M1b || fstate==X1 || fstate==X3 || fstate==X5) {
    i->set_analysis(analysis(lemma,"Zd"));
    TRACE(3,"Analysis set to: "+lemma+" Zd");
  }
  else {
    i->set_analysis(analysis(lemma,"Z"));
    TRACE(3,"Analysis set to: "+lemma+" Z");
  }
}


// undef codes to prepare for next module definition
#undef B1
#undef B2
#undef B3
#undef B4
#undef Bu
#undef B5
#undef B6
#undef B7
#undef B8
#undef Bk
#undef M1
#undef M1a
#undef M1b
#undef M2 
#undef M3 
#undef M4 
#undef Mu 
#undef M5 
#undef M6 
#undef M7 
#undef M8 
#undef Mk 
#undef S1 
#undef S1a
#undef S1b
#undef S2 
#undef S3 
#undef S4 
#undef Su 
#undef S5 
#undef S6 
#undef S7 
#undef S8 
#undef Sk
#undef COD
#undef STOP
#undef TK_c 
#undef TK_d 
#undef TK_u 
#undef TK_wy
#undef TK_wmedio
#undef TK_wcuarto
#undef TK_mil
#undef TK_mill
#undef TK_bill
#undef TK_num 
#undef TK_code
#undef TK_other


//---------------------------------------------------------------------------
//        Catalan number recognizer
//---------------------------------------------------------------------------

//// State codes 
#define B1 1  // initial state
#define B2 2  // got hundreds "doscientos"  (VALID NUMBER)
#define B3 3  // got tens "treinta" "docientos treinta"   (VALID NUMBER)
#define B4 4  // got "y" after tens
#define Bu 5  // got units after "y": "doscientos treinta y cuatro"   (VALID NUMBER)
#define B5 6  // got "mil" after unit "doscientos treinta y cuatro mil"   (VALID NUMBER)
#define B6 7  // got hundreds after "mil"   (VALID NUMBER)
#define B7 8  // got tens after "mil"   (VALID NUMBER)
#define B8 9  // got "y" after tens   (VALID NUMBER)
#define Bk 10 // got units after "y"   (VALID NUMBER)
#define M1 11 // got "billones" after a valid number  (VALID NUMBER)
#define M1a 12 // got "y" after "billones"
#define M1b 13 // got "... millones y medio/cuarto" 
#define M2 14 // got hundreds "doscientos" after billions  (VALID NUMBER)
#define M3 15 // got tens "treinta" "docientos treinta"   (VALID NUMBER)
#define M4 16 // got "y" after tens
#define Mu 17 // got units after "y": "doscientos treinta y cuatro"   (VALID NUMBER)
#define M5 18 // got "mil" after unit "doscientos treinta y cuatro mil"   (VALID NUMBER)
#define M6 19 // got hundreds after "mil"   (VALID NUMBER)
#define M7 20 // got tens after "mil"   (VALID NUMBER)
#define M8 21 // got "y" after tens   (VALID NUMBER)
#define Mk 22 // got units after "y"   (VALID NUMBER)
#define S1 23 // got "millones" after a valid number  (VALID NUMBER) 
#define S1a 24 // got "y" after "millones"
#define S1b 25 // got "... millones y medio/cuarto" 
#define S2 26 // got hundreds "doscientos" after millions  (VALID NUMBER)
#define S3 27 // got tens "treinta" "docientos treinta"   (VALID NUMBER)
#define S4 28 // got "y" after tens
#define Su 29 // got units after "y": "doscientos treinta y cuatro"   (VALID NUMBER)
#define S5 30 // got "mil" after unit "doscientos treinta y cuatro mil"   (VALID NUMBER)
#define S6 31 // got hundreds after "mil"   (VALID NUMBER)
#define S7 32 // got tens after "mil"   (VALID NUMBER)
#define S8 33 // got "y" after tens   (VALID NUMBER)
#define Sk 34 // got units after "y"   (VALID NUMBER)
#define COD 35 // got pseudo-numerical code from initial state
// stop state
#define STOP 36

// Token codes
#define TK_c      1  // hundreds "cien" "doscientos" 
#define TK_d      2  // tens "treinta" "cuarenta"
#define TK_u      3  // units "tres" "cuatro"
#define TK_wy     4  // word "y"
#define TK_wmedio 5  // word "medio"
#define TK_wcuarto 6 // word "cuarto"
#define TK_mil    7  // word "mil"   
#define TK_mill   8  // word "millon" "millones"
#define TK_bill   9  // word "billon" "billones"
#define TK_num   10  // a number (in digits)
#define TK_code  11  // a code (ex. LX-345-2)
#define TK_other 12
 
 
///////////////////////////////////////////////////////////////
///  Create a numbers recognizer for Catalan.
///////////////////////////////////////////////////////////////

numbers_ca::numbers_ca(const std::string &dec, const std::string &thou): numbers_module(dec,thou)
{  
  // Initializing value map
  value.insert(make_pair("cent",100));         value.insert(make_pair("dos-cents",200));
  value.insert(make_pair("dos-centes",200));   value.insert(make_pair("dues-centes",200));
  value.insert(make_pair("tres-cents",300));   value.insert(make_pair("tres-centes",300));
  value.insert(make_pair("quatre-cents",400)); value.insert(make_pair("quatre-centes",400));
  value.insert(make_pair("cinc-cents",500));   value.insert(make_pair("cinc-centes",500));
  value.insert(make_pair("sis-cents",600));    value.insert(make_pair("sis-centes",600));
  value.insert(make_pair("set-cents",700));    value.insert(make_pair("set-centes",700));
  value.insert(make_pair("vuit-cents",800));   value.insert(make_pair("vuit-centes",800));
  value.insert(make_pair("huit-cents",800));   value.insert(make_pair("huit-centes",800));
  value.insert(make_pair("nou-cents",900));    value.insert(make_pair("nou-centes",900));
  value.insert(make_pair("trenta",30));        value.insert(make_pair("quaranta",40));
  value.insert(make_pair("cinquanta",50));     value.insert(make_pair("seixanta",60));
  value.insert(make_pair("setanta",70));       value.insert(make_pair("vuitanta",80));
  value.insert(make_pair("huitanta",80));      value.insert(make_pair("noranta",90));     
  value.insert(make_pair("un",1));             value.insert(make_pair("una",1));
  value.insert(make_pair("dos",2));            value.insert(make_pair("dues",2));
  value.insert(make_pair("tres",3));           value.insert(make_pair("quatre",4));
  value.insert(make_pair("cinc",5));           value.insert(make_pair("sis",6));
  value.insert(make_pair("set",7));            value.insert(make_pair("vuit",8));
  value.insert(make_pair("nou",9));            value.insert(make_pair("deu",10));
  value.insert(make_pair("onze",11));          value.insert(make_pair("dotze",12));
  value.insert(make_pair("tretze",13));        value.insert(make_pair("catorze",14));
  value.insert(make_pair("quinze",15));        value.insert(make_pair("setze",16));
  value.insert(make_pair("disset",17));        value.insert(make_pair("desset",17));        
  value.insert(make_pair("dèsset",17));        
  value.insert(make_pair("divuit",18));        value.insert(make_pair("devuit",18)); 
  value.insert(make_pair("dinou",19));         value.insert(make_pair("denou",19)); 
  value.insert(make_pair("vint", 20));  
  value.insert(make_pair("vint-i-u",21));      value.insert(make_pair("vint-i-una",21));  
  value.insert(make_pair("vint-i-un",21));     value.insert(make_pair("vint-i-dos",22));
  value.insert(make_pair("vint-i-dues",22));   value.insert(make_pair("vint-i-tres",23));
  value.insert(make_pair("vint-i-quatre",24)); value.insert(make_pair("vint-i-cinc",25));  
  value.insert(make_pair("vint-i-sis",26));    value.insert(make_pair("vint-i-set",27));
  value.insert(make_pair("vint-i-vuit",28));   value.insert(make_pair("vint-i-nou",29));
  value.insert(make_pair("trenta-u",31));      value.insert(make_pair("trenta-una",31));  
  value.insert(make_pair("trenta-un",31));     value.insert(make_pair("trenta-dos",32));
  value.insert(make_pair("trenta-dues",32));   value.insert(make_pair("trenta-tres",33));
  value.insert(make_pair("trenta-quatre",34)); value.insert(make_pair("trenta-cinc",35));  
  value.insert(make_pair("trenta-sis",36));    value.insert(make_pair("trenta-set",37));
  value.insert(make_pair("trenta-vuit",38));   value.insert(make_pair("trenta-nou",39));
  value.insert(make_pair("quaranta-u",41));      value.insert(make_pair("quaranta-una",41));  
  value.insert(make_pair("quaranta-un",41));     value.insert(make_pair("quaranta-dos",42));
  value.insert(make_pair("quaranta-dues",42));   value.insert(make_pair("quaranta-tres",43));
  value.insert(make_pair("quaranta-quatre",44)); value.insert(make_pair("quaranta-cinc",45));  
  value.insert(make_pair("quaranta-sis",46));    value.insert(make_pair("quaranta-set",47));
  value.insert(make_pair("quaranta-vuit",48));   value.insert(make_pair("quaranta-nou",49));
  value.insert(make_pair("cinquanta-u",51));      value.insert(make_pair("cinquanta-una",51));  
  value.insert(make_pair("cinquanta-un",51));     value.insert(make_pair("cinquanta-dos",52));
  value.insert(make_pair("cinquanta-dues",52));   value.insert(make_pair("cinquanta-tres",53));
  value.insert(make_pair("cinquanta-quatre",54)); value.insert(make_pair("cinquanta-cinc",55));  
  value.insert(make_pair("cinquanta-sis",56));    value.insert(make_pair("cinquanta-set",57));
  value.insert(make_pair("cinquanta-vuit",58));   value.insert(make_pair("cinquanta-nou",59));
  value.insert(make_pair("seixanta-u",61));      value.insert(make_pair("seixanta-una",61));  
  value.insert(make_pair("seixanta-un",61));     value.insert(make_pair("seixanta-dos",62));
  value.insert(make_pair("seixanta-dues",62));   value.insert(make_pair("seixanta-tres",63));
  value.insert(make_pair("seixanta-quatre",64)); value.insert(make_pair("seixanta-cinc",65));  
  value.insert(make_pair("seixanta-sis",66));    value.insert(make_pair("seixanta-set",67));
  value.insert(make_pair("seixanta-vuit",68));   value.insert(make_pair("seixanta-nou",69));
  value.insert(make_pair("setanta-u",71));      value.insert(make_pair("setanta-una",71));  
  value.insert(make_pair("setanta-un",71));     value.insert(make_pair("setanta-dos",72));
  value.insert(make_pair("setanta-dues",72));   value.insert(make_pair("setanta-tres",73));
  value.insert(make_pair("setanta-quatre",74)); value.insert(make_pair("setanta-cinc",75));  
  value.insert(make_pair("setanta-sis",76));    value.insert(make_pair("setanta-set",77));
  value.insert(make_pair("setanta-vuit",78));   value.insert(make_pair("setanta-nou",79));
  value.insert(make_pair("vuitanta-u",81));      value.insert(make_pair("vuitanta-una",81));  
  value.insert(make_pair("vuitanta-un",81));     value.insert(make_pair("vuitanta-dos",82));
  value.insert(make_pair("vuitanta-dues",82));   value.insert(make_pair("vuitanta-tres",83));
  value.insert(make_pair("vuitanta-quatre",84)); value.insert(make_pair("vuitanta-cinc",85));  
  value.insert(make_pair("vuitanta-sis",86));    value.insert(make_pair("vuitanta-set",87));
  value.insert(make_pair("vuitanta-vuit",88));   value.insert(make_pair("vuitanta-nou",89));
  value.insert(make_pair("noranta-u",91));      value.insert(make_pair("noranta-una",91));  
  value.insert(make_pair("noranta-un",91));     value.insert(make_pair("noranta-dos",92));
  value.insert(make_pair("noranta-dues",92));   value.insert(make_pair("noranta-tres",93));
  value.insert(make_pair("noranta-quatre",94)); value.insert(make_pair("noranta-cinc",95));  
  value.insert(make_pair("noranta-sis",96));    value.insert(make_pair("noranta-set",97));
  value.insert(make_pair("noranta-vuit",98));   value.insert(make_pair("noranta-nou",99));

  value.insert(make_pair("mig",0.5));  value.insert(make_pair("quart",0.25));

  // Initializing token map
  tok.insert(make_pair("cent",TK_c));         tok.insert(make_pair("dos-cents",TK_c));
  tok.insert(make_pair("dos-centes",TK_c));   tok.insert(make_pair("dues-centes",TK_c));
  tok.insert(make_pair("tres-cents",TK_c));   tok.insert(make_pair("tres-centes",TK_c));
  tok.insert(make_pair("quatre-cents",TK_c)); tok.insert(make_pair("quatre-centes",TK_c));
  tok.insert(make_pair("cinc-cents",TK_c));   tok.insert(make_pair("cinc-centes",TK_c));
  tok.insert(make_pair("sis-cents",TK_c));    tok.insert(make_pair("sis-centes",TK_c));
  tok.insert(make_pair("set-cents",TK_c));    tok.insert(make_pair("set-centes",TK_c));
  tok.insert(make_pair("vuit-cents",TK_c));   tok.insert(make_pair("vuit-centes",TK_c));
  tok.insert(make_pair("huit-cents",TK_c));   tok.insert(make_pair("huit-centes",TK_c));
  tok.insert(make_pair("nou-cents",TK_c));    tok.insert(make_pair("nou-centes",TK_c));
  tok.insert(make_pair("trenta",TK_u));       tok.insert(make_pair("quaranta",TK_u));
  tok.insert(make_pair("cinquanta",TK_u));    tok.insert(make_pair("seixanta",TK_u));
  tok.insert(make_pair("setanta",TK_u));      tok.insert(make_pair("vuitanta",TK_u));
  tok.insert(make_pair("huitanta",TK_u));     tok.insert(make_pair("noranta",TK_u));     
  tok.insert(make_pair("un",TK_u));             tok.insert(make_pair("una",TK_u));
  tok.insert(make_pair("dos",TK_u));            tok.insert(make_pair("dues",TK_u));
  tok.insert(make_pair("tres",TK_u));           tok.insert(make_pair("quatre",TK_u));
  tok.insert(make_pair("cinc",TK_u));           tok.insert(make_pair("sis",TK_u));
  tok.insert(make_pair("set",TK_u));            tok.insert(make_pair("vuit",TK_u));
  tok.insert(make_pair("nou",TK_u));            tok.insert(make_pair("deu",TK_u));
  tok.insert(make_pair("onze",TK_u));          tok.insert(make_pair("dotze",TK_u));
  tok.insert(make_pair("tretze",TK_u));        tok.insert(make_pair("catorze",TK_u));
  tok.insert(make_pair("quinze",TK_u));        tok.insert(make_pair("setze",TK_u));
  tok.insert(make_pair("disset",TK_u));        tok.insert(make_pair("desset",TK_u)); 
  tok.insert(make_pair("dèsset",TK_u)); 
  tok.insert(make_pair("divuit",TK_u));        tok.insert(make_pair("devuit",TK_u)); 
  tok.insert(make_pair("dinou",TK_u));         tok.insert(make_pair("denou",TK_u));        
  tok.insert(make_pair("vint", TK_u));  
  tok.insert(make_pair("vint-i-u",TK_u));      tok.insert(make_pair("vint-i-una",TK_u));  
  tok.insert(make_pair("vint-i-un",TK_u));     tok.insert(make_pair("vint-i-dos",TK_u));
  tok.insert(make_pair("vint-i-dues",TK_u));   tok.insert(make_pair("vint-i-tres",TK_u));
  tok.insert(make_pair("vint-i-quatre",TK_u)); tok.insert(make_pair("vint-i-cinc",TK_u));  
  tok.insert(make_pair("vint-i-sis",TK_u));    tok.insert(make_pair("vint-i-set",TK_u));
  tok.insert(make_pair("vint-i-vuit",TK_u));   tok.insert(make_pair("vint-i-nou",TK_u));
  tok.insert(make_pair("trenta-u",TK_u));      tok.insert(make_pair("trenta-una",TK_u));  
  tok.insert(make_pair("trenta-un",TK_u));     tok.insert(make_pair("trenta-dos",TK_u));
  tok.insert(make_pair("trenta-dues",TK_u));   tok.insert(make_pair("trenta-tres",TK_u));
  tok.insert(make_pair("trenta-quatre",TK_u)); tok.insert(make_pair("trenta-cinc",TK_u));  
  tok.insert(make_pair("trenta-sis",TK_u));    tok.insert(make_pair("trenta-set",TK_u));
  tok.insert(make_pair("trenta-vuit",TK_u));   tok.insert(make_pair("trenta-nou",TK_u));
  tok.insert(make_pair("quaranta-u",TK_u));      tok.insert(make_pair("quaranta-una",TK_u));  
  tok.insert(make_pair("quaranta-un",TK_u));     tok.insert(make_pair("quaranta-dos",TK_u));
  tok.insert(make_pair("quaranta-dues",TK_u));   tok.insert(make_pair("quaranta-tres",TK_u));
  tok.insert(make_pair("quaranta-quatre",TK_u)); tok.insert(make_pair("quaranta-cinc",TK_u));  
  tok.insert(make_pair("quaranta-sis",TK_u));    tok.insert(make_pair("quaranta-set",TK_u));
  tok.insert(make_pair("quaranta-vuit",TK_u));   tok.insert(make_pair("quaranta-nou",TK_u));
  tok.insert(make_pair("cinquanta-u",TK_u));      tok.insert(make_pair("cinquanta-una",TK_u));  
  tok.insert(make_pair("cinquanta-un",TK_u));     tok.insert(make_pair("cinquanta-dos",TK_u));
  tok.insert(make_pair("cinquanta-dues",TK_u));   tok.insert(make_pair("cinquanta-tres",TK_u));
  tok.insert(make_pair("cinquanta-quatre",TK_u)); tok.insert(make_pair("cinquanta-cinc",TK_u));  
  tok.insert(make_pair("cinquanta-sis",TK_u));    tok.insert(make_pair("cinquanta-set",TK_u));
  tok.insert(make_pair("cinquanta-vuit",TK_u));   tok.insert(make_pair("cinquanta-nou",TK_u));
  tok.insert(make_pair("seixanta-u",TK_u));      tok.insert(make_pair("seixanta-una",TK_u));  
  tok.insert(make_pair("seixanta-un",TK_u));     tok.insert(make_pair("seixanta-dos",TK_u));
  tok.insert(make_pair("seixanta-dues",TK_u));   tok.insert(make_pair("seixanta-tres",TK_u));
  tok.insert(make_pair("seixanta-quatre",TK_u)); tok.insert(make_pair("seixanta-cinc",TK_u));  
  tok.insert(make_pair("seixanta-sis",TK_u));    tok.insert(make_pair("seixanta-set",TK_u));
  tok.insert(make_pair("seixanta-vuit",TK_u));   tok.insert(make_pair("seixanta-nou",TK_u));
  tok.insert(make_pair("setanta-u",TK_u));      tok.insert(make_pair("setanta-una",TK_u));  
  tok.insert(make_pair("setanta-un",TK_u));     tok.insert(make_pair("setanta-dos",TK_u));
  tok.insert(make_pair("setanta-dues",TK_u));   tok.insert(make_pair("setanta-tres",TK_u));
  tok.insert(make_pair("setanta-quatre",TK_u)); tok.insert(make_pair("setanta-cinc",TK_u));  
  tok.insert(make_pair("setanta-sis",TK_u));    tok.insert(make_pair("setanta-set",TK_u));
  tok.insert(make_pair("setanta-vuit",TK_u));   tok.insert(make_pair("setanta-nou",TK_u));
  tok.insert(make_pair("vuitanta-u",TK_u));      tok.insert(make_pair("vuitanta-una",TK_u));  
  tok.insert(make_pair("vuitanta-un",TK_u));     tok.insert(make_pair("vuitanta-dos",TK_u));
  tok.insert(make_pair("vuitanta-dues",TK_u));   tok.insert(make_pair("vuitanta-tres",TK_u));
  tok.insert(make_pair("vuitanta-quatre",TK_u)); tok.insert(make_pair("vuitanta-cinc",TK_u));  
  tok.insert(make_pair("vuitanta-sis",TK_u));    tok.insert(make_pair("vuitanta-set",TK_u));
  tok.insert(make_pair("vuitanta-vuit",TK_u));   tok.insert(make_pair("vuitanta-nou",TK_u));
  tok.insert(make_pair("noranta-u",TK_u));      tok.insert(make_pair("noranta-una",TK_u));  
  tok.insert(make_pair("noranta-un",TK_u));     tok.insert(make_pair("noranta-dos",TK_u));
  tok.insert(make_pair("noranta-dues",TK_u));   tok.insert(make_pair("noranta-tres",TK_u));
  tok.insert(make_pair("noranta-quatre",TK_u)); tok.insert(make_pair("noranta-cinc",TK_u));  
  tok.insert(make_pair("noranta-sis",TK_u));    tok.insert(make_pair("noranta-set",TK_u));
  tok.insert(make_pair("noranta-vuit",TK_u));   tok.insert(make_pair("noranta-nou",TK_u));
  tok.insert(make_pair("mil",TK_mil));
  tok.insert(make_pair("milió",TK_mill));  tok.insert(make_pair("milions",TK_mill));
  tok.insert(make_pair("bilió",TK_bill));  tok.insert(make_pair("bilions",TK_bill));
  tok.insert(make_pair("i",TK_wy));
  tok.insert(make_pair("mig",TK_wmedio));  tok.insert(make_pair("quart",TK_wcuarto));

  // Initializing power map
  power.insert(make_pair(TK_mil,  1000.0));
  power.insert(make_pair(TK_mill, 1000000.0));
  power.insert(make_pair(TK_bill, 1000000000000.0));

  // Initialize special state attributes
  initialState=B1; stopState=STOP;

  // Initialize Final state set 
  Final.insert(B2);  Final.insert(B3);  Final.insert(Bu);  Final.insert(B5);
  Final.insert(B6);  Final.insert(B7);  Final.insert(Bk); 
  Final.insert(M1);  Final.insert(M2);  Final.insert(M3);  Final.insert(Mu);
  Final.insert(M5);  Final.insert(M6);  Final.insert(M7);  Final.insert(Mk);
  Final.insert(S1);  Final.insert(S2);  Final.insert(S3);  Final.insert(Su);
  Final.insert(S5);  Final.insert(S6);  Final.insert(S7);  Final.insert(Sk);
  Final.insert(M1b); Final.insert(S1b); Final.insert(COD);  

  // Initialize transitions table. By default, stop state
  int s,t;
  for(s=0;s<MAX_STATES;s++) for(t=0;t<MAX_TOKENS;t++) trans[s][t]=STOP;
  
  // Initializing transitions table
  // State B1
  trans[B1][TK_c]=B2;   trans[B1][TK_d]=B3;   trans[B1][TK_u]=Bu; 
  trans[B1][TK_mil]=B5; trans[B1][TK_num]=Bu; trans[B1][TK_code]=COD;
  // State B2
  trans[B2][TK_d]=B3;    trans[B2][TK_u]=Bu;    trans[B2][TK_mil]=B5; 
  trans[B2][TK_bill]=M1; trans[B2][TK_mill]=S1; trans[B2][TK_num]=Bu;
  // State B3
  trans[B3][TK_wy]=B4;   trans[B3][TK_mil]=B5; 
  trans[B3][TK_bill]=M1; trans[B3][TK_mill]=S1;
  // State B4
  trans[B4][TK_u]=Bu; trans[B4][TK_num]=Bu;
  // State Bu
  trans[Bu][TK_mil]=B5; trans[Bu][TK_bill]=M1; trans[Bu][TK_mill]=S1;
  // State B5
  trans[B5][TK_c]=B6;    trans[B5][TK_d]=B7;    trans[B5][TK_u]=Bk; 
  trans[B5][TK_bill]=M1; trans[B5][TK_mill]=S1; trans[B5][TK_num]=Bk;
  // State B6
  trans[B6][TK_d]=B7;    trans[B6][TK_u]=Bk;    trans[B6][TK_num]=Bk;
  trans[B6][TK_bill]=M1; trans[B6][TK_mill]=S1;
  // State B7
  trans[B7][TK_wy]=B8; trans[B7][TK_bill]=M1; trans[B7][TK_mill]=S1;
  // State B8
  trans[B8][TK_u]=Bk;  trans[B8][TK_num]=Bk; 
  // State Bk
  trans[Bk][TK_bill]=M1; trans[Bk][TK_mill]=S1;

  // State M1
  trans[M1][TK_c]=M2; trans[M1][TK_d]=M3;   trans[M1][TK_num]=Mu; 
  trans[M1][TK_u]=Mu; trans[M1][TK_mil]=M5; trans[M1][TK_wy]=M1a; 
  // State M1a
  trans[M1a][TK_wmedio]=M1b; trans[M1a][TK_wcuarto]=M1b;
  // State M1b
    // nothing else expected
  // State M2
  trans[M2][TK_d]=M3;    trans[M2][TK_u]=Mu;   trans[M2][TK_num]=Mu; 
  trans[M2][TK_mil]=M5;  trans[M2][TK_mill]=S1;
  // State M3
  trans[M3][TK_wy]=M4;   trans[M3][TK_mil]=M5;  trans[M3][TK_mill]=S1;
  // State M4
  trans[M4][TK_u]=Mu;  trans[M4][TK_num]=Mu;
  // State Mu
  trans[Mu][TK_mil]=M5; trans[Mu][TK_mill]=S1;
  // State M5
  trans[M5][TK_c]=M6;   trans[M5][TK_d]=M7;   trans[M5][TK_num]=Mk; 
  trans[M5][TK_u]=Mk;   trans[M5][TK_mill]=S1;
  // State M6
  trans[M6][TK_d]=M7;    trans[M6][TK_u]=Mk;
  trans[M6][TK_mill]=S1; trans[M6][TK_num]=Mk; 
  // State M7
  trans[M7][TK_wy]=M8; trans[M7][TK_mill]=S1;
  // State M8
  trans[M8][TK_u]=Mk;   trans[M8][TK_num]=Mk; 
  // State Mk
  trans[Mk][TK_mill]=S1;

  // State S1
  trans[S1][TK_c]=S2; trans[S1][TK_d]=S3;   trans[S1][TK_num]=Su; 
  trans[S1][TK_u]=Su; trans[S1][TK_mil]=S5; trans[S1][TK_wy]=S1a; 
  // State S1a
  trans[S1a][TK_wmedio]=S1b; trans[S1a][TK_wcuarto]=S1b;
  // State S1b
     // nothing else expected
  // State S2
  trans[S2][TK_d]=S3;   trans[S2][TK_u]=Su; 
  trans[S2][TK_mil]=S5; trans[S2][TK_num]=Su; 
  // State S3
  trans[S3][TK_wy]=S4; trans[S3][TK_mil]=S5;
  // State S4
  trans[S4][TK_u]=Su;  trans[S4][TK_num]=Su;
  // State Su
  trans[Su][TK_mil]=S5;
  // State S5
  trans[S5][TK_c]=S6; trans[S5][TK_d]=S7;
  trans[S5][TK_u]=Sk; trans[S5][TK_num]=Sk; 
  // State S6
  trans[S6][TK_d]=S7; trans[S6][TK_u]=Sk; trans[S6][TK_num]=Sk;  
  // State S7
  trans[S7][TK_wy]=S8;
  // State S8
  trans[S8][TK_u]=Sk; trans[S8][TK_num]=Sk; 
  // State Sk
  // nothing else is expected
  // State COD
  // nothing else is expected
 
  TRACE(3,"analyzer succesfully created");
}


//-- Implementation of virtual functions from class automat --//

///////////////////////////////////////////////////////////////
///  Compute the right token code for word j from given state.
///////////////////////////////////////////////////////////////

int numbers_ca::ComputeToken(int state, sentence::iterator &j, sentence &se)
{
  string form;
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
  if (RE_number.Search(form)) token = TK_num;
  else if (RE_code.Search(form)) token = TK_code;

  TRACE(3,"Leaving state "+util::int2string(state)+" with token "+util::int2string(token)); 
  return (token);
}


///////////////////////////////////////////////////////////////
///   Reset acumulators used by state actions:
///   bilion, milion, units.
///////////////////////////////////////////////////////////////

void numbers_ca::ResetActions() 
{
 iscode=0;
 milion=0; bilion=0; units=0;
 block=0;
}


///////////////////////////////////////////////////////////////
///  Perform necessary actions in "state" reached from state 
///  "origin" via word j interpreted as code "token":
///  Basically, when reaching a state, update current 
///  nummerical value.
///////////////////////////////////////////////////////////////

void numbers_ca::StateActions(int origin, int state, int token, sentence::const_iterator j)
{
  string form;
  size_t i;
  long double num;

  form = util::lowercase(j->get_form());
  TRACE(3,"Reaching state "+util::int2string(state)+" with token "+util::int2string(token)+" for word ["+form+"]");

  // get token numerical value, if any
  num=0;
  if (token==TK_num) {
    // erase thousand points
    while ((i=form.find_first_of(MACO_Thousand))!=string::npos) {
      form.erase(i,1);
    }
    TRACE(3,"after erasing thousand "+form);
    // make sure decimal point is "."
    if ((i=form.find_last_of(MACO_Decimal))!=string::npos) {
      form[i]='.';
      TRACE(3,"after replacing decimal "+form);
    }

    num = util::string2longdouble(form);
  }

  // State actions
  switch (state) {
  // ---------------------------------
  case B2:  case M2:  case S2:
  case B3:  case M3:  case S3:
  case Bu:  case Mu:  case Su:
  case B6:  case M6:  case S6:
  case B7:  case M7:  case S7:
  case Bk:  case Mk:  case Sk:
    TRACE(3,"Actions for normal state");
    if (token==TK_num)
      units += num;
    else
      units += value[form];
    break;
  // ---------------------------------
  case B5:  case M5:  case S5:
    TRACE(3,"Actions for 'thousands' state");
    if (units==0) units=1; // not said: "uno mil ciento treinta y dos"
    units *= power[token];
    break;
  // ---------------------------------
  case M1:
    TRACE(3,"Actions for M1 (bilion) state");
    // store bilion value and get ready in case milions are caming
    bilion = units*power[token];
    units=0;
    break;
  // ---------------------------------
  case M1b:
    TRACE(3,"Actions for M1b state");
    // "dos billones y medio/cuarto"
    bilion += value[form]*power[TK_bill];
    break;
  // ---------------------------------
  case S1:
    TRACE(3,"Actions for S1 (milion) state");
    // store milion value and get ready in case thousands are caming
    milion = units*power[token];
    units=0;
    break;
  // ---------------------------------
  case S1b:
    TRACE(3,"Actions for S1b state");
    // "dos millones y medio/cuarto"
    milion += value[form]*power[TK_mill];
    break;
  // ---------------------------------
  case COD:
    TRACE(3,"Actions for COD state");
    iscode = CODE;
    break;
  // ---------------------------------
  default: break;
  }

  TRACE(3,"State actions completed. bilion="+util::longdouble2string(bilion)+" milion="+util::longdouble2string(milion)+" units="+util::longdouble2string(units));
}


///////////////////////////////////////////////////////////////
///   Set the appropriate lemma and parole for the 
///   new multiword.
///////////////////////////////////////////////////////////////

void numbers_ca::SetMultiwordAnalysis(sentence::iterator i, int fstate)
{
  string lemma;

  // Setting the analysis for the nummerical expression
  if (iscode==CODE) {
    lemma=i->get_form();
  }
  else {
    // compute nummerical value as lemma
    lemma=util::longdouble2string(bilion+milion+units);
  }

  i->set_analysis(analysis(lemma,"Z"));
  TRACE(3,"Analysis set to: "+lemma+" Z");
}


// undef codes to prepare for next module definition
#undef B1
#undef B2
#undef B3
#undef B4
#undef Bu
#undef B5
#undef B6
#undef B7
#undef B8
#undef Bk
#undef M1
#undef M1a
#undef M1b
#undef M2 
#undef M3 
#undef M4 
#undef Mu 
#undef M5 
#undef M6 
#undef M7 
#undef M8 
#undef Mk 
#undef S1 
#undef S1a
#undef S1b
#undef S2 
#undef S3 
#undef S4 
#undef Su 
#undef S5 
#undef S6 
#undef S7 
#undef S8 
#undef Sk
#undef COD
#undef STOP
#undef TK_c 
#undef TK_d 
#undef TK_u 
#undef TK_wy
#undef TK_wmedio
#undef TK_wcuarto
#undef TK_mil
#undef TK_mill
#undef TK_bill
#undef TK_num 
#undef TK_code
#undef TK_other

//---------------------------------------------------------------------------
//        Galician number recognizer
//---------------------------------------------------------------------------

//// State codes 
#define B1 1  // initial state
#define B2 2  // got hundreds "doscientos"  (VALID NUMBER)
#define B3 3  // got tens "treinta" "docientos treinta"   (VALID NUMBER)
#define B4 4  // got "y" after tens
#define Bu 5  // got units after "y": "doscientos treinta y cuatro"   (VALID NUMBER)
#define B5 6  // got "mil" after unit "doscientos treinta y cuatro mil"   (VALID NUMBER)
#define B6 7  // got hundreds after "mil"   (VALID NUMBER)
#define B7 8  // got tens after "mil"   (VALID NUMBER)
#define B8 9  // got "y" after tens   (VALID NUMBER)
#define Bk 10 // got units after "y"   (VALID NUMBER)
#define M1 11 // got "billones" after a valid number  (VALID NUMBER)
#define M1a 12 // got "y" after "billones"
#define M1b 13 // got "... millones y medio/cuarto" 
#define M2 14 // got hundreds "doscientos" after billions  (VALID NUMBER)
#define M3 15 // got tens "treinta" "docientos treinta"   (VALID NUMBER)
#define M4 16 // got "y" after tens
#define Mu 17 // got units after "y": "doscientos treinta y cuatro"   (VALID NUMBER)
#define M5 18 // got "mil" after unit "doscientos treinta y cuatro mil"   (VALID NUMBER)
#define M6 19 // got hundreds after "mil"   (VALID NUMBER)
#define M7 20 // got tens after "mil"   (VALID NUMBER)
#define M8 21 // got "y" after tens   (VALID NUMBER)
#define Mk 22 // got units after "y"   (VALID NUMBER)
#define S1 23 // got "millones" after a valid number  (VALID NUMBER) 
#define S1a 24 // got "y" after "millones"
#define S1b 25 // got "... millones y medio/cuarto" 
#define S2 26 // got hundreds "doscientos" after millions  (VALID NUMBER)
#define S3 27 // got tens "treinta" "docientos treinta"   (VALID NUMBER)
#define S4 28 // got "y" after tens
#define Su 29 // got units after "y": "doscientos treinta y cuatro"   (VALID NUMBER)
#define S5 30 // got "mil" after unit "doscientos treinta y cuatro mil"   (VALID NUMBER)
#define S6 31 // got hundreds after "mil"   (VALID NUMBER)
#define S7 32 // got tens after "mil"   (VALID NUMBER)
#define S8 33 // got "y" after tens   (VALID NUMBER)
#define Sk 34 // got units after "y"   (VALID NUMBER)
#define COD 35 // got pseudo-numerical code from initial state
#define X1 36 // got "millar", "docenas" "centenares" "decenas" after a number
#define X2 37 // got "y" after "millar/docenas/etc"
#define X3 38 // got "millar y medio/cuarto",  "X docenas y media"
#define X4 39 // got "medio" as 1st word
#define X5 40 // got "medio centenar/millar/etc"
// stop state
#define STOP 41

// Token codes
#define TK_c      1  // hundreds "cien" "doscientos" 
#define TK_d      2  // tens "treinta" "cuarenta"
#define TK_u      3  // units "tres" "cuatro"
#define TK_wy     4  // word "y"
#define TK_wmedio 5  // word "medio"
#define TK_wcuarto 6 // word "cuarto"
#define TK_mil    7  // word "mil"   
#define TK_mill   8  // word "millon" "millones"
#define TK_bill   9  // word "billon" "billones"
#define TK_num   10  // a number (in digits)
#define TK_code  11  // a code (ex. LX-345-2)
#define TK_milr  12  // word "millar/es"
#define TK_cent  13  // word "centenar/es"
#define TK_dec   14  // word "decena/s"
#define TK_doc   15  // word "docena/s
#define TK_other 16
 
 
///////////////////////////////////////////////////////////////
///  Create a numbers recognizer for Galician.
///////////////////////////////////////////////////////////////

numbers_gl::numbers_gl(const std::string &dec, const std::string &thou): numbers_module(dec,thou)
{  
  // Initializing value map
  value.insert(make_pair("cen",100));          value.insert(make_pair("cento",100));
  value.insert(make_pair("douscentos",200));    value.insert(make_pair("duascentas",200));
  value.insert(make_pair("trescentos",300));   value.insert(make_pair("trescentas",300));
  value.insert(make_pair("catrocentos",400)); value.insert(make_pair("catrocentas",400));
  value.insert(make_pair("cincocentos",500));    value.insert(make_pair("cincocentas",500));
  value.insert(make_pair("quiñentos",500));    value.insert(make_pair("quiñentas",500));
  value.insert(make_pair("seiscentos",600));   value.insert(make_pair("seiscentas",600));
  value.insert(make_pair("setecentos",700));   value.insert(make_pair("setecentas",700));
  value.insert(make_pair("oitocentos",800));   value.insert(make_pair("oitocentas",800));
  value.insert(make_pair("novecentos",900));   value.insert(make_pair("novecentas",900));
  value.insert(make_pair("trinta",30));     value.insert(make_pair("corenta",40));
  value.insert(make_pair("cincuenta",50));   value.insert(make_pair("sesenta",60));
  value.insert(make_pair("setenta",70));     value.insert(make_pair("oitenta",80));
  value.insert(make_pair("noventa",90));     
  value.insert(make_pair("un",1));           value.insert(make_pair("unha",1));
  value.insert(make_pair("dous",2));    value.insert(make_pair("dúas",2));
  value.insert(make_pair("tres",3));    value.insert(make_pair("catro",4));
  value.insert(make_pair("cinco",5));   value.insert(make_pair("seis",6));
  value.insert(make_pair("sete",7));   value.insert(make_pair("oito",8));
  value.insert(make_pair("nove",9));   value.insert(make_pair("dez",10));
  value.insert(make_pair("once",11));   value.insert(make_pair("doce",12));
  value.insert(make_pair("trece",13));  value.insert(make_pair("catorce",14));
  value.insert(make_pair("quince",15)); value.insert(make_pair("dezaseis",16));
  value.insert(make_pair("dezasete",17)); 
  value.insert(make_pair("dezaoito",18)); value.insert(make_pair("dezanove",19));
  value.insert(make_pair("vinte",20));
  value.insert(make_pair("medio",0.5));    value.insert(make_pair("media",0.5));  
  value.insert(make_pair("cuarto",0.25));

  // Initializing token map
  tok.insert(make_pair("cen",TK_c));        tok.insert(make_pair("cento",TK_c));
  tok.insert(make_pair("douscentos",TK_c));  tok.insert(make_pair("duascentas",TK_c));
  tok.insert(make_pair("trescentos",TK_c)); tok.insert(make_pair("trescentas",TK_c));
  tok.insert(make_pair("catrocentos",TK_c)); tok.insert(make_pair("catrocentas",TK_c));
  tok.insert(make_pair("cincocentos",TK_c));  tok.insert(make_pair("cincocentas",TK_c));
  tok.insert(make_pair("quiñentos",TK_c));  tok.insert(make_pair("quiñentas",TK_c));
  tok.insert(make_pair("seiscentos",TK_c)); tok.insert(make_pair("seiscentas",TK_c));
  tok.insert(make_pair("setecentos",TK_c)); tok.insert(make_pair("setecentas",TK_c));
  tok.insert(make_pair("oitocentos",TK_c)); tok.insert(make_pair("oitocentas",TK_c));
  tok.insert(make_pair("novecentos",TK_c)); tok.insert(make_pair("novecentas",TK_c));
  tok.insert(make_pair("trinta",TK_d));     tok.insert(make_pair("corenta",TK_d));
  tok.insert(make_pair("cincuenta",TK_d));   tok.insert(make_pair("sesenta",TK_d));
  tok.insert(make_pair("setenta",TK_d));     tok.insert(make_pair("oitenta",TK_d));
  tok.insert(make_pair("noventa",TK_d));     
  tok.insert(make_pair("un",TK_u));       tok.insert(make_pair("unha",TK_u));
  tok.insert(make_pair("dous",TK_u));      tok.insert(make_pair("dúas",TK_u));      
  tok.insert(make_pair("tres",TK_u));
  tok.insert(make_pair("catro",TK_u));   tok.insert(make_pair("cinco",TK_u));
  tok.insert(make_pair("seis",TK_u));     tok.insert(make_pair("sete",TK_u));
  tok.insert(make_pair("oito",TK_u));     tok.insert(make_pair("nove",TK_u));
  tok.insert(make_pair("dez",TK_u));     tok.insert(make_pair("once",TK_u));
  tok.insert(make_pair("doce",TK_u));     tok.insert(make_pair("trece",TK_u));
  tok.insert(make_pair("catorce",TK_u));  tok.insert(make_pair("quince",TK_u));
  tok.insert(make_pair("dezaseis",TK_u)); 
  tok.insert(make_pair("dezasete",TK_u)); tok.insert(make_pair("dezaoito",TK_u));
  tok.insert(make_pair("dezanove",TK_u)); tok.insert(make_pair("vinte",TK_u));
  tok.insert(make_pair("mil",TK_mil));
  tok.insert(make_pair("millon",TK_mill));    tok.insert(make_pair("millón",TK_mill));
  tok.insert(make_pair("millons",TK_mill));  tok.insert(make_pair("billon",TK_bill));
  tok.insert(make_pair("billón",TK_bill));    tok.insert(make_pair("billons",TK_bill));

  tok.insert(make_pair("milleiro",TK_milr));    tok.insert(make_pair("milleiros",TK_milr));
  tok.insert(make_pair("centenar",TK_cent));    tok.insert(make_pair("centenares",TK_cent));
  tok.insert(make_pair("decena",TK_dec));    tok.insert(make_pair("decenas",TK_dec));
  tok.insert(make_pair("ducia",TK_doc));    tok.insert(make_pair("ducias",TK_doc));

  tok.insert(make_pair("e",TK_wy));
  tok.insert(make_pair("medio",TK_wmedio));
  tok.insert(make_pair("media",TK_wmedio));
  tok.insert(make_pair("cuarto",TK_wcuarto));

  // Initializing power map
  power.insert(make_pair(TK_mil,  1000.0));
  power.insert(make_pair(TK_mill, 1000000.0));
  power.insert(make_pair(TK_bill, 1000000000000.0));
  power.insert(make_pair(TK_milr, 1000.0));
  power.insert(make_pair(TK_cent, 100.0));
  power.insert(make_pair(TK_doc,  12.0));
  power.insert(make_pair(TK_dec,  10.0));

  // Initialize special state attributes
  initialState=B1; stopState=STOP;

  // Initialize Final state set 
  Final.insert(B2);  Final.insert(B3);  Final.insert(Bu);  Final.insert(B5);
  Final.insert(B6);  Final.insert(B7);  Final.insert(Bk); 
  Final.insert(M1);  Final.insert(M2);  Final.insert(M3);  Final.insert(Mu);
  Final.insert(M5);  Final.insert(M6);  Final.insert(M7);  Final.insert(Mk);
  Final.insert(S1);  Final.insert(S2);  Final.insert(S3);  Final.insert(Su);
  Final.insert(S5);  Final.insert(S6);  Final.insert(S7);  Final.insert(Sk);
  Final.insert(M1b); Final.insert(S1b); Final.insert(COD);  
  Final.insert(X1);  Final.insert(X3);  Final.insert(X5);

  // Initialize transitions table. By default, stop state
  int s,t;
  for(s=0;s<MAX_STATES;s++) for(t=0;t<MAX_TOKENS;t++) trans[s][t]=STOP;
  
  // Initializing transitions table
  // State B1
  trans[B1][TK_c]=B2;   trans[B1][TK_d]=B3;   trans[B1][TK_u]=Bu; 
  trans[B1][TK_mil]=B5; trans[B1][TK_num]=Bu; trans[B1][TK_code]=COD;
  trans[B1][TK_wmedio]=X4;
  // State B2
  trans[B2][TK_d]=B3;    trans[B2][TK_u]=Bu;    trans[B2][TK_mil]=B5; 
  trans[B2][TK_bill]=M1; trans[B2][TK_mill]=S1; trans[B2][TK_num]=Bu;
  // State B3
  trans[B3][TK_wy]=B4;   trans[B3][TK_mil]=B5; 
  trans[B3][TK_bill]=M1; trans[B3][TK_mill]=S1;
  // State B4
  trans[B4][TK_u]=Bu; trans[B4][TK_num]=Bu;
  // State Bu
  trans[Bu][TK_mil]=B5; trans[Bu][TK_bill]=M1; trans[Bu][TK_mill]=S1;
  trans[Bu][TK_milr]=X1; trans[Bu][TK_cent]=X1;
  trans[Bu][TK_dec]=X1;  trans[Bu][TK_doc]=X1;
  // State B5
  trans[B5][TK_c]=B6;    trans[B5][TK_d]=B7;    trans[B5][TK_u]=Bk; 
  trans[B5][TK_bill]=M1; trans[B5][TK_mill]=S1; trans[B5][TK_num]=Bk;
  // State B6
  trans[B6][TK_d]=B7;    trans[B6][TK_u]=Bk;    trans[B6][TK_num]=Bk;
  trans[B6][TK_bill]=M1; trans[B6][TK_mill]=S1;
  // State B7
  trans[B7][TK_wy]=B8; trans[B7][TK_bill]=M1; trans[B7][TK_mill]=S1;
  // State B8
  trans[B8][TK_u]=Bk;  trans[B8][TK_num]=Bk; 
  // State Bk
  trans[Bk][TK_bill]=M1; trans[Bk][TK_mill]=S1;

  // State M1
  trans[M1][TK_c]=M2; trans[M1][TK_d]=M3;   trans[M1][TK_num]=Mu; 
  trans[M1][TK_u]=Mu; trans[M1][TK_mil]=M5; trans[M1][TK_wy]=M1a; 
  // State M1a
  trans[M1a][TK_wmedio]=M1b; trans[M1a][TK_wcuarto]=M1b;
  // State M1b
    // nothing else expected
  // State M2
  trans[M2][TK_d]=M3;    trans[M2][TK_u]=Mu;   trans[M2][TK_num]=Mu; 
  trans[M2][TK_mil]=M5;  trans[M2][TK_mill]=S1;
  // State M3
  trans[M3][TK_wy]=M4;   trans[M3][TK_mil]=M5;  trans[M3][TK_mill]=S1;
  // State M4
  trans[M4][TK_u]=Mu;  trans[M4][TK_num]=Mu;
  // State Mu
  trans[Mu][TK_mil]=M5; trans[Mu][TK_mill]=S1;
  // State M5
  trans[M5][TK_c]=M6;   trans[M5][TK_d]=M7;   trans[M5][TK_num]=Mk; 
  trans[M5][TK_u]=Mk;   trans[M5][TK_mill]=S1;
  // State M6
  trans[M6][TK_d]=M7;    trans[M6][TK_u]=Mk;
  trans[M6][TK_mill]=S1; trans[M6][TK_num]=Mk; 
  // State M7
  trans[M7][TK_wy]=M8; trans[M7][TK_mill]=S1;
  // State M8
  trans[M8][TK_u]=Mk;   trans[M8][TK_num]=Mk;
  // State Mk
  trans[Mk][TK_mill]=S1;

  // State S1
  trans[S1][TK_c]=S2; trans[S1][TK_d]=S3;   trans[S1][TK_num]=Su; 
  trans[S1][TK_u]=Su; trans[S1][TK_mil]=S5; trans[S1][TK_wy]=S1a; 
  // State S1a
  trans[S1a][TK_wmedio]=S1b; trans[S1a][TK_wcuarto]=S1b;
  // State S1b
     // nothing else expected
  // State S2
  trans[S2][TK_d]=S3;   trans[S2][TK_u]=Su; 
  trans[S2][TK_mil]=S5; trans[S2][TK_num]=Su; 
  // State S3
  trans[S3][TK_wy]=S4; trans[S3][TK_mil]=S5;
  // State S4
  trans[S4][TK_u]=Su;  trans[S4][TK_num]=Su;
  // State Su
  trans[Su][TK_mil]=S5;
  // State S5
  trans[S5][TK_c]=S6; trans[S5][TK_d]=S7;
  trans[S5][TK_u]=Sk; trans[S5][TK_num]=Sk; 
  // State S6
  trans[S6][TK_d]=S7; trans[S6][TK_u]=Sk; trans[S6][TK_num]=Sk;  
  // State S7
  trans[S7][TK_wy]=S8;
  // State S8
  trans[S8][TK_u]=Sk; trans[S8][TK_num]=Sk; 
  // State Sk
  // nothing else is expected
  // State COD
  // nothing else is expected
  // State X1
  trans[X1][TK_wy]=X2;
  // State X2
  trans[X2][TK_wmedio]=X3;  trans[X2][TK_wcuarto]=X3;
  // State X3
  // nothing else is expected
  // State X4
  trans[X4][TK_bill]=M1;  trans[X4][TK_mill]=S1;   
  trans[X4][TK_milr]=X5;  trans[X4][TK_cent]=X5;
  trans[X4][TK_dec]=X5;   trans[X4][TK_doc]=X5;
  // State X5
  // nothing else is expected
 
  TRACE(3,"analyzer succesfully created");
}


//-- Implementation of virtual functions from class automat --//

///////////////////////////////////////////////////////////////
///  Compute the right token code for word j from given state.
///////////////////////////////////////////////////////////////

int numbers_gl::ComputeToken(int state, sentence::iterator &j, sentence &se)
{
  string form;
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
  if (RE_number.Search(form)) token = TK_num;
  else if (RE_code.Search(form)) token = TK_code;

  TRACE(3,"Leaving state "+util::int2string(state)+" with token "+util::int2string(token)); 
  return (token);
}


///////////////////////////////////////////////////////////////
///   Reset acumulators used by state actions:
///   bilion, milion, units.
///////////////////////////////////////////////////////////////

void numbers_gl::ResetActions() 
{
 iscode=0;
 milion=0; bilion=0; units=0;
 block=0;
}


///////////////////////////////////////////////////////////////
///  Perform necessary actions in "state" reached from state 
///  "origin" via word j interpreted as code "token":
///  Basically, when reaching a state, update current 
///  nummerical value.
///////////////////////////////////////////////////////////////

void numbers_gl::StateActions(int origin, int state, int token, sentence::const_iterator j)
{
  string form;
  size_t i;
  long double num;

  form = util::lowercase(j->get_form());
  TRACE(3,"Reaching state "+util::int2string(state)+" with token "+util::int2string(token)+" for word ["+form+"]");

  // get token numerical value, if any
  num=0;
  if (token==TK_num) {
    // erase thousand points
    while ((i=form.find_first_of(MACO_Thousand))!=string::npos) {
      form.erase(i,1);
    }
    TRACE(3,"after erasing thousand "+form);
    // make sure decimal point is "."
    if ((i=form.find_last_of(MACO_Decimal))!=string::npos) {
      form[i]='.';
      TRACE(3,"after replacing decimal "+form);
    }

    num = util::string2longdouble(form);
  }

  // State actions
  switch (state) {
  // ---------------------------------
  case B2:  case M2:  case S2:
  case B3:  case M3:  case S3:
  case Bu:  case Mu:  case Su:
  case B6:  case M6:  case S6:
  case B7:  case M7:  case S7:
  case Bk:  case Mk:  case Sk:
    TRACE(3,"Actions for normal state");
    if (token==TK_num)
      units += num;
    else
      units += value[form];
    break;
  // ---------------------------------
  case B5:  case M5:  case S5:
    TRACE(3,"Actions for 'thousands' state");
    if (units==0) units=1; // not said: "uno mil ciento treinta y dos"
    units *= power[token];
    break;
  // ---------------------------------
  case M1:
    TRACE(3,"Actions for M1 (bilion) state");
    // store bilion value and get ready in case milions are caming
    bilion = units*power[token];
    units=0;
    break;
  // ---------------------------------
  case M1b:
    TRACE(3,"Actions for M1b state");
    // "dos billones y medio/cuarto"
    bilion += value[form]*power[TK_bill];
    break;
  // ---------------------------------
  case S1:
    TRACE(3,"Actions for S1 (milion) state");
    // store milion value and get ready in case thousands are caming
    milion = units*power[token];
    units=0;
    break;
  // ---------------------------------
  case S1b:
    TRACE(3,"Actions for S1b state");
    // "dos millones y medio/cuarto"
    milion += value[form]*power[TK_mill];
    break;
  // ---------------------------------
  case X1:
    TRACE(3,"Actions for X1 state");
    // "Z millares/decenas/centenares/docenas"
    units *= power[token];
    block = token;
    break;
  // ---------------------------------
  case X3:
    TRACE(3,"Actions for X3 state");
    // "Z millares/decenas/centenares/docenas y medio/cuarto"
    units += value[form]*power[block];
    break;
  // ---------------------------------
  case X4:
    TRACE(3,"Actions for X4 state");
    // "medio" as 1st word
    units = value[form];
    break;
  // ---------------------------------
  case X5:
    TRACE(3,"Actions for X5 state");
    // "medio millar/millon/centenar/docena/etc
    units *= power[token];
    break;
  // ---------------------------------
  case COD:
    TRACE(3,"Actions for COD state");
    iscode = CODE;
    break;
  // ---------------------------------
  default: break;
  }

  TRACE(3,"State actions completed. bilion="+util::longdouble2string(bilion)+" milion="+util::longdouble2string(milion)+" units="+util::longdouble2string(units));
}


///////////////////////////////////////////////////////////////
///   Set the appropriate lemma and parole for the 
///   new multiword.
///////////////////////////////////////////////////////////////

void numbers_gl::SetMultiwordAnalysis(sentence::iterator i, int fstate)
{
  string lemma;

  // Setting the analysis for the nummerical expression
  if (iscode==CODE) {
    lemma=i->get_form();
  }
  else {
    // compute nummerical value as lemma
    lemma=util::longdouble2string(bilion+milion+units);
  }

  i->set_analysis(analysis(lemma,"Z"));
  TRACE(3,"Analysis set to: "+lemma+" Z");
}


// undef codes to prepare for next module definition
#undef B1
#undef B2
#undef B3
#undef B4
#undef Bu
#undef B5
#undef B6
#undef B7
#undef B8
#undef Bk
#undef M1
#undef M1a
#undef M1b
#undef M2 
#undef M3 
#undef M4 
#undef Mu 
#undef M5 
#undef M6 
#undef M7 
#undef M8 
#undef Mk 
#undef S1 
#undef S1a
#undef S1b
#undef S2 
#undef S3 
#undef S4 
#undef Su 
#undef S5 
#undef S6 
#undef S7 
#undef S8 
#undef Sk
#undef COD
#undef STOP
#undef TK_c 
#undef TK_d 
#undef TK_u 
#undef TK_wy
#undef TK_wmedio
#undef TK_wcuarto
#undef TK_mil
#undef TK_mill
#undef TK_bill
#undef TK_num 
#undef TK_code
#undef TK_other

//---------------------------------------------------------------------------
//        English number recognizer
//---------------------------------------------------------------------------

//// State codes 
#define B1 1  // initial state
#define B2 2  // got units  (VALID NUMBER)
#define B3 3  // got "hundred" after units   (VALID NUMBER)
#define B4 4  // got units after "hundred"   (VALID NUMBER)
#define B5 5  // got "thousand" after valid number (VALID NUMBER)
#define B6 6  // got units after "thousand" (VALID NUMBER)
#define B7 7  // got "hundred" from B6  (VALID NUMBER)
#define B8 8  // got units form B7   (VALID NUMBER)
#define M1 9  // got "bilion/s" after a valid number  (VALID NUMBER)
#define M2 10 // see B2 
#define M3 11 // see B3
#define M4 12 // see B4
#define M5 13 // see B5
#define M6 14 // see B6
#define M7 15 // see B7
#define M8 16 // see B8
#define S1 17 // got "milion/s" after a valid number  (VALID NUMBER) 
#define S2 18 // see B2
#define S3 19 // see B3
#define S4 20 // see B4
#define S5 21 // see B5
#define S6 22 // see B6
#define S7 23 // see B7
#define S8 24 // see B8
#define COD 25 // got pseudo-numerical code from initial state
// stop state
#define STOP 26

// Token codes
#define TK_a      1  // word "a" (e.g. "a hundred")
#define TK_u      1  // units "three" "seventeen"
#define TK_hundr  2  // word "hundred"   
#define TK_thous  3  // word "mil"
#define TK_mill   4  // word "million" "millions"
#define TK_bill   5  // word "billion" "billions"
#define TK_num    6  // a number (in digits)
#define TK_code   7  // a code (ex. LX-345-2)
#define TK_and    8  // word "and"
#define TK_ord    9  // ordinal code: "1st", "2nd", "43rd", etc
#define TK_other  10

///////////////////////////////////////////////////////////////
///  Create a numbers recognizer for English
///////////////////////////////////////////////////////////////

numbers_en::numbers_en(const std::string &dec, const std::string &thou): numbers_module(dec,thou)
{
  // Initializing value map
  value.insert(make_pair("a",1));
  value.insert(make_pair("one",1));            value.insert(make_pair("two",2)); 
  value.insert(make_pair("three",3));          value.insert(make_pair("four",4));
  value.insert(make_pair("five",5));           value.insert(make_pair("six",6));
  value.insert(make_pair("seven",7));          value.insert(make_pair("eight",8));
  value.insert(make_pair("nine",9));           value.insert(make_pair("ten",10));
  value.insert(make_pair("eleven",11));        value.insert(make_pair("twelve",12));
  value.insert(make_pair("thirteen",13));      value.insert(make_pair("fourteen",14));
  value.insert(make_pair("quinze",15));        value.insert(make_pair("sixteen",16));
  value.insert(make_pair("seventeen",17));     value.insert(make_pair("eighteen",18)); 
  value.insert(make_pair("nineteen",19));      
  value.insert(make_pair("twenty", 20));       value.insert(make_pair("twenty-one",21));
  value.insert(make_pair("twenty-two",22));    value.insert(make_pair("twenty-three",23));
  value.insert(make_pair("twenty-four",24));   value.insert(make_pair("twenty-five",25));  
  value.insert(make_pair("twenty-six",26));    value.insert(make_pair("twenty-seven",27));
  value.insert(make_pair("twenty-eight",28));  value.insert(make_pair("twenty-nine",29));
  value.insert(make_pair("thirty",30));        value.insert(make_pair("thirty-one",31)); 
  value.insert(make_pair("thirty-two",32));    value.insert(make_pair("thirty-three",33));
  value.insert(make_pair("thirty-four",34));   value.insert(make_pair("thirty-five",35));  
  value.insert(make_pair("thirty-six",36));    value.insert(make_pair("thirty-seven",37));
  value.insert(make_pair("thirty-eight",38));  value.insert(make_pair("thirty-nine",39));
  value.insert(make_pair("fourty",40));        value.insert(make_pair("fourty-one",41));
  value.insert(make_pair("fourty-two",42));    value.insert(make_pair("fourty-three",43));
  value.insert(make_pair("fourty-four",44));   value.insert(make_pair("fourty-five",45));  
  value.insert(make_pair("fourty-six",46));    value.insert(make_pair("fourty-seven",47));
  value.insert(make_pair("fourty-eight",48));  value.insert(make_pair("fourty-nine",49));
  value.insert(make_pair("fifty",50));         value.insert(make_pair("fifty-one",51));
  value.insert(make_pair("fifty-two",52));     value.insert(make_pair("fifty-three",53));
  value.insert(make_pair("fifty-four",54));    value.insert(make_pair("fifty-five",55));  
  value.insert(make_pair("fifty-six",56));     value.insert(make_pair("fifty-seven",57));
  value.insert(make_pair("fifty-eight",58));   value.insert(make_pair("fifty-nine",59));
  value.insert(make_pair("sixty",60));         value.insert(make_pair("sixty-one",61));
  value.insert(make_pair("sixty-two",62));     value.insert(make_pair("sixty-three",63));
  value.insert(make_pair("sixty-four",64));    value.insert(make_pair("sixty-five",65));  
  value.insert(make_pair("sixty-six",66));     value.insert(make_pair("sixty-seven",67));
  value.insert(make_pair("sixty-eight",68));   value.insert(make_pair("sixty-nine",69));
  value.insert(make_pair("seventy",70));       value.insert(make_pair("seventy-one",71));
  value.insert(make_pair("seventy-two",72));   value.insert(make_pair("seventy-three",73));
  value.insert(make_pair("seventy-four",74));  value.insert(make_pair("seventy-five",75));  
  value.insert(make_pair("seventy-six",76));   value.insert(make_pair("seventy-seven",77));
  value.insert(make_pair("seventy-eight",78)); value.insert(make_pair("seventy-nine",79));
  value.insert(make_pair("eighty",80));        value.insert(make_pair("eighty-one",81));    
  value.insert(make_pair("eighty-two",82));    value.insert(make_pair("eighty-three",83));
  value.insert(make_pair("eighty-four",84));   value.insert(make_pair("eighty-five",85));  
  value.insert(make_pair("eighty-six",86));    value.insert(make_pair("eighty-seven",87));
  value.insert(make_pair("eighty-eight",88));  value.insert(make_pair("eighty-nine",89));
  value.insert(make_pair("ninety",90));        value.insert(make_pair("ninety-one",91)); 
  value.insert(make_pair("ninety-two",92));    value.insert(make_pair("ninety-three",93));
  value.insert(make_pair("ninety-four",94));   value.insert(make_pair("ninety-five",95));  
  value.insert(make_pair("ninety-six",96));    value.insert(make_pair("ninety-seven",97));
  value.insert(make_pair("ninety-eight",98));  value.insert(make_pair("ninety-nine",99));

  // Initializing token map
  tok.insert(make_pair("a",TK_a));
  tok.insert(make_pair("one",TK_u));             tok.insert(make_pair("two",TK_u));            
  tok.insert(make_pair("three",TK_u));          tok.insert(make_pair("four",TK_u));
  tok.insert(make_pair("five",TK_u));           tok.insert(make_pair("six",TK_u));
  tok.insert(make_pair("seven",TK_u));          tok.insert(make_pair("eight",TK_u));
  tok.insert(make_pair("nine",TK_u));           tok.insert(make_pair("ten",TK_u));
  tok.insert(make_pair("eleven",TK_u));        tok.insert(make_pair("twelve",TK_u));
  tok.insert(make_pair("thirteen",TK_u));      tok.insert(make_pair("fourteen",TK_u));
  tok.insert(make_pair("quinze",TK_u));        tok.insert(make_pair("sixteen",TK_u));
  tok.insert(make_pair("seventeen",TK_u));     tok.insert(make_pair("eighteen",TK_u)); 
  tok.insert(make_pair("nineteen",TK_u));      
  tok.insert(make_pair("twenty", TK_u));       tok.insert(make_pair("twenty-one",TK_u));
  tok.insert(make_pair("twenty-two",TK_u));    tok.insert(make_pair("twenty-three",TK_u));
  tok.insert(make_pair("twenty-four",TK_u));   tok.insert(make_pair("twenty-five",TK_u));
  tok.insert(make_pair("twenty-six",TK_u));    tok.insert(make_pair("twenty-seven",TK_u));
  tok.insert(make_pair("twenty-eight",TK_u));  tok.insert(make_pair("twenty-nine",TK_u));
  tok.insert(make_pair("thirty",TK_u));        tok.insert(make_pair("thirty-one",TK_u)); 
  tok.insert(make_pair("thirty-two",TK_u));    tok.insert(make_pair("thirty-three",TK_u));
  tok.insert(make_pair("thirty-four",TK_u));   tok.insert(make_pair("thirty-five",TK_u));  
  tok.insert(make_pair("thirty-six",TK_u));    tok.insert(make_pair("thirty-seven",TK_u));
  tok.insert(make_pair("thirty-eight",TK_u));  tok.insert(make_pair("thirty-nine",TK_u));
  tok.insert(make_pair("fourty",TK_u));        tok.insert(make_pair("fourty-one",TK_u));
  tok.insert(make_pair("fourty-two",TK_u));    tok.insert(make_pair("fourty-three",TK_u));
  tok.insert(make_pair("fourty-four",TK_u));   tok.insert(make_pair("fourty-five",TK_u));  
  tok.insert(make_pair("fourty-six",TK_u));    tok.insert(make_pair("fourty-seven",TK_u));
  tok.insert(make_pair("fourty-eight",TK_u));  tok.insert(make_pair("fourty-nine",TK_u));
  tok.insert(make_pair("fifty",TK_u));         tok.insert(make_pair("fifty-one",TK_u));
  tok.insert(make_pair("fifty-two",TK_u));     tok.insert(make_pair("fifty-three",TK_u));
  tok.insert(make_pair("fifty-four",TK_u));    tok.insert(make_pair("fifty-five",TK_u));  
  tok.insert(make_pair("fifty-six",TK_u));     tok.insert(make_pair("fifty-seven",TK_u));
  tok.insert(make_pair("fifty-eight",TK_u));   tok.insert(make_pair("fifty-nine",TK_u));
  tok.insert(make_pair("sixty",TK_u));         tok.insert(make_pair("sixty-one",TK_u));
  tok.insert(make_pair("sixty-two",TK_u));     tok.insert(make_pair("sixty-three",TK_u));
  tok.insert(make_pair("sixty-four",TK_u));    tok.insert(make_pair("sixty-five",TK_u));  
  tok.insert(make_pair("sixty-six",TK_u));     tok.insert(make_pair("sixty-seven",TK_u));
  tok.insert(make_pair("sixty-eight",TK_u));   tok.insert(make_pair("sixty-nine",TK_u));
  tok.insert(make_pair("seventy",TK_u));       tok.insert(make_pair("seventy-one",TK_u));
  tok.insert(make_pair("seventy-two",TK_u));   tok.insert(make_pair("seventy-three",TK_u));
  tok.insert(make_pair("seventy-four",TK_u));  tok.insert(make_pair("seventy-five",TK_u));  
  tok.insert(make_pair("seventy-six",TK_u));   tok.insert(make_pair("seventy-seven",TK_u));
  tok.insert(make_pair("seventy-eight",TK_u)); tok.insert(make_pair("seventy-nine",TK_u));
  tok.insert(make_pair("eighty",TK_u));        tok.insert(make_pair("eighty-one",TK_u));    
  tok.insert(make_pair("eighty-two",TK_u));    tok.insert(make_pair("eighty-three",TK_u));
  tok.insert(make_pair("eighty-four",TK_u));   tok.insert(make_pair("eighty-five",TK_u));  
  tok.insert(make_pair("eighty-six",TK_u));    tok.insert(make_pair("eighty-seven",TK_u));
  tok.insert(make_pair("eighty-eight",TK_u));  tok.insert(make_pair("eighty-nine",TK_u));
  tok.insert(make_pair("ninety",TK_u));        tok.insert(make_pair("ninety-one",TK_u)); 
  tok.insert(make_pair("ninety-two",TK_u));    tok.insert(make_pair("ninety-three",TK_u));
  tok.insert(make_pair("ninety-four",TK_u));   tok.insert(make_pair("ninety-five",TK_u));  
  tok.insert(make_pair("ninety-six",TK_u));    tok.insert(make_pair("ninety-seven",TK_u));
  tok.insert(make_pair("ninety-eight",TK_u));  tok.insert(make_pair("ninety-nine",TK_u));

  tok.insert(make_pair("hundred",TK_hundr));
  tok.insert(make_pair("thousand",TK_thous));
  tok.insert(make_pair("milion",TK_mill));  tok.insert(make_pair("milions",TK_mill));
  tok.insert(make_pair("bilion",TK_bill));  tok.insert(make_pair("bilions",TK_bill));
  tok.insert(make_pair("million",TK_mill));  tok.insert(make_pair("millions",TK_mill));
  tok.insert(make_pair("billion",TK_bill));  tok.insert(make_pair("billions",TK_bill));

  tok.insert(make_pair("and",TK_and));

  // Initializing power map
  power.insert(make_pair(TK_hundr, 100.0));
  power.insert(make_pair(TK_thous, 1000.0));
  power.insert(make_pair(TK_mill,  1000000.0));
  power.insert(make_pair(TK_bill,  1000000000.0));

  // Initialize special state attributes
  initialState=B1; stopState=STOP;

  // Initialize Final state set 
  Final.insert(B2);  Final.insert(B3);  Final.insert(B4);  Final.insert(B5);
  Final.insert(B6);  Final.insert(B7);  Final.insert(B8); 
  Final.insert(M1);  Final.insert(M2);  Final.insert(M3);  Final.insert(M4);
  Final.insert(M5);  Final.insert(M6);  Final.insert(M7);  Final.insert(M8);
  Final.insert(S1);  Final.insert(S2);  Final.insert(S3);  Final.insert(S4);
  Final.insert(S5);  Final.insert(S6);  Final.insert(S7);  Final.insert(S8);
  Final.insert(COD);  

  // Initialize transitions table. By default, stop state
  int s,t;
  for(s=0;s<MAX_STATES;s++) for(t=0;t<MAX_TOKENS;t++) trans[s][t]=STOP;
  
  // Initializing transitions table
  // State B1
  trans[B1][TK_u]=B2;     trans[B1][TK_a]=B2;     
  trans[B1][TK_num]=B2;   trans[B1][TK_code]=COD;   trans[B1][TK_ord]=COD; 
  // State B2
  trans[B2][TK_hundr]=B3; trans[B2][TK_thous]=B5; 
  trans[B2][TK_bill]=M1;  trans[B2][TK_mill]=S1;
  // State B3
  trans[B3][TK_u]=B4;     trans[B3][TK_num]=B4;     trans[B3][TK_thous]=B5; 
  trans[B3][TK_bill]=M1;  trans[B3][TK_mill]=S1;    trans[B3][TK_and]=B3;
  // State B4
  trans[B4][TK_thous]=B5; trans[B4][TK_bill]=M1;   trans[B4][TK_mill]=S1;
  // State B5
  trans[B5][TK_u]=B6;      trans[B5][TK_num]=B6;   trans[B5][TK_bill]=M1; trans[B5][TK_mill]=S1;
  trans[B5][TK_and]=B5; 
  // State B6
  trans[B6][TK_hundr]=B7; trans[B6][TK_bill]=M1;   trans[B6][TK_mill]=S1;
  // State B7
  trans[B7][TK_u]=B8;      trans[B7][TK_num]=B8;   trans[B7][TK_bill]=M1; 
  trans[B7][TK_mill]=S1;   trans[B7][TK_and]=B7;
  // State B8
  trans[B8][TK_bill]=M1;  trans[B8][TK_mill]=S1;
 
 // State M1
  trans[M1][TK_u]=M2;     trans[M1][TK_num]=M2;     trans[M1][TK_a]=M2;     trans[M1][TK_bill]=S1;
  // State M2
  trans[M2][TK_hundr]=M3; trans[M2][TK_thous]=M5;   trans[M2][TK_bill]=S1; 
  // State M3
  trans[M3][TK_u]=M4;     trans[M3][TK_num]=M4;     trans[M3][TK_thous]=M5; 
  trans[M3][TK_bill]=S1;  trans[M3][TK_and]=M3;
  // State M4
  trans[M4][TK_thous]=M5; trans[M4][TK_bill]=S1; 
  // State M5
  trans[M5][TK_u]=M6;     trans[M5][TK_num]=M6;     trans[M5][TK_bill]=S1; 
  trans[M5][TK_and]=M5; 
  // State M6
  trans[M6][TK_hundr]=M7; trans[M6][TK_bill]=S1; 
  // State M7
  trans[M7][TK_u]=M8;     trans[M7][TK_num]=M8;     trans[M7][TK_bill]=S1;
  trans[M7][TK_and]=M7;
  // State M8
  trans[M8][TK_mill]=S1; 

  // State S1
  trans[S1][TK_u]=S2;     trans[S1][TK_num]=S2;     trans[S1][TK_a]=S2;     
  // State S2
  trans[S2][TK_hundr]=S3; trans[S2][TK_thous]=S5; 
  // State S3
  trans[S3][TK_u]=S4;     trans[S3][TK_num]=S4;     trans[S3][TK_thous]=S5; 
  trans[S3][TK_and]=S3;
  // State S4
  trans[S4][TK_thous]=S5;
  // State S5
  trans[S5][TK_u]=S6;     trans[S5][TK_num]=S6; 
  trans[S5][TK_and]=S5; 
  // State S6
  trans[S6][TK_hundr]=S7;
  // State S7
  trans[S7][TK_u]=S8;     trans[S7][TK_num]=S8;     trans[S7][TK_and]=S7;
  // State S8
  // nothing else expected

  TRACE(3,"analyzer succesfully created");
}


//-- Implementation of virtual functions from class automat --//

///////////////////////////////////////////////////////////////
///  Compute the right token code for word j from given state.
///////////////////////////////////////////////////////////////

int numbers_en::ComputeToken(int state, sentence::iterator &j, sentence &se)
{
  string form;
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

  // split two last letters of the form
  string sfx,pref;
  if (form.length()>2) {
    sfx=form.substr(form.length()-2,2); // last two letters
    pref=form.substr(0,form.length()-2); // all but last two letters
  }
 
  // check to see if it is a numeric ordinal (1st, 42nd, 15th)
  if (form.length()>2 && (sfx=="st" || sfx=="nd" || sfx=="rd" || sfx=="th") && RE_number.Search(pref) )
      token=TK_ord;
  // check to see if it is a number
  else if (RE_number.Search(form)) token = TK_num;
  // check to see if it is an alphanumeric code
  else if (RE_code.Search(form)) token = TK_code;

  TRACE(3,"Leaving state "+util::int2string(state)+" with token "+util::int2string(token)); 
  return (token);
}


///////////////////////////////////////////////////////////////
///   Reset acumulators used by state actions:
///   bilion, milion, units.
///////////////////////////////////////////////////////////////

void numbers_en::ResetActions() 
{
 iscode=0;
 milion=0; bilion=0; units=0;
 block=0;
}



///////////////////////////////////////////////////////////////
///  Perform necessary actions in "state" reached from state 
///  "origin" via word j interpreted as code "token":
///  Basically, when reaching a state, update current 
///  nummerical value.
///////////////////////////////////////////////////////////////

void numbers_en::StateActions(int origin, int state, int token, sentence::const_iterator j)
{
  string form;
  size_t i;
  long double num;

  form = util::lowercase(j->get_form());
  TRACE(3,"Reaching state "+util::int2string(state)+" with token "+util::int2string(token)+" for word ["+form+"]");

  // get token numerical value, if any
  num=0;
  if (token==TK_num) {
    // erase thousand points
    while ((i=form.find_first_of(MACO_Thousand))!=string::npos) {
      form.erase(i,1);
    }
    TRACE(3,"after erasing thousand "+form);
    // make sure decimal point is "."
    if ((i=form.find_last_of(MACO_Decimal))!=string::npos) {
      form[i]='.';
      TRACE(3,"after replacing decimal "+form);
    }
    num = util::string2longdouble(form);
  }

  // State actions
  switch (state) {
  // ---------------------------------
  case B2:  case M2:  case S2:
  case B4:  case M4:  case S4:
  case B6:  case M6:  case S6:
  case B8:  case M8:  case S8:
    TRACE(3,"Actions for normal state");
    if (token==TK_num)
      units += num;
    else
      units += value[form];
    break;
  // ---------------------------------
  case B3:  case M3:  case S3:
  case B5:  case M5:  case S5:
  case B7:  case M7:  case S7:
    if (token != TK_and) {
      TRACE(3,"Actions for 'thousands' and 'hundreds' state");
      units *= power[token];
    }
    break;
  // ---------------------------------
  case M1:
    TRACE(3,"Actions for M1 (bilion) state");
    // store bilion value and get ready in case milions are caming
    bilion = units*power[token];
    units=0;
    break;
  case S1:
    TRACE(3,"Actions for S1 (milion) state");
    // store milion value and get ready in case thousands are caming
    milion = units*power[token];
    units=0;
    break;
  // ---------------------------------
  case COD:
    TRACE(3,"Actions for COD state");    
    if (token==TK_code) iscode=CODE;
    else if (token==TK_ord) iscode=ORD;
    break;
  // ---------------------------------
  default: break;
  }
  
  TRACE(3,"State actions completed. bilion="+util::longdouble2string(bilion)+" milion="+util::longdouble2string(milion)+" units="+util::longdouble2string(units));
}


///////////////////////////////////////////////////////////////
///   Set the appropriate lemma and parole for the 
///   new multiword.
///////////////////////////////////////////////////////////////

void numbers_en::SetMultiwordAnalysis(sentence::iterator i, int fstate)
{
  string lemma, tag;

  tag="Z";

  // Setting the analysis for the nummerical expression
  if (iscode==CODE) {
    lemma=i->get_form();
  }
  else if (iscode==ORD) {
    // ordinal adjective.  4th --> (4th, 4, JJ)
    // "fourth" gets the same analysis, but via dictionary.
    string fm=i->get_form();
    lemma = fm.substr(0,fm.length()-2); // all but last two letters
    tag="JJ";  
  }
  else {
    // compute nummerical value as lemma
    lemma=util::longdouble2string(bilion+milion+units);
  }

  i->set_analysis(analysis(lemma,tag));
  TRACE(3,"Analysis set to: "+lemma+" "+tag);
}



