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

using namespace std;
using namespace boost;

#define PORT_NUMBER 20003
#define MESSAGE_SIZE 140
#define VERSION 457
int DEBUG = 1;

int ROUTER_ID = -1;

struct packet{
	char message[MESSAGE_SIZE];
};	

struct neighbor {
	int id;
	int cost;
	int udp_port;
};

struct router_node {
	int id;
	int udp_port;
	vector<struct neighbor> neighbors;
};

void receive_manager_packet(int accept_socket){
	struct router_node router_info = {};
	int receive_result = recv(accept_socket, reinterpret_cast<char*>(&router_info), sizeof(router_info), 0);
	if(receive_result == -1){
		cout << "Error: Could not receive from manager." << endl;
	}
	if(DEBUG){ 
		cout << "Received from manager:  My ID: " << router_info.id << " My UDP Port: " << router_info.udp_port << endl;
		cout << "My Neighbors..." << endl;
		for(unsigned int i = 0; i < router_info.neighbors.size(); i++) {
			cout << "Neighbor ID: " << router_info.neighbors.at(i).id << " Cost: " << router_info.neighbors.at(i).id << " UDP Port: " << router_info.neighbors.at(i).udp_port << endl;
		}
	}
}

void send_message_to_manager(int router_socket){
	char message[] = "Hello manager! Nice to meet you! Can you pass along my routing information?";
	int send_result = send(router_socket, &message, sizeof(message), 0);
	if(send_result == -1){
		cout << "Error: Could not send message to manager." << endl;
	}
}

int run_client(int port, const char* host)
{
	struct sockaddr_in router_address = {};

	int router_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(router_socket == -1){
		cout << "Error: Could not create server socket." << endl;
		return -1;
	}

	struct hostent *manager = gethostbyname(host);
	if(manager == NULL){
		cout << "Error: Could not find manager." << endl;
		return -1;
	}

	router_address.sin_family = AF_INET; //set family
	router_address.sin_port = htons(PORT_NUMBER); //convert and set port
	router_address.sin_addr.s_addr = INADDR_ANY;

	int connect_result = connect(router_socket, (struct sockaddr*)&router_address, sizeof(router_address));
	if(connect_result == -1){
		close(router_socket);			
		cout << "Error: Could not connect to manager" << endl;
		return -1;
	}

	send_message_to_manager(router_socket);
	receive_manager_packet(router_socket);

	close(router_socket);
	return 0;
}

void sig_handler(int signal){
	exit(0);
}

int main(int argc, char* argv[]) {

	// handle signals
	signal(SIGINT/SIGTERM/SIGKILL, sig_handler);

	run_client(PORT_NUMBER, "localhost");

    return 0;
}
