#include "project3.h"

int DEBUG = 1;

int NUM_NODES = 0;

ofstream ROUTER_FILE;

int MY_UDP_SOCKET;
int MANAGER_SOCKET;

struct router_node MY_ROUTER_INFO;
map<int, sockaddr_in> NEIGHBOR_ADDRS;

map<int, router_node> ROUTERS;
map<int, vector<neighbor>> ROUTER_NEIGHBORS;

vector<int> KNOWN_ROUTERS;
map<int, int> COSTS;
map<int, int> PREVIOUS_STEP;

void print_network_to_file() {
	ROUTER_FILE << "--------------------NETWORK--------------------" << endl;
	for(auto const& router : ROUTERS) {
		ROUTER_FILE << "Router " << router.second.id << " UDP port: " << router.second.udp_port << endl;
		ROUTER_FILE << "    Neighbors:"  << endl;
		for(neighbor n : ROUTER_NEIGHBORS[router.second.id]) {
			ROUTER_FILE << "    Neighbor ID: " << n.id << " cost: " << n.cost << " UDP port: " << n.udp_port << endl;
		}
	}
}

void print_routing_table_to_file(){
	ROUTER_FILE << endl;
	ROUTER_FILE << "Routing table:" << endl;
	ROUTER_FILE << "ID\tCOST\tNEXT HOP" << endl;
	for(auto const& cost : COSTS) {
		ROUTER_FILE << cost.first << "\t" << cost.second << "\t" << PREVIOUS_STEP[cost.first] << endl;
	}
	ROUTER_FILE << endl;
}

vector<neighbor> neighbor_array_to_vector(struct neighbor* n_array, bool* n_map) {
	vector<struct neighbor> neighbor_vector;
	for(int i = 0; i < NUM_NODES; i++) {
		if(n_map[i] == true) {
			neighbor_vector.push_back(n_array[i]);
		}
	}
	return neighbor_vector;
}

void receive_instructions(){
	struct packet instruction;
	recv(MANAGER_SOCKET, reinterpret_cast<char*>(&instruction), sizeof(instruction), 0);
	ROUTER_FILE << "Received instruction from manager: " << instruction.message << endl;
	cout << "Receiving instructions..." << endl;
}	

void receive_manager_packet(){
	struct packet_header pack_head;
	recv(MANAGER_SOCKET, reinterpret_cast<char*>(&pack_head), sizeof(pack_head), 0);
	
	NUM_NODES = pack_head.num_routers;
	
	struct router_node router;
	recv(MANAGER_SOCKET, reinterpret_cast<char*>(&router), sizeof(router), 0);
	
	MY_ROUTER_INFO = router;
	ROUTERS[MY_ROUTER_INFO.id] = MY_ROUTER_INFO;
	ROUTER_NEIGHBORS[MY_ROUTER_INFO.id] = neighbor_array_to_vector(MY_ROUTER_INFO.neighbors, MY_ROUTER_INFO.has_neighbor);
	
	if(DEBUG) {
		cout << MY_ROUTER_INFO.id << ": Received router info. ID: " << MY_ROUTER_INFO.id << ", UDP Port: " << MY_ROUTER_INFO.udp_port << ", Num Neighbors: " << MY_ROUTER_INFO.num_neighbors << endl;
		cout << "My Neighbors: " << endl;
		for(neighbor n : ROUTER_NEIGHBORS[MY_ROUTER_INFO.id]) {
			cout << "    ID: " << n.id << " cost: " << n.cost << " UDP Port: " << n.udp_port << endl;
		}
		cout << endl;
	}
}

void send_hello_to_manager(){
	char message[] = "Hello manager!";
	int send_result = send(MANAGER_SOCKET, &message, sizeof(message), 0);
	if(send_result == -1){
		cout << "Error: Could not send message to manager." << endl;
	}
}

void send_ready_to_manager(){
	char message[] = "ready";
	int send_result = send(MANAGER_SOCKET, &message, sizeof(message), 0);
	if(send_result == -1){
		cout << "Error: Could not send message to manager." << endl;
	}
	cout << "Sent ready to manager" << endl;
}

void populate_neighbor_addrs() {
	for(neighbor n : ROUTER_NEIGHBORS[MY_ROUTER_INFO.id]) {
		struct sockaddr_in neighbor_addr;
		socket(AF_INET, SOCK_DGRAM, 0);
		
		neighbor_addr.sin_family = AF_INET;
		neighbor_addr.sin_port = n.udp_port;
		neighbor_addr.sin_addr.s_addr = INADDR_ANY;
		
		NEIGHBOR_ADDRS[n.id] = neighbor_addr;
	}
}

struct router_node receive_udp_router_node(int* sender_port) {
	struct sockaddr_in sender_addr;
	socklen_t addrlen = sizeof(sender_addr);
	
	struct router_node router;
	recvfrom(MY_UDP_SOCKET, reinterpret_cast<char*>(&router), sizeof(router), 0, (struct sockaddr*)&sender_addr, &addrlen);
	*sender_port = sender_addr.sin_port;
	
	ROUTER_FILE << "Received router info from router " << router.id << endl;
	
	return router;
}

void forward_router_info(struct router_node router_info, int sender_port, bool broadcast) {
	for(neighbor n : ROUTER_NEIGHBORS[MY_ROUTER_INFO.id]) {
		if(broadcast || n.udp_port != sender_port) {
			struct sockaddr_in neighbor_addr = NEIGHBOR_ADDRS[n.id];
			sendto(MY_UDP_SOCKET, reinterpret_cast<char*>(&router_info), sizeof(router_info), 0, (struct sockaddr*)&neighbor_addr, sizeof(neighbor_addr));
			ROUTER_FILE << "Send router info to router " << n.id << endl;
		}
	}
}

void listen_and_forward_router_info() {
	struct sockaddr_in my_addr = {};
	
	MY_UDP_SOCKET = socket(AF_INET, SOCK_DGRAM, 0);
	
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = MY_ROUTER_INFO.udp_port;
	my_addr.sin_addr.s_addr = INADDR_ANY;
	
	bind(MY_UDP_SOCKET, (struct sockaddr*)&my_addr, sizeof(my_addr));
	
	while(ROUTERS.size() < ((unsigned int) NUM_NODES)) {
		int sender_port;
		struct router_node router = receive_udp_router_node(&sender_port);
		if(ROUTERS.find(router.id) == ROUTERS.end()) {
			ROUTERS[router.id] = router;
			ROUTER_NEIGHBORS[router.id] = neighbor_array_to_vector(router.neighbors, router.has_neighbor);
			thread forward_info(forward_router_info, router, sender_port, false);
			forward_info.join();
		}
	}
}

void forward_my_router_info() {
	this_thread::sleep_for(chrono::milliseconds(3000));
	forward_router_info(MY_ROUTER_INFO, 0, true);
}

void initialize_costs() {
	for(int i = 0; i < NUM_NODES; i++) {
		COSTS[i] = INT_MAX;
	}
	COSTS[MY_ROUTER_INFO.id] = 0;
	PREVIOUS_STEP[MY_ROUTER_INFO.id] = MY_ROUTER_INFO.id;
}

int find_least_cost_unknown_router() {
	int least_cost_router = INT_MAX;
	int least_cost_router_id = -1;
	
	for(auto const& cost : COSTS) {
		if(cost.second < least_cost_router && find(KNOWN_ROUTERS.begin(), KNOWN_ROUTERS.end(), cost.first) == KNOWN_ROUTERS.end()) {
			least_cost_router = cost.second;
			least_cost_router_id = cost.first;
		}
	}
	
	return least_cost_router_id;
}

void run_link_state_alg() {
	ROUTER_FILE << endl << "Running link state algorithm..." << endl;
	initialize_costs();
	
	// assign own info for link state
	KNOWN_ROUTERS.push_back(MY_ROUTER_INFO.id);
	for(neighbor n : ROUTER_NEIGHBORS[MY_ROUTER_INFO.id]) {
		COSTS[n.id] = n.cost;
		PREVIOUS_STEP[n.id] = MY_ROUTER_INFO.id;
	}
	
	// find next least cost router and add it to known routers
	while(KNOWN_ROUTERS.size() < ((unsigned int) NUM_NODES)) {
		int next_router = find_least_cost_unknown_router();
		KNOWN_ROUTERS.push_back(next_router);
		
		for(neighbor n : ROUTER_NEIGHBORS[next_router]) {
			if(COSTS[n.id] > COSTS[next_router] + n.cost) {
				COSTS[n.id] = COSTS[next_router] + n.cost;
				PREVIOUS_STEP[n.id] = next_router;
			}
		}
	}
}

void run_client(int port, const char* host) {
	struct sockaddr_in router_address = {};

	MANAGER_SOCKET = socket(AF_INET, SOCK_STREAM, 0);
	if(MANAGER_SOCKET == -1){
		cout << "Error: Could not create server socket." << endl;
		return;
	}

	struct hostent *manager = gethostbyname(host);
	if(manager == NULL){
		cout << "Error: Could not find manager." << endl;
		return;
	}

	router_address.sin_family = AF_INET; //set family
	router_address.sin_port = htons(PORT_NUMBER); //convert and set port
	router_address.sin_addr.s_addr = INADDR_ANY;

	int connect_result = connect(MANAGER_SOCKET, (struct sockaddr*)&router_address, sizeof(router_address));
	if(connect_result == -1){
		close(MANAGER_SOCKET);			
		cout << "Error: Could not connect to manager" << endl;
		return;
	}

	send_hello_to_manager();
	receive_manager_packet();
}

void create_router_file(){
	string filename = "router" + to_string(MY_ROUTER_INFO.id) + ".out";
	remove(filename.c_str()); // clear the file if it already exists
	ROUTER_FILE.open(filename.c_str(), ios::out | ios::app);
	chmod(filename.c_str(), 0666);
	ROUTER_FILE << "-----------------------ROUTER-----------------------" << endl << endl;
	ROUTER_FILE << "Connected to manager" << endl << endl;
	ROUTER_FILE << "Sent to manager: " << "Hello manager!" << endl << endl;
	ROUTER_FILE << "Received from manager: " << endl;
	ROUTER_FILE << "\tID: " << MY_ROUTER_INFO.id << endl;
	ROUTER_FILE << "\tUDP Port: " << MY_ROUTER_INFO.udp_port << endl;
	ROUTER_FILE << "\tNum Neighbors: " << MY_ROUTER_INFO.num_neighbors << endl;
	ROUTER_FILE << "\tMy Neighbors: " << endl;
	for(neighbor n : ROUTER_NEIGHBORS[MY_ROUTER_INFO.id]) {
			ROUTER_FILE << "\tID: " << n.id << " Cost: " << n.cost << " UDP Port: " << n.udp_port << endl;
	}
	ROUTER_FILE << endl;
}

void sig_handler(int signal){
	exit(0);
}

int main(int argc, char* argv[]) {

	// handle signals
	signal(SIGINT/SIGTERM/SIGKILL, sig_handler);

	run_client(PORT_NUMBER, "localhost");
	create_router_file();
	populate_neighbor_addrs();
	
	// Listen and filter/forward packets
	thread t_server(listen_and_forward_router_info);
	thread t_client(forward_my_router_info);
	t_server.join();
	t_client.join();
	
	// Run link state algorithm and notify router when finished
	run_link_state_alg();
	print_routing_table_to_file();
	send_ready_to_manager();
	receive_instructions();

	ROUTER_FILE.close();
  return 0;
}
