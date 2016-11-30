#include "project3.h"

int DEBUG = 1;

struct router_node MY_ROUTER_INFO;
map<int, int> router_ports;

vector<int> known_routers;
map<int, int> costs;
map<int, int> previous_step;

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
	
	MY_ROUTER_INFO = router_node(route.id, route.num_routers, route.udp_port, r_neighbors);
	
	if(DEBUG) {
		cout << "Router Info ... ID: " << MY_ROUTER_INFO.id << " UDP Port: " << MY_ROUTER_INFO.udp_port << endl;
		cout << "Neighbors (" << MY_ROUTER_INFO.neighbors.size() << ")..." << endl;
		for(unsigned int i = 0; i < MY_ROUTER_INFO.neighbors.size(); i++) {
			cout << "    Neighbor - ID: " << MY_ROUTER_INFO.neighbors.at(i).id << " Cost: " << MY_ROUTER_INFO.neighbors.at(i).cost << " UDP Port: " << MY_ROUTER_INFO.neighbors.at(i).udp_port << endl;
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

void update_router_ports() {
	router_ports[MY_ROUTER_INFO.id] = MY_ROUTER_INFO.udp_port;
	
	for(neighbor n : MY_ROUTER_INFO.neighbors) {
		router_ports[n.id] = n.udp_port;
	}
}

int find_least_cost_unkown_router() {
	int least_cost_router = INT_MAX;
	int least_cost_router_id = -1;
	
	for(auto const& cost : costs) {
		if(cost.second < least_cost_router && find(known_routers.begin(), known_routers.end(), cost.first) == known_routers.end()) {
			least_cost_router = cost.second;
			least_cost_router_id = cost.first;
		}
	}
	
	return least_cost_router_id;
}

void run_link_state_alg() {
	// assign own info for link state
	known_routers.push_back(MY_ROUTER_INFO.id);
	for(neighbor n : MY_ROUTER_INFO.neighbors) {
		costs[n.id] = n.cost;
		previous_step[n.id] = MY_ROUTER_INFO.id;
	}
	
	// find next least cost router and add it to known routers
	int next_router = find_least_cost_unkown_router();
	known_routers.push_back(next_router);
	
	// ask this router for its neighbors' information
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
	update_router_ports();
	
	run_link_state_alg();

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
