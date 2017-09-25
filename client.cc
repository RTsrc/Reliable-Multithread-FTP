#include "stdlib.h"
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
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
#include "packet.h"
#define MAXHOSTNAME 1024
#define MYPORT "0"
using namespace std;

int alarm_set = 0;

int main(int argc, char* argv[]){
	//check to prevent segfault
	if(argc <= 3) {
                cerr << "USAGE: server <host> <port> F" << endl;
                return 0;
        }

	//calculate virtual filesize

	long int vfilesize;
	char* hostname = argv[1];
	char* server_port = argv[2];
	char* filename;
	char* cmdKey;
	int type = -1;
	int payload_size;
	int wait_time = 0;	
	if(argc >=4) {
		//cerr << "Assigning Command Key" << endl;
		cmdKey = argv[3];
	}
	if(argc >=6) {
		payload_size = atoi(argv[5]);
		filename = argv[4];
		cmdKey = argv[3];
		char* temp;
        	vfilesize = strtol(filename, &temp, 10);
        	if(*temp != '\0'){
                	vfilesize = -1;
        	}
		type = 0;
	}

	if (argc == 7){
		wait_time = atoi(argv[6]);
		////cerr << "wait time: " << wait_time << endl;
		type = 1;
	}
	
	int sockfd, rv;
	struct addrinfo hints, *serverinfo, *res;
	struct sockaddr_in sa;
	struct hostent *srv;
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = PF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	rv = getaddrinfo(argv[1], argv[2], &hints, &res);


	if(rv !=0) {
		//fprintf(stderr, "getaddrinfo: %s\n",  gai_strerror(rv));
		return 1;
	}

	memset(&sa, 0, sizeof(struct sockaddr_in)); /* clear our address */
        srv = gethostbyname(hostname);
	sa.sin_family = srv-> h_addrtype;
	sa.sin_addr.s_addr = *((long*)srv->h_addr_list[0]);
        sa.sin_port= htons(atoi(server_port));
			 
	sockfd=socket(PF_INET, SOCK_STREAM, 0);

        int srvfd = connect(sockfd,(struct sockaddr *)&sa, sizeof(struct sockaddr_in));

	if(sockfd == -1) {
                perror("client: socket");
                exit(1);
        }

	if(srvfd == -1) {
            close(sockfd);
            perror("client: connect");
            exit(1);
        }
	
	struct sockaddr_in address;
	struct sockaddr_storage client_addr;
	
	socklen_t sin_size;
	int i = 0;
        char* payload;
	char lastmsg;
	int total_bytes = 0, msgnum = 0, lostnum = 0;
	int pktsize = sizeof(Packet) + payload_size + 1;

	if(type == 1) {
	//upload
		
		cerr << "Uploading..." << endl;
		Packet *p = new Packet;
        	//create a new pkt with the key and cmd
        	*p = buffToPacket(cmdKey);
		//PrintPacket(*p);
		int resu = send(sockfd, p, sizeof(Packet),0);

		if(vfilesize  >=  0){
			uint32_t fsize = vfilesize;
                	uint32_t nfsize = htonl(fsize);
                	//cout << "vfilesize: " << fsize << endl;
                	int resu2 = send(sockfd, &nfsize, sizeof(uint32_t),0);

			//Sending virtual file packets;
		 	char* datablock = new char[payload_size];
                        int resu;
                        while(vfilesize > 0) {
                                if(vfilesize - payload_size <=0) {
                                        lastmsg = 4;
                                } else {
                                        lastmsg = '\0';
                                }

				cerr << "Uploading Virtual File" << endl;
                                resu = send(sockfd, datablock, payload_size,0);
				usleep(wait_time);
				//check whether packet was sent
                                if(resu >0) {
                                        total_bytes += resu;
					msgnum +=1;
                                } else {
					cerr << "Failed to Send to Server!" << endl;
				}

				for(int i=0; i < payload_size; i++) {
                                        datablock[i] = '\0';
                                }

                                vfilesize -= payload_size;
                        }

                        delete[] datablock;

		} else {
		//create the filestream from the name
		ifstream myfile(filename, ifstream::binary);
		char* datablock = new char[payload_size];

        	if(myfile.is_open()) {
			//calculate the filesize
               		myfile.seekg(0, myfile.end);
                	int filesize= myfile.tellg();
                	myfile.seekg(0, myfile.beg);
			//char* datablock = new char[payload_size + 1];
			int resu, resu2;
			uint32_t fsize = filesize;
			uint32_t nfsize = htonl(fsize);
                        //cout << "filesize: " << fsize << endl;
                        resu2 = send(sockfd, &nfsize, sizeof(uint32_t),0);

			while(filesize >  0) {
				for(int i=0; i < payload_size; i++) {
                                        datablock[i] = '\0';
                                }
				//cout << "Remaining: " << filesize << endl;
  			 	int leftsize = min(filesize, payload_size);
				myfile.read(datablock, leftsize);
			
				/*if(filesize - payload_size < 0 && filesize > 0) {
					datablock[leftsize - 1] = 4;
				}*/
				resu = send(sockfd, datablock, leftsize, 0);
				//resu2 = send(sockfd,&nfsize, sizeof(uint32_t),0);
				usleep(wait_time);
				//cout << "Sent PKT of size " << resu << endl;
				//clear the char array
				if(resu >= 0) {
					total_bytes += resu;
                                        msgnum +=1;
				}else {
                                        cerr << "Failed to Send to Server!" << endl;
                                }


				filesize -= payload_size;
			}	 
			//cout << "Upload Completed!" << endl;
        	}	
	
		myfile.close();
		delete [] datablock;
		}
		delete p;
	}else if(type == 0) { 
	//download
		//cout << "Downloading..." << endl;
		ofstream myfile(filename, ofstream::binary);
		string str;
		Packet *p = new Packet;
		*p = buffToPacket(cmdKey);
		//PrintPacket(*p);
		int resu = send(sockfd, p, sizeof(Packet),0);
		char* buffer = new char[payload_size];
		int32_t filesize = payload_size;
		char fs[4];

		resu = recv(sockfd,fs, sizeof(uint32_t), 0);
                if( resu == sizeof(int32_t)){
                	filesize = htonl(bytesToInt(fs, 0));
                        //cout << "File Size is: " << filesize  << endl;
                }

                while(filesize > 0){
                        //string str;
                        //getline(cin, str);
			//cout << "Receiving..." << endl;
			/*int resu = recv(sockfd,fs, sizeof(uint32_t), 0);
			if( resu == sizeof(uint32_t)){
				filesize = htonl(bytesToInt(fs, 0));
				cout << "File Size is: " << filesize  << endl;
			}*/	
                        int num = recv(sockfd, buffer, payload_size, 0);
                        if(num == -1){
                        	perror("recv");
                        }else if(num == 0){
                        	cerr << "Got nothing from Server! Transfer Complete!" << endl;
				break;
                        } else {
				//cout  << "Got Response of size: " << num << endl;	
				lastmsg = buffer[num - 1];
				
				if(myfile.is_open()) {
                        		string datastr(buffer);
                                	//myfile << buffer;
					myfile.write(buffer, num);
		
                        	} else {
					cerr << "Couldn't open " << filename << "for writing!" << endl;
				}
	
				
                                total_bytes += num;
                                msgnum +=1;
                         	filesize -= num;       

                        }

                        //if last msg is EOT, terminate;
                
			for(int i=0; i < payload_size; i++) {
                        	buffer[i] = '\0';
                        }


                }
		delete p;
		delete [] buffer;
		myfile.close();	

	} else {
		cerr << "Terminating..." << endl;
                Packet *p = new Packet;
                *p = buffToPacket(cmdKey);
                //PrintPacket(*p);
                int resu = send(sockfd, p, sizeof(Packet),0);

	}
	freeaddrinfo(res);
	//OUTPUT
	//cout << msgnum << " " << total_bytes << endl;
	//close the socket
	close(sockfd);
				
	return 0;
}
