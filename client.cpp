#include <iostream>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <vector>
#include <stdio.h>
#include <unistd.h>
#include <fstream>
#include <arpa/inet.h>
#include <sstream>

using namespace std;

string *parseURL(string URL);

int main(int argc, char *argv[]){

	if(argc < 1){
		cout << "Specify atleast 1 URL" << "\n";
		return -1;	
	}
	
	vector<string> urlList;
	for(int i = 1; i < argc; i++){
		urlList.push_back(argv[i]);
	}
	
	for(unsigned int i = 0; i < urlList.size(); i++){
		string *value = parseURL(urlList.at(i));
		cout << "Link entered: " << urlList.at(i) << "\n";		

		int socketfd;
		struct addrinfo hints, *servinfo, *p;

		memset(&hints,0,sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;

		if(getaddrinfo(value[0].c_str(),value[1].c_str(),&hints,&servinfo) != 0){
			cout << "Error retrieving host" << "\n";
			return -1;	
		}
		
		sockaddr_in *ip;
		for(p = servinfo; p != NULL; p = p->ai_next){
			ip = (sockaddr_in *)p->ai_addr;
			if((socketfd = socket(p->ai_family,p->ai_socktype,p->ai_protocol)) == -1){
				continue;
			} 
			if(connect(socketfd,p->ai_addr,p->ai_addrlen) == -1){
				close(socketfd);
				continue;			
			}
			break;
		}

		if(p == NULL){
			cout << "Failed to connect: " << value[0] << "\n";
			return -1;
		}
		freeaddrinfo(servinfo);
		cout << "Connect successful: " << value[0] << " IP: " << inet_ntoa((in_addr)ip->sin_addr) << "\n";
	
		if(value[2].empty()){
			value[2] = "/";
		}
		//string message = "EAT  HTTP/1.0\r\nUser-Agent: Wget/1.17.1 (linux-gnu)\r\nAccept: */*\r\nAccept Encoding: identity\r\nHost: " + value[0] + "\r\nConnection: close\r\n\r\n";
		string message = "GET " + value[2] + " HTTP/1.0\r\n\r\n";
		//User-Agent: Wget/1.17.1 (linux-gnu)\r\nAccept: */*\r\nAccept Encoding: identity\r\nHost: " + value[0] + "\r\nConnection: close\r\n\r\n";
		//char message[] = "GET /index.html \0HTTP/1.0\r\n\r\n";
		//if(write(socketfd,message,sizeof(message)) < 0){
		if(write(socketfd,message.c_str(),message.length()) < 0){
			cout << "Error writing to socket" << "\n";
			close(socketfd);		
		}

		char buffer[512];
		char c;
		memset(buffer,0,sizeof(buffer));
		string reply;
		int count = 0;
		while((count = recv(socketfd,&c,sizeof(c),0)) > 0){
			reply += c;
			//memset(buffer,0,sizeof(buffer));
			if(reply.find("\r\n\r\n") != string::npos){
				break;
			}
		}
		cout << reply;
		stringstream linesplit(reply);
		string token;
		getline(linesplit,token,'\n');
		string statusNum;
		string delimiter = " ";
		size_t position = 0;
		int index = 0;
		string code;
		while((position = token.find(delimiter)) != string::npos && index != 2){
			code = token.substr(0,position);
			token.erase(0,position + delimiter.length());
			index++;
		}
		
		if(code != "200"){
			cout << "Failure code: " << code << "\n\n";
			continue;
		}

		position = reply.find("\r\n\r\n");
		/*if(position == string::npos){
			cout << "Malformed HTTP response\n\n";
			continue;
		}*/
		//cout << "\n";
		reply = reply.substr(position + 4, reply.length()-1);
		ofstream file1;
		if(value[2] == "/"){
			file1.open("index.html");
		}else{
			delimiter = "/";
			position = 0;
			string copy = value[2];
			while((position = copy.find(delimiter)) != string::npos){
				token = copy.substr(0,position);
				copy.erase(0,position + delimiter.length());
			}
			
			if(copy.empty() == true){
				file1.open(token + ".html", ios::out|ios::binary);
			}else{
				file1.open(copy, ios::out|ios::binary);
			}
		}
		
		memset(buffer,0,sizeof(buffer));
		while((count = recv(socketfd,buffer,sizeof(buffer),0)) > 0){
			file1.write(buffer,count);
			file1.flush();
			memset(buffer,0,sizeof(buffer));
		}

		file1.close(); 
		close(socketfd);
	}
}

string *parseURL(string URL){
	string *parsedURL = new string[3];
	char host [1000] = {};
	int port = 0;
	char path [1000] = {};
	
	sscanf(URL.c_str(),"http://%[^:]:%d%[^\n]",host,&port,path);
	parsedURL[0] = host;
	parsedURL[1] = to_string(port);
	parsedURL[2] = path;
	
	cout << "Host: " << parsedURL[0] << "\n";
	cout << "Port: " << parsedURL[1] << "\n";
	cout << "URL Path: " << parsedURL[2] << "\n";
	return parsedURL;
}
