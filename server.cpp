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
#include <thread>
#include <netdb.h>

using namespace std;
class clientInfo{
	public:
	struct sockaddr_in clientSocket;
	int clientSocketfd;
	string fileDirectory;
	clientInfo(struct sockaddr_in,int,string);
};

clientInfo::clientInfo(struct sockaddr_in clientInput, int clientSocketfdInput, string fileDirectoryInput){
	clientSocket = clientInput;
	clientSocketfd = clientSocketfdInput;
	fileDirectory = fileDirectoryInput;
}

void serveClient(clientInfo client){
	char ipstr[INET_ADDRSTRLEN] = {'\0'};
	inet_ntop(client.clientSocket.sin_family, &client.clientSocket.sin_addr, ipstr, sizeof(ipstr));
	std::cout << "Accept a connection from: " << ipstr << ":" << ntohs(client.clientSocket.sin_port) << std::endl;

	// read/write data from/into the connection
	char buffer[512];
	memset(buffer,0,sizeof(buffer));
	string reply;
	int count = 0;
	while((count = recv(client.clientSocketfd,buffer,sizeof(buffer),0)) > 0){
		for(int j = 0; j < count; j++){
			reply += buffer[j];
		}
		memset(buffer,0,sizeof(buffer));
		if(reply.find("\r") != string::npos){
			break;
		}
	}
	//cout << reply;
	string delimiter = " ";
	size_t position = 0;
	int index = 0;
	string token,type,url,version;
	bool nullCheck = false;
	int spaceCount = 0;
	
	for(unsigned int i = 0 ;i < reply.length(); i++){
		if(reply[i] == '\0'){
			nullCheck = true;
		}
	}
	
	if(nullCheck == false){
		position = reply.find("\r");
		token = reply.substr(0,position);
	
		//Remove escape characters except null
		for(unsigned int i = 0 ; i < token.length(); i++){
			if(token[i] > 0 && token[i] < 32){
				token[i] = ' ';
			}
		}


		string spaceCheck = token;
		//Check if there are over 3 parameters in the request line
		while((position = spaceCheck.find(delimiter)) != string::npos){
			if(position != 0){
				spaceCount++;
			}
			spaceCheck.erase(0,position + delimiter.length());
		}
		//Grab the method and URL
		while((position = token.find(delimiter)) != string::npos && index <= 1){
			if(position != 0){
				if(index == 0){
					type = token.substr(0,position);
				}else if(index == 1){
					url = token.substr(0,position);
				}
				index++;
			}
			//Google doesn't allow start with space for request
			if(position == 0 && index == 0){
				break;
			}
			token.erase(0,position + delimiter.length());
		}
	
		//Remove the spaces arounded version
		for(unsigned int i = 0 ; i < token.length(); i++){
			if(token[i] != ' '){
				version+= token[i];
			}
			if(token[i] == ' ' && i != 0 && token[i-1] != ' '){
				break;
			
			}
		}
		//cout << type << " " << url << " " << version << "\n";
	}
	//Combine the path
	string completepath = client.fileDirectory+url;
	cout << "URL Received: " << completepath << endl;
    
	//Open the File
	ifstream file(completepath, ios::in|ios::binary);
	string message ="";
	if(nullCheck == true){
		message = "HTTP/1.0 400 Bad Request\r\n\r\n";
	}else if(file.is_open() == false){
		message = "HTTP/1.0 404 Not Found\r\n\r\n";
	}else if(url == "/"){
		message = "HTTP/1.0 404 Not Found\r\n\r\n";
	}else if(url.empty() == true || spaceCount > 3){
		message = "HTTP/1.0 400 Bad Request\r\n\r\n";
	}else if(url.empty() == false && url[0] != '/'){
		message = "HTTP/1.0 400 Bad Request\r\n\r\n";
	}else if(type != "GET"){
		message = "HTTP/1.0 501 Not Implemented\r\n\r\n";
	}else if(version != "HTTP/1.0"){
		message = "HTTP/1.0 505 HTTP Version not supported\r\n\r\n";
	}
	
	if(message.empty() == false){
		send(client.clientSocketfd,message.c_str(),message.length(),0);
	}else{
		//if(file.is_open()){
		//Get the body of the file
		file.seekg(0, file.end);
		int size = file.tellg();
		char buffer[1024] = {'\0'};
		file.seekg(0, file.beg);
		int count = 0;
		//message = "HTTP/1.0 200 OK\r\n\r\n"; //Malformed Response
		message = "HTTP/1.0 200 OK\r\n\r\n";
		if(write(client.clientSocketfd,message.c_str(),message.length()) < 0){
			cout << "Error writing to socket" <<"\n";
			close(client.clientSocketfd);
			return;
		}
		//Try sending the body of the file to client
		while((size - count) > 0){
			int read = 0;
			file.read(buffer, sizeof(buffer));
			read = file.gcount();
			count += read;
			//cout << size << " " << count << "\n";
			if(write(client.clientSocketfd,buffer,read) < 0){
				cout << "Error writing to socket" <<"\n";
				break;
			}
			memset(buffer,0,sizeof(buffer));
		}
		file.close();
     
			
	}
	//cout << "Thread exiting-Client served" << "\n";
	close(client.clientSocketfd);
}


int main(int argc, char* argv[]){
	if(argc != 1 && argc != 4){
		cout << "Invalid input" << "\n";
		return -1;
	}
   	 
	string hostname = "localhost";
	int portnumber = 4000;
	string filedirectory = ".";
    	struct hostent *he = gethostbyname("localhost");
	if(argc == 4){
		he = gethostbyname(argv[1]);
		if(he == NULL){
			cout << "Bad name" << "\n";
			return -1;
		}
		portnumber = atoi(argv[2]);
		filedirectory = argv[3];
	}
	// create a socket using TCP IP
	int socketfd = socket(AF_INET, SOCK_STREAM, 0);

	// allow others to reuse the address
	int yes = 1;
	if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		cout << "Error setting socket option" << "\n";
		return -1;
	}

	// bind address to socket
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(portnumber);     // short, network byte order
	memcpy(&addr.sin_addr,he->h_addr_list[0],he->h_length);
	memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

	if (::bind(socketfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		cout << "Could not bind to socket" << "\n";
		return -2;
	}
	// set socket to listen status
	if (listen(socketfd, 1) == -1) {
		cout << "Could not listen on port" << "\n";
		return -3;
	}
	// accept a new connection
	while(1){
		struct sockaddr_in clientAddr;
		socklen_t clientAddrSize = sizeof(clientAddr);
		int clientSocketfd = accept(socketfd, (struct sockaddr*)&clientAddr, &clientAddrSize);
	
		if (clientSocketfd == -1) {
			cout << "Could not connect with client" << "\n";
			return -1;
		}
		//cout << "Spawning thread" << "\n";
		clientInfo client(clientAddr,clientSocketfd,filedirectory);
		thread serve(serveClient,client);
		serve.detach();
	}

	/*char ipstr[INET_ADDRSTRLEN] = {'\0'};
	inet_ntop(clientAddr.sin_family, &clientAddr.sin_addr, ipstr, sizeof(ipstr));
	std::cout << "Accept a connection from: " << ipstr << ":" << ntohs(clientAddr.sin_port) << std::endl;

	// read/write data from/into the connection
	char buffer[512];
	memset(buffer,0,sizeof(buffer));
	string reply;
	int count = 0;
	while((count = recv(clientSocketfd,buffer,sizeof(buffer),0)) > 0){
		for(int j = 0; j < count; j++){
			reply += buffer[j];
		}
		memset(buffer,0,sizeof(buffer));
		if(reply.find("\r\n\r\n") != string::npos){
			break;
		}
	}
	cout << reply;
	stringstream linesplit(reply);
	string token;
	getline(linesplit,token,'\n');
	string delimiter = " ";
	size_t position = 0;
	int index = 0;
	string type,url,version;

	position = reply.find("\n");
	while((position = token.find(delimiter)) != string::npos){
		if(index == 0){
			type = token.substr(0,position);
		}else if(index == 1){
			url = token.substr(0,position);
		}
		token.erase(0,position + delimiter.length());
		index++;
	}
	token.erase(token.length()-1);
	version = token;
	
	string message ="";
	if(type != "GET"){
		message = "HTTP/1.0 501 Not Implemented\r\n\r\n";
	}else if(version != "HTTP/1.0"){
		message = "HTTP/1.0 505 HTTP Version not supported\r\n\r\n";
	}else if(url == "/"){
		message = "HTTP/1.0 404 Not Found\r\n\r\n";
	}
	
	if(message.empty() == false){
		send(clientSocketfd,message.c_str(),message.length(),0);
	}else{
		//Combine the path
		string completepath = filedirectory+url;
		std::cout << "Receiving a file from: " << completepath << endl;
    
		//Open the File
		ifstream file(completepath, ios::in|ios::binary);
     	
		if(file.is_open()){
			//Get the body of the file
			file.seekg(0, file.end);
			int size = file.tellg();
			char* buffer = new char [size];
			file.seekg(0, file.beg);
			int count = 0;
			//message = "HTTP/1.0 200 OK\r\n\r\n"; //Malformed Response
			message = "HTTP/1.0 200 OK\r\n\r\n";
			if(write(clientSocketfd,message.c_str(),message.length()) < 0){
				cout << "Error writing to socket" <<"\n";
				close(clientSocketfd);
				return 0;
			}
			//Try sending the body of the file to client
			while((size - count) > 0){
				file.read(buffer, size);
				count += file.gcount();
				if(write(clientSocketfd,buffer,size) < 0){
					cout << "Error writing to socket" <<"\n";
					break;
				}
				//cout << "test loop" << "\n";
			}
			file.close();
     
			//Get the body of the file using stat()
			struct stat buff;
			int rc = stat(completepath, &buff);
			int size = buff.st_size;
     	   		char* buffer = new char [size];
			file.read (buffer, size);
			file.close();
			
			}else{
				message = "HTTP/1.0 404 Not Found\r\n\r\n";   //ERROR: Can't Open file
				send(clientSocketfd,message.c_str(),message.length(),0);
		}
	}

	CHANGES ENDED
	close(clientSocketfd);*/
	return 0;
}

