//////////////////////////////////////////////////////////////////
//    Class for the feature extractor.
//////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>

#include "freeling.h"
#include "freeling/traces.h"

//////////////////////////////////////////////////////////////////
///    Constructor. Sets de defaults
//////////////////////////////////////////////////////////////////

coref_fex::coref_fex(){
	typeVector = TYPE_TWO;
	vectors = DIST | IPRON | JPRON | IPRONM | JPRONM | STRMATCH | DEFNP | DEMNP | NUMBER | GENDER | SEMCLASS | PROPNAME | ALIAS | APPOS;
}

coref_fex::~coref_fex(){
}

//////////////////////////////////////////////////////////////////
///    Function that jumps in the list of tags the tags that aren't relevant.
//////////////////////////////////////////////////////////////////

int coref_fex::jump(vector<string> &list){
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

int coref_fex::get_dist(struct EXAMPLE *ex){
	return ex->sent;
}

//////////////////////////////////////////////////////////////////
///    Returns the distance in DE's of the example.
//////////////////////////////////////////////////////////////////

int coref_fex::get_dedist(struct EXAMPLE *ex){
	int res = ex->sample2.posbegin - ex->sample1.posend;
	if(res > 0)
		return res;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'i' are a pronoun
//////////////////////////////////////////////////////////////////

int coref_fex::get_i_pronoum(struct EXAMPLE *ex){
	int pos = 0;

	pos = jump(ex->sample1.tags);
	if(ex->sample1.tags[pos].compare(0, 1, "p") == 0)
		return 1;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'j' are a pronoun
//////////////////////////////////////////////////////////////////

int coref_fex::get_j_pronoum(struct EXAMPLE *ex){
	int pos = 0;

	pos = jump(ex->sample2.tags);
	if(ex->sample2.tags[0].compare(0, 1, "p") == 0)
		return 1;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'i' are a pronoun of type 'personal'
//////////////////////////////////////////////////////////////////

int coref_fex::get_i_pronoum_p(struct EXAMPLE *ex){
	int pos = 0;

	pos = jump(ex->sample1.tags);
	if(ex->sample1.tags[pos].compare(0, 2, "pp") == 0)
		return 1;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'j' are a pronoun of type 'personal'
//////////////////////////////////////////////////////////////////

int coref_fex::get_j_pronoum_p(struct EXAMPLE *ex){
	int pos = 0;

	pos = jump(ex->sample2.tags);
	if(ex->sample2.tags[0].compare(0, 2, "pp") == 0)
		return 1;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'i' are a pronoun of type 'demostrativo'
//////////////////////////////////////////////////////////////////

int coref_fex::get_i_pronoum_d(struct EXAMPLE *ex){
	int pos = 0;

	pos = jump(ex->sample1.tags);
	if(ex->sample1.tags[pos].compare(0, 2, "pd") == 0)
		return 1;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'j' are a pronoun of type 'demostrativo'
//////////////////////////////////////////////////////////////////

int coref_fex::get_j_pronoum_d(struct EXAMPLE *ex){
	int pos = 0;

	pos = jump(ex->sample2.tags);
	if(ex->sample2.tags[0].compare(0, 2, "pd") == 0)
		return 1;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'i' are a pronoun of type 'posesivo'
//////////////////////////////////////////////////////////////////

int coref_fex::get_i_pronoum_x(struct EXAMPLE *ex){
	int pos = 0;

	pos = jump(ex->sample1.tags);
	if(ex->sample1.tags[pos].compare(0, 2, "px") == 0)
		return 1;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'j' are a pronoun of type 'posesivo'
//////////////////////////////////////////////////////////////////

int coref_fex::get_j_pronoum_x(struct EXAMPLE *ex){
	int pos = 0;

	pos = jump(ex->sample2.tags);
	if(ex->sample2.tags[0].compare(0, 2, "px") == 0)
		return 1;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'i' are a pronoun of type 'indefinido'
//////////////////////////////////////////////////////////////////

int coref_fex::get_i_pronoum_i(struct EXAMPLE *ex){
	int pos = 0;

	pos = jump(ex->sample1.tags);
	if(ex->sample1.tags[pos].compare(0, 2, "pi") == 0)
		return 1;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'j' are a pronoun of type 'indefinido'
//////////////////////////////////////////////////////////////////

int coref_fex::get_j_pronoum_i(struct EXAMPLE *ex){
	int pos = 0;

	pos = jump(ex->sample2.tags);
	if(ex->sample2.tags[0].compare(0, 2, "pi") == 0)
		return 1;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'i' are a pronoun of type 'interrogativo'
//////////////////////////////////////////////////////////////////

int coref_fex::get_i_pronoum_t(struct EXAMPLE *ex){
	int pos = 0;

	pos = jump(ex->sample1.tags);
	if(ex->sample1.tags[pos].compare(0, 2, "pt") == 0)
		return 1;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'j' are a pronoun of type 'interrogativo'
//////////////////////////////////////////////////////////////////

int coref_fex::get_j_pronoum_t(struct EXAMPLE *ex){
	int pos = 0;

	pos = jump(ex->sample2.tags);
	if(ex->sample2.tags[0].compare(0, 2, "pt") == 0)
		return 1;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'i' are a pronoun of type 'relativo'
//////////////////////////////////////////////////////////////////

int coref_fex::get_i_pronoum_r(struct EXAMPLE *ex){
	int pos = 0;

	pos = jump(ex->sample1.tags);
	if(ex->sample1.tags[pos].compare(0, 2, "pr") == 0)
		return 1;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'j' are a pronoun of type 'relativo'
//////////////////////////////////////////////////////////////////

int coref_fex::get_j_pronoum_r(struct EXAMPLE *ex){
	int pos = 0;

	pos = jump(ex->sample2.tags);
	if(ex->sample2.tags[0].compare(0, 2, "pr") == 0)
		return 1;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'i' are a pronoun of type 'exclamativo'
//////////////////////////////////////////////////////////////////

int coref_fex::get_i_pronoum_e(struct EXAMPLE *ex){
	int pos = 0;

	pos = jump(ex->sample1.tags);
	if(ex->sample1.tags[pos].compare(0, 2, "pe") == 0)
		return 1;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'j' are a pronoun of type 'exclamativo'
//////////////////////////////////////////////////////////////////

int coref_fex::get_j_pronoum_e(struct EXAMPLE *ex){
	int pos = 0;

	pos = jump(ex->sample2.tags);
	if(ex->sample2.tags[0].compare(0, 2, "pe") == 0)
		return 1;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'i' matches the string of 'j' 
//////////////////////////////////////////////////////////////////

int coref_fex::get_str_match(struct EXAMPLE *ex){
	vector<string>::iterator itT1, itT2, tag1, tag2;
	string str1, str2;

	itT1 = ex->sample1.texttok.begin();
	tag1 = ex->sample1.tags.begin();
	while(itT1 != ex->sample1.texttok.end() && tag1 != ex->sample1.tags.end()){
		//Eliminates punctuation, articles and demonstrative pronouns.
		if((*tag1).compare(0, 1, "f") != 0 && (*tag1).compare(0, 1, "d") != 0  && (*tag1).compare(0, 2, "pd") != 0
		  && (*tag1).compare(0, 1, "s") != 0 && (*tag1).compare(0, 1, "i") != 0 && (*tag1).compare(0, 1, "r") != 0
		  && (*tag1).compare(0, 1, "c") != 0){
			str1 += (*itT1);
		}
		++itT1;
		++tag1;
	}

	itT2 = ex->sample2.texttok.begin();
	tag2 = ex->sample2.tags.begin();
	while(itT2 != ex->sample2.texttok.end() && tag2 != ex->sample2.tags.end()){
		if((*tag2).compare(0, 1, "f") != 0 && (*tag2).compare(0, 1, "d") != 0  && (*tag2).compare(0, 2, "pd") != 0
		  && (*tag2).compare(0, 1, "s") != 0 && (*tag2).compare(0, 1, "i") != 0 && (*tag2).compare(0, 1, "r") != 0
		  && (*tag2).compare(0, 1, "c") != 0){
			str2 += (*itT2);
		}
		++itT2;
		++tag2;
	}

	if(str1 == str2 && str1.size() > 1){
		return 1;
	}else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'j' are definite noun phrase
//////////////////////////////////////////////////////////////////

int coref_fex::get_def_np(struct EXAMPLE *ex){
	unsigned int pos = 0;

	pos = jump(ex->sample2.tags);
	if(ex->sample2.tags.size() > (pos+1)){
		if((ex->sample2.tags[pos].compare(0, 2, "da") == 0) && ex->sample2.tags[pos+1].compare(0, 2, "nc") == 0)
			return 1;
		else
			return 0;
	} else {
		return 0;
	}
}

//////////////////////////////////////////////////////////////////
///    Returns if 'j' are demonstrative noun phrase
//////////////////////////////////////////////////////////////////

int coref_fex::get_dem_np(struct EXAMPLE *ex){
	unsigned int pos = 0;

	pos = jump(ex->sample2.tags);
	if(ex->sample2.tags.size() > (pos+1)){
		if(ex->sample2.tags[pos].compare(0, 2, "dd") == 0 && ex->sample2.tags[pos+1].compare(0, 2, "nc") == 0)
			return 1;
		else
			return 0;
	} else {
		return 0;
	}
}

//////////////////////////////////////////////////////////////////
///    Returns if 'i' and 'j' agree in number
//////////////////////////////////////////////////////////////////

int coref_fex::get_number(struct EXAMPLE *ex){
	char num1='0', num2='0';
	unsigned int pos = 0;

	pos = jump(ex->sample1.tags);

	//Articulos, adjetivos y pronombres
	if(ex->sample1.tags[pos].compare(0, 1, "a") == 0 || ex->sample1.tags[pos].compare(0, 1, "d") == 0 || ex->sample1.tags[pos].compare(0, 1, "p") == 0){
		num1 = ex->sample1.tags[pos][4];
	//Nombres y preposiciones
	} else if(ex->sample1.tags[pos].compare(0, 2, "nc") == 0 || ex->sample1.tags[pos].compare(0, 3, "spc") == 0 ){
		num1 = ex->sample1.tags[pos][3];
	}

	pos = jump(ex->sample2.tags);
	
	//Articulos, adjetivos y pronombres
	if(ex->sample2.tags[pos].compare(0, 1, "a") == 0 || ex->sample2.tags[pos].compare(0, 1, "d") == 0 || ex->sample2.tags[pos].compare(0, 1, "p") == 0){
		num2 = ex->sample2.tags[pos][4];
	//Nombres y preposiciones
	} else if(ex->sample2.tags[pos].compare(0, 2, "nc") == 0 || ex->sample2.tags[pos].compare(0, 3, "spc") == 0 ){
		num2 = ex->sample2.tags[pos][3];
	}

	if(num1 == num2 && num1 != '0')
		return 1;
	else if((num1 == '0' || num2 == '0') && typeVector == TYPE_THREE)
		return 2;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'i' and 'j' are of the same semantic class
///    Uses the NEC if the word is a proper noun or wordnet if the
///    is a common noun
//////////////////////////////////////////////////////////////////

int coref_fex::get_semclass(struct EXAMPLE *ex){
	unsigned int ret = 2, pos1, pos2;
	vector<string> txt1 = tokenize(ex->sample1.text," ");
	vector<string> txt2 = tokenize(ex->sample2.text," ");
	string t1, t2, tag1, tag2, type1, type2;

	pos1 = 0;
	pos2 = 0;
	tag1 = ex->sample1.tags[pos1];
	tag2 = ex->sample2.tags[pos2];
	t1 = txt1[pos1];
	t2 = txt2[pos2];

	while (tag1.compare(0, 1, "n") != 0 && txt1.size() > (pos1+1)) {
		pos1++;
		t1 = txt1[pos1];
		tag1 = ex->sample1.tags[pos1];
	}
	while (tag2.compare(0, 1, "n") != 0 && txt2.size() > (pos2+1)) {
		pos2++;
		t2 = txt2[pos2];
		tag2 = ex->sample2.tags[pos2];
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
		semanticDB *sdb = new semanticDB("/usr/local/share/FreeLing/es/senses16.db", "/usr/local/share/FreeLing/common/wn16.db");
		if(tag1.compare(0, 2, "nc") == 0 || type1 == "00"){
			l1 = sdb->get_word_senses(t1, "N");
			if((*l1.begin()).size() > 0){
				sense_info si = sdb->get_sense_info (l1.front(), "N");
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
		if(tag2.compare(0, 2, "nc") == 0 || type2 == "00"){
			l2 = sdb->get_word_senses(t2, "N");
			if((*l2.begin()).size() > 0){
				sense_info si = sdb->get_sense_info (l2.front(), "N");
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
		delete(sdb);
	}

	if(ret != 1 && ex->sample1.tags[pos1].compare(0, 1, "n") == 0 && ex->sample2.tags[pos2].compare(0, 1, "n") == 0 ){
		if(ex->sample1.tags[pos1].substr(4,2) == ex->sample2.tags[pos2].substr(4,2) && ex->sample1.tags[pos1].substr(4,2) != "00"){
			ret = 1;
		}
	}

	if(typeVector == TYPE_TWO && ret == 2)
		ret = 0;
	return ret;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'i' and 'j' agree in gender
//////////////////////////////////////////////////////////////////

int coref_fex::get_gender(struct EXAMPLE *ex){
	char gen1='0', gen2='0';
	unsigned int pos = 0;

	pos = jump(ex->sample1.tags);
	
	//Articulos, adjetivos y pronombres
	if(ex->sample1.tags[pos].compare(0, 1, "a") == 0 || ex->sample1.tags[pos].compare(0, 1, "d") == 0 || ex->sample1.tags[pos].compare(0, 1, "p") == 0){
		gen1 = ex->sample1.tags[pos][3];
	//Nombres y preposiciones
	} else if(ex->sample1.tags[pos].compare(0, 1, "n") == 0 || ex->sample1.tags[pos].compare(0, 3, "spc") == 0 ){
		gen1 = ex->sample1.tags[pos][2];
	}


	pos = jump(ex->sample2.tags);
	
	//Articulos, adjetivos y pronombres
	if(ex->sample2.tags[pos].compare(0, 1, "a") == 0 || ex->sample2.tags[pos].compare(0, 1, "d") == 0 || ex->sample2.tags[pos].compare(0, 1, "p") == 0){
		gen2 = ex->sample2.tags[pos][3];
	//Nombres y preposiciones
	} else if(ex->sample2.tags[pos].compare(0, 1, "n") == 0 || ex->sample2.tags[pos].compare(0, 3, "spc") == 0 ){
		gen2 = ex->sample2.tags[pos][2];
	}

	if(gen1 == gen2 && gen1 != '0')
		return 1;
	else if((gen1 == '0' || gen2 == '0') && typeVector == TYPE_THREE)
		return 2;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
///    Returns if 'i' and 'j' are proper nouns
//////////////////////////////////////////////////////////////////

int coref_fex::get_proper_name(struct EXAMPLE *ex){
	int pos1, pos2;

	pos1 = jump(ex->sample1.tags);
	pos2 = jump(ex->sample2.tags);
	if(ex->sample1.tags[pos1].compare(0, 2, "np") == 0 && ex->sample2.tags[pos2].compare(0, 2, "np") == 0)
		return 1;
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

int coref_fex::check_acronim(struct EXAMPLE *ex){
	string::iterator si, si2;
	int ret=1;
	int cEquals, cTotal;

	if(ex->sample2.tags.size() != 1 && ex->sample2.tags.size() != 3 )
		return 0;
	if(ex->sample1.tags[0].compare(0, 2, "np") != 0)
		return 0;
	if(ex->sample2.tags.size() == 1){
		if(ex->sample2.tags[0].compare(0, 2, "np") != 0)
			return 0;
	} else if(ex->sample2.tags.size() == 3){
		if(ex->sample2.tags[1].compare(0, 2, "np") != 0)
			return 0;
	}

	if(ret){
		bool wbegin = true;

		cEquals=0, cTotal=1;
		si = ex->sample2.text.begin();
		si2 = ex->sample1.text.begin();
		while(si != ex->sample2.text.end() && si2 != ex->sample1.text.end()){
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
		while(si2 != ex->sample1.text.end()){
			if( *si2 == ' ' || *si2 == '_'){
				cTotal++;
			}
			++si2;
		}
		if(si != ex->sample2.text.end()){
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

int coref_fex::check_fixesleft(struct EXAMPLE *ex){
	vector<string>::iterator itT1, itT2;
	vector<string>::reverse_iterator ritT1, ritT2;
	int total1 = ex->sample1.texttok.size();
	int total2 = ex->sample2.texttok.size();
	int max = (total1 > total2) ? total1 : total2;
	int count;
	int ret = 0;
	
	if(total1 >= 1 && total2 >= 1 && max > 1){
		itT1 = ex->sample1.texttok.begin();
		itT2 = ex->sample2.texttok.begin();
		count = 0;
		while(itT1 != ex->sample1.texttok.end() && itT2 != ex->sample2.texttok.end()){
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

int coref_fex::check_fixesright(struct EXAMPLE *ex){
	vector<string>::iterator itT1, itT2;
	vector<string>::reverse_iterator ritT1, ritT2;
	int total1 = ex->sample1.texttok.size();
	int total2 = ex->sample2.texttok.size();
	int max = (total1 > total2) ? total1 : total2;
	int count;
	int ret = 0;
	
	if(total1 >= 1 && total2 >= 1 && max > 1){
		ritT1 = ex->sample1.texttok.rbegin();
		ritT2 = ex->sample2.texttok.rbegin();
		count = 0;
		while(ritT1 != ex->sample1.texttok.rend() && ritT2 != ex->sample2.texttok.rend()){
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

int coref_fex::check_order(struct EXAMPLE *ex){
	vector<string>::iterator itT1, itT2;

	itT1 = ex->sample1.texttok.begin();
	itT2 = ex->sample2.texttok.begin();
	while(itT1 != ex->sample1.texttok.end() && itT2 != ex->sample2.texttok.end()){
		if(check_word(*itT1, *itT2)){
			++itT2;
		}
		++itT1;
	}
	if(itT2 == ex->sample2.texttok.end() && ex->sample2.texttok.size() > 1){
		return 1;
	}else{
		return 0;
	}
}

//////////////////////////////////////////////////////////////////
///    Returns if 'j' are an acronim of 'i'
//////////////////////////////////////////////////////////////////

int coref_fex::get_alias_acro(struct EXAMPLE *ex){
	return(check_acronim(ex));
}

//////////////////////////////////////////////////////////////////
///    Returns if 'j' are an prefix of 'i'
//////////////////////////////////////////////////////////////////

int coref_fex::get_alias_fixleft(struct EXAMPLE *ex){
	return(check_fixesleft(ex));
}

//////////////////////////////////////////////////////////////////
///    Returns if 'j' are an suffix of 'i'
//////////////////////////////////////////////////////////////////

int coref_fex::get_alias_fixright(struct EXAMPLE *ex){
	return(check_fixesright(ex));
}

//////////////////////////////////////////////////////////////////
///    Returns if the words of 'j' appears in ths same order in 'i'
//////////////////////////////////////////////////////////////////

int coref_fex::get_alias_order(struct EXAMPLE *ex){
	return(check_order(ex));
}

//////////////////////////////////////////////////////////////////
///    Returns if 'j' are in apposition of 'i'
//////////////////////////////////////////////////////////////////

int coref_fex::get_appositive(struct EXAMPLE *ex){
	bool ret = 0;

	if( (ex->sample2.posbegin > ex->sample1.posbegin && ex->sample1.posend == ex->sample2.posend) ||
		(ex->sample2.posbegin == (ex->sample1.posend+1))) {
		if(ex->sample2.tags[0] == "fpa" || ex->sample2.tags[0] == "fc"){
			ret = 1;
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

void coref_fex::extract(struct EXAMPLE &ex, std::vector<int> *result){
	if(vectors & DIST)		result->push_back(get_dist(&ex));
	if(vectors & DIST)		result->push_back(get_dedist(&ex));
	if(vectors & IPRON)		result->push_back(get_i_pronoum(&ex));
	if(vectors & JPRON)		result->push_back(get_j_pronoum(&ex));
	if(vectors & IPRONM){
		result->push_back(get_i_pronoum_p(&ex));
		result->push_back(get_i_pronoum_d(&ex));
		result->push_back(get_i_pronoum_x(&ex));
		result->push_back(get_i_pronoum_i(&ex));
		result->push_back(get_i_pronoum_t(&ex));
		result->push_back(get_i_pronoum_r(&ex));
		result->push_back(get_i_pronoum_e(&ex));
	}
	if(vectors & JPRONM){
		result->push_back(get_j_pronoum_p(&ex));
		result->push_back(get_j_pronoum_d(&ex));
		result->push_back(get_j_pronoum_x(&ex));
		result->push_back(get_j_pronoum_i(&ex));
		result->push_back(get_j_pronoum_t(&ex));
		result->push_back(get_j_pronoum_r(&ex));
		result->push_back(get_j_pronoum_e(&ex));
	}
	if(vectors & STRMATCH)	result->push_back(get_str_match(&ex));
	if(vectors & DEFNP)		result->push_back(get_def_np(&ex));
	if(vectors & DEMNP)		result->push_back(get_dem_np(&ex));
	if(vectors & NUMBER)	result->push_back(get_number(&ex));
	if(vectors & GENDER)	result->push_back(get_gender(&ex));
	if(vectors & SEMCLASS)	result->push_back(get_semclass(&ex));
	if(vectors & PROPNAME)	result->push_back(get_proper_name(&ex));
	if(vectors & ALIAS){
		result->push_back(get_alias_acro(&ex));
		result->push_back(get_alias_fixleft(&ex));
		result->push_back(get_alias_fixright(&ex));
		result->push_back(get_alias_order(&ex));
	}
	if(vectors & APPOS)		result->push_back(get_appositive(&ex));
}

//////////////////////////////////////////////////////////////////
///    Configure the features to be extracted
//////////////////////////////////////////////////////////////////

void coref_fex::setVectors(int vec){
	vectors = vec;
}
