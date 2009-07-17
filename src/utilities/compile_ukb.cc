
#include "globalVars.h"
#include "kbGraph.h"

#include <string>
#include <iostream>
#include <fstream>


using namespace std;
using namespace ukb;

int main(int argc, char *argv[]) {

  glVars::kb::v1_kb = false; // Use v2 format
  glVars::kb::filter_src = false; // by default, don't filter relations by src

  set<string> filter;

  cout<<"Loading file "<<string(argv[1])<<" to "<<argv[2]<<endl;
  Kb::create_from_txt(argv[1], filter);
  Kb::instance().add_comment("Created by FreeLing installer with compile_ukb");
  cout<<"Writting file "<<string(argv[2])<<endl;
  Kb::instance().write_to_binfile(argv[2]);
}

