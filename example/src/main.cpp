/*
 * A Remote Debugger Example for SpiderMonkey Java Script Engine.
 * Copyright (C) 2014-2015 Slawomir Wojtasiak
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/**
 * Character encoding support is definitely broken here. It's quite complex
 * problem, so if you are interested how to handle it correctly head over
 * to the common utility code in $(top_srcdir)/utils
 */

#include <iostream>
#include <locale.h>
#include <stdlib.h>

//#include "JSThread.h"
#include "JSThread.cpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

//using namespace std;

static const int number_of_threads = 4;
static JSThread *threads[number_of_threads];

static void error(const char *msg)
{
    perror(msg);
    exit(1);
}


static int setupNetworkSocket(void) {
     int sockfd, newsockfd, portno;
     socklen_t clilen;
     char buffer[256];
     struct sockaddr_in serv_addr, cli_addr;
     int n;
	 
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = 8099;//atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     listen(sockfd,5);
     clilen = sizeof(cli_addr);
     newsockfd = accept(sockfd, 
                 (struct sockaddr *) &cli_addr, 
                 &clilen);
	 
     if (newsockfd < 0) 
          error("ERROR on accept");
	 return newsockfd;

	 //for read operation
     bzero(buffer,256);
     n = read(newsockfd,buffer,255);
     if (n < 0) error("ERROR reading from socket");
     printf("Here is the message: %s\n",buffer);
     n = write(newsockfd,"I got your message",18);
     if (n < 0) error("ERROR writing to socket");
     close(newsockfd);
     close(sockfd);
     return 0; 
}

static ssize_t
readLine(int fd, void *buffer, size_t n)
{
    ssize_t numRead;                    /* # of bytes fetched by last read() */
    size_t totRead;                     /* Total bytes read so far */
    char *buf;
    char ch;

    if (n <= 0 || buffer == NULL) {
        errno = EINVAL;
        return -1;
    }

    buf = (char *)buffer;                       /* No pointer arithmetic on "void *" */

    totRead = 0;
    for (;;) {
        numRead = read(fd, &ch, 1);

        if (numRead == -1) {
            if (errno == EINTR)         /* Interrupted --> restart read() */
                continue;
            else
                return -1;              /* Some other error */

        } else if (numRead == 0) {      /* EOF */
            if (totRead == 0)           /* No bytes read; return 0 */
                return 0;
            else                        /* Some bytes read; add '\0' */
                break;

        } else {                        /* 'numRead' must be 1 if we get here */
            if (totRead < n - 1) {      /* Discard > (n - 1) bytes */
                totRead++;
                *buf++ = ch;
            }

            if (ch == '\n')
                break;
        }
    }

    *buf = '\0';
    return totRead;
}

static void * helper_thread(void *arg) {
	//Init socket server
	setupsocket:
	cout<<"Setting up trigger socket"<<endl;
	int fd = setupNetworkSocket();

	while(true) {
		//Listen on socket for contextId
		//TODO:
		char buffer[256];// =  "ThreadX";
		
		int n = readLine(fd,buffer, 255);

     	if (n < 0) { 
			error("ERROR reading from socket");
			goto setupsocket;
     	}

		cout<<"ContextName from socket: " << buffer <<endl;
	 
		int threadId = buffer[6] - '0';
		
		threads[threadId]->ExecuteFunction(0, "processDebugCommand");
		//wait before next read and execute
		//usleep(1000*100);
	}
	
}

static void startHelperThread(void) {
	static pthread_t helper;
	int ret = pthread_create(&helper, NULL, &helper_thread, NULL);
}



int main(int argc, char **argv) { 
	;

	for(int i=0;i<number_of_threads;i++) {
		string name = "ThreadN";
		name[6] = '0' + i; 
		
		threads[i]= new JSThread(name);
	}

	int threadIndex, scriptIndex;
	string funcName;
	startHelperThread();

	while(true) {
		cout<<"Enter [<threadIndex> <scriptIndex> <funcName>]: ";
		cin>>threadIndex;
		cin>>scriptIndex;
		cin>>funcName;

		threads[threadIndex]->ExecuteFunction(scriptIndex, funcName);
	}
}

