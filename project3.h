#ifndef PROJECT3_H
#define PROJECT3_H

#include <stdio.h>
#include <stdlib.h>
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
#include <chrono>
#include <thread>
#include <tuple>

using namespace std;
using namespace boost;

#define PORT_NUMBER 20003
#define MESSAGE_SIZE 140
#define VERSION 457
#define BASE_UDP_PORT 40000

struct packet_header {
	int num_neighbors;
};

struct packet{
	char message[MESSAGE_SIZE];
};

struct neighbor {
	int id;
	int cost;
	int udp_port;
	neighbor(){}
	neighbor(int _id, int _cost, int _udp_port) {
		id = _id;
		cost = _cost;
		udp_port = _udp_port;
	}
};

struct router_node {
	int id;
	int num_routers;
	int udp_port;
	router_node(){}
	router_node(int _id, int _num_routers, int _udp_port) {
		id = _id;
		num_routers = _num_routers;
		udp_port = _udp_port;
	}
};

#endif
