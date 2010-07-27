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

////////////////////////////////////////////////////////////////
//   
//   Convert sense dictionary from FreeLing format to UKB format.
//
//   Data read from stdin, results written to stdout
//
////////////////////////////////////////////////////////////////

#include <iostream>
#include <string>
#include <map>
#include <vector>

#include "fries/util.h"

using namespace std;


int main() {

  string s, pos, syn;	
  int nlin;
  size_t i;
  vector<string> line_elem;
  vector<string> entry_elem;
  map<string, list<string> > m;
  map<string, list<string> >::iterator im;
  
  //reading input dictionary
  for (nlin = 0; getline(cin, s); nlin++) {
    if (s[0]=='W') {
      
      line_elem = util::string2vector(s, " ");
      entry_elem = util::string2vector(line_elem[0], ":");
      
      pos = util::lowercase(entry_elem[2]);
      if (pos == "j") pos = "a";
      
      list<string> lsyn;
      for (i = 1; i < line_elem.size(); i++) {
	syn = line_elem[i] + "-" + pos;
	lsyn.push_back(syn);
      }
      
      im = m.find(entry_elem[1]);
      if (im != m.end()) {
	(im->second).insert((im->second).begin(), lsyn.begin(), lsyn.end());
      } else {
	m.insert(make_pair(entry_elem[1], lsyn));
      }
    }
  }
  
  //writing output dictionary
  im = m.begin();
  while (im != m.end()) {
    cout << (im->first);
    for (list<string>::iterator lsi = (im->second).begin(); lsi != (im->second).end(); lsi++) {
      cout << " " << (*lsi) << ":0";
    }
    cout << endl;
    
    im++;
  }
}

