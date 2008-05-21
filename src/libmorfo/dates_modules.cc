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

#include "freeling/dates_modules.h"
#include "fries/util.h"
#include "freeling/traces.h"

using namespace std;

#define MOD_TRACENAME "DATES"
#define MOD_TRACECODE DATES_TRACE

///////////////////////////////////////////////////////////////
///  Abstract class constructor.
///////////////////////////////////////////////////////////////

dates_module::dates_module(const std::string &rd, const std::string &rt1, const std::string &rt2, const std::string &rtrom): automat(), RE_Date(rd), RE_Time1(rt1), RE_Time2(rt2), RE_Roman(rtrom)
{}

//-----------------------------------------------//
//        Default date/time recognizer           //
//        Only recognize simple patterns         //
//-----------------------------------------------//

// State codes 
#define A  1     // A: initial state
#define B  2     // B: Date pattern found
#define C  3     // C: hour pattern found after date
#define D  4     // D: hh+mm patter found after date
// stop state
#define STOP 5

// Token codes
#define TK_hour    1  // hour: something matching just first part of RE_TIME1: 3h, 17h
#define TK_hhmm    2  // hhmm: something matching whole RE_TIME1: 3h25min, 3h25m, 3h25minutos,
#define TK_min     3  // min: something matching RE_TIME2: 25min, 3minutos
#define TK_date    4  // date: something matching RE_DATE: 3/ago/1992, 3/8/92, 3/agosto/1992, ...
#define TK_other   5  // other: any other token, not relevant in current state


///////////////////////////////////////////////////////////////
///  Create a default dates recognizer.
///////////////////////////////////////////////////////////////

dates_default::dates_default(): dates_module(RE_DATE_DF, RE_TIME1_DF, RE_TIME2_DF, RE_ROMAN)
{
  // Initialize special state attributes
  initialState=A; stopState=STOP;

  // Initialize Final state set 
  Final.insert(B); Final.insert(C); Final.insert(D);

  // Initialize transitions table. By default, stop state
  int s,t;
  for(s=0;s<MAX_STATES;s++) for(t=0;t<MAX_TOKENS;t++) trans[s][t]=STOP;

  // State A
  trans[A][TK_date]=B;
  // State B
  trans[B][TK_hour]=C; trans[B][TK_hhmm]=D; 
  // State C
  trans[C][TK_min]=D;
  // State D
  // nothing else expected. 

  TRACE(3,"analyzer succesfully created");
}


//-- Implementation of virtual functions from class automat --//

///////////////////////////////////////////////////////////////
///  Compute the right token code for word j from given state.
///////////////////////////////////////////////////////////////

int dates_default::ComputeToken(int state, sentence::iterator &j, sentence &se)
{
  string form,formU;
  int token;

  formU = j->get_form();
  form = util::lowercase(formU);

  token = TK_other;
  if (RE_Date.Search(form)) {
    TRACE(3,"Match DATE regex. "+RE_Date.Match(0));
    token = TK_date;
  }
  else if (RE_Time1.Search(form)) {
    if(RE_Time1.Match(2)!=""){
      TRACE(3,"Match TIME1 regex (hour+min)");
      token = TK_hhmm;
    }
    else{
      TRACE(3,"Partial match TIME1 regex (hour)");
      token = TK_hour;
    }
  }
  else if (RE_Time2.Search(form)) {
    TRACE(3,"Match TIME2 regex (minutes)");
    token = TK_min;
  }

  TRACE(3,"Leaving state "+util::int2string(state)+" with token "+util::int2string(token));
  return (token);
}



///////////////////////////////////////////////////////////////
///   Reset acumulators used by state actions:
///   day, year, month, hour, minute, etc.
///////////////////////////////////////////////////////////////

void dates_default::ResetActions() 
{
  century="??";
  weekday="??";
  day="??";
  month="??";
  year="??";
  hour="??";
  minute="??";
  meridian="??";
  temp = -1;
}


///////////////////////////////////////////////////////////////
///  Perform necessary actions in "state" reached from state 
///  "origin" via word j interpreted as code "token":
///  Basically, when reaching a state with an informative 
///  token (day, year, month, etc) store that part of the date.
///////////////////////////////////////////////////////////////

void dates_default::StateActions(int origin, int state, int token, sentence::const_iterator j)
{
  string form;

  form = util::lowercase(j->get_form());
  TRACE(3,"Reaching state "+util::int2string(state)+" with token "+util::int2string(token)+" for word ["+form+"]");

  // switch (state)

  TRACE(3,"State actions finished. ["+day+":"+month+":"+year+":"+hour+":"+minute+"]");
}



///////////////////////////////////////////////////////////////
///   Set the appropriate lemma and parole for the
///   new multiword.
///////////////////////////////////////////////////////////////

void dates_default::SetMultiwordAnalysis(sentence::iterator i, int fstate)
{
  list<analysis> la;
  string lemma;

  // Setting the analysis for the date
  lemma=string("["+weekday+":"+day+"/"+month+"/"+year+":"+hour+"."+minute+":"+meridian+"]");

  la.push_back(analysis(lemma,"W"));
  i->set_analysis(la);
  TRACE(3,"Analysis set to: ("+lemma+" W)");
}


// undef literals, prepare for next module
#undef A
#undef B
#undef C
#undef D
#undef STOP
#undef TK_hour
#undef TK_hhmm
#undef TK_min
#undef TK_date
#undef TK_other  


//-----------------------------------------------//
//        Spanish date recognizer                //
//-----------------------------------------------//

// State codes 
// states for a date
#define A  1     // A: initial state
#define B  2     // B: got a week day                           (VALID DATE)
#define C  3     // C: got a week day and a comma
#define D  4     // D: got the word "dia" from A,B,C 
#define E  5     // E: got day number from A,B,C,D              (VALID DATE)
#define Eb 40    // Eb: as E but not final
#define F  6     // F: got "de" or "del" from E (a month may come)
#define G  7     // G: got word "mes" from A,F
#define H  8     // H: got "de" from G (a month may come)
#define I  9     // I: got a month name o number from A,F,G,H.  (VALID DATE)
#define Ib 10    // Ib: got a short month name from A.
#define J 11     // J: got "de" or "del" from J (a year may come)
#define K 12     // K: got "año" from A,J (a year may come)
#define L 13     // L: got a number from J,K or a date from A.  (VALID DATE)
#define P 14     // P: got "a.c." or "d.c" from L,S2            (VALID DATE)
#define S1 15    // S1: got "siglo" from A (a century may come)
#define S2 16    // S2: got roman number from S1                (VALID DATE)
// states for time, after finding a date (e.g. 22 de febrero a las cuatro)
#define Ha 17    // Ha: got "hacia" "sobre" "a" after a valid date
#define Hb 18    // Hb: got "eso" from Ha
#define Hc 19    // Hc: got "de" from Hb
#define Hd 20    // Hd: got "por" after a valid date
#define He 21    // He: got "la" "las" "el" from Hd
#define AH 22    // AH: got "a las" "sobre las" "hacia eso de las", etc. An hour is expected.
#define BH 23    // BH: got a valid hour+min from AH,CH,DH or EH. (VALID DATE)
#define CH 24    // CH: got a valid hour (no min) from AH         (VALID DATE)
#define DH 25    // DH: got "en" from CH (expecting "punto")
#define EH 26    // EH: got "y" from CH (expecting minutes)
#define EHb 27    // EH: got "menos" from CH (expecting minutes)
#define FH  28    // FH: got "de" "del" from BH,CH
#define GH  29    // GH: got "tarde" "noche" "mediodia" etc from BH,CH,FH  (VALID DATE)
// states for a time non preceded by a date
#define BH1  30   // BH1: got valid hour+min from A,CH1 (could be anything "dos y veinte")
#define BH2  31   // BH2: got valid hour+quartern from DH1,EH1,EH1b  (e.g "dos y media, dos en punto, dos y cuarto") (VALID DATE)
#define CH1  32   // CH1: got valid hour (no min) from A (but could be a day number)
#define DH1  33   // DH1: got "en" from CH1 (expecting "punto")
#define EH1  34   // EH1: got "y" from CH1 (expecting minutes)
#define EH1b 35   // EH1: got "menos" from CH1 (expecting minutes)
#define FH1  36   // FH1B: got "de" "del" from BH1 (must be a time)
#define FH1b 37  // FH1: got "de" "del" from CH1 (a month name may follow)
#define GH1  38   // GH1: got "tarde" "noche" "mediodia" etc from BH1,CH1,FH1b  (VALID DATE)
// stop state
#define STOP 39

// Token codes
#define TK_weekday   1      // weekday: "Lunes", "Martes", etc
#define TK_daynum    2      // daynum:  1-31
#define TK_month     3      // month: "enero", "febrero", "ene." "feb." etc,
#define TK_shmonth   4      // short month: "ene", "feb", "mar" etc,
#define TK_monthnum  5      // monthnum: 1-12
#define TK_number    6      // number: 23, 1976, 543, etc.
#define TK_comma     7      // comma: ","
#define TK_dot       8     // dot: "."
#define TK_colon     9     // colon:  ":"
#define TK_wday      10    // wdia: "día"
#define TK_wmonth    11    // wmonth: "mes"
#define TK_wyear     12    // wyear: "año"
#define TK_wpast     13    // wpast: "pasado", "presente", "proximo"
#define TK_wcentury  14    // wcentury:  "siglo", "sig", "sgl", etc.
#define TK_roman     15    // roman: XVI, VII, III, XX, etc.
#define TK_wacdc     16    // wacdc: "a.c.", "d.c."
#define TK_wampm     17    // wacdc: "a.m.", "p.m."
#define TK_hournum   18    // hournum: 0-24
#define TK_minnum    19    // minnum: 0-60
#define TK_wquart    20    // wquart: "cuarto" "media" "pico" "algo" "poco"
#define TK_wy        21    // wy: "y"
#define TK_wmenos    22    // wy: "wmenos"
#define TK_wmorning  23    // wmorning: "mañana", "tarde", "noche", "madrugada"
#define TK_wmidnight 24    // wmidnight: "mediodia", "medianoche"
#define TK_wen       25    // wen: "en"
#define TK_wa        26    // wa: "a" "al"
#define TK_wde       27    // wde: "de"
#define TK_wdel      28    // wdel "del
#define TK_wel       29    // wel: "el"
#define TK_wpor      30    // wpor: "por" 
#define TK_whacia    31    // wpor: "sobre" "hacia"
#define TK_weso      32    // weso: "eso"
#define TK_wla       33    // wla: "la" "las"
#define TK_wpunto    34    // wpunto: "punto"
#define TK_whour     35    // whour: "horas" "h."
#define TK_wmin      36    // wpico: "minutos" "min"
#define TK_hour      37    // hour: something matching just first part of RE_TIME1: 3h, 17h
#define TK_hhmm      38    // hhmm: something matching whole RE_TIME1: 3h25min, 3h25m, 3h25minutos,
#define TK_min       39    // min: something matching RE_TIME2: 25min, 3minutos
#define TK_date      40    // date: something matching RE_DATE: 3/ago/1992, 3/8/92, 3/agosto/1992, ...
#define TK_other     41    // other: any other token, not relevant in current state


///////////////////////////////////////////////////////////////
///  Create a dates recognizer for Spanish.
///////////////////////////////////////////////////////////////

dates_es::dates_es(): dates_module(RE_DATE_ES, RE_TIME1_ES, RE_TIME2_ES, RE_ROMAN)
{
  //Initialize token translation map
  tok.insert(make_pair("lunes",TK_weekday));     tok.insert(make_pair("martes",TK_weekday));
  tok.insert(make_pair("miércoles",TK_weekday)); tok.insert(make_pair("miercoles",TK_weekday));
  tok.insert(make_pair("jueves",TK_weekday));    tok.insert(make_pair("viernes",TK_weekday));
  tok.insert(make_pair("sábado",TK_weekday));    tok.insert(make_pair("sabado",TK_weekday));
  tok.insert(make_pair("domingo",TK_weekday));

  tok.insert(make_pair("por",TK_wpor));  tok.insert(make_pair("hacia",TK_whacia));
  tok.insert(make_pair("sobre",TK_whacia)); tok.insert(make_pair("eso",TK_weso));      
  tok.insert(make_pair("y",TK_wy));      tok.insert(make_pair("menos",TK_wmenos));
  tok.insert(make_pair("de",TK_wde));    tok.insert(make_pair("del",TK_wdel));
  tok.insert(make_pair("a",TK_wa));      tok.insert(make_pair("al",TK_wa));
  tok.insert(make_pair("en",TK_wen));    tok.insert(make_pair("punto",TK_wpunto));
  tok.insert(make_pair("la",TK_wla));    tok.insert(make_pair("las",TK_wla));
  tok.insert(make_pair("el",TK_wel));
  tok.insert(make_pair(",",TK_comma));   tok.insert(make_pair(".",TK_dot));
  tok.insert(make_pair(":",TK_colon));   tok.insert(make_pair("día",TK_wday)); 
  tok.insert(make_pair("dia",TK_wday));  tok.insert(make_pair("mes",TK_wmonth)); 
  tok.insert(make_pair("año",TK_wyear)); tok.insert(make_pair("presente",TK_wpast));
  tok.insert(make_pair("pasado",TK_wpast)); tok.insert(make_pair("próximo",TK_wpast));
	     
  tok.insert(make_pair("media",TK_wquart)); tok.insert(make_pair("cuarto",TK_wquart));
  tok.insert(make_pair("pico",TK_wquart));  tok.insert(make_pair("algo",TK_wquart));
  tok.insert(make_pair("poco",TK_wquart));   

  tok.insert(make_pair("siglo",TK_wcentury)); tok.insert(make_pair("s.",TK_wcentury));
  tok.insert(make_pair("sig.",TK_wcentury));  tok.insert(make_pair("sgl.",TK_wcentury));
  tok.insert(make_pair("sigl.",TK_wcentury));

  tok.insert(make_pair("a.c.",TK_wacdc));  tok.insert(make_pair("d.c.",TK_wacdc));
  tok.insert(make_pair("a.m.",TK_wampm));  tok.insert(make_pair("p.m.",TK_wampm));
  						 
  tok.insert(make_pair("medianoche",TK_wmidnight)); tok.insert(make_pair("mediodía",TK_wmidnight));
  tok.insert(make_pair("mediodia",TK_wmidnight));   tok.insert(make_pair("mañana",TK_wmorning)); 
  tok.insert(make_pair("tarde",TK_wmorning));       tok.insert(make_pair("madrugada",TK_wmorning));
  tok.insert(make_pair("noche",TK_wmorning)); 
	     
  tok.insert(make_pair("horas",TK_whour)); tok.insert(make_pair("hora",TK_whour));
  tok.insert(make_pair("h.",TK_whour));    tok.insert(make_pair("h",TK_whour));
  tok.insert(make_pair("m",TK_wmin));      tok.insert(make_pair("m.",TK_wmin));
  tok.insert(make_pair("min",TK_wmin));    tok.insert(make_pair("min.",TK_wmin));
  tok.insert(make_pair("minuto",TK_wmin)); tok.insert(make_pair("minutos",TK_wmin));

  tok.insert(make_pair("enero",TK_month));      tok.insert(make_pair("ene",TK_shmonth));
  tok.insert(make_pair("febrero",TK_month));    tok.insert(make_pair("feb",TK_shmonth));
  tok.insert(make_pair("marzo",TK_month));      tok.insert(make_pair("mar",TK_shmonth));
  tok.insert(make_pair("abril",TK_month));      tok.insert(make_pair("abr",TK_shmonth));
  tok.insert(make_pair("mayo",TK_month));       tok.insert(make_pair("may",TK_shmonth));
  tok.insert(make_pair("junio",TK_month));      tok.insert(make_pair("jun",TK_shmonth));
  tok.insert(make_pair("julio",TK_month));      tok.insert(make_pair("jul",TK_shmonth));
  tok.insert(make_pair("agosto",TK_month));     tok.insert(make_pair("ago",TK_shmonth));
  tok.insert(make_pair("septiembre",TK_month)); tok.insert(make_pair("sep",TK_shmonth));
  tok.insert(make_pair("octubre",TK_month));    tok.insert(make_pair("oct",TK_shmonth));
  tok.insert(make_pair("noviembre",TK_month));  tok.insert(make_pair("nov",TK_shmonth));
  tok.insert(make_pair("diciembre",TK_month));  tok.insert(make_pair("dic",TK_shmonth));
  tok.insert(make_pair("noviem",TK_month));     tok.insert(make_pair("sept",TK_shmonth));
  tok.insert(make_pair("diciem",TK_month));     tok.insert(make_pair("agos",TK_shmonth)); 
  	     
  // initialize map with month numeric values
  nMes.insert(make_pair("enero",1));      nMes.insert(make_pair("ene",1));
  nMes.insert(make_pair("febrero",2));    nMes.insert(make_pair("feb",2));
  nMes.insert(make_pair("marzo",3));      nMes.insert(make_pair("mar",3));
  nMes.insert(make_pair("abril",4));      nMes.insert(make_pair("abr",4));
  nMes.insert(make_pair("mayo",5));       nMes.insert(make_pair("may",5));
  nMes.insert(make_pair("junio",6));      nMes.insert(make_pair("jun",6));
  nMes.insert(make_pair("julio",7));      nMes.insert(make_pair("jul",7));
  nMes.insert(make_pair("agosto",8));     nMes.insert(make_pair("ago",8));
  nMes.insert(make_pair("septiembre",9)); nMes.insert(make_pair("sep",9));
  nMes.insert(make_pair("octubre",10));   nMes.insert(make_pair("oct",10));
  nMes.insert(make_pair("noviembre",11)); nMes.insert(make_pair("nov",11));
  nMes.insert(make_pair("diciembre",12)); nMes.insert(make_pair("dic",12));
  nMes.insert(make_pair("agos",8));     nMes.insert(make_pair("sept",9));
  nMes.insert(make_pair("noviem",11));  nMes.insert(make_pair("diciem",12));  

  // initialize map with weekday numeric values
  nDia.insert(make_pair("lunes","L"));       nDia.insert(make_pair("martes","M"));
  nDia.insert(make_pair("miércoles","X"));   nDia.insert(make_pair("miercoles","X"));
  nDia.insert(make_pair("jueves","J"));      nDia.insert(make_pair("viernes","V"));
  nDia.insert(make_pair("sábado","S"));      nDia.insert(make_pair("sabado","S"));
  nDia.insert(make_pair("domingo","G"));
  					      
  // Initialize special state attributes
  initialState=A; stopState=STOP;

  // Initialize Final state set 
  Final.insert(B);   Final.insert(E);     Final.insert(I);   Final.insert(L);
  Final.insert(P);   Final.insert(S2);    Final.insert(BH);  Final.insert(BH2);  
  Final.insert(CH);  Final.insert(GH);     Final.insert(GH1); 

  // Initialize transitions table. By default, stop state
  int s,t;
  for(s=0;s<MAX_STATES;s++) for(t=0;t<MAX_TOKENS;t++) trans[s][t]=STOP;

  // State A
  trans[A][TK_weekday]=B;   trans[A][TK_wday]=D;   trans[A][TK_wmonth]=G;
  trans[A][TK_wyear]=K;     trans[A][TK_date]=L;     
  trans[A][TK_month]=I;     trans[A][TK_shmonth]=Ib;  
  trans[A][TK_wcentury]=S1; trans[A][TK_wpast]=A;  trans[A][TK_daynum]=CH1;
  trans[A][TK_hour]=CH1;    trans[A][TK_hhmm]=BH1; trans[A][TK_wmidnight]=GH1;
  // State B+
  trans[B][TK_wpast]=B;   trans[B][TK_daynum]=E; trans[B][TK_comma]=C; 
  trans[B][TK_wday]=D;    
  trans[B][TK_wpor]=Hd;  trans[B][TK_wa]=Ha; trans[B][TK_whacia]=Ha;
  // State C
  trans[C][TK_daynum]=E; trans[C][TK_wday]=D;
  // State D
  trans[D][TK_daynum]=E;
  // State E+
  trans[E][TK_wde]=F;    trans[E][TK_wdel]=F;  
  trans[E][TK_month]=I;  trans[E][TK_shmonth]=I;
  trans[E][TK_monthnum]=I;
  trans[E][TK_wpor]=Hd;  trans[E][TK_wa]=Ha;   trans[E][TK_whacia]=Ha;
  // State Eb
  trans[Eb][TK_wde]=F;    trans[Eb][TK_wdel]=F;  
  trans[Eb][TK_month]=I;  trans[Eb][TK_shmonth]=I;
  trans[Eb][TK_monthnum]=I;
  trans[Eb][TK_wpor]=Hd;  trans[Eb][TK_wa]=Ha;   trans[Eb][TK_whacia]=Ha;
  // State F
  trans[F][TK_wpast]=F;  trans[F][TK_month]=I;   trans[F][TK_shmonth]=I; 
  trans[F][TK_wmonth]=G; trans[F][TK_monthnum]=I;
  // State G
  trans[G][TK_wde]=H;   trans[G][TK_monthnum]=I;   
  trans[G][TK_month]=I; trans[G][TK_shmonth]=I; 
  // State H
  trans[H][TK_month]=I; trans[H][TK_shmonth]=I; trans[H][TK_monthnum]=I;   
  // State I+
  trans[I][TK_wde]=J;   trans[I][TK_wdel]=J;
  trans[I][TK_wpor]=Hd; trans[I][TK_wa]=Ha;  trans[I][TK_whacia]=Ha;
  // State Ib
  trans[Ib][TK_wde]=J;   trans[Ib][TK_wdel]=J; 
  // State J
  trans[J][TK_wpast]=J; trans[J][TK_number]=L; trans[J][TK_wyear]=K;
  // State K
  trans[K][TK_wpast]=K; trans[K][TK_number]=L;
  // State L+
  trans[L][TK_wacdc]=P;
  trans[L][TK_wpor]=Hd; trans[L][TK_wa]=Ha;   trans[L][TK_whacia]=Ha;
  // State P+
  trans[P][TK_wpor]=Hd; trans[P][TK_wa]=Ha;   trans[P][TK_whacia]=Ha;
  // State S1
  trans[S1][TK_roman]=S2; trans[S1][TK_dot]=S1;
  // State S2+
  trans[S2][TK_wacdc]=S2;

  // State Ha
  trans[Ha][TK_weso]=Hb; trans[Ha][TK_wel]=AH; trans[Ha][TK_wla]=AH; 
  trans[Ha][TK_wmidnight]=GH; 
  // State Hb
  trans[Hb][TK_wde]=Hc;  trans[Hb][TK_wdel]=AH;
  // State Hc
  trans[Hc][TK_wla]=AH;
  // State Hd
  trans[Hd][TK_wel]=He; trans[Hd][TK_wla]=He;
  // State He
  trans[He][TK_wmidnight]=GH;   trans[He][TK_wmorning]=GH; 
  // State AH
  trans[AH][TK_hhmm]=BH; trans[AH][TK_hour]=CH; 
  trans[AH][TK_hournum]=CH; trans[AH][TK_wmidnight]=GH;
  // State BH+
  trans[BH][TK_wmin]=BH; trans[BH][TK_wampm]=GH;
  trans[BH][TK_wdel]=FH; trans[BH][TK_wde]=FH; 
  // State CH+
  trans[CH][TK_whour]=CH; trans[CH][TK_minnum]=BH; trans[CH][TK_min]=BH;
  trans[CH][TK_wen]=DH;   trans[CH][TK_wy]=EH;     trans[CH][TK_wmenos]=EHb;
  trans[CH][TK_wde]=FH;   trans[CH][TK_wdel]=FH;   trans[CH][TK_wampm]=GH; 
  // State DH
  trans[DH][TK_wpunto]=BH;
  // State EH
  trans[EH][TK_wquart]=BH; trans[EH][TK_minnum]=BH; trans[EH][TK_min]=BH;
  // State EHb
  trans[EHb][TK_wquart]=BH; trans[EHb][TK_minnum]=BH;  trans[EHb][TK_min]=BH;
  // State FH
  trans[FH][TK_wla]=FH; trans[FH][TK_wmorning]=GH; trans[FH][TK_wmidnight]=GH;
  // State GH+
  // nothing else expected. 

  // State BH1
  trans[BH1][TK_wmin]=BH1; trans[BH1][TK_wampm]=GH1; trans[BH1][TK_wdel]=FH1;
  trans[BH1][TK_wde]=FH1;  trans[BH1][TK_wel]=A;     trans[BH1][TK_wen]=DH1;
  // State BH2+
  trans[BH2][TK_wampm]=GH1; trans[BH2][TK_wdel]=FH1;
  trans[BH2][TK_wde]=FH1;   trans[BH2][TK_wel]=A;
  // State CH1
  trans[CH1][TK_whour]=CH1; trans[CH1][TK_minnum]=BH1;  trans[CH1][TK_min]=BH1;
  trans[CH1][TK_wen]=DH1;   trans[CH1][TK_wmenos]=EH1b; trans[CH1][TK_wy]=EH1;
  trans[CH1][TK_wde]=FH1b;  trans[CH1][TK_wdel]=FH1b;   trans[CH1][TK_wampm]=GH1;
  trans[CH1][TK_month]=I;   trans[CH1][TK_shmonth]=I;
  // State DH1
  trans[DH1][TK_wpunto]=BH2;
  // State EH1
  trans[EH1][TK_wquart]=BH2; trans[EH1][TK_minnum]=BH1;   trans[EH1][TK_min]=BH1;
  // State EH1b
  trans[EH1b][TK_wquart]=BH2; trans[EH1b][TK_minnum]=BH1; trans[EH1b][TK_min]=BH1;
  // State FH1
  trans[FH1][TK_wla]=FH1;       trans[FH1][TK_wmorning]=GH1; 
  trans[FH1][TK_wmidnight]=GH1; trans[FH1][TK_weekday]=B; 
  trans[FH1][TK_wday]=D;        trans[FH1][TK_daynum]=Eb; 
  // State FH1b
  trans[FH1b][TK_wla]=FH1;    trans[FH1b][TK_wmonth]=G;
  trans[FH1b][TK_month]=I;    trans[FH1b][TK_shmonth]=I;   
  trans[FH1b][TK_wpast]=FH1b; trans[FH1b][TK_weekday]=B; 
  trans[FH1b][TK_wday]=D;     trans[FH1b][TK_wyear]=K;
  trans[FH1b][TK_daynum]=Eb;
  trans[FH1b][TK_wmorning]=GH1;   trans[FH1b][TK_wmidnight]=GH1; 
  // State GH1+
  trans[GH1][TK_wde]=A;    trans[GH1][TK_wdel]=A;    trans[GH1][TK_wel]=A;

  TRACE(3,"analyzer succesfully created");
}


//-- Implementation of virtual functions from class automat --//

///////////////////////////////////////////////////////////////
///  Compute the right token code for word j from given state.
///////////////////////////////////////////////////////////////

int dates_es::ComputeToken(int state, sentence::iterator &j, sentence &se)
{
  string form,formU;
  int token,value;
  map<string,int>::iterator im;

  formU = j->get_form();
  form = util::lowercase(formU);

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
  value=0;
  if (j->get_n_analysis() && j->get_parole()=="Z") {
    token = TK_number;
    value = util::string2int(j->get_lemma());
    TRACE(3,"Numerical value of form '"+j->get_lemma()+"': "+util::int2string(value));
  }
  
  // determine how to interpret that number, or if not number,
  // check for specific regexps.
  switch (state) {
  // --------------------------------
  case A: 
    TRACE(3,"In state A");     
    if (token==TK_number && value>=1 && value<=31 && form!="una" && form!="un") {
      token = TK_daynum;
    }
    else if (RE_Date.Search(form)) {
      TRACE(3,"Match DATE regex. "+RE_Date.Match(0));
      token = TK_date;
    }
    else if (RE_Time1.Search(form)) {
      if(RE_Time1.Match(2)!=""){
	TRACE(3,"Match TIME1 regex (hour+min)");
	token = TK_hhmm;
      }
      else{
	TRACE(3,"Partial match TIME1 regex (hour)");
	token = TK_hour;
      }	      
    }
    break;
  // --------------------------------
  case B:
  case C:
  case D:
  case FH1:
  case FH1b:
    TRACE(3,"In state B/C/D");     
    if (token==TK_number && value>=1 && value<=31 && form!="una" && form!="un") 
      // ("un/una de enero" is an invalid date)
      token = TK_daynum;
    break;
  // --------------------------------
  case E:
  case Eb:
  case F:
  case G:
  case H:
    TRACE(3,"In state E/F/G/H");     
    if (token==TK_number && value>=1 && value<=12) 
      token = TK_monthnum;
    break;
  // --------------------------------
  case S1:
    if (RE_Roman.Search(formU)) {
      TRACE(3,"Match ROMAN regex. "+RE_Roman.Match(0));
      token=TK_roman;
    }
    break;
  // --------------------------------
  case AH:
    TRACE(3,"In state AH");     
    if (token==TK_number && value>=0 && value<=24) {
      token = TK_hournum;
    }
    else if (RE_Time1.Search(form)) {
      if(RE_Time1.Match(2)!=""){
	TRACE(3,"Match TIME1 regex (hour+min)");
	token = TK_hhmm;
      }
      else{
	TRACE(3,"Partial match TIME1 regex (hour)");
	token = TK_hour;
      }	      
    }
    break;
  // --------------------------------
  case CH:
  case CH1:
    TRACE(3,"In state CH/CH1");
    if (token==TK_number && value>=0 && value<=60) {
      token=TK_minnum;
    }
    else if (RE_Time2.Search(form)) {
      TRACE(3,"Match TIME2 regex (minutes)");
      token = TK_min;
    }    
    break;
  // --------------------------------
  case EH:
  case EHb:
  case EH1:
  case EH1b:
    TRACE(3,"In state EH/EH1");     
    if (token==TK_number && value>=0 && value<=60){
      token=TK_minnum;
    }
    else if (RE_Time2.Search(form)) {
      TRACE(3,"Match TIME2 regex (minutes)");
      token = TK_min;
    }
    break;
  // --------------------------------  
  default: break;
  }
  
  TRACE(3,"Leaving state "+util::int2string(state)+" with token "+util::int2string(token)); 
  return (token);
}



///////////////////////////////////////////////////////////////
///   Reset acumulators used by state actions:
///   day, year, month, hour, minute, etc.
///////////////////////////////////////////////////////////////

void dates_es::ResetActions() 
{
  century="??";
  weekday="??";
  day="??";
  month="??";
  year="??";
  hour="??";
  minute="??";
  meridian="??";
  temp = -1;
}


///////////////////////////////////////////////////////////////
///  Perform necessary actions in "state" reached from state 
///  "origin" via word j interpreted as code "token":
///  Basically, when reaching a state with an informative 
///  token (day, year, month, etc) store that part of the date.
///////////////////////////////////////////////////////////////

void dates_es::StateActions(int origin, int state, int token, sentence::const_iterator j)
{
  string form;
  int value;
  map<string,int>::iterator im;

  form = util::lowercase(j->get_form());
  TRACE(3,"Reaching state "+util::int2string(state)+" with token "+util::int2string(token)+" for word ["+form+"]");

  // get token numerical value, if any
  value=0;
  if ((token==TK_number || token==TK_daynum || token==TK_monthnum ||
       token==TK_hournum || token==TK_minnum ) &&
      j->get_n_analysis() && j->get_parole()=="Z") {
    value = util::string2int(j->get_lemma());
  }

  // State actions
  switch (state) {
  // ---------------------------------
  case B:
    if (token==TK_weekday) {
      TRACE(3,"Actions for state B");
      weekday=nDia[form];
      if (origin==FH1b) {
	// what we found in CH1 was an hour. Day is coming now
	minute="00";
	if (temp>12) { 
	  meridian="pm";
	  hour=util::int2string(temp-12);
	}
	else if (temp!= -1) {
	  hour=util::int2string(temp);
	}
      }
    }
    break;
  // ---------------------------------
  case D:
    if (origin==FH1b) {
      TRACE(3,"Actions for state D");
      // what we found in CH1 was an hour. Day is coming now
      minute="00";
      if (temp>12) {
	meridian="pm";
	hour=util::int2string(temp-12);
      }
      else  if (temp!= -1) {
	hour=util::int2string(temp);
      }
    }
    break;
  // ---------------------------------
  case E:
  case Eb:
    if (token==TK_daynum) {
      TRACE(3,"Actions for state E");
      day=util::int2string(value);
    }

    if (origin==FH1b) {
      // what we found in CH1 was an hour. Day is coming now
      minute="00";
      if (temp>12) { 
	meridian="pm";
	hour=util::int2string(temp-12);
      }
      else if (temp!= -1) {
	hour=util::int2string(temp);
      }
    }
    break;
  // ---------------------------------
  case G:
    if (origin==FH1b) {
      TRACE(3,"Actions for state G/K");
      // what we found in CH1 was a day, not an hour
      day=util::int2string(temp);
    }
    break;
  // ---------------------------------
  case K:
    if (origin==FH1b) {
      TRACE(3,"Actions for state G/K");
      // what we found in CH1 was a month
      month=util::int2string(temp);
    }
    break;
  // ---------------------------------
  case I:
    TRACE(3,"Actions for state I");
    if (origin==FH1b || origin==CH1) {
      // what we found in CH1 was a day, not an hour
      day=util::int2string(temp);
    }
    if (token==TK_monthnum) {
      month = j->get_lemma();
    }
    else if (token==TK_month || token==TK_shmonth) {
      month = util::int2string(nMes[form]);
    }
    break;
  // ---------------------------------
  case Ib:
    TRACE(3,"Actions for state Ib");
    if (token==TK_shmonth)
      month = util::int2string(nMes[form]);
    break;
  // ---------------------------------
  case L:
    TRACE(3,"Actions for state L");
    if (token==TK_number) {
      year=util::int2string(value);
    }
    else if (token==TK_date) {
      // it has to match since the token is TK_date
      RE_Date.Search(form);
      // day number
      day=RE_Date.Match(1);
      // to unify notation (01 -> 1)
      day=util::int2string(util::string2int(day));
      // month number (translating month name if necessary)
      im=nMes.find(util::lowercase(RE_Date.Match(2)));
      if(im!=nMes.end()) {
	month = util::int2string((*im).second);
      }
      else {
	month = RE_Date.Match(2);
	// to unify notation (01 -> 1)
	month=util::int2string(util::string2int(month));
      }
      // year number
      year = RE_Date.Match(3);
      // if year has only two digits, it can be 19xx or 20xx
      if (util::string2int(year)>=0 && util::string2int(year)<=20) { year="20"+year;} 
      else if (util::string2int(year)>=50 && util::string2int(year)<=99) {year="19"+year;}
    }
    break;
  // ---------------------------------
  case P:
    TRACE(3,"Actions for state P");
    if (form=="a.c.") {
      if(century!="") century="-"+century;   
      if(year!="??") year="-"+year;
    }
    break;
  // ---------------------------------
  case S2:
    TRACE(3,"Actions for state S2");
    if (token==TK_roman) {
      century=form; }
    else if (form=="a.c.") {
      century = "-"+century; }
    break;
  // ---------------------------------
  case BH:
  case BH1:
  case BH2:
    TRACE(3,"Actions for state BH/BH1/BH2");
    // what we found in CH1 was an hour
    if (origin!=A && (state==BH1 || state==BH2)) {
      if (temp>12) {
	meridian="pm";
	hour = util::int2string(temp-12);
      }
      else if (temp!= -1) {
	hour = util::int2string(temp);
      }
    }
    if (token==TK_minnum) {
      // "y veinte" vs "menos veinte"
      if (origin==EHb || origin==EH1b) {
	value = 60-value;
	if(hour=="1") hour="13"; // una menos veinte = 12:40
	hour = util::int2string(util::string2int(hour)-1);
      }
      minute=util::int2string(value);
    }
    else if (token==TK_wquart) {
      if (form=="media") {
	minute="30";
      }
      else if (form=="cuarto") {
	// "y cuarto" vs "menos cuarto"
	if (origin==EHb || origin==EH1b) {
          minute="45";
	  if(hour=="1") hour="13"; // una menos veinte = 12:40
	  hour = util::int2string(util::string2int(hour)-1);
	}
	else minute="15";
      }
    }
    else if (token==TK_wpunto && minute=="??") {
      minute="00";
    }
    else if (token==TK_hhmm) {
      // it has to match, since the token is TK_hhmm
      RE_Time1.Search(form);
      hour=RE_Time1.Match(1);
      minute=RE_Time1.Match(2);
    }
    else if (token==TK_min) {
      // it has to match, since the token is TK_min
      RE_Time2.Search(form);
      value=util::string2int(RE_Time2.Match(1));
      // "y veinte" vs "menos veinte"
      if (origin==EHb || origin==EH1b) {
	value = 60-value;
	if(hour=="1") hour="13"; // una menos veinte = 12:40
	hour = util::int2string(util::string2int(hour)-1);
      }
      minute=util::int2string(value);
    }
    break;
  // ---------------------------------
  case CH:
    TRACE(3,"Actions for state CH");
    if (token==TK_hournum) {
      if (value>12) { 
	meridian="pm";
	hour=util::int2string(value-12);
      }
      else hour=util::int2string(value);
      //set the minute to 0, if it isn't so, latter it will be replaced.
      minute="00";
    }
    else if (token==TK_hour) {
      // it has to match, since the token is TK_hour
      RE_Time1.Search(form);
      hour=RE_Time1.Match(1);
    }
    break;
  // ---------------------------------
  case CH1:
    TRACE(3,"Actions for state CH1");
    if (token==TK_daynum) {
      // maybe day or hour.  Store for later decision in BH1/B/D/G/I/K
      temp=value;
    }
    else if (token==TK_hour) {
      // it has to match, since the token is TK_hour
      RE_Time1.Search(form);
      hour=RE_Time1.Match(1);
    }
    break;
  // ---------------------------------
  case FH1:
    TRACE(3,"Actions for state FH1");
    if(origin==FH1b || origin==CH1) {
      // what we found in CH1 was an hour, not a day
      // put minute to 00 and set hour. 
      minute="00";
      if (temp>12) {
	meridian="pm";
	hour = util::int2string(temp-12);
      }
      else if (temp!= -1) {
	hour = util::int2string(temp);
      }
    }
    break;
  // ---------------------------------
  case GH:
  case GH1:
    TRACE(3,"Actions for state GH/GH1");

    if (origin==FH1b) {
      // what we found in CH1 was an hour, set minute to 00
      minute="00";
      if (temp>12) { 
	meridian="pm";
	hour=util::int2string(temp-12);
      }
      else if (temp!= -1) {
	hour=util::int2string(temp);
      }
    }

    if (token==TK_wampm) {
      if (form=="a.m.") {
	meridian="am"; }
      else if (form=="p.m.") {
	meridian="pm"; }
    }
    else if (token==TK_wmorning) {
      if (form=="mañana" || form=="madrugada")  {
	meridian="am"; }
      else if (form=="tarde" || form=="noche") {
	meridian="pm"; }
    }
    //else if (token==TK_wmidnight && hour=="??") {
    else if (token==TK_wmidnight) {
      if (form=="medianoche") {
	meridian="am";
	hour="00"; 
	minute="00";
      }
      else if (form=="mediodia" || form=="mediodía") {
	meridian="pm";
	if (hour=="??") {
	  hour="12"; 
	  minute="00";
	}
      }
    }
    break;
  // ---------------------------------
  default: break;
  }

  TRACE(3,"State actions finished. ["+weekday+":"+day+":"+month+":"+year+":"+hour+":"+minute+":"+meridian+"]");
}


///////////////////////////////////////////////////////////////
///   Set the appropriate lemma and parole for the 
///   new multiword.
///////////////////////////////////////////////////////////////

void dates_es::SetMultiwordAnalysis(sentence::iterator i, int fstate)
{
  list<analysis> la;
  string lemma;

  // Setting the analysis for the date
  if(century!="??") {
    lemma=string("[s:"+century+"]"); }
  else {
    lemma=string("["+weekday+":"+day+"/"+month+"/"+year+":"+hour+"."+minute+":"+meridian+"]"); }

  la.push_back(analysis(lemma,"W"));   
  i->set_analysis(la);
  TRACE(3,"Analysis set to: "+lemma+" W");
}


// undef codes to prepare for next module definition
#undef A
#undef B
#undef C
#undef D
#undef E
#undef Eb
#undef F
#undef G
#undef H
#undef I
#undef Ib
#undef J
#undef K
#undef L
#undef P
#undef S1
#undef S2
#undef Ha
#undef Hb
#undef Hc
#undef Hd
#undef He
#undef AH
#undef BH
#undef CH
#undef DH
#undef EH
#undef EHb
#undef FH
#undef GH
#undef BH1
#undef BH2
#undef CH1
#undef DH1
#undef EH1
#undef EH1b
#undef FH1
#undef FH1b
#undef GH1
#undef STOP
#undef TK_weekday
#undef TK_daynum
#undef TK_month
#undef TK_shmonth
#undef TK_monthnum
#undef TK_number
#undef TK_comma
#undef TK_dot
#undef TK_colon
#undef TK_wday
#undef TK_wmonth
#undef TK_wyear
#undef TK_wpast
#undef TK_wcentury
#undef TK_roman
#undef TK_wacdc
#undef TK_wampm
#undef TK_hournum
#undef TK_minnum
#undef TK_wquart
#undef TK_wy
#undef TK_wmenos
#undef TK_wmorning
#undef TK_wmidnight
#undef TK_wen
#undef TK_wa
#undef TK_wde
#undef TK_wdel
#undef TK_wel
#undef TK_wpor
#undef TK_whacia
#undef TK_weso
#undef TK_wla
#undef TK_wpunto
#undef TK_whour
#undef TK_wmin
#undef TK_hour
#undef TK_hhmm
#undef TK_min
#undef TK_date
#undef TK_other



//---------------------------------------------------------------------------
//        Catalan date recognizer
//---------------------------------------------------------------------------

//// State codes 
// states for a date
#define A  1     // A: initial state
#define B  2     // B: got a week day                           (VALID DATE)
#define C  3     // C: got a week day and a comma
#define D  4     // D: got the word "dia" from A,B,C 
#define E  5     // E: got day number from A,B,C,D              (VALID DATE)
#define Eb  54   // Eb: as E but not final
#define F  6     // F: got "de" or "del" from E (a month may come)
#define G  7     // G: got word "mes" from A,F
#define H  8     // H: got "de" from G (a month may come)
#define I  9     // I: got a month name o number from A,F,G,H.  (VALID DATE)
#define Ib 10    // Ib: got a short month name from A.
#define J 11     // J: got "de" or "del" from J (a year may come)
#define K 12     // K: got "año" from A,J (a year may come)
#define L 13     // L: got a number from J,K or a date from A.  (VALID DATE)
#define P 14     // P: got "a.c." or "d.c" from L,S2            (VALID DATE)
#define S1 15    // S1: got "siglo" from A (a century may come)
#define S2 16    // S2: got roman number from S1                (VALID DATE)
// states for time, after finding a date (e.g. 22 de febrero a las cuatro)
#define Ha 17    // Ha: got "hacia" "sobre" "a" after a valid date
#define Hb 18    // Hb: got "eso" from Ha
#define Hc 19    // Hc: got "de" from Hb
#define AH 22    // AH: got "a las" "sobre las" "hacia eso de las", etc. An hour is expected.
#define BH 23    // BH: got a valid hour+min from AH,CH,DH or EH. (VALID DATE)
#define CH 24    // CH: got a valid hour (no min) from AH         (VALID DATE)
#define DH 25    // DH: got "en" from CH (expecting "punto")
#define EH 26    // EH: got "y" from CH (expecting minutes)
#define EHb 27    // EH: got "menos" from CH (expecting minutes)
#define FH  28    // FH: got "de" "del" from BH,CH
#define GH  29    // GH: got "tarde" "noche" "mediodia" etc from BH,CH,FH  (VALID DATE)
// states for a time non preceded by a date
#define BH1  30   // BH1: got valid hour+min from A,CH1  (could be anything "dos y veinte")
#define BH2  31   // BH2: got valid hour+min from DH1,EH1,LH1  ("tres i quart", "dos quarts de tres") (VALID DATE)
#define CH1  32   // CH1: got valid hour (no min) from A (but could be a day number)
#define DH1  33   // DH1: got "en" from CH1 (expecting "punto")
#define EH1  34   // EH1: got "y" from CH1 (expecting minutes)
#define EH1b 35   // EH1: got "menos" from CH1 (expecting minutes)
#define FH1  36   // FH1B: got "de" "del" from BH1 (must be a time)
#define FH1b 37  // FH1: got "de" "del" from CH1 (a month name may follow)
#define GH1  38   // GH1: got "tarde" "noche" "mediodia" etc from BH1,CH1,FH1b  (VALID DATE)
// stop state
#define STOP 39

#define IH   40
#define JH   41
#define KH   42
#define LH   43
#define MH   44
#define MHb  45 
#define NH   46

#define IH1  47
#define JH1  48
#define KH1  49
#define LH1  50
#define MH1  51
#define MH1b  52
#define NH1  53



// Token codes
#define TK_weekday   1      // weekday: "Lunes", "Martes", etc
#define TK_daynum    2      // daynum:  1-31
#define TK_month     3      // month: "enero", "febrero", "ene." "feb." etc,
#define TK_shmonth   4      // short month: "ene", "feb", "mar" etc,
#define TK_monthnum  5      // monthnum: 1-12
#define TK_number    6      // number: 23, 1976, 543, etc.
#define TK_comma     7      // comma: ","
#define TK_dot       8     // dot: "."
#define TK_colon     9     // colon:  ":"
#define TK_wday      10    // wdia: "día"
#define TK_wmonth    11    // wmonth: "mes"
#define TK_wyear     12    // wyear: "año"
#define TK_wpast     13    // wpast: "pasado", "presente", "proximo"
#define TK_wcentury  14    // wcentury:  "siglo", "sig", "sgl", etc.
#define TK_roman     15    // roman: XVI, VII, III, XX, etc.
#define TK_wacdc     16    // wacdc: "a.c.", "d.c."
#define TK_wampm     17    // wacdc: "a.m.", "p.m."
#define TK_hournum   18    // hournum: 0-24
#define TK_minnum    19    // minnum: 0-60
#define TK_wquart    20    // wquart: "quarts" "quart"
#define TK_wi        21    // wy: "i"
#define TK_wmenys    22    // wmenys: "menys"
#define TK_wmorning  23    // wmorning: "mañana", "tarde", "noche", "madrugada"
#define TK_wmidnight 24    // wmidnight: "mediodia", "medianoche"
#define TK_wen       25    // wen: "en"
#define TK_wa        26    // wa: "a" 
#define TK_wal       27    // wa: "al"
#define TK_wde       28    // wde: "de"
#define TK_wdel      29    // wdel "del
#define TK_wel       30    // wel: "el"
#define TK_wpor      31    // wpor: "por" 
#define TK_wcap      32    // wpor: "sobre" "cap"
#define TK_walla     33    // weso: "alla"
#define TK_wla       34    // wla: "la" "les"
#define TK_wpunto    35    // wpunto: "punt"
#define TK_whour     36    // whour: "horas" "h."
#define TK_wmin      37    // wpico: "minutos" "min"
#define TK_hour      38    // hour: something matching just first part of RE_TIME1: 3h, 17h
#define TK_hhmm      39    // hhmm: something matching whole RE_TIME1: 3h25min, 3h25m, 3h25minutos,
#define TK_min       40    // min: something matching RE_TIME2: 25min, 3minutos
#define TK_date      41    // date: something matching RE_DATE: 3/ago/1992, 3/8/92, 3/agosto/1992, ...
#define TK_other     42    // other: any other token, not relevant in current state
#define TK_wmitja    43    // wmitja: "cuarto" "media" "pico" "algo" "poco" "escatx"
#define TK_wmig      44    // wmig: "mig"
#define TK_quartnum  45    // quartnum: 1, 2, 3 (quarts de...)


///////////////////////////////////////////////////////////////
///  Create a dates recognizer for Catalan.
///////////////////////////////////////////////////////////////

dates_ca::dates_ca(): dates_module(RE_DATE_CA, RE_TIME1_CA, RE_TIME2_CA, RE_ROMAN)
{
  //Initialize token translation map
  tok.insert(make_pair("dilluns",TK_weekday));   tok.insert(make_pair("dimarts",TK_weekday));
  tok.insert(make_pair("dimecres",TK_weekday));  tok.insert(make_pair("dijous",TK_weekday));
  tok.insert(make_pair("divendres",TK_weekday)); tok.insert(make_pair("dissabte",TK_weekday));
  tok.insert(make_pair("diumenge",TK_weekday));
  
  tok.insert(make_pair("cap",TK_wcap));
  tok.insert(make_pair("sobre",TK_wcap)); tok.insert(make_pair("alla",TK_walla)); tok.insert(make_pair("allà",TK_walla));      
  tok.insert(make_pair("i",TK_wi));       tok.insert(make_pair("menys",TK_wmenys));
  tok.insert(make_pair("de",TK_wde));     tok.insert(make_pair("del",TK_wdel));
  tok.insert(make_pair("d'",TK_wde)); 
  tok.insert(make_pair("a",TK_wa));      tok.insert(make_pair("al",TK_wal));
  tok.insert(make_pair("en",TK_wen));    tok.insert(make_pair("punt",TK_wpunto)); 
  tok.insert(make_pair("en_punt",TK_wpunto));
  tok.insert(make_pair("la",TK_wla));    tok.insert(make_pair("les",TK_wla));
  tok.insert(make_pair("el",TK_wel));    tok.insert(make_pair("l'",TK_wel));
  tok.insert(make_pair(",",TK_comma));   tok.insert(make_pair(".",TK_dot));
  tok.insert(make_pair(":",TK_colon));   
  tok.insert(make_pair("dia",TK_wday));     tok.insert(make_pair("mes",TK_wmonth)); 
  tok.insert(make_pair("any",TK_wyear));    tok.insert(make_pair("present",TK_wpast));
  tok.insert(make_pair("passat",TK_wpast)); tok.insert(make_pair("proper",TK_wpast));
  tok.insert(make_pair("vinent",TK_wpast)); tok.insert(make_pair("entrant",TK_wpast));
	     
  tok.insert(make_pair("mitja",TK_wmitja)); tok.insert(make_pair("escatx",TK_wmitja)); 
  tok.insert(make_pair("pico",TK_wmitja)); 
  tok.insert(make_pair("poc",TK_wmitja));

  tok.insert(make_pair("quart",TK_wquart)); tok.insert(make_pair("quarts",TK_wquart));

  tok.insert(make_pair("mig",TK_wmig));

  tok.insert(make_pair("segle",TK_wcentury)); tok.insert(make_pair("s.",TK_wcentury));
  tok.insert(make_pair("seg.",TK_wcentury));  tok.insert(make_pair("sgl.",TK_wcentury));
  tok.insert(make_pair("segl.",TK_wcentury));

  tok.insert(make_pair("a.c.",TK_wacdc));  tok.insert(make_pair("d.c.",TK_wacdc));
  tok.insert(make_pair("a.m.",TK_wampm));  tok.insert(make_pair("p.m.",TK_wampm));
  						 
  tok.insert(make_pair("mitjanit",TK_wmidnight)); tok.insert(make_pair("migdia",TK_wmidnight));
  tok.insert(make_pair("matí",TK_wmorning));      tok.insert(make_pair("tarda",TK_wmorning));
  tok.insert(make_pair("matinada",TK_wmorning));  tok.insert(make_pair("nit",TK_wmorning)); 
  tok.insert(make_pair("vespre",TK_wmorning));    tok.insert(make_pair("capvespre",TK_wmorning)); 
	     
  tok.insert(make_pair("hores",TK_whour)); tok.insert(make_pair("hora",TK_whour));
  tok.insert(make_pair("h.",TK_whour));    tok.insert(make_pair("h",TK_whour)); 
  tok.insert(make_pair("m",TK_wmin));      tok.insert(make_pair("m.",TK_wmin));
  tok.insert(make_pair("min",TK_wmin));    tok.insert(make_pair("min.",TK_wmin));
  tok.insert(make_pair("minut",TK_wmin));  tok.insert(make_pair("minuts",TK_wmin));

  tok.insert(make_pair("gener",TK_month));    tok.insert(make_pair("gen.",TK_shmonth));
  tok.insert(make_pair("febrer",TK_month));   tok.insert(make_pair("feb.",TK_shmonth));
  tok.insert(make_pair("març",TK_month));     tok.insert(make_pair("mar.",TK_shmonth));
  tok.insert(make_pair("abril",TK_month));    tok.insert(make_pair("abr.",TK_shmonth));
  tok.insert(make_pair("maig",TK_month));     tok.insert(make_pair("mai.",TK_shmonth));
  tok.insert(make_pair("juny",TK_month));     tok.insert(make_pair("jun.",TK_shmonth));
  tok.insert(make_pair("juliol",TK_month));   tok.insert(make_pair("jul.",TK_shmonth));
  tok.insert(make_pair("agost",TK_month));    tok.insert(make_pair("ago.",TK_shmonth));
  tok.insert(make_pair("setembre",TK_month)); tok.insert(make_pair("set.",TK_shmonth));
  tok.insert(make_pair("octubre",TK_month));  tok.insert(make_pair("oct.",TK_shmonth));
  tok.insert(make_pair("novembre",TK_month)); tok.insert(make_pair("nov.",TK_shmonth));
  tok.insert(make_pair("desembre",TK_month)); tok.insert(make_pair("des.",TK_shmonth));
  tok.insert(make_pair("novem",TK_month));    tok.insert(make_pair("agos",TK_shmonth));  
  tok.insert(make_pair("desem",TK_month));    
  	     
  // initialize map with month numeric values
  nMes.insert(make_pair("gener",1));     nMes.insert(make_pair("gen",1));
  nMes.insert(make_pair("febrer",2));    nMes.insert(make_pair("feb",2));
  nMes.insert(make_pair("març",3));      nMes.insert(make_pair("mar",3));
  nMes.insert(make_pair("abril",4));     nMes.insert(make_pair("abr",4));
  nMes.insert(make_pair("maig",5));      nMes.insert(make_pair("mai",5));
  nMes.insert(make_pair("juny",6));      nMes.insert(make_pair("jun",6));
  nMes.insert(make_pair("juliol",7));    nMes.insert(make_pair("jul",7));
  nMes.insert(make_pair("agost",8));     nMes.insert(make_pair("ago",8));
  nMes.insert(make_pair("setembre",9));  nMes.insert(make_pair("set",9));
  nMes.insert(make_pair("octubre",10));  nMes.insert(make_pair("oct",10));
  nMes.insert(make_pair("novembre",11)); nMes.insert(make_pair("nov",11));
  nMes.insert(make_pair("desembre",12)); nMes.insert(make_pair("des",12));
  nMes.insert(make_pair("agos",8));     
  nMes.insert(make_pair("novem",11));    nMes.insert(make_pair("desem",12));
  					      
  // initialize map with weekday numeric values
  nDia.insert(make_pair("dilluns","L"));     nDia.insert(make_pair("dimarts","M"));
  nDia.insert(make_pair("dimecres","X"));    nDia.insert(make_pair("dijous","J"));
  nDia.insert(make_pair("divendres","V"));   nDia.insert(make_pair("dissabte","S")); 
  nDia.insert(make_pair("diumenge","G"));
  					      
  // Initialize special state attributes
  initialState=A; stopState=STOP;

  // Initialize Final state set
  Final.insert(B);   Final.insert(E);     Final.insert(I);   Final.insert(L);
  Final.insert(P);   Final.insert(S2);    Final.insert(BH);  Final.insert(BH2); 
  Final.insert(CH);  Final.insert(GH);    Final.insert(GH1); 

  // Initialize transitions table. By default, stop state
  int s,t;
  for(s=0;s<MAX_STATES;s++) for(t=0;t<MAX_TOKENS;t++) trans[s][t]=STOP;

  // State A
  trans[A][TK_weekday]=B;   trans[A][TK_wday]=D;       trans[A][TK_wmonth]=G;    
  trans[A][TK_wyear]=K;     trans[A][TK_date]=L;     
  trans[A][TK_month]=I;     trans[A][TK_shmonth]=Ib;  
  trans[A][TK_wcentury]=S1; trans[A][TK_wpast]=A;      trans[A][TK_daynum]=CH1;
  trans[A][TK_hour]=CH1;    trans[A][TK_hhmm]=BH1;     trans[A][TK_wmidnight]=GH1;
  trans[A][TK_wmig]=IH1;    trans[A][TK_wquart]=KH1; 
 
  // State B+
  trans[B][TK_wpast]=B;   trans[B][TK_daynum]=E; trans[B][TK_comma]=C; 
  trans[B][TK_wday]=D;    
  trans[B][TK_wcap]=Ha; trans[B][TK_wal]=AH;
  trans[B][TK_wa]=Hc;   
  // State C
  trans[C][TK_daynum]=E; trans[C][TK_wday]=D;
  // State D
  trans[D][TK_daynum]=E;
  // State E+
  trans[E][TK_wde]=F;    trans[E][TK_wdel]=F;  
  trans[E][TK_month]=I;  trans[E][TK_shmonth]=I;
  trans[E][TK_monthnum]=I;
  trans[E][TK_wcap]=Ha; trans[E][TK_wal]=AH; trans[E][TK_wa]=Hc;   
  // State Eb
  trans[Eb][TK_wde]=F;    trans[Eb][TK_wdel]=F;  
  trans[Eb][TK_month]=I;  trans[Eb][TK_shmonth]=I;
  trans[Eb][TK_monthnum]=I;
  trans[Eb][TK_wcap]=Ha; trans[Eb][TK_wal]=AH; trans[Eb][TK_wa]=Hc;   
  // State F
  trans[F][TK_wpast]=F;  trans[F][TK_month]=I;   trans[F][TK_shmonth]=I; 
  trans[F][TK_wmonth]=G; trans[F][TK_monthnum]=I;
  // State G
  trans[G][TK_wde]=H;   trans[G][TK_monthnum]=I;   
  trans[G][TK_month]=I; trans[G][TK_shmonth]=I; 
  // State H
  trans[H][TK_month]=I; trans[H][TK_shmonth]=I; trans[H][TK_monthnum]=I;   
  // State I+
  trans[I][TK_wde]=J;   trans[I][TK_wdel]=J; 
  trans[I][TK_wcap]=Ha; trans[I][TK_wal]=AH; trans[I][TK_wa]=Hc;   
  // State Ib
  trans[Ib][TK_wde]=J;   trans[Ib][TK_wdel]=J; 
  // State J
  trans[J][TK_wpast]=J; trans[J][TK_number]=L; trans[J][TK_wyear]=K;  
  // State K
  trans[K][TK_wpast]=K; trans[K][TK_number]=L;
  // State L+
  trans[L][TK_wacdc]=P;
  trans[L][TK_wcap]=Ha; trans[L][TK_wal]=AH; trans[L][TK_wa]=Hc;   
  // State P+
  trans[P][TK_wcap]=Ha; trans[P][TK_wal]=AH; trans[P][TK_wa]=Hc;   
  // State S1
  trans[S1][TK_roman]=S2; trans[S1][TK_dot]=S1;
  // State S2+
  trans[S2][TK_wacdc]=S2;

  // State Ha
  trans[Ha][TK_walla]=Hb; trans[Ha][TK_wal]=AH; trans[Ha][TK_wel]=AH; 
  trans[Ha][TK_wa]=Hc;    
  // State Hb
  trans[Hb][TK_wel]=AH;  trans[Hb][TK_wla]=AH;
  trans[Hb][TK_wquart]=KH;
  trans[Hb][TK_wmig]=IH; trans[Hc][TK_quartnum]=IH;
  // State Hc
  trans[Hc][TK_wla]=AH;
  trans[Hc][TK_wquart]=KH;
  trans[Hc][TK_wmig]=IH; trans[Hc][TK_quartnum]=IH;
  // State AH
  trans[AH][TK_hhmm]=BH; trans[AH][TK_hour]=CH; 
  trans[AH][TK_hournum]=CH; trans[AH][TK_wmidnight]=GH;
  // State BH+
  trans[BH][TK_wmin]=BH; trans[BH][TK_wampm]=GH;
  trans[BH][TK_wdel]=FH; trans[BH][TK_wde]=FH; 
  // State CH+
  trans[CH][TK_whour]=CH; trans[CH][TK_minnum]=BH; trans[CH][TK_min]=BH;  
  trans[CH][TK_wen]=DH;   trans[CH][TK_wi]=EH;     trans[CH][TK_wmenys]=EHb;
  trans[CH][TK_wde]=FH;   trans[CH][TK_wdel]=FH;   trans[CH][TK_wampm]=GH; 
  trans[CH][TK_wpunto]=BH; 
  // State DH
  trans[DH][TK_wpunto]=BH;
  // State EH
  trans[EH][TK_wmitja]=BH; trans[EH][TK_wquart]=BH; trans[EH][TK_minnum]=BH; trans[EH][TK_min]=BH;
  // State EHb
  trans[EHb][TK_wmitja]=BH; trans[EHb][TK_wquart]=BH; trans[EHb][TK_minnum]=BH; trans[EHb][TK_min]=BH;
  // State FH
  trans[FH][TK_wla]=FH; trans[FH][TK_wmorning]=GH; trans[FH][TK_wmidnight]=GH;
  // State GH+
  // nothing else expected. 
  // State IH
  trans[IH][TK_wquart]=JH;
  // State JH
  trans[JH][TK_wmenys]=MHb;
  trans[JH][TK_wi]=MH;
  trans[JH][TK_wde]=LH; 
  // State KH
  trans[KH][TK_wde]=LH;
  // State LH
  trans[LH][TK_hour]=BH;  trans[LH][TK_hournum]=BH;
  // State MH
  trans[MH][TK_wmig]=NH;  trans[MH][TK_minnum]=NH;  trans[MH][TK_min]=NH;
  // State MHb
  trans[MHb][TK_wmig]=NH;  trans[MHb][TK_minnum]=NH;  trans[MHb][TK_min]=NH;
  // State NH
  trans[NH][TK_wmin]=NH;  
  trans[NH][TK_wde]=LH;

  // State BH1
  trans[BH1][TK_wmin]=BH1; trans[BH1][TK_wampm]=GH1; trans[BH1][TK_wdel]=FH1;
  trans[BH1][TK_wde]=FH1;  trans[BH1][TK_wel]=A;     trans[BH1][TK_wen]=DH1;
  // State BH2
  trans[BH2][TK_wampm]=GH1; trans[BH2][TK_wdel]=FH1;
  trans[BH2][TK_wde]=FH1;   trans[BH2][TK_wel]=A;
  // State CH1
  trans[CH1][TK_whour]=CH1; trans[CH1][TK_minnum]=BH1;  trans[CH1][TK_min]=BH1;
  trans[CH1][TK_wen]=DH1;   trans[CH1][TK_wmenys]=EH1b; trans[CH1][TK_wi]=EH1;
  trans[CH1][TK_wde]=FH1b;  trans[CH1][TK_wdel]=FH1b;   trans[CH1][TK_wampm]=GH1;
  trans[CH1][TK_month]=I;   trans[CH1][TK_shmonth]=I;
  trans[CH1][TK_wpunto]=BH1;   trans[CH1][TK_wquart]=JH1;
  // State DH1
  trans[DH1][TK_wpunto]=BH2;
  // State EH1
  trans[EH1][TK_wmitja]=BH2; trans[EH1][TK_wquart]=BH2; trans[EH1][TK_minnum]=BH1; trans[EH1][TK_min]=BH1;
  // State EH1b
  trans[EH1b][TK_wmitja]=BH2; trans[EH1b][TK_wquart]=BH2; trans[EH1b][TK_minnum]=BH1; trans[EH1b][TK_min]=BH1;
  // State FH1
  trans[FH1][TK_wla]=FH1;       trans[FH1][TK_wmorning]=GH1; 
  trans[FH1][TK_wmidnight]=GH1; trans[FH1][TK_weekday]=B; 
  trans[FH1][TK_wday]=D;        trans[FH1][TK_daynum]=Eb; 
  // State FH1b
  trans[FH1b][TK_wla]=FH1;    trans[FH1b][TK_wmonth]=G;
  trans[FH1b][TK_month]=I;    trans[FH1b][TK_shmonth]=I;   
  trans[FH1b][TK_wpast]=FH1b; trans[FH1b][TK_weekday]=B; 
  trans[FH1b][TK_wday]=D;     trans[FH1b][TK_wyear]=K;
  trans[FH1b][TK_daynum]=Eb; 
  trans[FH1b][TK_wmorning]=GH1;   trans[FH1b][TK_wmidnight]=GH1;
  // State GH1+
  trans[GH1][TK_wde]=A;    trans[GH1][TK_wdel]=A;    trans[GH1][TK_wel]=A;
  // State IH1
  trans[IH1][TK_wquart]=JH1;
  // State JH1
  trans[JH1][TK_wmenys]=MH1b;
  trans[JH1][TK_wi]=MH1;
  trans[JH1][TK_wde]=LH1; 
  // State KH1
  trans[KH1][TK_wde]=LH1;
  // State LH1
  trans[LH1][TK_hour]=BH2;  trans[LH1][TK_hournum]=BH2;
  // State MH1
  trans[MH1][TK_wmig]=NH1;  trans[MH1][TK_minnum]=NH1; trans[MH1][TK_min]=NH1;
  // State MH1b
  trans[MH1b][TK_wmig]=NH1;  trans[MH1b][TK_minnum]=NH1; trans[MH1b][TK_min]=NH1;
  // State NH1
  trans[NH1][TK_wmin]=NH1;  
  trans[NH1][TK_wde]=LH1;

  TRACE(3,"analyzer succesfully created");
}


//-- Implementation of virtual functions from class automat --//

///////////////////////////////////////////////////////////////
///  Compute the right token code for word j from given state.
///////////////////////////////////////////////////////////////

int dates_ca::ComputeToken(int state, sentence::iterator &j, sentence &se)
{
  string form,formU;
  int token,value;
  map<string,int>::iterator im;

  formU = j->get_form();
  form = util::lowercase(formU);

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
  value=0;
  if (j->get_n_analysis() && j->get_parole()=="Z") {
    token = TK_number;
    value = util::string2int(j->get_lemma());
    TRACE(3,"Numerical value of form: "+util::int2string(value));
  }
  
  // determine how to interpret that number, or if not number,
  // check for specific regexps.
  switch (state) {
  // --------------------------------
  case A: 
    TRACE(3,"In state A");     
    if (token==TK_number && value>=0 && value<=31) {
      token = TK_daynum; // it can be a "quart" number, an hour number  or a day number
    }
    else if (RE_Date.Search(form)) {
      TRACE(3,"Match DATE regex. "+RE_Date.Match(0));
      token = TK_date;
    }
    else if (RE_Time1.Search(form)) {
      if(RE_Time1.Match(2)!=""){
	TRACE(3,"Match TIME1 regex (hour+min)");
	token = TK_hhmm;
      }
      else{
	TRACE(3,"Partial match TIME1 regex (hour)");
	token = TK_hour;
      }	      
    }
    break;
  // --------------------------------
  case B:
  case C:
  case D:
  case FH1:
    TRACE(3,"In state B/C/D");     
    if (token==TK_number && value>=1 && value<=31 && form!="una" && form!="un") 
      // ("un/una de enero" is an invalid date)
      token = TK_daynum;
    break;
  // --------------------------------
  case E:
  case Eb:
  case F:
  case G:
  case H:
    TRACE(3,"In state E/F/G/H");     
    if (token==TK_number && value>=1 && value<=12) 
      token = TK_monthnum;
    break;
  // --------------------------------
  case S1:
    if (RE_Roman.Search(formU)) {
      TRACE(3,"Match ROMAN regex. "+RE_Roman.Match(0));
      token=TK_roman;
    }
    break;
  // --------------------------------
  case AH:
    TRACE(3,"In state AH");     
    if (token==TK_number && value>=0 && value<=24) {
      token = TK_hournum;
    }
    else if (RE_Time1.Search(form)) {
      if(RE_Time1.Match(2)!=""){
	TRACE(3,"Match TIME1 regex (hour+min)");
	token = TK_hhmm;
      }
      else{
	TRACE(3,"Partial match TIME1 regex (hour)");
	token = TK_hour;
      }	      
    }
    break;
  // --------------------------------
  case CH:
  case CH1:
    TRACE(3,"In state CH/CH1");
    if (token==TK_number && value>=0 && value<=60) {
      token=TK_minnum;
    }
    else if (RE_Time2.Search(form)) {
      TRACE(3,"Match TIME2 regex (minutes)");
      token = TK_min;
    }    
    break;
  // --------------------------------
  case EH:
  case EHb:
  case EH1:
  case EH1b:
    TRACE(3,"In state EH/EH1/EHb/EH1b");     
    if (token==TK_number && value>=0 && value<=60) {
      token=TK_minnum;
    }
    else if (RE_Time2.Search(form)) {
      TRACE(3,"Match TIME2 regex (minutes)");
      token = TK_min;
    }
    break;
  // -------------------------------- 
  case Hb:
  case Hc:
    TRACE(3,"In state Hb/Hc");     
    if (token==TK_number && (value==1  || value==2 || value==3 )) // "1, 2, 3 quarts de..."
      token=TK_quartnum;
    break;
  // -------------------------------- 
  case MH:
  case MHb:
  case MH1:
  case MH1b:
    TRACE(3,"In state MH/MH1");     
    if (token==TK_number && value>0 && value<15) { // "i cinc, i deu..."
      token=TK_minnum;
    }
    else if (RE_Time2.Search(form)) {
      TRACE(3,"Match TIME2 regex (minutes)");
      token = TK_min;
    }
    break;
  // -------------------------------- 
  case LH:
  case LH1:
    TRACE(3,"In state LH");     
    if (token==TK_number && value>=0 && value<=24) {
      token = TK_hournum;
    }
    else if (RE_Time1.Search(form)) {
      if(RE_Time1.Match(2)==""){
	TRACE(3,"Partial match TIME1 regex (hour)");
	token = TK_hour;
      }	      
    }
    break;
  default: break;
  }
  
  TRACE(3,"Leaving state "+util::int2string(state)+" with token "+util::int2string(token)); 
  return (token);
}



///////////////////////////////////////////////////////////////
///   Reset acumulators used by state actions:
///   day, year, month, hour, minute, etc.
///////////////////////////////////////////////////////////////

void dates_ca::ResetActions() 
{
  century="??";
  weekday="??";
  day="??";
  month="??";
  year="??";
  hour="??";
  minute="??";
  meridian="??";
  temp = -1;
  sign=0;
}


///////////////////////////////////////////////////////////////
///  Perform necessary actions in "state" reached from state 
///  "origin" via word j interpreted as code "token":
///  Basically, when reaching a state with an informative 
///  token (day, year, month, etc) store that part of the date.
///////////////////////////////////////////////////////////////

void dates_ca::StateActions(int origin, int state, int token, sentence::const_iterator j)
{
  string form;
  int value;
  map<string,int>::iterator im;

  form = util::lowercase(j->get_form());
  TRACE(3,"Reaching state "+util::int2string(state)+" with token "+util::int2string(token)+" for word ["+form+"]");

  // get token numerical value, if any
  value=0;
  if ((token==TK_number || token==TK_daynum || token==TK_monthnum ||
       token==TK_hournum || token==TK_minnum || token==TK_quartnum ) && 
      j->get_n_analysis() && j->get_parole()=="Z") {
    value = util::string2int(j->get_lemma());
  }

  // State actions
  switch (state) {
  // ---------------------------------
  case B:
    if (token==TK_weekday) { 
      TRACE(3,"Actions for state B");
      weekday=nDia[form];
      if (origin==FH1b) {
	// what we found in CH1 was an hour. Day is coming now
	minute="00";
	if (temp>12) { 
	  meridian="pm";
	  hour=util::int2string(temp-12);
	}
	else  if (temp!= -1) {
	  hour=util::int2string(temp);
	}
      }
    }
    break;
  // ---------------------------------
  case D:
    if (origin==FH1b) {
      TRACE(3,"Actions for state D");
      // what we found in CH1 was an hour. Day is coming now
      minute="00";
      if (temp>12){
	meridian="pm";
	hour=util::int2string(temp-12);
      }
      else if (temp!= -1) {
	hour=util::int2string(temp);
      }
    }
    break;
  // ---------------------------------
  case E:
  case Eb:
    if (origin==FH1b) {
      TRACE(3,"Actions for state D");
      // what we found in CH1 was an hour. Day is coming now
      minute="00";
      if (temp>12){
	meridian="pm";
	hour=util::int2string(temp-12);
      }
      else if (temp!= -1) {
	hour=util::int2string(temp);
      }
    }

    if (token==TK_daynum) {
      TRACE(3,"Actions for state E");
      day=util::int2string(value);
    }
    break;
  // ---------------------------------
  case G:
  case K:
    if (origin==FH1b) {
      TRACE(3,"Actions for state G/K");
      // what we found in CH1 was a day, not an hour
      day=util::int2string(temp);
    }
    break;
  // ---------------------------------
  case I:
    TRACE(3,"Actions for state I");
    if (origin==FH1b || origin==CH1) {
      // what we found in CH1 was a day, not an hour
      day=util::int2string(temp);
    }
    if (token==TK_monthnum) {
      month = j->get_lemma(); }
    else if (token==TK_month || token==TK_shmonth) {
      month = util::int2string(nMes[form]);}
    break;
  // ---------------------------------
  case Ib:
    TRACE(3,"Actions for state Ib");
    if (token==TK_shmonth)
      month = util::int2string(nMes[form]);
    break;
  // ---------------------------------
  case L:
    TRACE(3,"Actions for state L");
    if (token==TK_number) {
      year=util::int2string(value); }
    else if (token==TK_date) {
      // it has to match since the token is TK_date
      RE_Date.Search(form);
      // day number
      day=RE_Date.Match(1);
      // to unify notation (01 -> 1)
      day=util::int2string(util::string2int(day));
      // month number (translating month name if necessary) 
      im=nMes.find(util::lowercase(RE_Date.Match(2)));
      if(im!=nMes.end())
        month = util::int2string((*im).second);
      else {
        month = RE_Date.Match(2);
        // to unify notation (01 -> 1) 
        month=util::int2string(util::string2int(month));
      }
      // year number
      year = RE_Date.Match(3);
      // if year has only two digits, it can be 19xx or 20xx 
      if (util::string2int(year)>=0 && util::string2int(year)<=20) {year="20"+year;}
      else if (util::string2int(year)>=50 && util::string2int(year)<=99) {year="19"+year;}
    }

    break;
  // ---------------------------------
  case P:
    TRACE(3,"Actions for state P");
    if (form=="a.c.") {
      if(century!="") century="-"+century;   
      if(year!="??") year="-"+year;
    }
    break;
  // ---------------------------------
  case S2:
    TRACE(3,"Actions for state S2");
    if (token==TK_roman) {
      century=form;}
    else if (form=="a.c.") {
      century = "-"+century; }
    break;
  // ---------------------------------
  case BH:
  case BH1:
  case BH2:
    TRACE(3,"Actions for state BH/BH1");
    // what we found in CH1 was an hour
    if (origin!=A && (state==BH1 || state==BH2)) {
      if (temp>12) {
	meridian="pm";
	hour = util::int2string(temp-12);
      }
      else  if (temp!= -1) {
	hour = util::int2string(temp);
      }
    }
    // coming from LH or LH1 (tres quarts de 12 = 11:45)
    if (token==TK_hournum) {
      value=value-1;
      if(value==0) value=12;
      hour=util::int2string(value);
    }
    else if (token==TK_hour) {
      // it has to match, since the token is TK_hour
      RE_Time1.Search(form);
      hour = RE_Time1.Match(1);
      if(hour=="1") hour="13"; // tres quarts d'una= 12:45, no 0:45
      hour = util::int2string(util::string2int(hour)-1);
    }
    // coming from CH, CH1
    else if (token==TK_minnum) {
      // "y veinte" vs "menos veinte"
      if (origin==EHb || origin==EH1b) {
        value = 60-value;
	if(hour=="1") hour="13";
	hour = util::int2string(util::string2int(hour)-1);
      }
      minute=util::int2string(value);
    }
    else if (token==TK_wpunto && minute=="??") {
      minute="00";
    }
    // coming from EH
    else if (token==TK_wmitja) {
      if (form=="mitja") {
	minute="30";
      }
    }
    else if(token==TK_wquart) {
      if (form=="quart") {
	// "i quart" vs "menys quart"
	if (origin==EHb || origin==EH1b) {
	  minute="45";
	  if(hour=="1") hour="13";
	  hour = util::int2string(util::string2int(hour)-1);
	}
	else minute="15";
      }
    }
    // coming from DH
    else if (token==TK_wpunto && minute=="??") {
      minute="00";
    }
    else if (token==TK_hhmm) {
      // it has to match, since the token is TK_hhmm
      RE_Time1.Search(form);
      hour=RE_Time1.Match(1);
      minute=RE_Time1.Match(2);
    }
    else if (token==TK_min) {
      // it has to match, since the token is TK_min
      RE_Time2.Search(form);
      value=util::string2int(RE_Time2.Match(1));
      // "y veinte" vs "menos veinte"
      if (origin==EHb || origin==EH1b) {
        value = 60-value;
	if(hour=="1") hour="13";
	hour = util::int2string(util::string2int(hour)-1);
      }
      minute=util::int2string(value);
    }
    break;
  // ---------------------------------
  case CH:
    TRACE(3,"Actions for state CH");
    if (token==TK_hournum) {
      if (value>12) {
	meridian="pm";
	hour=util::int2string(value-12);
      }
      else hour=util::int2string(value);
      //set the minute to 0, if it isn't so latter it will be replaced.
      minute="00";
    }
    else if (token==TK_hour) {
      // it has to match, since the token is TK_hour
      RE_Time1.Search(form);
      hour=RE_Time1.Match(1);
    }
    break;
  // ---------------------------------
  case CH1:
    TRACE(3,"Actions for state CH1");
    if (token==TK_daynum) {
      // maybe day or hour.  Store for later decision in BH1/B/D/G/I/K
      temp=value;
    }
    else if (token==TK_hour) {
      // it has to match, since the token is TK_hour
      RE_Time1.Search(form);
      hour=RE_Time1.Match(1);
    }
    break;
  // ---------------------------------
  case FH1:
    // if coming from FH1b
    // what we found in CH1 was an hour, not a day.
    // put minute to 00. (A-> CH1->FH1b)
    if (origin==FH1b) {
      minute="00"; 
      if (temp>12) {
	hour = util::int2string(temp-12);
	meridian="pm";
      }
      else if (temp!= -1) {
        hour = util::int2string(temp);
      }
    }
    break;
  // ---------------------------------
  case GH:
  case GH1:
    TRACE(3,"Actions for state GH/GH1");

    if (origin==FH1b) {
      // what we found in CH1 was an hour. 
      minute="00";
      if (temp>12) { 
	meridian="pm";
	hour=util::int2string(temp-12);
      }
      else  if (temp!= -1) {
	hour=util::int2string(temp);
      }
    }

    if (token==TK_wampm) {
      if (form=="a.m.") 
	meridian="am";
      else if (form=="p.m.") 
	meridian="pm";
    }
    else if (token==TK_wmorning) {
      if (form=="mati" || form== "matí" || form=="matinada") 
	meridian="am";
      else if (form=="tarda" || form=="nit") 
	meridian="pm";
    }
    //else if (token==TK_wmidnight && hour=="??") {
    else if (token==TK_wmidnight) {
      if (form=="mitjanit") {
	meridian="am";
	hour="00"; 
	minute="00";
      }
      else if (form=="migdia") {
	meridian="pm";
	if (hour=="??") {
	  hour="12"; 
	  minute="00";
	}
      }
    }
    break;
  // ---------------------------------
  case IH:
  case IH1:
    if (token==TK_quartnum)
      temp=value;
    else if(token==TK_wmig) 
      temp=0;
    break;
  // ---------------------------------
  case MH:
  case MH1:
    sign=1;
    break;
  // ---------------------------------
  case MHb:
  case MH1b:
    sign=-1;
    break;
  // ---------------------------------
  case NH:
  case NH1:
    TRACE(3,"Actions for state NH/NH1");
    if(token==TK_wmig) {
      value=7; }
    else if(token==TK_min) {
      // it has to match, since the token is TK_min
      RE_Time2.Search(form);
      value=util::string2int(RE_Time2.Match(1));
    }
    if(temp!=0)
      minute=util::int2string(temp*15+sign*value);
    else minute=util::int2string(7+sign*value);
    break;
  // ---------------------------------
  case LH:
  case LH1:
    if(origin==JH || origin==JH1) {
      if(temp!=0)
	minute=util::int2string(temp*15);
      else minute="07";
    }
  // ---------------------------------
  default: break;
  }
  
  TRACE(3,"State actions finished. ["+weekday+":"+day+":"+month+":"+year+":"+hour+":"+minute+":"+meridian+"]");
}



///////////////////////////////////////////////////////////////
///   Set the appropriate lemma and parole for the 
///   new multiword.
///////////////////////////////////////////////////////////////

void dates_ca::SetMultiwordAnalysis(sentence::iterator i, int fstate)
{
  list<analysis> la;
  string lemma;

  // Setting the analysis for the date
  if(century!="??")
    lemma=string("[s:"+century+"]");
  else
    lemma=string("["+weekday+":"+day+"/"+month+"/"+year+":"+hour+"."+minute+":"+meridian+"]");

  la.push_back(analysis(lemma,"W"));   
  i->set_analysis(la);
  TRACE(3,"Analysis set to: "+lemma+" W");
}


// undef codes to prepare for next module definition
#undef A
#undef B
#undef C
#undef D
#undef E
#undef Eb
#undef F
#undef G
#undef H
#undef I
#undef Ib
#undef J
#undef K
#undef L
#undef P
#undef S1
#undef S2
#undef Ha
#undef Hb
#undef Hc
#undef Hd
#undef He
#undef AH
#undef BH
#undef CH
#undef DH
#undef EH
#undef EHb
#undef FH
#undef GH
#undef IH
#undef JH
#undef KH
#undef LH
#undef MH
#undef MHb
#undef NH
#undef BH1
#undef CH1
#undef DH1
#undef EH1
#undef EH1b
#undef FH1
#undef FH1b
#undef GH1
#undef IH1
#undef JH1
#undef KH1
#undef LH1
#undef MH1
#undef MH1b
#undef NH1
#undef STOP

#undef TK_weekday
#undef TK_daynum
#undef TK_month
#undef TK_shmonth
#undef TK_monthnum
#undef TK_number
#undef TK_comma
#undef TK_dot
#undef TK_colon
#undef TK_wday
#undef TK_wmonth
#undef TK_wyear
#undef TK_wpast
#undef TK_wcentury
#undef TK_roman
#undef TK_wacdc
#undef TK_wampm
#undef TK_hournum
#undef TK_minnum
#undef TK_wmitja
#undef TK_wmig
#undef TK_wquart
#undef TK_wy
#undef TK_wmenos
#undef TK_wmorning
#undef TK_wmidnight
#undef TK_wen
#undef TK_wa
#undef TK_wde
#undef TK_wdel
#undef TK_wel
#undef TK_wpor
#undef TK_whacia
#undef TK_weso
#undef TK_wla
#undef TK_wpunto
#undef TK_whour
#undef TK_wmin
#undef TK_hour
#undef TK_hhmm
#undef TK_min
#undef TK_date
#undef TK_other


//---------------------------------------------------------------------------
//        English date recognizer
//---------------------------------------------------------------------------

//// State codes 
// states for a date
#define A   1     // A: initial state
#define B   2     // B: got a week day                           (VALID DATE)
#define C   3     // C: got a week day and a comma
#define D   4     // D: got the word "day" from A,B,C 
#define E   5     // E: got day number from B,C,D     
#define Eb  6     // Eb: got a month from A                    (VALID DATE)
#define Ebb 7     // Ebb: got a month from C
#define Ec  8     // Ec: got a day number from A 
#define F   9     // F: got "of" from E (a month may come)
#define Fb  10 
#define G   11    // G: got word "month" from A,F
#define Gb  12    // Gb: got day number from Fb                 (VALID DATE)
#define Gbb 42    // as Gb but not a final state
#define H   13    // H: got "of" from G (a month may come)
#define I   14    // I: got a month name o number from A,F,G,H.  (VALID DATE)
#define Ib  15    // Ib: got a short month name from A.
#define Ic  43    // Ic: got a monthnum: as state I but not a final state
#define J   16    // J: got "of" from J (a year may come)
#define Jb  17    // Jb: got a centnum (num between 13 and 19) from J, K, I, Gb (first two numbers of the year)
#define K   18    // K: got "year" from A,J (a year may come)
#define L   19    // L: got a number from J,K or a date from A.  (VALID DATE)
#define M   20 
#define N   21 
#define O   22 
#define P   23    // P: got "b.c." or "a.c" from L               (VALID DATE)


//#define S1 15    // S1: got "siglo" from A (a century may come)
//#define S2 16    // S2: got roman number from S1                (VALID DATE)


// states for time, after finding a date (e.g. August 23th at a quarter past three)
#define Ha 24    // Ha: got "at" "about" after a valid date
#define AH 25    // AH: got hhmm.                                 (VALID DATE)
#define BH 26    // BH:                                        
#define CH 27    // CH:                                         
#define DH 28    // DH: 
#define EH 29    // EH:                                           (VALID DATE)
#define EHb 30   // EHb:                                          (VALID DATE)
#define FH  31   // FH: 
#define GH  32   // GH:                                           (VALID DATE)

// states for a time non preceded by a date
#define AH1   33   // AH1                                          (VALID DATE)
#define BH1   34   // BH1: 
#define CH1   35   // CH1: 
#define DH1   36   // DH1: 
#define DH1b  45   
#define EH1   37   // EH1:                                         (VALID DATE)  
#define EH1b  38  
#define EH1c  46  
#define FH1   39   // FH1: 
#define GH1   40   // GH1:                                         (VALID DATE)

// auxiliar state
#define aux 44

// stop state
#define STOP 41

////////
// Token codes
#define TK_weekday   1      // weekday: "Monday", "Tuesday", etc
#define TK_daynum    2      // daynum:  1st-31st
#define TK_month     3      // month: "enero", "febrero", "ene." "feb." etc,
#define TK_shmonth   4      // short month: "ene", "feb", "mar" etc,
#define TK_monthnum  5      // monthnum: 1-12
#define TK_number    6      // number: 23, 1976, 543, etc.
#define TK_comma     7      // comma: ","
#define TK_dot       8     // dot: "." 
#define TK_colon     9     // colon:  ":"                  ***not in use***
#define TK_wday      10    // wdia: "día"
#define TK_wmonth    11    // wmonth: "mes"
#define TK_wyear     12    // wyear: "año"
#define TK_wpast     13    // wpast: "pasado", "presente", "proximo"
#define TK_wcentury  14    // wcentury:  "siglo", "sig", "sgl", etc.       ***not in use***
#define TK_roman     15    // roman: XVI, VII, III, XX, etc.              ***not in use***
#define TK_wacdc     16    // wacdc: "b.c.", "a.d."
#define TK_wampm     17    // wacdc: "a.m.", "p.m."
#define TK_hournum   18    // hournum: 0-24
#define TK_minnum    19    // minnum: 0-60
#define TK_wquarter  20    // wquarter: "quarter"
#define TK_whalf     21    // 
#define TK_wto       22    // wto: "to"
#define TK_wmorning  23    // wmorning: 
#define TK_wmidnight 24    // wmidnight: 
#define TK_wat       25    // wat: "at"
#define TK_wof       26    // wof: "of"
#define TK_win       43    // win: "in", "on"
#define TK_wa        27    // wa: "a"
#define TK_wabout    28    // wabout: "about"
#define TK_wthe      29    // wthe: "the"
#define TK_wand      30    // wand: "and", "oh" (for the year)
#define TK_woclock   31    // woclock: "o'clock"
#define TK_wmin      32    // wmin: "minute(s)" "min" "hour" "hours"
#define TK_hour      33    // hour: something matching just first part of RE_TIME1: 3h, 17h
#define TK_hhmm      34    // hhmm: something matching whole RE_TIME1: 3h25min, 3h25m, 3h25minutes,
#define TK_min       35    // min: something matching RE_TIME2: 25min, 3minutes
#define TK_date      36    // date: something matching RE_DATE: 3/ago/1992, 3/8/92, 3/agosto/1992, ...
#define TK_other     37    // other: any other token, not relevant in current state
#define TK_kyearnum  38
#define TK_centnum   39
#define TK_yearnum   40
#define TK_whundred  41


///////////////////////////////////////////////////////////////
///  Create a dates recognizer for English.
///////////////////////////////////////////////////////////////

dates_en::dates_en(): dates_module(RE_DATE_EN, RE_TIME1_EN, RE_TIME2_EN, RE_ROMAN)
{
  //Initialize token translation map
  tok.insert(make_pair("monday",TK_weekday));     tok.insert(make_pair("thursday",TK_weekday));
  tok.insert(make_pair("wednesday",TK_weekday));  tok.insert(make_pair("tuesday",TK_weekday));
  tok.insert(make_pair("friday",TK_weekday));     tok.insert(make_pair("saturday",TK_weekday));
  tok.insert(make_pair("sunday",TK_weekday));    

  tok.insert(make_pair("the",TK_wthe));    
  tok.insert(make_pair("of",TK_wof));      tok.insert(make_pair("on",TK_wof));     
  tok.insert(make_pair("a",TK_wa));        tok.insert(make_pair("to",TK_wto));      
  tok.insert(make_pair("at",TK_wat));      tok.insert(make_pair("about",TK_wabout));
  tok.insert(make_pair("in",TK_win));
  tok.insert(make_pair("and",TK_wand));    tok.insert(make_pair("oh",TK_wand));

  tok.insert(make_pair(",",TK_comma));        tok.insert(make_pair(".",TK_dot));
  tok.insert(make_pair(":",TK_colon));        tok.insert(make_pair("day",TK_wday)); 
  tok.insert(make_pair("month",TK_wmonth));   tok.insert(make_pair("year",TK_wyear)); 
  tok.insert(make_pair("current",TK_wpast));  tok.insert(make_pair("last",TK_wpast)); 
  tok.insert(make_pair("next",TK_wpast));     tok.insert(make_pair("coming",TK_wpast));
  tok.insert(make_pair("past",TK_wpast));
     
  tok.insert(make_pair("half",TK_whalf));    tok.insert(make_pair("quarter",TK_wquarter));
  tok.insert(make_pair("o'clock",TK_woclock));    
  //tok.insert(make_pair("siglo",TK_wcentury)); tok.insert(make_pair("s.",TK_wcentury));
  //tok.insert(make_pair("sig.",TK_wcentury));  tok.insert(make_pair("sgl.",TK_wcentury));
  //tok.insert(make_pair("sigl.",TK_wcentury));

  tok.insert(make_pair("hundred",TK_whundred));

  tok.insert(make_pair("b.c.",TK_wacdc));  tok.insert(make_pair("a.d.",TK_wacdc));
  tok.insert(make_pair("a.m.",TK_wampm));  tok.insert(make_pair("p.m.",TK_wampm));
  tok.insert(make_pair("am.",TK_wampm));  tok.insert(make_pair("pm.",TK_wampm));
  						 
  tok.insert(make_pair("midnight",TK_wmidnight));   tok.insert(make_pair("midday",TK_wmidnight));
  tok.insert(make_pair("noon",TK_wmidnight));
  tok.insert(make_pair("morning",TK_wmorning));     tok.insert(make_pair("afternoon",TK_wmorning));     
  tok.insert(make_pair("evening",TK_wmorning));     tok.insert(make_pair("night",TK_wmorning));  
	     
  tok.insert(make_pair("m",TK_wmin));      tok.insert(make_pair("m.",TK_wmin));
  tok.insert(make_pair("min",TK_wmin));    tok.insert(make_pair("min.",TK_wmin));
  tok.insert(make_pair("minute",TK_wmin)); tok.insert(make_pair("minutes",TK_wmin));
  tok.insert(make_pair("hour",TK_wmin)); tok.insert(make_pair("hours",TK_wmin));

  tok.insert(make_pair("january",TK_month));    tok.insert(make_pair("jan",TK_shmonth));
  tok.insert(make_pair("february",TK_month));   tok.insert(make_pair("feb",TK_shmonth));
  tok.insert(make_pair("march",TK_month));      tok.insert(make_pair("mar",TK_shmonth));
  tok.insert(make_pair("april",TK_month));      tok.insert(make_pair("apr",TK_shmonth));
  tok.insert(make_pair("may",TK_month));       
  tok.insert(make_pair("june",TK_month));       tok.insert(make_pair("jun",TK_shmonth));
  tok.insert(make_pair("july",TK_month));       tok.insert(make_pair("jul",TK_shmonth));
  tok.insert(make_pair("august",TK_month));     tok.insert(make_pair("aug",TK_shmonth));
  tok.insert(make_pair("september",TK_month));  tok.insert(make_pair("sep",TK_shmonth));
  tok.insert(make_pair("october",TK_month));    tok.insert(make_pair("oct",TK_shmonth));
  tok.insert(make_pair("november",TK_month));   tok.insert(make_pair("nov",TK_shmonth));
  tok.insert(make_pair("december",TK_month));   tok.insert(make_pair("dec",TK_shmonth));
  tok.insert(make_pair("novemb",TK_shmonth));   tok.insert(make_pair("sept",TK_shmonth));
  tok.insert(make_pair("decemb",TK_shmonth));   tok.insert(make_pair("septemb",TK_shmonth)); 

  // day numbers
  tok.insert(make_pair("first",TK_daynum));       tok.insert(make_pair("second",TK_daynum));
  tok.insert(make_pair("third",TK_daynum));       tok.insert(make_pair("fourth",TK_daynum)); 
  tok.insert(make_pair("fifth",TK_daynum));       tok.insert(make_pair("sixth",TK_daynum));
  tok.insert(make_pair("seventh",TK_daynum));     tok.insert(make_pair("eighth",TK_daynum)); 
  tok.insert(make_pair("ninth",TK_daynum));       tok.insert(make_pair("tenth",TK_daynum));
  tok.insert(make_pair("eleventh",TK_daynum));   tok.insert(make_pair("twelfth",TK_daynum)); 
  tok.insert(make_pair("thirteenth",TK_daynum));  tok.insert(make_pair("fourteenth",TK_daynum));
  tok.insert(make_pair("fifteenth",TK_daynum));   tok.insert(make_pair("sixteenth",TK_daynum)); 
  tok.insert(make_pair("seventeenth",TK_daynum)); tok.insert(make_pair("eighteenth",TK_daynum));
  tok.insert(make_pair("nineteenth",TK_daynum));  tok.insert(make_pair("twentieth",TK_daynum)); 
  tok.insert(make_pair("twenty-first",TK_daynum));  tok.insert(make_pair("twenty-second",TK_daynum)); 
  tok.insert(make_pair("twenty-third",TK_daynum));  tok.insert(make_pair("twenty-fourth",TK_daynum)); 
  tok.insert(make_pair("twenty-fifth",TK_daynum));  tok.insert(make_pair("twenty-sixth",TK_daynum)); 
  tok.insert(make_pair("twenty-seventh",TK_daynum)); tok.insert(make_pair("twenty-eighth",TK_daynum)); 
  tok.insert(make_pair("twenty-ninth",TK_daynum));   
  tok.insert(make_pair("thirtieth",TK_daynum));     tok.insert(make_pair("thirty-first",TK_daynum)); 
  	     
  // initialize map with month numeric values
  nMes.insert(make_pair("january",1));    nMes.insert(make_pair("jan",1));
  nMes.insert(make_pair("february",2));   nMes.insert(make_pair("feb",2));
  nMes.insert(make_pair("march",3));      nMes.insert(make_pair("mar",3));
  nMes.insert(make_pair("april",4));      nMes.insert(make_pair("apr",4));
  nMes.insert(make_pair("may",5));       
  nMes.insert(make_pair("june",6));       nMes.insert(make_pair("jun",6));
  nMes.insert(make_pair("july",7));       nMes.insert(make_pair("jul",7));
  nMes.insert(make_pair("august",8));     nMes.insert(make_pair("aug",8));
  nMes.insert(make_pair("september",9));  nMes.insert(make_pair("sep",9));
  nMes.insert(make_pair("october",10));   nMes.insert(make_pair("oct",10));
  nMes.insert(make_pair("november",11));  nMes.insert(make_pair("nov",11));
  nMes.insert(make_pair("december",12));  nMes.insert(make_pair("dec",12));
  nMes.insert(make_pair("novemb",11));    nMes.insert(make_pair("sept",9));
  nMes.insert(make_pair("decemb",12));    nMes.insert(make_pair("septemb",9));  
      
  // initialize map with weekday values 
  nDia.insert(make_pair("sunday","G"));     nDia.insert(make_pair("monday","L"));
  nDia.insert(make_pair("tuesday","M"));    nDia.insert(make_pair("wednesday","X"));
  nDia.insert(make_pair("thursday","J"));   nDia.insert(make_pair("friday","V"));
  nDia.insert(make_pair("saturday","S"));

  // initialize map with day numeric values 
  numDay.insert(make_pair("first",1));       numDay.insert(make_pair("second",2));
  numDay.insert(make_pair("third",3));       numDay.insert(make_pair("fourth",4)); 
  numDay.insert(make_pair("fifth",5));       numDay.insert(make_pair("sixth",6));
  numDay.insert(make_pair("seventh",7));     numDay.insert(make_pair("eighth",8)); 
  numDay.insert(make_pair("ninth",9));       numDay.insert(make_pair("tenth",10));
  numDay.insert(make_pair("eleventh",11));   numDay.insert(make_pair("twelfth",12)); 
  numDay.insert(make_pair("thirteenth",13));  numDay.insert(make_pair("fourteenth",14));
  numDay.insert(make_pair("fifteenth",15));   numDay.insert(make_pair("sixteenth",16)); 
  numDay.insert(make_pair("seventeenth",17)); numDay.insert(make_pair("eighteenth",18));
  numDay.insert(make_pair("nineteenth",19));  numDay.insert(make_pair("twentieth",20)); 
  numDay.insert(make_pair("twenty-first",21));  numDay.insert(make_pair("twenty-second",22)); 
  numDay.insert(make_pair("twenty-third",23));  numDay.insert(make_pair("twenty-fourth",24)); 
  numDay.insert(make_pair("twenty-fifth",25));  numDay.insert(make_pair("twenty-sixth",26)); 
  numDay.insert(make_pair("twenty-seventh",27)); numDay.insert(make_pair("twenty-eighth",28)); 
  numDay.insert(make_pair("twenty-ninth",29));   
  numDay.insert(make_pair("thirtieth",30));     numDay.insert(make_pair("thirty-first",31)); 

  // Initialize special state attributes
  initialState=A; stopState=STOP;

  // Initialize Final state set 
  Final.insert(B);   Final.insert(I);     Final.insert(L);     Final.insert(E);
  Final.insert(P);   Final.insert(Eb);    Final.insert(Gb);    Final.insert(Gbb);  
  Final.insert(AH);  Final.insert(CH);    Final.insert(GH);    Final.insert(EH); 
  Final.insert(EHb); Final.insert(AH1);   Final.insert(GH1);   Final.insert(EH1);

  // Initialize transition table. By default, stop state
  int s,t;
  for(s=0;s<MAX_STATES;s++) for(t=0;t<MAX_TOKENS;t++) trans[s][t]=STOP;

  // State A
  trans[A][TK_weekday]=B;   trans[A][TK_wday]=D;   trans[A][TK_wmonth]=G;
  trans[A][TK_wyear]=K;     trans[A][TK_win]=J;    trans[A][TK_date]=L;     
  trans[A][TK_month]=Eb;    trans[A][TK_shmonth]=Ib;  //trans[A][TK_wcentury]=S1; 
  trans[A][TK_wpast]=A;     trans[A][TK_daynum]=Ec;
  trans[A][TK_hhmm]=AH1;    trans[A][TK_wmidnight]=GH1;  
  trans[A][TK_hour]=CH1;    trans[A][TK_hournum]=CH1; 
  trans[A][TK_min]=CH1;     trans[A][TK_minnum]=CH1; 
  trans[A][TK_whalf]=BH1;    trans[A][TK_wquarter]=BH1; 
  // State B+
  trans[B][TK_daynum]=E;      trans[B][TK_comma]=C; 
  trans[B][TK_wday]=D;    
  trans[B][TK_wmidnight]=GH;  trans[B][TK_wmorning]=GH;  
  trans[B][TK_wat]=Ha;        trans[B][TK_wabout]=Ha;    trans[B][TK_win]=FH;
  trans[B][TK_wthe]=B;        trans[B][TK_hhmm]=AH1;     trans[B][TK_date]=L;     
  // State C
  trans[C][TK_daynum]=E;      trans[C][TK_wday]=D; 
  trans[C][TK_month]=Ebb;     trans[C][TK_shmonth]=Ebb;   
  trans[C][TK_hhmm]=AH1;      trans[C][TK_date]=L;     
  // State D
  trans[D][TK_daynum]=E;
  trans[D][TK_hhmm]=AH1;      trans[D][TK_date]=L;     
  // State E
  trans[E][TK_wof]=F;  
  trans[E][TK_month]=I;       trans[E][TK_shmonth]=I;
  trans[E][TK_monthnum]=Ic;
  trans[E][TK_wmidnight]=GH;  trans[E][TK_wmorning]=GH;  
  trans[E][TK_wat]=Ha;        trans[E][TK_wabout]=Ha;    trans[E][TK_win]=FH;
  // State Eb+
  trans[Eb][TK_wthe]=Fb;  
  trans[Eb][TK_wof]=J;         trans[Eb][TK_comma]=J; 
  trans[Eb][TK_kyearnum]=L;    trans[Eb][TK_centnum]=Jb;
  trans[Eb][TK_wmidnight]=GH;  trans[Eb][TK_wmorning]=GH;  
  trans[Eb][TK_wat]=Ha;        trans[Eb][TK_wabout]=Ha;     trans[Eb][TK_win]=FH;
  trans[Eb][TK_daynum]=Gbb;
  // State Ebb
  trans[Ebb][TK_wthe]=Fb; 
  trans[Ebb][TK_daynum]=Gb;
  // State Ec
  trans[Ec][TK_wof]=F;  
  trans[Ec][TK_month]=I;  trans[Ec][TK_shmonth]=I; trans[Ec][TK_monthnum]=Ic;
  // State F
  trans[F][TK_wpast]=F;  trans[F][TK_wthe]=F;
  trans[F][TK_month]=I;   trans[F][TK_shmonth]=I; 
  trans[F][TK_wmonth]=G; trans[F][TK_monthnum]=Ic;
  // State Fb
  trans[Fb][TK_daynum]=Gb; 
  // State G
  trans[G][TK_wof]=H;   trans[G][TK_monthnum]=Ic;   
  trans[G][TK_month]=I; trans[G][TK_shmonth]=I;
  // State Gb+
  trans[Gb][TK_comma]=J;       trans[Gb][TK_wof]=J;
  trans[Gb][TK_kyearnum]=L;    trans[Gb][TK_centnum]=Jb;
  trans[Gb][TK_wmidnight]=GH;  trans[Gb][TK_wmorning]=GH;  
  trans[Gb][TK_wat]=Ha;        trans[Gb][TK_wabout]=Ha;    trans[Gb][TK_win]=FH;
  // State Gbb
  trans[Gbb][TK_comma]=J;       trans[Gbb][TK_wof]=J;
  trans[Gbb][TK_kyearnum]=L;    trans[Gbb][TK_centnum]=Jb;
  trans[Gbb][TK_wmidnight]=GH;  trans[Gbb][TK_wmorning]=GH;  
  trans[Gbb][TK_wat]=Ha;        trans[Gbb][TK_wabout]=Ha;    trans[Gbb][TK_win]=FH;
  // State H
  trans[H][TK_month]=I; trans[H][TK_shmonth]=I; trans[H][TK_monthnum]=Ic;   
  // State I+
  trans[I][TK_wof]=J;         trans[I][TK_comma]=J;
  trans[I][TK_kyearnum]=L;    trans[I][TK_centnum]=Jb;
  trans[I][TK_wmidnight]=GH;  trans[I][TK_wmorning]=GH;  
  trans[I][TK_wat]=Ha;        trans[I][TK_wabout]=Ha;    trans[I][TK_win]=FH;
  // State Ib
  trans[Ib][TK_wof]=J;   trans[Ib][TK_comma]=J; 
  // State Ic
  trans[Ic][TK_wof]=J;         trans[Ic][TK_comma]=J;
  trans[Ic][TK_kyearnum]=L;    trans[Ic][TK_centnum]=Jb;
  trans[Ic][TK_wmidnight]=GH;  trans[Ic][TK_wmorning]=GH;  
  trans[Ic][TK_wat]=Ha;        trans[Ic][TK_wabout]=Ha;     trans[Ic][TK_win]=FH;

  // State J
  trans[J][TK_wpast]=J;     trans[J][TK_wthe]=J;
  trans[J][TK_kyearnum]=L;  trans[J][TK_centnum]=Jb;  trans[J][TK_wyear]=K;
  // State Jb
  trans[Jb][TK_yearnum]=L;     trans[Jb][TK_wand]=M;     trans[Jb][TK_whundred]=N;
  trans[Jb][TK_comma]=J;       trans[Jb][TK_wof]=J;
  trans[Jb][TK_centnum]=Jb;
  trans[Jb][TK_wmidnight]=GH;  trans[Jb][TK_wmorning]=GH;    
  trans[Jb][TK_wat]=Ha;        trans[Jb][TK_wabout]=Ha;    trans[Jb][TK_win]=FH;
  trans[Jb][TK_kyearnum]=L;  
  trans[Jb][TK_other]=aux;    trans[Jb][TK_dot]=aux;  
  // State K
  trans[K][TK_centnum]=Jb; trans[K][TK_kyearnum]=L;
  // State L+
  trans[L][TK_wacdc]=P;
  trans[L][TK_wmidnight]=GH;  trans[L][TK_wmorning]=GH;  
  trans[L][TK_wat]=Ha;        trans[L][TK_wabout]=Ha;   trans[L][TK_win]=FH;
  // State M
  trans[M][TK_wand]=L;  trans[M][TK_yearnum]=L;
  // State N
  trans[N][TK_wand]=O;
  // State O
  trans[O][TK_yearnum]=L;
  // State P+
  trans[P][TK_wmidnight]=GH;  trans[P][TK_wmorning]=GH;  
  trans[P][TK_wat]=Ha;        trans[P][TK_wabout]=Ha;    trans[P][TK_win]=FH;

  // State S1
  //trans[S1][TK_roman]=S2; trans[S1][TK_dot]=S1;
  // State S2+
  //trans[S2][TK_wacdc]=S2;

  // State Ha
  trans[Ha][TK_wa]=Ha; trans[Ha][TK_wabout]=Ha; trans[Ha][TK_wthe]=Ha; 
  trans[Ha][TK_whalf]=BH; trans[Ha][TK_wquarter]=BH;  
  trans[Ha][TK_hour]=CH;  trans[Ha][TK_minnum]=CH; trans[Ha][TK_min]=CH;
  trans[Ha][TK_hhmm]=AH; 
  trans[Ha][TK_wmidnight]=GH; trans[Ha][TK_wmorning]=GH;  
  // State AH+
  trans[AH][TK_wampm]=GH; 
  trans[AH][TK_wof]=FH; trans[AH][TK_wat]=FH; trans[AH][TK_win]=FH;   
  // State BH
  trans[BH][TK_wpast]=DH; trans[BH][TK_wto]=DH; 
  // State CH
  trans[CH][TK_wpast]=DH;     trans[CH][TK_wto]=DH;
  trans[CH][TK_minnum]=EHb;   trans[CH][TK_min]=EHb; 
  trans[CH][TK_woclock]=AH; 
  trans[CH][TK_wof]=FH;       trans[CH][TK_wat]=FH;   trans[CH][TK_win]=FH;
  trans[CH][TK_wmin]=CH;      trans[CH][TK_wand]=CH; 
  trans[CH][TK_other]=aux;    trans[CH][TK_dot]=aux;  
  // State DH
  trans[DH][TK_hour]=EH;  trans[DH][TK_hournum]=EH;
  // State EH+
  trans[EH][TK_wampm]=GH; 
  trans[EH][TK_wof]=FH;  trans[EH][TK_wat]=FH;  trans[EH][TK_win]=FH;  
  trans[EH][TK_wmidnight]=GH; trans[EH][TK_wmorning]=GH;  
  trans[EH][TK_wmin]=EH; 
  // State EHb+
  trans[EHb][TK_wmin]=EHb; 
  trans[EHb][TK_wampm]=GH; trans[EHb][TK_wmidnight]=GH; trans[EHb][TK_wmorning]=GH;  
  trans[EHb][TK_wat]=FH; trans[EHb][TK_wof]=FH; trans[EHb][TK_win]=FH;   
  // State FH
  trans[FH][TK_wthe]=FH; trans[FH][TK_wmorning]=GH; trans[FH][TK_wmidnight]=GH;
  // State GH+
  trans[GH][TK_wat]=Ha; trans[GH][TK_wabout]=Ha;

  // State AH1
  trans[AH1][TK_wof]=A;
  trans[AH1][TK_wampm]=GH1; 
  trans[AH1][TK_wat]=FH1; trans[AH1][TK_win]=FH1;
  // State BH1
  trans[BH1][TK_wpast]=DH1; trans[BH1][TK_wto]=DH1;
  // State CH1
  trans[CH1][TK_wpast]=DH1b; trans[CH1][TK_wto]=DH1b;
  trans[CH1][TK_minnum]=EH1b;  trans[CH1][TK_min]=EH1b;
  trans[CH1][TK_woclock]=AH1;
  trans[CH1][TK_wat]=FH1;
  trans[CH1][TK_wmin]=CH1;  trans[CH1][TK_wand]=CH1;
  trans[CH1][TK_wof]=F;
  trans[CH1][TK_month]=I;  trans[CH1][TK_shmonth]=I;
  // State DH1
  trans[DH1][TK_hournum]=EH1;  trans[DH1][TK_hour]=EH1;
  // State DH1b
  trans[DH1b][TK_hournum]=EH1c;  trans[DH1b][TK_hour]=EH1c;
  // State EH1+
  trans[EH1][TK_wampm]=GH1;
  trans[EH1][TK_wof]=A;  trans[EH1][TK_wat]=FH1; trans[EH1][TK_win]=FH1;
  // State EH1b
  trans[EH1b][TK_wmin]=EH1b;  trans[EH1b][TK_wampm]=GH1;
  trans[EH1b][TK_wof]=A;  trans[EH1b][TK_wat]=FH1; trans[EH1b][TK_win]=FH1;
  trans[EH1b][TK_wmin]=EH1b; 
  // State EH1c
  trans[EH1c][TK_wampm]=GH1;
  trans[EH1c][TK_wof]=A;  trans[EH1c][TK_wat]=FH1; trans[EH1c][TK_win]=FH1;
  // State FH1
  trans[FH1][TK_wthe]=FH1;      
  trans[FH1][TK_wmorning]=GH1; trans[FH1][TK_wmidnight]=GH1;
   // State GH1+
  trans[GH1][TK_wof]=A;  trans[GH1][TK_win]=A;

  TRACE(3,"analyzer succesfully created");
}


//-- Implementation of virtual functions from class automat --//

///////////////////////////////////////////////////////////////
///  Compute the right token code for word j from given state.
///////////////////////////////////////////////////////////////
int dates_en::ComputeToken(int state, sentence::iterator &j, sentence &se)
{
  string form,formU;
  int token,value;
  map<string,int>::iterator im;

  formU = j->get_form();
  form = util::lowercase(formU);

  token = TK_other;
  im = tok.find(form);
  if (im!=tok.end()) {
    token = (*im).second;
  }
  
  TRACE(3,"Next word form is: ["+form+"] token="+util::int2string(token));     

  // if the token was in the table, we're done
  if (token != TK_other) return (token);

  // Token not found in translation table, let's have a closer look.

  // look if any lemma is an ordinal adjective
  bool ordinal=false;
  word::iterator a;
  for (a=j->begin(); a!=j->end() && !ordinal; a++) {
    ordinal = (a->get_parole()=="JJ" && util::isnumber(a->get_lemma()) &&
	       util::string2int(a->get_lemma())>0 && util::string2int(a->get_lemma())<=31);
    TRACE(3, "studying possible analysis of word ["+form+"]:"+a->get_parole()+" "+a->get_lemma());
  }
  // if any, consider this a fraction word (fifth, eleventh, ...) 
  if (ordinal) {
    a--;
    j->unselect_all_analysis();
    j->select_analysis(a);
    token=TK_daynum;
    value=util::string2int(j->get_lemma());
    TRACE(3,"Ordinal value of form: "+util::int2string(value)+". New token: "+util::int2string(token));
    return (token);
  } 

  // check to see if it is a number
  value=0;
  if (j->get_n_analysis() && j->get_parole()=="Z") {
    token = TK_number;
    value = util::string2int(j->get_lemma());
    TRACE(3,"Numerical value of form: "+util::int2string(value));
  }

  // determine how to interpret that number, or if not number,
  // check for specific regexps.
  switch (state) {
  // --------------------------------
  case A: 
    TRACE(3,"In state A");     
    if (token==TK_number && value>=0 && value<=59) {
        token = TK_minnum; // it can also be an hournum
    }
    else if (RE_Date.Search(form)) {
      TRACE(3,"Match DATE regex. "+RE_Date.Match(0));
      token = TK_date;
    }
    else if (RE_Time1.Search(form)) {
      if(RE_Time1.Match(2)!=""){
	TRACE(3,"Match TIME1 regex (hour+min)");
	token = TK_hhmm;
      }
      else { 
	TRACE(3,"Partial match TIME1 regex (hour)");
	token = TK_hour;
      }	 
    }
    else if (RE_Time2.Search(form)) 
      token = TK_min;
	
    break;
  // --------------------------------
  case B:
  case C:
  case D:
    TRACE(3,"In state B/C/D");
    if (token==TK_number && value>=0 && value<=31){
      token = TK_daynum;
    }
    else if (RE_Date.Search(form)) {
      TRACE(3,"Match DATE regex. "+RE_Date.Match(0));
      token = TK_date;
    }
    else if (RE_Time1.Search(form)) {
      if(RE_Time1.Match(2)!=""){
	TRACE(3,"Match TIME1 regex (hour+min)");
	token = TK_hhmm;
      }
      else { 
	TRACE(3,"Partial match TIME1 regex (hour)");
	token = TK_hour;
      }	 
    }
    else if (RE_Time2.Search(form)) 
      token = TK_min;

    break;
  // --------------------------------
  case Ebb:
  case Fb:
    TRACE(3,"In state B/C/D");
    if (token==TK_number && value>=0 && value<=31){
      token = TK_daynum;
    }
    break;
  // --------------------------------    
  case E:
  case Ec:
  case F:
  case G:
  case H:
    TRACE(3,"In state E/Ec/F/G/H");     
    if (token==TK_number && value>=1 && value<=12) 
      token = TK_monthnum;
    break;
	
  // --------------------------------
  case Eb:
    TRACE(3,"In state Eb");     
    if (token==TK_number && (value>=10 && value<=99))  // years til 9999, could also be a daynumber
      token = TK_centnum;	
    else if (token==TK_number && value>=100)
      token = TK_kyearnum;
    else if (token==TK_number && value<10 && value>0)
      token = TK_daynum;
    break;	

  // -----------------------------------
  case Gb:
  case Gbb:
  case K:
  case J:
  case I:
  case Ic:
    TRACE(3,"In state Gb/Gbb/K/J/I");     
    if (token==TK_number && (value>=10 && value<=99))  // years til 9999
      token = TK_centnum;	
    else if (token==TK_number && value>=100)
      token = TK_kyearnum;

    break;	

  // --------------------------------
  case Jb:
    TRACE(3,"In state Jb");     
    if (token==TK_number && (value>=1 && value<=99))  // decada
      token = TK_yearnum;	
    else if (token==TK_number && value>=100)
      token = TK_kyearnum;
    break;	
	
  // --------------------------------
  case M:
    TRACE(3,"In state M");     
    if (token==TK_number && value>=1 && value<=99)  // unit
      token = TK_yearnum;	

    break;	

 // --------------------------------
  case O:
    TRACE(3,"In state O");     
    if (token==TK_number && (value>=1 && value<=99))  // decada
      token = TK_yearnum;	
	
    break;	

  // --------------------------------
  case Ha:
    TRACE(3,"In state Ha");     
    if (token==TK_number && value>=0 && value<=60) {
      token = TK_minnum;
    }
    else if (RE_Time1.Search(form)) {
      if(RE_Time1.Match(2)!=""){
	TRACE(3,"Match TIME1 regex (hour+min)");
	token = TK_hhmm;
      }
      else{
	TRACE(3,"Partial match TIME1 regex (hour)");
	token = TK_hour;
      }	      
    }
    break;
  // --------------------------------
  case CH:
  case CH1:
    TRACE(3,"In state CH/CH1");
    if (token==TK_number && value>=0 && value<=60) {
      token=TK_minnum;
    }
    else if (RE_Time2.Search(form)) {
      TRACE(3,"Match TIME2 regex (minutes)");
      token = TK_min;
    }    
    break;

    // -------------------------------   
  case DH:
  case DH1:
  case DH1b:
    TRACE(3,"In state DH/DH1/DH1b");
    if (token==TK_number && value>=0 && value<=24) {
      token = TK_hournum;
    }
    else if (RE_Time1.Search(form)) {
      if(RE_Time1.Match(2)!=""){
        TRACE(3,"Match TIME1 regex (hour+min)");
        token = TK_hhmm;
      }
      else{
        TRACE(3,"Partial match TIME1 regex (hour)");
        token = TK_hour;
      }
    }
    break;
  // --------------------------------  
  default: break;
  }
  
  TRACE(3,"Leaving state "+util::int2string(state)+" with token "+util::int2string(token)); 
  return (token);
}



///////////////////////////////////////////////////////////////
///   Reset acumulators used by state actions:
///   day, year, month, hour, minute, etc.
///////////////////////////////////////////////////////////////

void dates_en::ResetActions() 
{
  century="??";
  weekday="??";
  day="??";
  month="??";
  year="??";
  hour="??";
  minute="??";
  meridian="??";
  temp = -1;
  sign=0;
  inGbb=false;
  daytemp=-1;
}


///////////////////////////////////////////////////////////////
///  Perform necessary actions in "state" reached from state 
///  "origin" via word j interpreted as code "token":
///  Basically, when reaching a state with an informative 
///  token (day, year, month, etc) store that part of the date.
///////////////////////////////////////////////////////////////

void dates_en::StateActions(int origin, int state, int token, sentence::const_iterator j)
{
  string form;
  int value;
  map<string,int>::iterator im;

  form = util::lowercase(j->get_form());
  TRACE(3,"Reaching state "+util::int2string(state)+" with token "+util::int2string(token)+" for word ["+form+"]");

  // get token numerical value, if any
  value=0;
  if ((token==TK_number || token==TK_daynum || token==TK_monthnum ||
       token==TK_hournum || token==TK_minnum || token==TK_kyearnum || token==TK_centnum || token==TK_yearnum) &&
      j->get_n_analysis() && j->get_parole()=="Z") {
    value = util::string2int(j->get_lemma());
  }
  else if (token==TK_daynum){
    // if its in the nDay map get value from there
    map<string,int>::iterator k=numDay.find(form);
    if(k!=numDay.end())
      value=numDay[form];
    else
      value = util::string2int(j->get_lemma());
  }

  // State actions
  switch (state) {
  // ---------------------------------
  case B:
    TRACE(3,"Actions for state B");
    if (token==TK_weekday)
      weekday=nDia[form];
    break;
  // ---------------------------------
  case E:
    TRACE(3,"Actions for state E");
    if (token==TK_daynum) {
      day=util::int2string(value);
    }
    break;
  // ---------------------------------
  case Ec:
  case Gb:
    TRACE(3,"Actions for state Ec/Gb");
    if (token==TK_daynum) {
      day=util::int2string(value);
    }
    break;
  // ---------------------------------
  case Gbb:
    TRACE(3,"Actions for state Gbb");
    if (token==TK_daynum) {
      daytemp=value;
      inGbb=true;
      TRACE(3, "in Gbb "+util::int2string(inGbb)+" daytemp "+util::int2string(daytemp));
      day=util::int2string(value);
    }
    break;
  // ---------------------------------
  case Eb:
  case Ebb:
    TRACE(3,"Actions for state Eb/Ebb");
    if (token==TK_month || token==TK_shmonth)
      month = util::int2string(nMes[form]);
    break;
  // ---------------------------------
  case F:
    TRACE(3,"Actions for state F");
    if (origin==CH1) { //what we found in CH1 was a day
      day=minute;
      minute="??";
    }
    else if (origin==E)
      day= util::int2string(temp);
    break;
  // ---------------------------------   
  case I:
  case Ib:
  case Ic:
    TRACE(3,"Actions for state I/Ib/Ic");
    if (origin==CH1) { //what we found in CH1 was a day
      day=util::int2string(temp);
    }
    else if (origin==E)
      day= util::int2string(temp);

    if (token==TK_monthnum)
      month = j->get_lemma();
    else if (token==TK_month || token==TK_shmonth)
      month = util::int2string(nMes[form]);
    break; 
  // ---------------------------------
  case Jb:
    TRACE(3,"Actions for state Jb");
    if (token==TK_centnum) {
      // received first part of a year (century number between 10 and 99)
      //  or a day number
	temp=value;
    }
    else 
      temp=0;
    break;
  // ---------------------------------
  case J:
    TRACE(3,"Actions for state Jb");
    if (origin==Jb) {
      // the centnum received in Jb (temp) was the daynumber
      daytemp=temp;
      inGbb=true;
      TRACE(3, "in Gbb "+util::int2string(inGbb)+" daytemp "+util::int2string(daytemp));
    }
    break;
  // ---------------------------------
  case L:
    TRACE(3,"Actions for state L");
    if (token==TK_yearnum) {
      if (origin==Jb || origin==M || origin==O) {
	// received second part of the year (decade number between 1 and 99)
	//  year is centnum*100+value
	temp=temp*100+value;
	year=util::int2string(temp);
      }
    }
    else if (token==TK_kyearnum) { 
      // received year 2000 or bigger (also accepted 100-1100)
      year=util::int2string(value);
      // temp is the day seen from Gb to Jb
      if (origin==Gb || origin==Gbb || origin==Jb)
	day=util::int2string(temp);
    }
    //if we passed trough Gbb daytemp is the day
    if (inGbb)
      day=util::int2string(daytemp);

    if (token==TK_date) {
      // it has to match since the token is TK_date
      RE_Date.Search(form);
      // day number
      day=RE_Date.Match(1);
      // to unify notation (01 -> 1)
      day=util::int2string(util::string2int(day));
      // month number (translating month name if necessary) 
      im=nMes.find(util::lowercase(RE_Date.Match(2)));
      if(im!=nMes.end())
        month = util::int2string((*im).second);
      else {
        month = RE_Date.Match(2);
        // to unify notation (01 -> 1) 
        month=util::int2string(util::string2int(month));
      }
      // year number
      year = RE_Date.Match(3);
      // if year has only two digits, it can be 19xx or 20xx 
      if (util::string2int(year)>=0 && util::string2int(year)<=20) {year="20"+year;}
      else if (util::string2int(year)>=50 && util::string2int(year)<=99) {year="19"+year;}
    }
    break;
  // ---------------------------------
  case P:
    TRACE(3,"Actions for state P");
    if (form=="b.c."){
      //if(century!="") century="-"+century;   
      if(year!="??") year="-"+year;
    }
    break;

  // ---------------------------------
  // time
  // ---------------------------------
  case BH:
  case BH1:
    TRACE(3,"Actions for state BH/BH1");
    
    if (token==TK_whalf) 
      minute=util::int2string(30);
    else if  (token==TK_wquarter)
      minute=util::int2string(15);

    break;
  // ---------------------------------
  case CH:
  case CH1:
    TRACE(3,"Actions for state CH/CH1"); 
    if (token==TK_minnum) {
      temp=value; // can be a minute, hour or day
    }
    else if (token==TK_min) {
      // it has to match, since the token is TK_min
      RE_Time2.Search(form);
      minute=RE_Time2.Match(1);
      temp=util::string2int(minute); // to be used in DH, DH1 or DH1b if it is 15min *to* 12.
    }
    else if (token==TK_hour) {
      // it has to match, since the token is TK_hour
      RE_Time1.Search(form);
      hour=RE_Time1.Match(1);
    }
    break;
  // ---------------------------------
  case DH:
  case DH1:
  case DH1b:
    TRACE(3,"Actions for state DH/DH1/DH1b");

    if (origin==CH1 || origin==CH) //what we stored in CH1 was a minute
      minute=util::int2string(temp);

    if (token==TK_wpast)
      sign=1;
    else if (token==TK_wto)
      sign=-1;

    break;

  // ---------------------------------
  case EH:
  case EH1:
  case EH1c:
    TRACE(3,"Actions for state EH/EH1/EH1c");
    if (token==TK_hournum){
      if (value==24 || value==0) {
	meridian="am";
	hour="0";
      }
      else if (value>12) { 
	meridian="pm";
	hour=util::int2string(value-12);
      }
      else hour=util::int2string(value);
    }
    else if (token==TK_hour) {
      // it has to match, since the token is TK_hour
      RE_Time1.Search(form);
      hour=RE_Time1.Match(1);
      
    }

    // correct hour if necessary (quarter to two is 1:45) 
    //  and minute (20 to --> min=40)
    if (sign==-1) {
      if(hour=="1") hour="13"; // una menos veinte = 12:40
      else if(hour=="0") hour="12";
      hour = util::int2string(util::string2int(hour)-1);
    
      // set minute
      minute = util::int2string(60-util::string2int(minute));
    }
    break;  
  
  // ---------------------------------
  case EHb:
  case EH1b:
    TRACE(3,"Actions for state EHb/EH1b");

    // if it comes from CH, what we stored in temp was an hour
    if ((origin==CH || origin==CH1) && temp<=24 && temp>=0) {
      hour=util::int2string(temp);
      if (temp==24) {
	meridian="am";
	hour="0";
      }
      else if (temp>12) { 
	meridian="pm";
	hour=util::int2string(temp-12);
      }
      minute ="00";
    }

    if (token==TK_minnum) {
      minute= util::int2string(value); }
    else if (token==TK_min) {
      // it has to match, since the token is TK_min
      RE_Time2.Search(form);
      minute=RE_Time2.Match(1);
    }

    break;

  // ---------------------------------
  case AH:
  case AH1:
    TRACE(3,"Actions for state AH/AH1");

    // if it comes from CH, what we stored in temp was an hour
    if ((origin==CH || origin==CH1 && temp<=24) && temp>=0) {
      hour=util::int2string(temp);
      if (temp==24) {
	meridian="am";
	hour="0";
      }
      else if (temp>12) { 
	meridian="pm";
	hour=util::int2string(temp-12);
      }
    }

    if (token==TK_woclock){
      minute="00";}
    else if (token==TK_hhmm) {
      // it has to match, since the token is TK_hhmm
      RE_Time1.Search(form);
      hour=RE_Time1.Match(1);
      minute=RE_Time1.Match(2);
    }

    break;  

  // ---------------------------------
  case GH:
  case GH1:
    TRACE(3,"Actions for state GH/GH1");
    if (token==TK_wampm) {
      if (form=="a.m.")
	meridian="am";
      else if (form=="p.m.") 
	meridian="pm";
    }
    else if (token==TK_wmorning) {
      if (form=="morning") 
	meridian="am";
      else if (form=="afternoon" || form=="evening" || form=="night") 
	meridian="pm";
    }
    else if (token==TK_wmidnight) {
      if (hour=="??") {
	if (form=="midnight") {
	  meridian="am";
	  hour="00"; 
	  minute="00";
	}
	else if (form=="midday" || form=="noon") {
	  meridian="pm";
	  hour="12"; 
	  minute="00";
	}
      }
      else {
	if (form=="midnight") 
          meridian="am";
	else if (form=="midday" || form=="noon")
	  meridian="pm";
      }
    }

    break;  

  // ---------------------------------
  case FH:
    TRACE(3,"Actions for state FH");
    // if it comes from CH, what we stored in temp was an hour
    if (origin==CH && temp<=24  && temp>=0) {
      hour=util::int2string(temp);
      minute="00";
      if (temp==24) {
	meridian="am";
	hour="0";
      }
      else if (temp>12) { 
	meridian="pm";
	hour=util::int2string(temp-12);
      }
    }
    break;  

  // ---------------------------------
  case aux:
    TRACE(3,"Actions for state aux");
    // if it comes from CH, what we stored in temp was an hour
    // and this is the end of the date
    if (origin==CH && temp<=24  && temp>=0) {
      hour=util::int2string(temp);
      minute="00";
      if (temp==24) {
	meridian="am";
	hour="0";
      }
      else if (temp>12) { 
	meridian="pm";
	hour=util::int2string(temp-12);
      }
    }

    break;  
    

  // ---------------------------------
  default: break;
  }

  TRACE(3,"State actions finished. ["+weekday+":"+day+":"+month+":"+year+":"+hour+":"+minute+":"+meridian+"]");
}
///////////////////////////////////////////////////////////////
///   Set the appropriate lemma and parole for the 
///   new multiword.
///////////////////////////////////////////////////////////////

void dates_en::SetMultiwordAnalysis(sentence::iterator i, int fstate) {
  list<analysis> la;
  string lemma;

  // Setting the analysis for the date     
  if(century!="??")
    lemma=string("[s:"+century+"]");
  else
    lemma=string("["+weekday+":"+day+"/"+month+"/"+year+":"+hour+"."+minute+":"+meridian+"]");

  la.push_back(analysis(lemma,"W"));
  i->set_analysis(la);
  TRACE(3,"Analysis set to: "+lemma+" W");
}

// undef codes to prepare for next module definition
#undef A     
#undef B   
#undef C      
#undef D  
#undef E   
#undef Eb   
#undef Ebb 
#undef Ec  
#undef F   
#undef Fb   
#undef G   
#undef Gb  
#undef Gbb
#undef H   
#undef I   
#undef Ib  
#undef Ic
#undef J   
#undef Jb  
#undef K   
#undef L   
#undef M   
#undef N   
#undef O   
#undef P   
#undef Ha 
#undef AH 
#undef BH 
#undef CH 
#undef DH 
#undef EH 
#undef EHb
#undef FH 
#undef GH 
#undef AH1
#undef BH1
#undef CH1 
#undef DH1 
#undef DH1b 
#undef EH1 
#undef EH1b 
#undef EH1c
#undef FH1  
#undef GH1 
#undef aux
#undef STOP
#undef TK_weekday  
#undef TK_daynum   
#undef TK_month    
#undef TK_shmonth  
#undef TK_monthnum 
#undef TK_number   
#undef TK_comma    
#undef TK_dot      
#undef TK_colon    
#undef TK_wday     
#undef TK_wmonth   
#undef TK_wyear    
#undef TK_wpast    
#undef TK_wcentury 
#undef TK_roman    
#undef TK_wacdc    
#undef TK_wampm    
#undef TK_hournum  
#undef TK_minnum   
#undef TK_wquarter 
#undef TK_whalf    
#undef TK_wto      
#undef TK_wmorning 
#undef TK_wmidnight
#undef TK_wat      
#undef TK_wof      
#undef TK_win      
#undef TK_wa       
#undef TK_wabout   
#undef TK_wthe     
#undef TK_wand     
#undef TK_woclock  
#undef TK_wmin     
#undef TK_hour     
#undef TK_hhmm      
#undef TK_min      
#undef TK_date     
#undef TK_other    
#undef TK_kyearnum 
#undef TK_centnum 
#undef TK_yearnum    
#undef TK_whundred  

