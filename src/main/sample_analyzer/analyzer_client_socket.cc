
#include <string>
#include <iostream>
#include "socket.h"


using namespace std;

int main(int argc, char *argv[])
{

    if (argc < 3) {
      cerr<<"usage "<<string(argv[0])<<" hostname port"<<endl;
      exit(0);
    }
    
    socket_CS sock(string(argv[1]),atoi(argv[2]));

    string s,r;
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
