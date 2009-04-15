//////////////////////////////////////////////////////////////////
//    Class for the feature extractor.
//////////////////////////////////////////////////////////////////

#define COREFEX_FEATURE_SENT_SAME		0
#define COREFEX_FEATURE_SENT_1			1
#define COREFEX_FEATURE_SENT_2			2
#define COREFEX_FEATURE_SENT_3			3
#define COREFEX_FEATURE_SENT_4MORE		4
#define COREFEX_FEATURE_DIST0			10
#define COREFEX_FEATURE_DIST1			11
#define COREFEX_FEATURE_DIST2			12
#define COREFEX_FEATURE_DIST3			13
#define COREFEX_FEATURE_DIST4			14
#define COREFEX_FEATURE_DIST5MORE		15
#define COREFEX_FEATURE_IPRON			20
#define COREFEX_FEATURE_IPRONP			21
#define COREFEX_FEATURE_IPROND			22
#define COREFEX_FEATURE_IPRONX			23
#define COREFEX_FEATURE_IPRONI			24
#define COREFEX_FEATURE_IPRONT			25
#define COREFEX_FEATURE_IPRONR			26
#define COREFEX_FEATURE_IPRONE			27
#define COREFEX_FEATURE_JPRON			30
#define COREFEX_FEATURE_JPRONP			31
#define COREFEX_FEATURE_JPROND			32
#define COREFEX_FEATURE_JPRONX			33
#define COREFEX_FEATURE_JPRONI			34
#define COREFEX_FEATURE_JPRONT			35
#define COREFEX_FEATURE_JPRONR			36
#define COREFEX_FEATURE_JPRONE			37
#define COREFEX_FEATURE_STRMATH			40
#define COREFEX_FEATURE_DEFNP			41
#define COREFEX_FEATURE_DEMNP			42
#define COREFEX_FEATURE_NUMBER			43
#define COREFEX_FEATURE_GENDER			44
#define COREFEX_FEATURE_SEMCLASS		45
#define COREFEX_FEATURE_PROPERNAME		46
#define COREFEX_FEATURE_ACRONIM			50
#define COREFEX_FEATURE_FIXLEFT			51
#define COREFEX_FEATURE_FIXRIGHT		52
#define COREFEX_FEATURE_ORDER			53
#define COREFEX_FEATURE_APPOS			60

#include <stdio.h>
#include <string.h>

#include "freeling/coref_fex.h"
#include "freeling/traces.h"

using namespace std;

#define MOD_TRACENAME "COREF_FEX"
#define MOD_TRACECODE COREF_TRACE

//////////////////////////////////////////////////////////////////
///    Constructor. Sets de defaults
//////////////////////////////////////////////////////////////////

coref_fex::coref_fex(const int t, const int v, const string &sf, const string &wf) {

  typeVector = t;
  if (v==0) 
    vectors = COREFEX_DIST | COREFEX_IPRON | COREFEX_JPRON | COREFEX_IPRONM | COREFEX_JPRONM 
              | COREFEX_STRMATCH | COREFEX_DEFNP | COREFEX_DEMNP | COREFEX_GENDER | COREFEX_NUMBER
              | COREFEX_SEMCLASS | COREFEX_PROPNAME | COREFEX_ALIAS | COREFEX_APPOS;
  else 
    vectors = v;

      
 if ( !(sf.empty() && wf.empty()) ) {
    semdb= new semanticDB(sf,wf);
    TRACE(3,"Coreference solver loaded SemDB");
  }

}

coref_fex::~coref_fex(){
}

//////////////////////////////////////////////////////////////////
///    Function that jumps in the list of tags the tags that aren't relevant.
//////////////////////////////////////////////////////////////////

int coref_fex::jump(const vector<string> &list){
	unsigned int pos = 0;
	//If the first word is a preposition, an interjection, a conjunction, punctuation or an adverb, get the next.
	while((list[pos].compare(0, 1, "s") == 0 || list[pos].compare(0, 1, "i") == 0 ||
			  list[pos].compare(0, 1, "c") == 0 || list[pos].compare(0, 1, "f") == 0 ||
			  list[pos].compare(0, 1, "r") == 0) && list.size() > (pos+1)) {
		pos++;
	}
	return(pos);
}

//////////////////////////////////////////////////////////////////
///    Returns the distance in sentences of the example.
//////////////////////////////////////////////////////////////////

int coref_fex::get_dist(const EXAMPLE &ex){
	switch(ex.sent){
		case 0:
			return COREFEX_FEATURE_SENT_SAME;
			break;
		case 1:
			return COREFEX_FEATURE_SENT_1;
			break;
		case 2:
			return COREFEX_FEATURE_SENT_2;
			break;
		case 3:
			return COREFEX_FEATURE_SENT_3;
			break;
		default:
			return COREFEX_FEATURE_SENT_4MORE;
			break;
	}
}

//////////////////////////////////////////////////////////////////
///    Returns the distance in DE's of the example.
//////////////////////////////////////////////////////////////////

int coref_fex::get_dedist(const EXAMPLE &ex){
	int res = ex.sample2.posbegin - ex.sample1.posend;

	switch(res){
		case 0:
			return COREFEX_FEATURE_DIST0;
			break;
		case 1:
			return COREFEX_FEATURE_DIST1;
			break;
		case 2:
			return COREFEX_FEATURE_DIST2;
			break;
		case 3:
			return COREFEX_FEATURE_DIST3;
			break;
		case 4:
			return COREFEX_FEATURE_DIST4;
			break;
		default:
			return COREFEX_FEATURE_DIST5MORE;
			break;
	}
}

//////////////////////////////////////////////////////////////////
///    Returns if 'i' are a pronoun
//////////////////////////////////////////////////////////////////

int coref_fex::get_i_pronoum(const EXAMPLE &ex){
	int pos = 0;

	pos = jump(ex.sample1.tags);
	if(ex.sample1.tags[pos].compare(0, 1, "p") == 0)
		return COREFEX_FEATURE_IPRON;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'j' are a pronoun
//////////////////////////////////////////////////////////////////

int coref_fex::get_j_pronoum(const EXAMPLE &ex){
	int pos = 0;

	pos = jump(ex.sample2.tags);
	if(ex.sample2.tags[0].compare(0, 1, "p") == 0)
		return COREFEX_FEATURE_JPRON;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'i' are a pronoun of type 'personal'
//////////////////////////////////////////////////////////////////

int coref_fex::get_i_pronoum_p(const EXAMPLE &ex){
	int pos = 0;

	pos = jump(ex.sample1.tags);
	if(ex.sample1.tags[pos].compare(0, 2, "pp") == 0)
		return COREFEX_FEATURE_IPRONP;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'j' are a pronoun of type 'personal'
//////////////////////////////////////////////////////////////////

int coref_fex::get_j_pronoum_p(const EXAMPLE &ex){
	int pos = 0;

	pos = jump(ex.sample2.tags);
	if(ex.sample2.tags[0].compare(0, 2, "pp") == 0)
		return COREFEX_FEATURE_JPRONP;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'i' are a pronoun of type 'demostrativo'
//////////////////////////////////////////////////////////////////

int coref_fex::get_i_pronoum_d(const EXAMPLE &ex){
	int pos = 0;

	pos = jump(ex.sample1.tags);
	if(ex.sample1.tags[pos].compare(0, 2, "pd") == 0)
		return COREFEX_FEATURE_IPROND;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'j' are a pronoun of type 'demostrativo'
//////////////////////////////////////////////////////////////////

int coref_fex::get_j_pronoum_d(const EXAMPLE &ex){
	int pos = 0;

	pos = jump(ex.sample2.tags);
	if(ex.sample2.tags[0].compare(0, 2, "pd") == 0)
		return COREFEX_FEATURE_JPROND;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'i' are a pronoun of type 'posesivo'
//////////////////////////////////////////////////////////////////

int coref_fex::get_i_pronoum_x(const EXAMPLE &ex){
	int pos = 0;

	pos = jump(ex.sample1.tags);
	if(ex.sample1.tags[pos].compare(0, 2, "px") == 0)
		return COREFEX_FEATURE_IPRONX;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'j' are a pronoun of type 'posesivo'
//////////////////////////////////////////////////////////////////

int coref_fex::get_j_pronoum_x(const EXAMPLE &ex){
	int pos = 0;

	pos = jump(ex.sample2.tags);
	if(ex.sample2.tags[0].compare(0, 2, "px") == 0)
		return COREFEX_FEATURE_JPRONX;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'i' are a pronoun of type 'indefinido'
//////////////////////////////////////////////////////////////////

int coref_fex::get_i_pronoum_i(const EXAMPLE &ex){
	int pos = 0;

	pos = jump(ex.sample1.tags);
	if(ex.sample1.tags[pos].compare(0, 2, "pi") == 0)
		return COREFEX_FEATURE_IPRONI;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'j' are a pronoun of type 'indefinido'
//////////////////////////////////////////////////////////////////

int coref_fex::get_j_pronoum_i(const EXAMPLE &ex){
	int pos = 0;

	pos = jump(ex.sample2.tags);
	if(ex.sample2.tags[0].compare(0, 2, "pi") == 0)
		return COREFEX_FEATURE_JPRONI;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'i' are a pronoun of type 'interrogativo'
//////////////////////////////////////////////////////////////////

int coref_fex::get_i_pronoum_t(const EXAMPLE &ex){
	int pos = 0;

	pos = jump(ex.sample1.tags);
	if(ex.sample1.tags[pos].compare(0, 2, "pt") == 0)
		return COREFEX_FEATURE_IPRONT;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'j' are a pronoun of type 'interrogativo'
//////////////////////////////////////////////////////////////////

int coref_fex::get_j_pronoum_t(const EXAMPLE &ex){
	int pos = 0;

	pos = jump(ex.sample2.tags);
	if(ex.sample2.tags[0].compare(0, 2, "pt") == 0)
		return COREFEX_FEATURE_JPRONT;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'i' are a pronoun of type 'relativo'
//////////////////////////////////////////////////////////////////

int coref_fex::get_i_pronoum_r(const EXAMPLE &ex){
	int pos = 0;

	pos = jump(ex.sample1.tags);
	if(ex.sample1.tags[pos].compare(0, 2, "pr") == 0)
		return COREFEX_FEATURE_IPRONR;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'j' are a pronoun of type 'relativo'
//////////////////////////////////////////////////////////////////

int coref_fex::get_j_pronoum_r(const EXAMPLE &ex){
	int pos = 0;

	pos = jump(ex.sample2.tags);
	if(ex.sample2.tags[0].compare(0, 2, "pr") == 0)
		return COREFEX_FEATURE_JPRONR;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'i' are a pronoun of type 'exclamativo'
//////////////////////////////////////////////////////////////////

int coref_fex::get_i_pronoum_e(const EXAMPLE &ex){
	int pos = 0;

	pos = jump(ex.sample1.tags);
	if(ex.sample1.tags[pos].compare(0, 2, "pe") == 0)
		return COREFEX_FEATURE_IPRONE;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'j' are a pronoun of type 'exclamativo'
//////////////////////////////////////////////////////////////////

int coref_fex::get_j_pronoum_e(const EXAMPLE &ex){
	int pos = 0;

	pos = jump(ex.sample2.tags);
	if(ex.sample2.tags[0].compare(0, 2, "pe") == 0)
		return COREFEX_FEATURE_JPRONE;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'i' matches the string of 'j' 
//////////////////////////////////////////////////////////////////

int coref_fex::get_str_match(const EXAMPLE &ex){
	vector<string>::const_iterator itT1, itT2, tag1, tag2;
	string str1, str2;

	itT1 = ex.sample1.texttok.begin();
	tag1 = ex.sample1.tags.begin();
	while(itT1 != ex.sample1.texttok.end() && tag1 != ex.sample1.tags.end()){
		//Eliminates punctuation, articles and demonstrative pronouns.
		if((*tag1).compare(0, 1, "f") != 0 && (*tag1).compare(0, 1, "d") != 0  && (*tag1).compare(0, 2, "pd") != 0
		  && (*tag1).compare(0, 1, "s") != 0 && (*tag1).compare(0, 1, "i") != 0 && (*tag1).compare(0, 1, "r") != 0
		  && (*tag1).compare(0, 1, "c") != 0){
			str1 += (*itT1);
		}
		++itT1;
		++tag1;
	}

	itT2 = ex.sample2.texttok.begin();
	tag2 = ex.sample2.tags.begin();
	while(itT2 != ex.sample2.texttok.end() && tag2 != ex.sample2.tags.end()){
		if((*tag2).compare(0, 1, "f") != 0 && (*tag2).compare(0, 1, "d") != 0  && (*tag2).compare(0, 2, "pd") != 0
		  && (*tag2).compare(0, 1, "s") != 0 && (*tag2).compare(0, 1, "i") != 0 && (*tag2).compare(0, 1, "r") != 0
		  && (*tag2).compare(0, 1, "c") != 0){
			str2 += (*itT2);
		}
		++itT2;
		++tag2;
	}

	if(str1 == str2 && str1.size() > 1){
		return COREFEX_FEATURE_STRMATH;
	}else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'j' are definite noun phrase
//////////////////////////////////////////////////////////////////

int coref_fex::get_def_np(const EXAMPLE &ex){
	unsigned int pos = 0;

	pos = jump(ex.sample2.tags);
	if(ex.sample2.tags.size() > (pos+1)){
		if((ex.sample2.tags[pos].compare(0, 2, "da") == 0) && ex.sample2.tags[pos+1].compare(0, 2, "nc") == 0)
			return COREFEX_FEATURE_DEFNP;
		else
			return 0;
	} else {
		return 0;
	}
}

//////////////////////////////////////////////////////////////////
///    Returns if 'j' are demonstrative noun phrase
//////////////////////////////////////////////////////////////////

int coref_fex::get_dem_np(const EXAMPLE &ex){
	unsigned int pos = 0;

	pos = jump(ex.sample2.tags);
	if(ex.sample2.tags.size() > (pos+1)){
		if(ex.sample2.tags[pos].compare(0, 2, "dd") == 0 && ex.sample2.tags[pos+1].compare(0, 2, "nc") == 0)
			return COREFEX_FEATURE_DEMNP;
		else
			return 0;
	} else {
		return 0;
	}
}

//////////////////////////////////////////////////////////////////
///    Returns if 'i' and 'j' agree in number
//////////////////////////////////////////////////////////////////

int coref_fex::get_number(const EXAMPLE &ex){
	char num1='0', num2='0';
	unsigned int pos = 0;

	pos = jump(ex.sample1.tags);

	//Articulos, adjetivos y pronombres
	if(ex.sample1.tags[pos].compare(0, 1, "a") == 0 || ex.sample1.tags[pos].compare(0, 1, "d") == 0 || ex.sample1.tags[pos].compare(0, 1, "p") == 0){
		num1 = ex.sample1.tags[pos][4];
	//Nombres y preposiciones
	} else if(ex.sample1.tags[pos].compare(0, 2, "nc") == 0 || ex.sample1.tags[pos].compare(0, 3, "spc") == 0 ){
		num1 = ex.sample1.tags[pos][3];
	}

	pos = jump(ex.sample2.tags);
	
	//Articulos, adjetivos y pronombres
	if(ex.sample2.tags[pos].compare(0, 1, "a") == 0 || ex.sample2.tags[pos].compare(0, 1, "d") == 0 || ex.sample2.tags[pos].compare(0, 1, "p") == 0){
		num2 = ex.sample2.tags[pos][4];
	//Nombres y preposiciones
	} else if(ex.sample2.tags[pos].compare(0, 2, "nc") == 0 || ex.sample2.tags[pos].compare(0, 3, "spc") == 0 ){
		num2 = ex.sample2.tags[pos][3];
	}

	if(num1 == num2 && num1 != '0')
		return COREFEX_FEATURE_NUMBER;
//	else if((num1 == '0' || num2 == '0') && typeVector == COREFEX_TYPE_THREE)
//		return 2;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'i' and 'j' are of the same semantic class
///    Uses the NEC if the word is a proper noun or wordnet if the
///    is a common noun
//////////////////////////////////////////////////////////////////

int coref_fex::get_semclass(const EXAMPLE &ex){
	unsigned int ret = 0, pos1, pos2;
	vector<string> txt1 = tokenize(ex.sample1.text," ");
	vector<string> txt2 = tokenize(ex.sample2.text," ");
	string t1, t2, tag1, tag2, type1, type2;

	pos1 = 0;
	pos2 = 0;
	tag1 = ex.sample1.tags[pos1];
	tag2 = ex.sample2.tags[pos2];
	t1 = txt1[pos1];
	t2 = txt2[pos2];

	while (tag1.compare(0, 1, "n") != 0 && txt1.size() > (pos1+1)) {
		pos1++;
		t1 = txt1[pos1];
		tag1 = ex.sample1.tags[pos1];
	}
	while (tag2.compare(0, 1, "n") != 0 && txt2.size() > (pos2+1)) {
		pos2++;
		t2 = txt2[pos2];
		tag2 = ex.sample2.tags[pos2];
	}
	//Gets the NEC if the tag are a proper noun
	if(tag1.compare(0, 2, "np") == 0){
		type1 = tag1[4];
		type1 += tag1[5];
	}
	if(tag2.compare(0, 2, "np") == 0){
		type2 = tag2[4];
		type2 += tag2[5];
	}

	//Gets the class from the wordnet if the tag is a common noun
	if(tag1.compare(0, 2, "nc") == 0 || tag2.compare(0, 2, "nc") == 0 || type1 == "00" || type2 == "00"){
		list< std::string > l1, l2;
		list< std::string >::iterator it;

		if(tag1.compare(0, 2, "nc") == 0 || type1 == "00"){
			l1 = semdb->get_word_senses(t1, "N");
//			if((*l1.begin()).size() > 0){
			if (not l1.empty()) {
				if (not l1.begin()->empty()) {
					sense_info si = semdb->get_sense_info (l1.front(), "N");
					for(it = si.tonto.begin(); it != si.tonto.end() ; ++it){
						if((*it) == "Human"){
							type1 = "sp";
						} else if((*it) == "Group"){
							type1 = "o0";
						} else if((*it) == "Part"){
							type1 = "g0";
						} else if(type1 == "00"){
							type1 = "v0";
						}
					}
				}
			}
		}

		if(tag2.compare(0, 2, "nc") == 0 || type2 == "00"){

		  l2 = semdb->get_word_senses(t2, "N");
		  
		  if (not l2.empty()) {
		    if (not l2.begin()->empty()) {
		      sense_info si = semdb->get_sense_info (l2.front(), "N");
		      for(it = si.tonto.begin(); it != si.tonto.end() ; ++it){
			if((*it) == "Human"){
			  type2 = "sp";
			} else if((*it) == "Group"){
			  type2 = "o0";
			} else if((*it) == "Part"){
			  type2 = "g0";
			} else if(type2 == "00"){
			  type2 = "v0";
			}
		      }
		    }
		  }
		}
	}

	if(ret != 1 && ex.sample1.tags[pos1].compare(0, 1, "n") == 0 && ex.sample2.tags[pos2].compare(0, 1, "n") == 0 ){
		if(ex.sample1.tags[pos1].substr(4,2) == ex.sample2.tags[pos2].substr(4,2) && ex.sample1.tags[pos1].substr(4,2) != "00"){
			ret = COREFEX_FEATURE_SEMCLASS;
		}
	}

//	if(typeVector == COREFEX_TYPE_TWO && ret == 2)
//		ret = 0;
	return ret;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'i' and 'j' agree in gender
//////////////////////////////////////////////////////////////////

int coref_fex::get_gender(const EXAMPLE &ex){
	char gen1='0', gen2='0';
	unsigned int pos = 0;

	pos = jump(ex.sample1.tags);
	
	//Articulos, adjetivos y pronombres
	if(ex.sample1.tags[pos].compare(0, 1, "a") == 0 || ex.sample1.tags[pos].compare(0, 1, "d") == 0 || ex.sample1.tags[pos].compare(0, 1, "p") == 0){
		gen1 = ex.sample1.tags[pos][3];
	//Nombres y preposiciones
	} else if(ex.sample1.tags[pos].compare(0, 1, "n") == 0 || ex.sample1.tags[pos].compare(0, 3, "spc") == 0 ){
		gen1 = ex.sample1.tags[pos][2];
	}


	pos = jump(ex.sample2.tags);
	
	//Articulos, adjetivos y pronombres
	if(ex.sample2.tags[pos].compare(0, 1, "a") == 0 || ex.sample2.tags[pos].compare(0, 1, "d") == 0 || ex.sample2.tags[pos].compare(0, 1, "p") == 0){
		gen2 = ex.sample2.tags[pos][3];
	//Nombres y preposiciones
	} else if(ex.sample2.tags[pos].compare(0, 1, "n") == 0 || ex.sample2.tags[pos].compare(0, 3, "spc") == 0 ){
		gen2 = ex.sample2.tags[pos][2];
	}

	if(gen1 == gen2 && gen1 != '0')
		return COREFEX_FEATURE_GENDER;
//	else if((gen1 == '0' || gen2 == '0') && typeVector == COREFEX_TYPE_THREE)
//		return 2;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'i' and 'j' are proper nouns
//////////////////////////////////////////////////////////////////

int coref_fex::get_proper_name(const EXAMPLE &ex){
	int pos1, pos2;

	pos1 = jump(ex.sample1.tags);
	pos2 = jump(ex.sample2.tags);
	if(ex.sample1.tags[pos1].compare(0, 2, "np") == 0 && ex.sample2.tags[pos2].compare(0, 2, "np") == 0)
		return COREFEX_FEATURE_PROPERNAME;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if the two words matches
//////////////////////////////////////////////////////////////////

bool coref_fex::check_word(string w1, string w2){
	string::iterator si, si2;

	si = w1.begin();
	si2 = w2.begin();
	while (si != w1.end() && si2 != w2.end()){
		if(std::toupper(*si) == std::toupper(*si2)){
			++si2;
		}
		++si;
	}
	if(si2 == w2.end())
		return true;
	else
		return false;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'j' are an acronim of 'i'
//////////////////////////////////////////////////////////////////

int coref_fex::check_acronim(const EXAMPLE &ex){
	string::const_iterator si, si2;
	int ret=1;
	int cEquals, cTotal;

	if(ex.sample2.tags.size() != 1 && ex.sample2.tags.size() != 3 )
		return 0;
	if(ex.sample1.tags[0].compare(0, 2, "np") != 0)
		return 0;
	if(ex.sample2.tags.size() == 1){
		if(ex.sample2.tags[0].compare(0, 2, "np") != 0)
			return 0;
	} else if(ex.sample2.tags.size() == 3){
		if(ex.sample2.tags[1].compare(0, 2, "np") != 0)
			return 0;
	}

	if(ret){
		bool wbegin = true;

		cEquals=0, cTotal=1;
		si = ex.sample2.text.begin();
		si2 = ex.sample1.text.begin();
		while(si != ex.sample2.text.end() && si2 != ex.sample1.text.end()){
			while(*si == '(' || *si == ')' || *si == '.' || *si == '_' || *si == ' ')
				++si;
			if(wbegin && std::toupper(*si) == std::toupper(*si2)){
				cEquals++;
				++si;
			}
			if( *si2 != ' ' && *si2 != '_'){
				wbegin = false;
			}else{
				cTotal++;
				wbegin = true;
			}
			++si2;
		}
		while(si2 != ex.sample1.text.end()){
			if( *si2 == ' ' || *si2 == '_'){
				cTotal++;
			}
			++si2;
		}
		if(si != ex.sample2.text.end()){
			ret=0;
		} else if(cEquals <= (cTotal/2)){
			ret=0;
		}
	}
	return ret;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'j' are an prefix of 'i'
//////////////////////////////////////////////////////////////////

int coref_fex::check_fixesleft(const EXAMPLE &ex){
	vector<string>::const_iterator itT1, itT2;
	vector<string>::const_reverse_iterator ritT1, ritT2;
	int total1 = ex.sample1.texttok.size();
	int total2 = ex.sample2.texttok.size();
	int max = (total1 > total2) ? total1 : total2;
	int count;
	int ret = 0;
	
	if(total1 >= 1 && total2 >= 1 && max > 1){
		itT1 = ex.sample1.texttok.begin();
		itT2 = ex.sample2.texttok.begin();
		count = 0;
		while(itT1 != ex.sample1.texttok.end() && itT2 != ex.sample2.texttok.end()){
			if(check_word(*itT1, *itT2)){
				count++;
			} else {
				break;
			}
			++itT1;
			++itT2;
		}
		if(count > max/2)
			ret = 1;;
	}

	return ret;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'j' are an suffix of 'i'
//////////////////////////////////////////////////////////////////

int coref_fex::check_fixesright(const EXAMPLE &ex){
	vector<string>::const_iterator itT1, itT2;
	vector<string>::const_reverse_iterator ritT1, ritT2;
	int total1 = ex.sample1.texttok.size();
	int total2 = ex.sample2.texttok.size();
	int max = (total1 > total2) ? total1 : total2;
	int count;
	int ret = 0;
	
	if(total1 >= 1 && total2 >= 1 && max > 1){
		ritT1 = ex.sample1.texttok.rbegin();
		ritT2 = ex.sample2.texttok.rbegin();
		count = 0;
		while(ritT1 != ex.sample1.texttok.rend() && ritT2 != ex.sample2.texttok.rend()){
			if(check_word(*ritT1, *ritT2)){
				count++;
			} else {
				break;
			}
			++ritT1;
			++ritT2;
		}
		if(count > max/2)
			ret = 1;
	}

	return ret;
}

//////////////////////////////////////////////////////////////////
///    Returns if the words of 'j' appears in ths same order in 'i'
//////////////////////////////////////////////////////////////////

int coref_fex::check_order(const EXAMPLE &ex){
	vector<string>::const_iterator itT1, itT2;

	itT1 = ex.sample1.texttok.begin();
	itT2 = ex.sample2.texttok.begin();
	while(itT1 != ex.sample1.texttok.end() && itT2 != ex.sample2.texttok.end()){
		if(check_word(*itT1, *itT2)){
			++itT2;
		}
		++itT1;
	}
	if(itT2 == ex.sample2.texttok.end() && ex.sample2.texttok.size() > 1){
		return 1;
	}else{
		return 0;
	}
}

//////////////////////////////////////////////////////////////////
///    Returns if 'j' are an acronim of 'i'
//////////////////////////////////////////////////////////////////

int coref_fex::get_alias_acro(const EXAMPLE &ex){
	if(check_acronim(ex) != 0)
		return COREFEX_FEATURE_ACRONIM;
	else
		return(0);
}

//////////////////////////////////////////////////////////////////
///    Returns if 'j' are an prefix of 'i'
//////////////////////////////////////////////////////////////////

int coref_fex::get_alias_fixleft(const EXAMPLE &ex){
	if(check_fixesleft(ex) != 0)
		return COREFEX_FEATURE_FIXLEFT;
	else
		return(0);
}

//////////////////////////////////////////////////////////////////
///    Returns if 'j' are an suffix of 'i'
//////////////////////////////////////////////////////////////////

int coref_fex::get_alias_fixright(const EXAMPLE &ex){
	if(check_fixesright(ex) != 0)
		return COREFEX_FEATURE_FIXRIGHT;
	else
		return(0);
}

//////////////////////////////////////////////////////////////////
///    Returns if the words of 'j' appears in ths same order in 'i'
//////////////////////////////////////////////////////////////////

int coref_fex::get_alias_order(const EXAMPLE &ex){
	if(check_order(ex) != 0)
		return COREFEX_FEATURE_ORDER;
	else
		return(0);
}

//////////////////////////////////////////////////////////////////
///    Returns if 'j' are in apposition of 'i'
//////////////////////////////////////////////////////////////////

int coref_fex::get_appositive(const EXAMPLE &ex){
	bool ret = 0;

	if( (ex.sample2.posbegin > ex.sample1.posbegin && ex.sample1.posend == ex.sample2.posend) ||
		(ex.sample2.posbegin == (ex.sample1.posend+1))) {
		if(ex.sample2.tags[0] == "fpa" || ex.sample2.tags[0] == "fc"){
			ret = COREFEX_FEATURE_APPOS;
		}
	}
	return ret;
}

//////////////////////////////////////////////////////////////////
///    Tokenize a string with delimiters and return a vector of strings with the tokens
//////////////////////////////////////////////////////////////////

vector<string> coref_fex::tokenize(const string& str,const string& delimiters){
	vector<string> tokens;
	// skip delimiters at beginning.
	string::size_type lastPos = str.find_first_not_of(delimiters, 0);
	// find first "non-delimiter".
	string::size_type pos = str.find_first_of(delimiters, lastPos);

	while (string::npos != pos || string::npos != lastPos){
		// found a token, add it to the vector.
		tokens.push_back(str.substr(lastPos, pos - lastPos));
		// skip delimiters.  Note the "not_of"
		lastPos = str.find_first_not_of(delimiters, pos);
		// find next "non-delimiter"
		pos = str.find_first_of(delimiters, lastPos);
	}
	return tokens;
}

//////////////////////////////////////////////////////////////////
///    Extract the features configured to be extracted
//////////////////////////////////////////////////////////////////
void coref_fex::put_feature(int f, std::vector<int> &result){
	if(f > 0)
		result.push_back(f);
}
void coref_fex::extract(EXAMPLE &ex, std::vector<int> &result){
	result.clear();

	if (vectors & COREFEX_DIST){
		result.push_back(get_dist(ex));
		result.push_back(get_dedist(ex));
	}
	if (vectors & COREFEX_IPRON)
		put_feature(get_i_pronoum(ex), result);
	if (vectors & COREFEX_JPRON)
		put_feature(get_j_pronoum(ex), result);

	if (vectors & COREFEX_IPRONM) {
		put_feature(get_i_pronoum_p(ex), result);
		put_feature(get_i_pronoum_d(ex), result);
		put_feature(get_i_pronoum_x(ex), result);
		put_feature(get_i_pronoum_i(ex), result);
		put_feature(get_i_pronoum_t(ex), result);
		put_feature(get_i_pronoum_r(ex), result);
		put_feature(get_i_pronoum_e(ex), result);
	}


	if (vectors & COREFEX_JPRONM) {
		put_feature(get_j_pronoum_p(ex), result);
		put_feature(get_j_pronoum_d(ex), result);
		put_feature(get_j_pronoum_x(ex), result);
		put_feature(get_j_pronoum_i(ex), result);
		put_feature(get_j_pronoum_t(ex), result);
		put_feature(get_j_pronoum_r(ex), result);
		put_feature(get_j_pronoum_e(ex), result);
	}

	if (vectors & COREFEX_STRMATCH)
		put_feature(get_str_match(ex), result);
	if (vectors & COREFEX_DEFNP)
		put_feature(get_def_np(ex), result);
	if (vectors & COREFEX_DEMNP)
		put_feature(get_dem_np(ex), result);
	if (vectors & COREFEX_NUMBER)
		put_feature(get_number(ex), result);
	if (vectors & COREFEX_GENDER)
		put_feature(get_gender(ex), result);
	if (vectors & COREFEX_SEMCLASS)
		put_feature(get_semclass(ex), result);
	if (vectors & COREFEX_PROPNAME)
		put_feature(get_proper_name(ex), result);

	if (vectors & COREFEX_ALIAS) {
		put_feature(get_alias_acro(ex), result);
		put_feature(get_alias_fixleft(ex), result);
		put_feature(get_alias_fixright(ex), result);
		put_feature(get_alias_order(ex), result);
	}

	if (vectors & COREFEX_APPOS)
		put_feature(get_appositive(ex), result);

}

//////////////////////////////////////////////////////////////////
///    Configure the features to be extracted
//////////////////////////////////////////////////////////////////

void coref_fex::setVectors(int vec){
	vectors = vec;
}
