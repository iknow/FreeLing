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

#include "freeling/maco_options.h"
#include "freeling/traces.h"

using namespace std;

#define MOD_TRACENAME "MACO_OPTIONS"
#define MOD_TRACECODE OPTIONS_TRACE

///////////////////////////////////////////////////////////////
///  Create an options set for morpho analyzer. 
///  Initialize with default values.
///////////////////////////////////////////////////////////////

maco_options::maco_options(const std::string &lg) {

  Lang=lg;
  SuffixAnalysis=false;   MultiwordsDetection=false;
  NumbersDetection=false; PunctuationDetection=false;
  DatesDetection=false;   QuantitiesDetection=false;
  DictionarySearch=false; ProbabilityAssignment=false;
  NERecognition=false;    

  Decimal=",";  Thousand=".";

  LocutionsFile="";   QuantitiesFile="";    SuffixFile=""; 
  ProbabilityFile=""; DictionaryFile=""; 
  NPdataFile="";      PunctuationFile="";

  ProbabilityThreshold=0.001;
}

void maco_options::set_active_modules(bool suf, bool mw, bool num, bool pun, bool dat, 
                                      bool qt, bool dic, bool prb, bool ner) {
  SuffixAnalysis=suf;    MultiwordsDetection=mw;
  NumbersDetection=num;  PunctuationDetection=pun;
  DatesDetection=dat;    QuantitiesDetection=qt;
  DictionarySearch=dic;  ProbabilityAssignment=prb;
  NERecognition=ner;  
}

void maco_options::set_nummerical_points(const std::string &dec,const std::string &tho) {
  Decimal=dec;  
  Thousand=tho;
}

void maco_options::set_data_files(const std::string &loc,const std::string &qty,const std::string &suf,
                                  const std::string &prb,const std::string &dic,const std::string &nps,
                                  const std::string &pun) {
  LocutionsFile=loc;    QuantitiesFile=qty;    SuffixFile=suf; 
  ProbabilityFile=prb;  DictionaryFile=dic;  NPdataFile=nps;
  PunctuationFile=pun;  
}

void maco_options::set_threshold(double t) {
  ProbabilityThreshold=t;
}
