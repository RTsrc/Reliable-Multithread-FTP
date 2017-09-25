#include "packet.h"
using namespace std;
uint32_t bytesToInt(char* buff, int start) {
        ByteConverter bc;
        for (int i = 0; i < 4; i++){
                bc.bytes[i] = buff[start+i];
        }
        return bc.ui;
}

Packet buffToPacket(char* buff) {
        Packet p;
	string str(buff);
	//cerr << str << endl;
	padKey(p);
	//cerr << "Padded" << endl;
	if(buff[0]  == 'F') {
		strncpy(p.cmd,buff, 1);
	}else{
		//cerr << "copying cmd" << endl;
		strncpy(p.cmd,buff, 1);
		//cerr << "populating key" << endl;
        	strncpy(p.Key,buff+1, 8);
	//	cerr << "Key: " << p.Key << endl;
	}
     	//p.cmd[1] = '\0'; 
        return p;
}

Packet padKey(Packet &p){
	for (int i = 0; i <= 8; i++){
		p.Key[i] = '\0';
	}
	return p;
}
void PrintPacket(Packet &p) {
        cout << "Packet Cmd " << p.cmd[0] << endl;
        cout << "Packet Key: " << p.Key << endl;
 
}
char* getPayload(char* src, char* dest, int size) {
	strncpy(dest, src+sizeof(Packet), size);
	cout << "SRC: " << src << endl;
	return dest;
}

int searchKey(string Key, vector<DLKey> &db) {
	int i = 0;
	for (vector<DLKey>::iterator it = db.begin(); it != db.end(); ++it) {
		string skey(it->Key);
		if(it->Key == Key)
			return i;
		i++; 
	}
 	return -1;		
}
DLKey::DLKey(string key, int upfd, int dnfd){
	ULfd = upfd;
	DLfd = dnfd;
	Key = key;
}

DLKey::DLKey(string key) {
	Key = key;
	ULfd = -1;
	DLfd = -1;
}
