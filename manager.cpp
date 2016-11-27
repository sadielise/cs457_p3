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
#include <map>
#include <sstream>

using namespace std;
using namespace boost;

#define PORT_NUMBER 20003
#define MESSAGE_SIZE 140
#define VERSION 457
#define BASE_UDP_PORT 40000

int NUM_NODES = 0;
int DEBUG = 1;
int MAX_CONNECTION_LENGTH = 8;
vector<int> ROUTER_SOCKETS;
map<int, struct router> ROUTERS;

struct packet{
	char message[MESSAGE_SIZE];
};

struct neighbor {
	int id;
	int cost;
	int udp_port;
};

struct router {
	int id;
	int udp_port;
	vector<struct neighbor> neighbors;
};

int print_help_message() {
    cout << endl << "manager help:" << endl << endl;
    cout << "$manager <input file>" << endl;
    cout << "input file: network topology description" << endl;
	exit(EXIT_FAILURE);
    return -1;
}

void print_topology(int numNodes, vector<string>* topology){
	cout << "Number of nodes: " << numNodes << endl;
	for(unsigned int i = 0; i < (*topology).size(); i++){
		cout << (*topology).at(i) << endl;
	}
}

vector<string> read_topology_file(string* filename){

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
			int router_id = connection[0] - '0';
			int neighbor_id = connection[2] - '0';
			int cost = connection[4] - '0';
			
			struct neighbor new_neighbor = {};
			new_neighbor.id = neighbor_id;
			new_neighbor.cost = cost;
			
			if(ROUTERS.find(router_id) != ROUTERS.end()) { // router struct has already been made
				struct router r = ROUTERS[router_id];
				r.neighbors.push_back(new_neighbor);
			} else { // need to create new router struct
				struct router new_router = {};
				new_router.id = router_id;
				new_router.udp_port = BASE_UDP_PORT + router_id; // All UDP ports = BASE_UDP_PORT + router id
				new_router.neighbors.push_back(new_neighbor);
				ROUTERS[router_id] = new_router;
			}
			
			string line(connection);
			topology.push_back(line);
		}
	}
	
	return topology;
}

void receive_router_packet(int accept_socket, int i){
	struct packet receive_packet = {};
	char* receive_array = reinterpret_cast<char*>(&receive_packet);
	int receive_result = recv(accept_socket, receive_array, sizeof(receive_packet), 0);
	if(receive_result == -1){
		cout << "Error: Could not receive from router." << endl;
	}
	if(DEBUG){ cout << "Received from router " << i << ": " << receive_packet.message << endl; }
}

void send_message_to_router(int accept_socket){
	char message[] = "Greetings, router! Nice to meet you too!";
	int send_result = send(accept_socket, &message, sizeof(message), 0);
	if(send_result == -1){
		cout << "Error: Could not send message to router." << endl;
	}

}

int accept_router_connection(int manager_socket){
	struct sockaddr_in client_address = {};
	socklen_t client_address_len = sizeof(client_address);
	int accept_socket = accept(manager_socket, (struct sockaddr*)&client_address, &client_address_len);
	if(accept_socket == -1){
		cout << "Error: Could not accept." << endl;
		return -1;
	}
	return accept_socket;
}

void close_router_connections(){
	for(unsigned int i = 0; i < ROUTER_SOCKETS.size(); i++){
		close(ROUTER_SOCKETS.at(i));
	}
}

int connect_to_routers(int manager_socket){
	for(int i = 0; i < NUM_NODES; i++){
		int pid = fork();
		if(pid == 0){ // parent
			int accept_socket = accept_router_connection(manager_socket);
			if(accept_socket == -1){
				return -1;
			}
			ROUTER_SOCKETS.push_back(accept_socket);
			receive_router_packet(accept_socket, i);
			send_message_to_router(accept_socket);
			_exit(0);
		}
		if(pid > 0){ // child
			system("./router");
		}
		else if(pid < 0){
			cout << "fork failed" << endl;
			exit(-1);
		}
	}
	return 0;
}

int start_listening()
{
	struct sockaddr_in manager_address = {}; //initialize server sockaddr structure

	int manager_socket = socket(AF_INET, SOCK_STREAM, 0); //create manager socket
	if(manager_socket == -1){
		cout << "Error: Could not create server socket." << endl;
		return -1;
	}

	manager_address.sin_family = AF_INET; //set family
	manager_address.sin_port = htons(PORT_NUMBER); //convert and set port
	manager_address.sin_addr.s_addr = INADDR_ANY;
	
	int bind_result = bind(manager_socket, (struct sockaddr*)&manager_address, sizeof(manager_address)); //bind socket to port
	if(bind_result == -1){
		printf("Error: Could not bind server socket.\n");
		return -1;
	}

	int listen_result = listen(manager_socket, NUM_NODES); //start listening
	if(listen_result == -1){
		printf("Error: Could not begin listening.\n");
		return -1;
	}
	if(DEBUG){ cout << "Manager is listening..." << endl; }

	return manager_socket;
}

void sig_handler(int signal){
	exit(0);
}

int main(int argc, char* argv[]) {

	// handle signals
	signal(SIGINT/SIGTERM/SIGKILL, sig_handler);

	// check args
	if(argc != 2){ return print_help_message(); }

	// get filename
	string filename = argv[1];
	
	// read topology file
	if(DEBUG){ cout << "Reading topology file..." << endl; }
	vector<string> topology = read_topology_file(&filename);
	if(DEBUG){ print_topology(NUM_NODES, &topology); }

	// create the output file
	if(DEBUG){ cout << "Creating output file..." << endl; }
	int fd = open("output.txt", O_RDWR | O_CREAT);
	chmod("output.txt", 0666);

	// start listening for routers
	int manager_router = start_listening();

	// connect to routers
	connect_to_routers(manager_router);

	// close router connections
	close_router_connections();
	
	// close manager 
	close(manager_router);
	
	/*int status;
	int tempN = NUM_NODES;
	while(tempN > 0){
		wait(&status);
		tempN--;
	}*/		

	close(fd);
    exit(0);
}
