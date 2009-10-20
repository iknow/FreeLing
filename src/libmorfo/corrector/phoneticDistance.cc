#include <iostream>
#include "freeling/phoneticDistance.h"
#include "freeling/traces.h"

#define MOD_TRACENAME "PHONETICDISTANCE"
#define MOD_TRACECODE PHONETICDISTANCE_TRACE


///////////////////////////////////////////////////////////////
///  Creator
///////////////////////////////////////////////////////////////

phoneticDistance::phoneticDistance(string featuresFile) {
  ifstream F;
  F.open(featuresFile.c_str()); 
  if(!F) { ERROR_CRASH("Error opening file "+featuresFile);}
  F.close();
  al = new aligner<int>(featuresFile,10);
  
}


///////////////////////////////////////////////////////////////
///  Destroy 
///////////////////////////////////////////////////////////////

phoneticDistance::~phoneticDistance(){

  delete al;
 
}

///////////////////////////////////////////////////////////////
///  Returns the phonetic distance between two words 
///////////////////////////////////////////////////////////////

int phoneticDistance::getPhoneticDistance(string word1, string word2){
   	
  char* a=(char *) word1.c_str();
  char* b=(char *) word2.c_str();
  aligner<int>::alin* result = al->align(a,strlen(a),b,strlen(b),GLOBAL);
  int resultado=result->score;
  delete(result);
  TRACE(4,"word1: "+word1+" word2: "+word2+" result: "+resultado);
  return resultado;
}

