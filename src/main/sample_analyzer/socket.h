#ifndef _SOCKET_CS
#define _SOCKET_CS

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <fries/language.h>

#define BUFF_SZ 2048
class socket_CS {
  private:
  int sock, sock2;
  void error(const std::string &,int) const;

  public:
    socket_CS(int);
    socket_CS(const std::string&, int);

    ~socket_CS();

    void wait_client();
    int read_message(std::string&);
    void write_message(const std::string &);
    void close_connection();
    void shutdown_connection(int);
};


void socket_CS::error(const std::string &msg, int code) const {
  perror(msg.c_str());
  exit(code);
}


socket_CS::socket_CS(int port) {

  struct sockaddr_in server;
  
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) error("ERROR opening socket", sock);
  int len=sizeof(server);
  bzero((char *) &server, len);
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(port);
  
  int n=bind(sock, (struct sockaddr *) &server,len);
  if (n < 0) error("ERROR on binding",n);
}



socket_CS::socket_CS(const std::string &host, int port) {

  struct sockaddr_in server;

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) error("ERROR opening socket",sock);

  struct hostent *hp = gethostbyname(host.c_str());
  if (hp == NULL) error("Unknown host",0);

  bzero((char *) &server, sizeof(server));
  server.sin_family = AF_INET;
  bcopy((char *)hp->h_addr,(char *)&server.sin_addr.s_addr,hp->h_length);
  server.sin_port = htons(port);

  int n=connect(sock,(struct sockaddr *) &server,sizeof(server));
  if (n < 0) error("ERROR connecting",n);

  sock2=sock;
}


socket_CS::~socket_CS() {}

void socket_CS::wait_client() {  
  struct sockaddr_in client;
  listen(sock,5);
  socklen_t len = sizeof(client);
  sock2 = accept(sock,(struct sockaddr *) &client, &len);
  if (sock2 < 0) error("ERROR on accept",sock2);
}


int socket_CS::read_message(std::string &s) {
    
  int n,nt;
  char buffer[BUFF_SZ+1];

  s.clear();
  n = read(sock2,buffer,BUFF_SZ);
  if (n < 0) error("ERROR reading from socket",n);
  buffer[n]=0;
  s = s + std::string(buffer);
  nt = n;
  while (n>0 and buffer[n-1]!=0) {
    n = read(sock2,buffer,BUFF_SZ);
    if (n < 0) error("ERROR reading from socket",n);
    buffer[n]=0;
    s=s + std::string(buffer);
    nt += n;
  }

  return nt;
}

void socket_CS::write_message(const std::string &s) {
  int n;
  n = write(sock2,s.c_str(),s.length()+1); 
  if (n < 0) error("ERROR writing to socket",n);
}


void socket_CS::close_connection() {
  int n;
  n=close(sock2);
  if (n < 0) error("ERROR closing socket",n);
}


void socket_CS::shutdown_connection(int how) {
  int n;
  n=shutdown(sock2,how);
  if (n < 0) error("ERROR shutting down socket",n);
}

#endif
