
#include "globalVars.h"
#include "kbGraph.h"

#include <string>
#include <iostream>
#include <fstream>


using namespace std;
using namespace ukb;

int main(int argc, char *argv[]) {

  if (argc!=3) {
    cerr<<"Usage: compile_ukb infile outfile"<<endl;
    exit(1);
  }  

  glVars::kb::v1_kb = false; // Use v2 format
  glVars::kb::filter_src = false; // by default, don't filter relations by src

  set<string> filter;
  Kb::create_from_txt(argv[1], filter);
  Kb::instance().add_comment("Created by FreeLing installer with compile_ukb");
  Kb::instance().write_to_binfile(argv[2]);
}

