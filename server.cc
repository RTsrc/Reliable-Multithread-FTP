#include "stdlib.h"
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <errno.h>
#include <sstream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <algorithm>
#include "packet.h"
#define MAX_CONNUM 100
#define MAXHOSTNAME 1024
#define MYPORT "0"
#define MAXMSGSIZE 4096
using namespace std;

//function from beej.us
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void sigchld_handler(int s) {
	int saved_errno = errno;
	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}

string toTitleCase(string s){ //server-side string logic
	
	string str = s;
	int n = s.length();
	for(int i =0; i < n; i++){
		if(i==0 || isspace(str[i-1])){
			str[i] = toupper(str[i]);
		}else if(isupper(str[i])){
			str[i] = tolower(str[i]);
		}
	}
	return str;
}
int isUp = 0;
int isDown = 0;
vector<int>DLfds;
vector<int>ULfds;
vector<int>Socklst;
vector <DLKey>DB;
vector <DLKey>UB;
vector <pthread_t*>thread_lst;
string Key;
pthread_mutex_t mutexnum;

typedef struct Args {
    int sendfd;
    int recvfd;
}Args;

void *SendFile(void *arguments) {
//get the args
	Args *args = (struct Args*)arguments;
	int sendfd = args->sendfd;
	int recvfd = args->recvfd;
	int num;
	//cout << "send to " << sendfd << endl;
 	//cout << "recv from " << recvfd << endl; 
	char* buffer = new char[MAXMSGSIZE];
	char* filelen = new char[4];
	uint32_t flen = 0;
	int terminate = 0;
	int resu = recv(recvfd, filelen, sizeof(uint32_t), 0);
                if(resu == sizeof(uint32_t)) {
                        flen = bytesToInt(filelen, 0);
                        uint32_t flen2 = ntohl(flen);
                        //cout << "Filesize: " << flen2 << endl;
                        if (send(sendfd,&flen, resu, 0) == -1){
                                perror("send");
                }

       	}
	
	while(!terminate && flen > 0){

		num = recv(recvfd, buffer, MAXMSGSIZE, 0);
        	if( num > 0 ) {
                	//cerr << "Forwarding PKT of size "  << num << endl;
                	if (send(sendfd,buffer, num, 0) == -1){
                        	perror("send");
                        }
			flen -= num;
                        	//cerr << "Payload Sent! " << buffer << "to socket" << i <<  endl;
		} else {
                	cerr << "Downloader Finished!" << endl;
			terminate = 1;
			break;
                }
		for(int j=0; j < MAXMSGSIZE; j++) {
                	//cerr<< "j: " << j << endl;
                        buffer[j] = '\0';
                }

	
       }
      /* buffer[0] = 4;
	cerr << "Done Sending... Notify Termination" << endl;
       int last = send(sendfd,buffer, 0, 0);*/
	delete [] buffer;
	delete [] filelen;
	pthread_mutex_lock(&mutexnum);
	ULfds.erase(remove(ULfds.begin(), ULfds.end(), recvfd), ULfds.end());
	pthread_mutex_unlock (&mutexnum);
}

int main(){
	char server_addr[MAXHOSTNAME+1];	 
	string server_port= MYPORT;
	gethostname(server_addr, MAXHOSTNAME); 
	stringstream ss;
	char s[INET_ADDRSTRLEN];
	int yes =1;
	int sockfd, newfd, rv, DLfd, ULfd;
	struct addrinfo hints, *serverinfo, *p;	
	fd_set master;    // master file descriptor list
    	fd_set read_fds;  // temp file descriptor list for select()
    	int fdmax;        // maximum file descriptor number
	FD_ZERO(&master);    // clear the master and temp sets
    	FD_ZERO(&read_fds);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	rv = getaddrinfo(NULL, MYPORT, &hints, &serverinfo);

	int terminate = 0;
	if(rv !=0) {
		//fprintf(stderr, "getaddrinfo: %s\n",  gai_strerror(rv));
		return 1;
	}
	
	for(p = serverinfo; p != NULL; p = p->ai_next){

		sockfd = socket(p->ai_family , p->ai_socktype , INADDR_ANY);	
		if(sockfd == -1){
			perror("server: socket");
			continue;
		}
		if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}
		if(bind(sockfd, p->ai_addr, p-> ai_addrlen) == -1){
			close(sockfd);
			perror("server: bind");
			exit(1);
		}
		break;
			
	}
	
	/*sockfd = socket(serverinfo->ai_family , SOCK_STREAM , 0);

        if(sockfd == -1){
                perror("server: socket");
        }
        if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
                perror("setsockopt");
                exit(1);
        }
        if(bind(sockfd, serverinfo->ai_addr, serverinfo-> ai_addrlen) == -1){
                close(sockfd);
                perror("server: bind");
                exit(1);
        }*/


	freeaddrinfo(serverinfo);

	if(listen(sockfd,MAX_CONNUM) == -1) {
                perror("listen");
                exit(3);
        }

	FD_SET(sockfd, &master);
	fdmax = sockfd;

	struct sockaddr_in address;
	struct sockaddr_storage client_addr;

	socklen_t sin_size;
	//pid_t pid = getpid();
	if(getsockname(sockfd, (struct sockaddr*)&address, &sin_size) ==-1){
		perror("getsockname");
		return -1;
	}else{
		//server_port = address.sin_port;
		
		cout << server_addr << endl;
        	//cout << "SERVER_PORT " << ntohs(address.sin_port) << endl;
		int portnum = ntohs(address.sin_port);
                ofstream portfile("port");
                portfile << portnum << endl;
                portfile.close();
                cout << portnum << endl;
	}
	//printf("server: waiting for connections...\n");
	int i;
	//string Key;
	DLfd = -1;
	while(!terminate) {
		read_fds = master;
		char* buffer = new char[MAXMSGSIZE]; //store received message in this buffer
		char* response = new char[MAXMSGSIZE]; //store the payload in this buffer
		//cerr <<  "Waiting for Connections..." << endl;
		if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            		perror("select");
        	}
		
		for(i = 0; i <= fdmax; i++) {
            		if (FD_ISSET(i, &read_fds)) { // we got one!!
                		if (i == sockfd) {
					sin_size = sizeof client_addr;
					newfd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
        				if (newfd == -1) {
            					perror("accept");
            					continue;
        				}else{
						FD_SET(newfd, &master); // add to master set
                        			if (newfd > fdmax) {    // keep track of the max
                            				fdmax = newfd;
                        			}
                            			inet_ntop(client_addr.ss_family,(struct sockaddr *)&client_addr,s, sizeof s);
                 				//printf("server: got connection from %s\n", s);
						Socklst.push_back(newfd);
						

					}
				} else {
					pthread_mutex_lock(&mutexnum);
					if(find(ULfds.begin(), ULfds.end(), i) != ULfds.end() || i == ULfd ){
						pthread_mutex_unlock(&mutexnum);
						continue; 
					}
					pthread_mutex_unlock(&mutexnum);
					//cout << "Receiving..." << endl;
					int num = recv(i, buffer, MAXMSGSIZE, 0);
					
					if(num <=0){
						if(num == -1){
                                        		perror("recv");
                                        		break;
                                		}
						if(num == 0){
                                        		//printf("selectserver: socket %d hung up\n", i);
						}
						//ULfds.erase(remove(ULfds.begin(), ULfds.end(), ULfd), ULfds.end());
						close(i); // close the lost connection
                        			FD_CLR(i, &master);
                                	} else {
						//if(pid) cerr << "Got something from sock " << i << endl;
					if(num  == sizeof(Packet))  {
							Packet *content = new Packet;
							*content = buffToPacket(buffer);
                                			//PrintPacket(*content);
			
						if(content->cmd[0] == 'P' && !isUp) {
								//declare a new thread for listening to DLs
								//cerr << "clear the response buffer" << endl;
								for(int j=0; j < MAXMSGSIZE; j++) {
									//cerr<< "j: " << j << endl;
                                                        		buffer[j] = '\0';
                                                		}
								
								//cerr << "comparing keys" << endl;
								string uKey(content->Key);
                                                                string dKey = Key;
								DLKey upkey = DLKey(uKey);
								upkey.ULfd = i;		
                                                                
									int index = searchKey(uKey,DB); 
									if(index >= 0) {
                                                                        	//cout << " Got a matching Key: " << uKey << endl;
										ULfd = i;
										int num;
										DLKey dkey = DB.at(index);
                                                                                DLfd = dkey.DLfd;
										Args arglst;
										arglst.sendfd = DLfd;
										arglst.recvfd = i;
										ULfds.push_back(i);
										pthread_t *t = new pthread_t;
                                                                		if(pthread_create(t, NULL, SendFile, (void*)&arglst)){
                                                                        		exit(1);
                                                                		}
										thread_lst.push_back(t);

										/*while(num = recv(ULfd, buffer, MAXMSGSIZE, 0)){
                                                                                	if( num > 0 ) {
                                                                                  
												cerr << "Forwarding PKT of size "  << num << "to sock " << DLfd <<  endl;
                                                                                        	if (send(DLfd,buffer, num, 0) == -1){
                                                                                                	perror("send");
                                                                                        	}
                                                                                        	if(buffer[num - 1] == '\0') cerr << "Last PKT Sent!" << endl;
                                                                                        //cerr << "Payload Sent! " << buffer << "to socket" << i <<  endl;
                                                                                	} else {
                                                                                        	cerr << "Downloader Hung Up!" << endl;
                                                                                	}
                                                                        	}*/
                                                                        	//cerr << "Server has finished forwarding. Exit!" << endl;
										DB.erase(DB.begin() + index);
										DLfds.erase(remove(DLfds.begin(), DLfds.end(), DLfd), DLfds.end());
										//FD_CLR(i, &master);
									} else {
										//cout << "CProc: " << getpid() << " Keys didn't match! " << " Expected: " << uKey << endl;
										UB.push_back(upkey);
										ULfds.push_back(i);	
									}
								
						}

						if(content->cmd[0] == 'G' && !isDown) {
								string dKey(content->Key);
								string uKey(Key);
								DLKey dkey = DLKey(dKey);
                                                                dkey.DLfd = i;

                                                                int index = searchKey(dKey,UB);

								for(int j=0; j < MAXMSGSIZE; j++) {
                                                                        buffer[j] = '\0';
                                                                }
	
								if(index >= 0) {
									
									//cout << "Proc:" << getpid() << " Got a matching Key: " << dKey << endl;
									int num;
									DLKey ukey = UB.at(index);
                                                                        ULfd = ukey.ULfd;
									Args arglst;
                                                                        arglst.recvfd = ULfd;
                                                                        arglst.sendfd = i;
									ULfds.push_back(i);
									//cerr << "Receiving from socket " << ULfd << endl;
									pthread_t *t = new pthread_t;
                                                                        if(pthread_create(t, NULL, SendFile, (void*)&arglst)){
                                                                           	exit(1);
                                                                        }
                                                                        thread_lst.push_back(t);
								
									//cout << "Server has finished forwarding. Exit!" << endl;
                                                                        UB.erase(UB.begin() + index);
                                                                        //ULfds.erase(remove(ULfds.begin(), ULfds.end(), ULfd), ULfds.end());
									//FD_CLR(i, &master);

								} else {
										//cerr << "CProc: " << getpid() << "Keys didn't match! " << " Expected: " << dKey << endl;
										DB.push_back(dkey);
										DLfds.push_back(i);
								}	
								
						}
						if(content->cmd[0] == 'F') {
							terminate = 1;
							int status = 1;
							//cerr << "waiting for " << thread_lst.size() << " threads to finish" << endl;
							for (std::vector<pthread_t*>::iterator it = thread_lst.begin() ; it != thread_lst.end(); ++it)
								pthread_join(**it, NULL);

							//close all pending connections
							for (std::vector<int>::iterator it = Socklst.begin() ; it != Socklst.end(); ++it)
								close(*it);	

						}
					}
						//delete [] response;	
					}	
						 						
				}
			}
		}
		delete [] buffer;
		delete [] response;
		}
		//CLEANUP
	//cerr << "deleting thread pointers" << endl;
        for (std::vector<pthread_t*>::iterator it = thread_lst.begin() ; it != thread_lst.end(); ++it)
                delete  *it;

	//delete [] thread_lst;
	close(newfd);
	close(sockfd);
	return 0;
}
