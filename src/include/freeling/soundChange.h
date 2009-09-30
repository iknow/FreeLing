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

#ifndef _SOUNDCHANGE
#define _SOUNDCHANGE

#include <string>
#include <map>
#include <vector>

using namespace std;



////////////////////////////////////////////////////////////////
/// This class is a sound change applier. With it, you can construct
/// objects which will generate consistent sound changes.
////////////////////////////////////////////////////////////////

class soundChange {

   private:
      
	/// variables and his valor
	map <string, string> vars;
	/// rules
	vector <string> rules;
	/// what to seacrh
	vector <string> from;
	/// what is the replace
	vector <string> to;
	/// what are the conditions to replace
	vector <string> env;

	/// compile the rules
	void compile_rules();
	/// compile de variables
	void compile_vars();
	/// find in a text a string and replace it with another string under certain conditions
	string find_and_replace(string , string , string , string );
	/// check that the conditions for the sound change are true
	bool check_cond(string, string, int, string );
	/// Calculate the location of the _ in a pattern
	int calculatePosition(string);
	

   public:
      /// Constructor with vars and rules
      soundChange(map<string, string>, vector <string>);
      /// Destructor
      ~soundChange();

	/// Returns the sound changed for the word
	string change(string);



};



#endif
 
