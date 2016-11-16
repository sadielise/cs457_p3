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
	struct sockaddr_in manager_address; //initialize server sockaddr structure
	fd_set router_connections; // list of socket descriptors for router connections
	int router_sockets[NUM_NODES]; // list to keep track of the router sockets
	for(int i = 0; i < NUM_NODES; i++){
		router_sockets[i] = 0;
	}

	int manager_socket = socket(AF_INET, SOCK_STREAM, 0); //create manager socket
	if(manager_socket == -1){
		printf("Error: Could not create server socket.\n");
		return -1;
	}

	int opt = 1;
	int allow_multiple = setsockopt(manager_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if(allow_multiple == -1){
		printf("Error: Could not allow multiple TCP connections.\n");
		return -1;
	}

	manager_address.sin_family = AF_INET; //set family
	manager_address.sin_port = htons(PORT_NUMBER); //convert and set port
	manager_address.sin_addr.s_addr = INADDR_ANY;
	
	int listen_result = listen(manager_socket, NUM_NODES); //start listening
	if(listen_result == -1){
		printf("Error: Could not begin listening.\n");
		return -1;
	}

	socklen_t manager_address_len = sizeof(manager_address); //get length of socket

	while(true){
		FD_ZERO(&router_connections); // clear the sockets
		FD_SET(manager_socket, &router_connections);

		for(int i = 0; i < NUM_NODES; i++){
			
		}

	}

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
