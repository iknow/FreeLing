//////////////////////////////////////////////////////////////////
//    Class for the feature extractor.
//////////////////////////////////////////////////////////////////

#ifndef CORE_FEX_H
#define CORE_FEX_H

#include <list>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

#include "fries.h"

using namespace std;


//////////////////////////////////////////////////////////////////
///    Struct that contains one sample
//////////////////////////////////////////////////////////////////

struct SAMPLE {
	int sent;
	int posbegin;
	int posend;
	node *node1;
	string text;
	vector<string> texttok;
	vector<string> tags;
};

//////////////////////////////////////////////////////////////////
///    Struct of an example (contains two samples)
//////////////////////////////////////////////////////////////////

struct EXAMPLE {
	int sent;
	struct SAMPLE sample1;
	struct SAMPLE sample2;
};

#define TYPE_TWO	0
#define TYPE_THREE	1

#define COREFEX_DIST		0x00000001
#define COREFEX_IPRON		0x00000002
#define COREFEX_JPRON		0x00000004
#define COREFEX_IPRONM		0x00000008
#define COREFEX_JPRONM		0x00000010
#define COREFEX_STRMATCH	0x00000020
#define COREFEX_DEFNP		0x00000040
#define COREFEX_DEMNP		0x00000080
#define COREFEX_NUMBER		0x00000100
#define COREFEX_GENDER		0x00000200
#define COREFEX_SEMCLASS	0x00000400
#define COREFEX_PROPNAME	0x00000800
#define COREFEX_ALIAS		0x00001000
#define COREFEX_APPOS		0x00002000

//////////////////////////////////////////////////////////////////
///    Class for the feature extractor.
//////////////////////////////////////////////////////////////////

class coref_fex{
	private:
		int vectors;

		int jump(vector<string> &list);
		int get_dist(struct EXAMPLE *ex);
		int get_dedist(struct EXAMPLE *ex);
		int get_i_pronoum(struct EXAMPLE *ex);
		int get_j_pronoum(struct EXAMPLE *ex);
		int get_i_pronoum_p(struct EXAMPLE *ex);
		int get_j_pronoum_p(struct EXAMPLE *ex);
		int get_i_pronoum_d(struct EXAMPLE *ex);
		int get_j_pronoum_d(struct EXAMPLE *ex);
		int get_i_pronoum_x(struct EXAMPLE *ex);
		int get_j_pronoum_x(struct EXAMPLE *ex);
		int get_i_pronoum_i(struct EXAMPLE *ex);
		int get_j_pronoum_i(struct EXAMPLE *ex);
		int get_i_pronoum_t(struct EXAMPLE *ex);
		int get_j_pronoum_t(struct EXAMPLE *ex);
		int get_i_pronoum_r(struct EXAMPLE *ex);
		int get_j_pronoum_r(struct EXAMPLE *ex);
		int get_i_pronoum_e(struct EXAMPLE *ex);
		int get_j_pronoum_e(struct EXAMPLE *ex);
		int get_str_match(struct EXAMPLE *ex);
		int get_def_np(struct EXAMPLE *ex);
		int get_dem_np(struct EXAMPLE *ex);
		int get_number(struct EXAMPLE *ex);
		int get_semclass(struct EXAMPLE *ex);
		int get_gender(struct EXAMPLE *ex);
		int get_proper_name(struct EXAMPLE *ex);
		bool check_word(string w1, string w2);
		int check_acronim(struct EXAMPLE *ex);
		int check_fixesleft(struct EXAMPLE *ex);
		int check_fixesright(struct EXAMPLE *ex);
		int check_order(struct EXAMPLE *ex);
		int get_alias_acro(struct EXAMPLE *ex);
		int get_alias_fixleft(struct EXAMPLE *ex);
		int get_alias_fixright(struct EXAMPLE *ex);
		int get_alias_order(struct EXAMPLE *ex);
		int get_appositive(struct EXAMPLE *ex);

	public:
		int typeVector;
		
		coref_fex();
		~coref_fex();
		void setVectors(int vec);
		vector<string> tokenize(const string& str,const string& delimiters);
		void extract(struct EXAMPLE &ex, std::vector<int> *result);
};
#endif
