#include "project3.h"

int DEBUG = 1;

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
	for(auto const& router : ROUTERS) {
		ROUTER_FILE << "Router " << router.second.id << " UDP port: " << router.second.udp_port << endl;
		ROUTER_FILE << "    Neighbors:"  << endl;
		for(neighbor n : ROUTER_NEIGHBORS[router.second.id]) {
			ROUTER_FILE << "    Neighbor ID: " << n.id << " cost: " << n.cost << " UDP port: " << n.udp_port << endl;
		}
	}
}

void receive_manager_packet(int accept_socket){
	struct router_node route;
	int receive_result = recv(accept_socket, reinterpret_cast<char*>(&route), sizeof(route), 0);
	if(receive_result == -1){
		cout << "Error: Could not receive from manager." << endl;
	}
	
	struct packet_header pack_head;
	recv(accept_socket, reinterpret_cast<char*>(&pack_head), sizeof(pack_head), 0);
	
	vector<neighbor> r_neighbors;
	
	for(int i = 0; i < pack_head.num_neighbors; i++) {
		struct neighbor n;
		recv(accept_socket, reinterpret_cast<char*>(&n), sizeof(n), 0);
		r_neighbors.push_back(n);
	}
	
	MY_ROUTER_INFO = router_node(route.id, route.num_routers, route.udp_port);
	ROUTER_NEIGHBORS[MY_ROUTER_INFO.id] = r_neighbors;
	
	if(DEBUG) {
		cout << "Router Info ... ID: " << MY_ROUTER_INFO.id << " UDP Port: " << MY_ROUTER_INFO.udp_port << endl;
		cout << "Neighbors (" << ROUTER_NEIGHBORS[MY_ROUTER_INFO.id].size() << ")..." << endl;
		for(unsigned int i = 0; i < ROUTER_NEIGHBORS[MY_ROUTER_INFO.id].size(); i++) {
			cout << "    Neighbor - ID: " << ROUTER_NEIGHBORS[MY_ROUTER_INFO.id].at(i).id << " Cost: " << ROUTER_NEIGHBORS[MY_ROUTER_INFO.id].at(i).cost << " UDP Port: " << ROUTER_NEIGHBORS[MY_ROUTER_INFO.id].at(i).udp_port << endl;
		}
		cout << endl;
	}
	
}

void send_message_to_manager(int router_socket){
	char message[] = "Hello manager! Nice to meet you! Can you pass along my routing information?";
	int send_result = send(router_socket, &message, sizeof(message), 0);
	if(send_result == -1){
		cout << "Error: Could not send message to manager." << endl;
	}
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

tuple<struct router_node, vector<neighbor>> receive_udp_router_node(int* sender_port) {
	struct sockaddr_in sender_addr;
	socklen_t addrlen = sizeof(sender_addr);
	
	struct router_node router_info;
	recvfrom(MY_UDP_SOCKET, reinterpret_cast<char*>(&router_info), sizeof(router_info), 0, (struct sockaddr*)&sender_addr, &addrlen);
	
	struct packet_header pack_head;
	recvfrom(MY_UDP_SOCKET, reinterpret_cast<char*>(&pack_head), sizeof(pack_head), 0, (struct sockaddr*)&sender_addr, &addrlen);
	
	vector<neighbor> router_neighbors;
	
	for(int i = 0; i < pack_head.num_neighbors; i++) {
		struct neighbor n;
		recvfrom(MY_UDP_SOCKET, reinterpret_cast<char*>(&n), sizeof(n), 0, (struct sockaddr*)&sender_addr, &addrlen);
		router_neighbors.push_back(n);
	}
	
	*sender_port = sender_addr.sin_port;
	return make_tuple(router_node(router_info.id, router_info.num_routers, router_info.udp_port), router_neighbors);
}

void forward_router_info(struct router_node router_info, vector<neighbor> neighbors, int sender_port, bool broadcast) {
	for(neighbor n : ROUTER_NEIGHBORS[MY_ROUTER_INFO.id]) {
		if(broadcast || n.udp_port != sender_port) {
			struct sockaddr_in neighbor_addr = NEIGHBOR_ADDRS[n.id];
			sendto(MY_UDP_SOCKET, reinterpret_cast<char*>(&router_info), sizeof(router_info), 0, (struct sockaddr*)&neighbor_addr, sizeof(neighbor_addr));
			
			struct packet_header pack_head;
			pack_head.num_neighbors = neighbors.size();
			sendto(MY_UDP_SOCKET, reinterpret_cast<char*>(&pack_head), sizeof(pack_head), 0, (struct sockaddr*)&neighbor_addr, sizeof(neighbor_addr));
			
			for(unsigned int i = 0; i < neighbors.size(); i++) {
				struct neighbor n = neighbors.at(i);
				sendto(MY_UDP_SOCKET, reinterpret_cast<char*>(&n), sizeof(n), 0, (struct sockaddr*)&neighbor_addr, sizeof(neighbor_addr));
			}
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
	
	while(ROUTERS.size() < ((unsigned int) MY_ROUTER_INFO.num_routers)) {
		int sender_port;
		tuple<struct router_node, vector<neighbor>> router_and_neighbors = receive_udp_router_node(&sender_port);
		
		if(ROUTERS.find(get<0>(router_and_neighbors).id) == ROUTERS.end()) {
			ROUTERS[get<0>(router_and_neighbors).id] = get<0>(router_and_neighbors);
			ROUTER_NEIGHBORS[get<0>(router_and_neighbors).id] = get<1>(router_and_neighbors);
			thread forward_info(forward_router_info, get<0>(router_and_neighbors), get<1>(router_and_neighbors), sender_port, false);
			forward_info.join();
		}
	}
}

void forward_my_router_info() {
	this_thread::sleep_for(chrono::milliseconds(3000));
	forward_router_info(MY_ROUTER_INFO, ROUTER_NEIGHBORS[MY_ROUTER_INFO.id], 0, true);
}

int find_least_cost_unkown_router() {
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
	// assign own info for link state
	KNOWN_ROUTERS.push_back(MY_ROUTER_INFO.id);
	for(neighbor n : ROUTER_NEIGHBORS[MY_ROUTER_INFO.id]) {
		COSTS[n.id] = n.cost;
		PREVIOUS_STEP[n.id] = MY_ROUTER_INFO.id;
	}
	
	// find next least cost router and add it to known routers
	int next_router = find_least_cost_unkown_router();
	KNOWN_ROUTERS.push_back(next_router);
	
	// ask this router for its neighbors' information
}

void run_client(int port, const char* host) {
	struct sockaddr_in router_address = {};

	int router_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(router_socket == -1){
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

	int connect_result = connect(router_socket, (struct sockaddr*)&router_address, sizeof(router_address));
	if(connect_result == -1){
		close(router_socket);			
		cout << "Error: Could not connect to manager" << endl;
		return;
	}

	send_message_to_manager(router_socket);
	receive_manager_packet(router_socket);
	
	MANAGER_SOCKET = router_socket;
}

void sig_handler(int signal){
	exit(0);
}

int main(int argc, char* argv[]) {

	// handle signals
	signal(SIGINT/SIGTERM/SIGKILL, sig_handler);
	
	// Get MY_ROUTER_INFO
	run_client(PORT_NUMBER, "localhost");
	populate_neighbor_addrs();
	
	// Create router file
	string filename = to_string(MY_ROUTER_INFO.id) + ".out";
	remove(filename.c_str()); // clear the file if it already exists
	ROUTER_FILE.open(filename.c_str(), ios::out | ios::app);
	chmod(filename.c_str(), 0666);
	
	// Listen and filter/forward packets
	thread t_server(listen_and_forward_router_info);
	thread t_client(forward_my_router_info);
	t_server.join();
	t_client.join();
	
	print_network_to_file();

	ROUTER_FILE.close();
    return 0;
}
