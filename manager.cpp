#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <iostream>
#include <string>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <limits.h>
#include <netdb.h>
#include <fstream>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <ctime>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cstdlib>
#include <sys/stat.h>

using namespace std;
using namespace boost;

#define PORT_NUMBER 20001
#define MESSAGE_SIZE 140
#define VERSION 457
int NUM_NODES;
int DEBUG = 1;
int MAX_CONNECTION_LENGTH = 8;

struct packet{
	short version;
	short length;
	char message[MESSAGE_SIZE];
};	

int printHelpMessage() {
    cout << endl << "manager help:" << endl << endl;
    cout << "$manager <input file>" << endl;
    cout << "input file: network topology description" << endl;
	exit(EXIT_FAILURE);
    return -1;
}

void printTopology(int numNodes, vector<string>* topology){
	cout << "Number of nodes: " << numNodes << endl;
	for(unsigned int i = 0; i < (*topology).size(); i++){
		cout << (*topology).at(i) << endl;
	}
}

vector<string> readTopologyFile(string* filename){

	ifstream file;
    file.open(*filename);
	file >> NUM_NODES;
	char tempArray[1];
	file.getline(tempArray, 1);

	vector<string> topology;
	bool eof = false;
	while(eof == false){
		char connection[MAX_CONNECTION_LENGTH];
		file.getline(connection, MAX_CONNECTION_LENGTH);
		if(connection[0] == '-'){ eof = true; }
		else{
			string temp(connection);
			topology.push_back(temp);
		}
	}
	if(DEBUG){ printTopology(NUM_NODES, &topology); } 
	
	return topology;
}

void parentProcess(pid_t pid, int fd){
	int pidParent = getpid();
	string pidVal = to_string(pidParent);
	string temp = "parent process " + pidVal + "\n";
	write(fd, temp.c_str(), temp.size());
}

void childProcess(pid_t pid, int fd){
	int pidChild = getpid();
	string pidVal = to_string(pidChild);
	string temp = "child process " + pidVal + "\n";
	write(fd, temp.c_str(), temp.size());
}

int run_server()
{
	char host_name[HOST_NAME_MAX]; //character array for host name
	gethostname(host_name, sizeof(host_name)); //read host name into char array
	struct hostent* host_info = gethostbyname(host_name); //get IP address
	struct sockaddr_in server_address = {}; //initialize server sockaddr structure
	server_address.sin_family = AF_INET; //set family
	server_address.sin_port = htons(PORT_NUMBER); //convert and set port
	bcopy(host_info->h_addr, (char*)&server_address.sin_addr, host_info->h_length); //set ip address

	char ip_address[INET_ADDRSTRLEN]; //character array for ip address
	inet_ntop(AF_INET, &server_address.sin_addr, ip_address, sizeof(ip_address));	//convert binary IP to string
	printf("Welcome to Chat!\n");
	printf("Waiting for a connection on %s port %i\n", ip_address, PORT_NUMBER);

	int file_descriptor = socket(AF_INET, SOCK_STREAM, 0); //create socket
	if(file_descriptor == -1){
		printf("Error: Could not create server socket.\n");
		return -1;
	}

	int bind_result = bind(file_descriptor, (struct sockaddr*)&server_address, sizeof(server_address)); //bind socket to port
	if(bind_result == -1){
		printf("Error: Could not bind server socket.\n");
		return -1;
	}

	int listen_result = listen(file_descriptor, 1); //start listening
	if(listen_result == -1){
		printf("Error: Could not begin listening.\n");
		return -1;
	}

	socklen_t server_address_len = sizeof(server_address); //get length of socket
	int connection_descriptor = accept(file_descriptor, (struct sockaddr*)&server_address, &server_address_len); //connect to client
	if(connection_descriptor == -1){
		printf("Error: Could not accept.\n");
		return -1;
	}

	struct packet receive_packet = {}; //buffer to receive message
	char* receive_array = reinterpret_cast<char*>(&receive_packet); //cast the packet to a char* so that it can be used by recv
	int receive_result = recv(connection_descriptor, receive_array, sizeof(receive_packet), 0); //receive message
	receive_packet.version = ntohs(receive_packet.version); //change version back to host byte order
	receive_packet.length = ntohs(receive_packet.length); //change length back to host by order
	if(receive_result == -1){
		printf("Error: Could not receive from client.\n");
		return -1;
	}
	printf("Client: %s\n", receive_packet.message); //print message from client

	char send_buffer[MESSAGE_SIZE]; //buffer to send message
	memset(send_buffer, 0, sizeof(send_buffer)); //set the buffer to zeros
	string temp = "hello from the server";
	for(unsigned int i = 0; i < temp.size(); i++){ //copy string into char array
		send_buffer[i] = temp.at(i);
	}
	struct packet send_packet = {}; //create packet to send
	send_packet.version = htons(VERSION); //set version
	send_packet.length = htons(temp.length()); //set length
	bcopy(send_buffer, send_packet.message, sizeof(send_packet.message)); //set message
	const char* send_array = reinterpret_cast<const char*>(&send_packet);
	int send_result = send(connection_descriptor, send_array, sizeof(send_packet), 0); //send message
	if(send_result == -1){
		printf("Error: Could not send to server.\n");
		return -1;
	}
	temp.clear(); //clear the temp string
		
	close(connection_descriptor);
	close(file_descriptor);
	return 0;
}

void sig_handler(int signal){
	exit(0);
}

int main(int argc, char* argv[]) {

	// handle signals
	signal(SIGINT/SIGTERM/SIGKILL, sig_handler);

	// check args
	if(argc != 2){ return printHelpMessage(); }

	// get filename
	string filename = argv[1];
	
	// read topology file
	vector<string> topology = readTopologyFile(&filename);

	// create the output file
	int fd = open("output.txt", O_RDWR | O_CREAT);
	chmod("output.txt", 0666);

	pid_t pids[NUM_NODES];
	cout << "hello world parent" << endl;
	run_server();

	for(int i = 0; i < NUM_NODES; i++){
		pids[i] = fork();
		if(pids[i] == 0){
			cout << "hello world child" << endl;
			system("./router");
			_exit(0);
		}
		else if(pids[i] < 0){
			cout << "fork failed" << endl;
			exit(-1);
		}
	}

	int status;
	int tempN = NUM_NODES;
	while(tempN > 0){
		wait(&status);
		tempN--;
	}		

	close(fd);

    return 0;
}
