//////////////////////////////////////////////////////////////////
//
//    FreeLing - Open Source Language Analyzers
//
//    Copyright (C) 2004   TALP Research Center
//                         Universitat Politecnica de Catalunya
//
//    This library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU General Public
//    License as published by the Free Software Foundation; either
//    version 2.1 of the License, or (at your option) any later version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    General Public License for more details.
//
//    You should have received a copy of the GNU General Public
//    License along with this library; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
//    contact: Lluis Padro (padro@lsi.upc.es)
//             TALP Research Center
//             despatx C6.212 - Campus Nord UPC
//             08034 Barcelona.  SPAIN
//
////////////////////////////////////////////////////////////////


//------------------------------------------------------------------//
//
//                    IMPORTANT NOTICE
//
//  This file contains a simple main program to illustrate 
//  usage of FreeLing analyzers library.
//
//  This sample main program may be used straightforwardly as 
//  a basic front-end for the analyzers (e.g. to analyze corpora)
//
//  Neverthless, if you want embed the FreeLing libraries inside
//  a larger application, or you want to deal with other 
//  input/output formats (e.g. XML), the efficient and elegant 
//  way to do so is consider this file as a mere example, and call 
//  the library from your your own main code.
//
//------------------------------------------------------------------//


#include <fstream>
#include <iostream>
#include <string>
#include <pthread.h>
#include "iso2utf.h"

using namespace std;

/// ----------------------------------------------
/// read lines from cin and send them to server

void * send_requests(void *fname) {

  // open server input pipe to write requests
  ofstream fin(((string *)fname)->c_str()); 

  // read all input from cin and send it to the server
  string line;
  while (getline(cin,line)) fin << line << endl;  
  fin.close();

  return(NULL); //shut up compiler
}


/// ----------------------------------------------
/// read replies from server and send them to cout

void * get_replies(void *fname) {

 // open server output pipe to read answers
  ifstream fout(((string *)fname)->c_str());

  // get all output from the server and write it to cout
  string line;
  while (getline(fout,line)) cout << line <<endl;  
  fout.close(); 

  return(NULL); //shut up compiler
}



/// ----------------------------------------------
/// read lines from cin and send them to server

void * send_requestsUTF(void *fname) {

  // open server input pipe to write requests
  ofstream fin(((string *)fname)->c_str()); 

  // read all input from cin and send it to the server
  string line;
  while (getline(cin,line)){
	  //unsigned char* utf8toLatin ( char* cadena) {
	  char* aux=utf8toLatin((char*) line.c_str());
	  line=aux;
	  fin << line << endl;  
	  free(aux);
  }
  fin.close();

  return(NULL); //shut up compiler
}


/// ----------------------------------------------
/// read replies from server and send them to cout

void * get_repliesUTF(void *fname) {

 // open server output pipe to read answers
  ifstream fout(((string *)fname)->c_str());

  // get all output from the server and write it to cout
  string line;
  while (getline(fout,line)) {
	  //string Latin1toUTF8( const char* szStr )
	  string aux=Latin1toUTF8(line.c_str());
	  cout << aux <<endl;
  }
  fout.close(); 

  return(NULL); //shut up compiler
}



/// ----------------------------------------------
/// main: just start reader and writer threads

int main(int argc, char **argv) {

		
  // use first parameter as the name for the named pipe
  string FIFO_in = string(argv[1])+".in";
  string FIFO_out = string(argv[1])+".out";
  
  string option;
  
  if (argc==3) option=argv[2];
  
  pthread_t th_reader, th_writer;
  
  if (option=="--utf"){
	  
	
    	/* Create independent threads each of which will execute function */  
  	pthread_create( &th_reader, NULL, send_requestsUTF, (void *) &FIFO_in);
  	pthread_create( &th_writer, NULL, get_repliesUTF, (void *) &FIFO_out);
  }
  
  else{ 
	  
	/* Create independent threads each of which will execute function */  
  	pthread_create( &th_reader, NULL, send_requests, (void *) &FIFO_in);
  	pthread_create( &th_writer, NULL, get_replies, (void *) &FIFO_out);
  }
  	
  /* Wait till threads are complete before main continues. Unless we  */
  /* wait we run the risk of executing an exit which will terminate   */
  /* the process and all threads before the threads have completed.   */  
  pthread_join(th_reader, NULL);
  pthread_join(th_writer, NULL); 
  
}


