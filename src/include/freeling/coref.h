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

#ifndef COREF_H
#define COREF_H

#include <set>

#include "fries.h"
#include "omlet.h"
#include "coref_fex.h"

////////////////////////////////////////////////////////////////
///  The class coref implements a ML-based coreference classificator
////////////////////////////////////////////////////////////////

class coref {
	private:
		/// feature extractor
		coref_fex *extractor;
		/// adaboost classifier
		adaboost* classifier;
		/// Max distance to search for a coreference node
		int distance;

		bool check_coref(struct SAMPLE & sa1, struct SAMPLE & sa2) const;
		void set_sample(parse_tree::iterator pt, struct SAMPLE & sample) const;
		void add_candidates(int sent, int & word, parse_tree::iterator pt, list<struct SAMPLE> & candidates) const;

	public:
		/// Constructor
		coref(const std::string &, const int, const int);

		/// Classify SN's in given sentence in groups of coreference
		void analyze(document &) const;

};

#endif
