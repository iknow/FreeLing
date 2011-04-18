
#include <string>
#include <iostream>
#include "socket.h"
#include "iso2utf.h"


using namespace std;

//------------------------------------------
void output_result (string r, bool utf) {
  if (utf) 
    cout<<Latin1toUTF8(r.c_str());
  else 
    cout<<r;
}


//------------------------------------------
int main(int argc, char *argv[]) {

  if (argc < 3 or (argc>=4 and (string(argv[3])!="--utf")) ) {
    cerr<<"usage: "<<string(argv[0])<<" hostname port [--utf]"<<endl;
    exit(0);
  }
  
  bool utf=false;
  if (argc>=4)
    utf = (string(argv[3])=="--utf");


  // connect to server  
  socket_CS sock(string(argv[1]),atoi(argv[2]));
  
  // send lines to the server, and get answers
  string s,r;
  while (getline(cin,s)) { 
    if (utf) s= utf8toLatin(s.c_str());

    sock.write_message(s);
    cerr<<"Enviat ["<<s<<"]"<<endl; 

    sock.read_message(r);
    cerr<<"Rebut ["<<r<<"]"<<endl; 

    if (r!="FL-SERVER-READY") 
      output_result(r,utf);
  }
  
  // input ended. Make sure to flush server's buffer
  sock.shutdown_connection(SHUT_WR);
  cerr<<"Enviat SHUT_WR"<<endl;
  sock.read_message(r);
  cerr<<"Rebut ["<<r<<"]"<<endl; 

  if (r!="FL-SERVER-READY")
    output_result(r,utf);

  // terminate connection
  sock.close_connection();
  return 0;
}
