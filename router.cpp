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

#define PORT_NUMBER 20001
#define MESSAGE_SIZE 140
#define VERSION 457
int DEBUG = 1;

struct packet{
	short version;
	short length;
	char message[MESSAGE_SIZE];
};	

int run_client(int port, char* ip)
{
	struct sockaddr_in client_address = {}; //initialize client sockaddr structure
	client_address.sin_family = AF_INET; //set family
	client_address.sin_port = htons(port); //convert and set port
	struct in_addr in_addr_temp = {}; //setup in_addr for ip number
	inet_pton(AF_INET, ip, &in_addr_temp); //set s_addr in in_addr_temp
	client_address.sin_addr = in_addr_temp; //add in_addr to sockaddr_in

	int file_descriptor = socket(AF_INET, SOCK_STREAM, 0); //create socket
	if(file_descriptor == -1){
		printf("Error: Could not create server socket.\n");
		return -1;
	}

	int connect_result = connect(file_descriptor, (struct sockaddr*)&client_address, sizeof(client_address)); //connect to server
	if(connect_result == -1){
		close(file_descriptor);			
		printf("\nError: Could not connect to server.\n");
		return -1;
	}

	printf("Connected!\n"); 

	char send_buffer[MESSAGE_SIZE]; //buffer to send message
	memset(send_buffer, 0, sizeof(send_buffer)); //set the buffer to zeros
	string temp = "hello from client";
	for(unsigned int i = 0; i < temp.size(); i++){ //copy string into char array
		send_buffer[i] = temp.at(i);
	}
	struct packet send_packet = {}; //create packet to send
	send_packet.version = htons(VERSION); //set version
	send_packet.length = htons(temp.length()); //set length
	bcopy(send_buffer, send_packet.message, sizeof(send_packet.message)); //set message
	const char* send_array = reinterpret_cast<const char*>(&send_packet);
	int send_result = send(file_descriptor, send_array, sizeof(send_packet), 0); //send message
	if(send_result == -1){
		printf("Error: Could not send to server.\n");
		return -1;
	}
	temp.clear(); //clear the temp string

	struct packet receive_packet = {}; //buffer to receive message
	char* receive_array = reinterpret_cast<char*>(&receive_packet); //cast the packet to a char* so that it can be used by recv
	int receive_result = recv(file_descriptor, receive_array, sizeof(receive_packet), 0); //receive message
	receive_packet.version = ntohs(receive_packet.version); //change version back to host byte order
	receive_packet.length = ntohs(receive_packet.length); //change length back to host by order
	if(receive_result == -1){
		printf("Error: Could not receive from client.\n");
		return -1;
	}
		printf("Server: %s\n", receive_packet.message); //print message from client
	
	close(file_descriptor);
	return 0;
}

void sig_handler(int signal){
	exit(0);
}

int main(int argc, char* argv[]) {

	// handle signals
	signal(SIGINT/SIGTERM/SIGKILL, sig_handler);

	cout << "got to the client" << endl;
	char ip[] = "129.82.44.134";
	run_client(PORT_NUMBER, ip);

    return 0;
}
