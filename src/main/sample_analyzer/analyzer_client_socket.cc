
#include <string>
#include <iostream>
#include "socket.h"
#include "iso2utf.h"


using namespace std;

int main(int argc, char *argv[])
{

    if (argc < 3) {
      cerr<<"usage "<<string(argv[0])<<" hostname port [--utf]"<<endl;
      exit(0);
    }
    
    string option;
    
    if (argc==4) option=argv[3];
    
    socket_CS sock(string(argv[1]),atoi(argv[2]));

    string s,r;
    
    if (option!="--utf")
    {
    
    while (getline(cin,s)) {

      sock.write_message(s);

      sock.read_message(r);
      if (r!="FL-SERVER-READY") cout<<r;
    }

    if (r=="FL-SERVER-READY") {
      sock.shutdown_connection(SHUT_WR);
      sock.read_message(r);
      cout<<r;
    }
    sock.close_connection();
    return 0;
    }
    
    else{
	    
	while (getline(cin,s)){
		char *aux=(char*) utf8toLatin((char*) s.c_str());
	     	sock.write_message(aux);
      	     	sock.read_message(r);
             	if (r!="FL-SERVER-READY") cout<<Latin1toUTF8((char*) r.c_str());
	}

    	if (r=="FL-SERVER-READY") {
      		sock.shutdown_connection(SHUT_WR);
      		sock.read_message(r);
      		cout<<Latin1toUTF8((char*) r.c_str());
    	}
    	sock.close_connection();
    	return 0;
    }
    
}
