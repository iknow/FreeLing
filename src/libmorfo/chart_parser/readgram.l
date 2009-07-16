/*
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
*/
/***
*   This file is used to generate readgram.cc with the 'flex' scanner
*  generator. The FreeLing building procedure will use the readgram.cc
*  file, not this one, in order not to force users to install flex.
* 
*   This file is included only in case you want to modify the program,
*  you know flex, and you know what you're doing. 
*  Take into account that this file depends on "tokens.h" and "FlexLexer.h",
*  both in src/include/freeling
*
*   If you don't intend to modify the grammar loader, you need
*  neither to look at this file, nor to have flex installed
*
*   If you do, the readgram.cc file was generated with the 
*  following commands:
*
*     flex -+ -PGram_ -B -oreadgram.cc readgram.l
*     cat readgram.cc | sed 's%<FlexLexer.h>%"freeling/FlexLexer.h"%g' >pp;
*     mv pp readgram.cc
***/

%{
#include <stdio.h>
#include "freeling/tokens.h"
using namespace std;
%}

%option yylineno
%option noyywrap

%%
[ \t\n\r]+                   ;
"%".*                       {return(COMMENT);}
"==>"                       {return(ARROW);}
"("[a-zביםףתאטלעשחסןü_'\-\·]+")" {return(FORM);}
"<"[a-zביםףתאטלעשחסןü_'\-\·]+">" {return(LEMMA);}
"("\"[a-zA-Z_\-\./]+\"")" {return(FILENAME);}
"<"\"[a-zA-Z_\-\./]+\"">" {return(FILENAME);}
[A-Za-z][\-A-Za-z0-9]*[*]?  {return(CATEGORY); }
"@PRIOR"                    {return(PRIOR); }
"@START"                    {return(START); }
"@HIDDEN"                   {return(HIDDEN); }
"@FLAT"                     {return(FLAT); }
"@NOTOP"                    {return(NOTOP); }
"@ONLYTOP"                  {return(ONLYTOP); }
"|"                         {return(BAR);}
"."                         {return(DOT);}
","                         {return(COMMA);}
"+"                         {return(HEAD);}
.                           {return(ERROR);}

%%
