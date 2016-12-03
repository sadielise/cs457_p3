#include "project3.h"

int NUM_NODES = 0;
int DEBUG = 1;
int MAX_CONNECTION_LENGTH = 8;
int MANAGER_SOCKET;
vector<int> ROUTER_SOCKETS;
map<int, struct router_node> ROUTERS;
map<int, vector<neighbor>> ROUTER_NEIGHBORS;
ifstream TOPOLOGY_FILE;
ofstream MANAGER_FILE;

int print_help_message() {
    cout << endl << "manager help:" << endl << endl;
    cout << "$manager <input file>" << endl;
    cout << "input file: network topology description" << endl;
	exit(EXIT_FAILURE);
    return -1;
}

void copy_neighbors_to_routers() {
	for(auto const& neighbor_map : ROUTER_NEIGHBORS) {
		for(neighbor n : neighbor_map.second) {
			ROUTERS[neighbor_map.first].neighbors[n.id] = n;
			ROUTERS[neighbor_map.first].has_neighbor[n.id] = true;
		}
		ROUTERS[neighbor_map.first].num_neighbors = neighbor_map.second.size();
	}
}

vector<string> split(const string &s, char delim) {
    stringstream ss(s);
    string item;
    vector<string> tokens;
    while (getline(ss, item, delim)) {
        tokens.push_back(item);
    }
    return tokens;
}

void create_routers() {
	for(int i = 0; i < NUM_NODES; i++) {
		struct router_node new_router = {};
		new_router.id = i;
		new_router.udp_port = BASE_UDP_PORT + i; // All UDP ports = BASE_UDP_PORT + router id
		ROUTERS[i] = new_router;
	}
}

void print_topology_to_file(int num_nodes, vector<string>* topology){
	MANAGER_FILE << "Number of nodes: " << num_nodes << endl;
	for(unsigned int i = 0; i < (*topology).size(); i++){
		MANAGER_FILE << (*topology).at(i) << endl;
	}
	MANAGER_FILE << endl;
}

void print_topology(int num_nodes, vector<string>* topology){
	cout << "Number of nodes: " << num_nodes << endl;
	for(unsigned int i = 0; i < (*topology).size(); i++){
		cout << (*topology).at(i) << endl;
	}
}

vector<string> read_topology_file(string* filename){

  TOPOLOGY_FILE.open(*filename);
	TOPOLOGY_FILE >> NUM_NODES;
	
	char connection[MAX_CONNECTION_LENGTH];
	TOPOLOGY_FILE.getline(connection, MAX_CONNECTION_LENGTH);
	
	create_routers();

	vector<string> topology;
	bool end_of_topology = false;
	while(end_of_topology == false){
		char connection[MAX_CONNECTION_LENGTH];
		TOPOLOGY_FILE.getline(connection, MAX_CONNECTION_LENGTH);
		if(connection[0] == '-'){ end_of_topology = true; }
		else{
			string line(connection);
			trim(line);
			vector<string> elements = split(line, ' ');
			
			int router_id = stoi(elements.at(0));
			int neighbor_id = stoi(elements.at(1));
			int cost = stoi(elements.at(2));
			
			ROUTER_NEIGHBORS[router_id].push_back(neighbor(neighbor_id, cost, BASE_UDP_PORT + neighbor_id));
			ROUTER_NEIGHBORS[neighbor_id].push_back(neighbor(router_id, cost, BASE_UDP_PORT + router_id));
			
			topology.push_back(line);
		}
	}
	
	copy_neighbors_to_routers();
	
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
	MANAGER_FILE << "Received from router " << i << ": " << receive_packet.message << endl;
}

bool receive_ready_packet(int accept_socket){
	struct packet receive_packet = {};
	char* receive_array = reinterpret_cast<char*>(&receive_packet);
	int receive_result = recv(accept_socket, receive_array, sizeof(receive_packet), 0);
	if(receive_result == -1){
		cout << "Error: Could not receive from router." << endl;
	}
	char ready[] = "ready";
	if(strcmp(ready, receive_packet.message) == 0){
		return true;
	}
	else{
		return false;
	}
}

void send_message_to_router(int accept_socket, int router_id){
	struct packet_header pack_head = {};
	pack_head.num_routers = NUM_NODES;
	send(accept_socket, reinterpret_cast<char*>(&pack_head), sizeof(pack_head), 0);
	
	struct router_node r = ROUTERS[router_id];
	send(accept_socket, reinterpret_cast<char*>(&r), sizeof(r), 0);
	if(DEBUG){ cout << "Sent topology to router " << router_id << endl; }
	MANAGER_FILE << "Sent topology to router " << router_id << endl << endl;
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

void confirm_router_ready(int i){
	if(receive_ready_packet(ROUTER_SOCKETS.at(i)) == true){
		if(DEBUG){ cout << "Router " << i << " ready" << endl; }
		MANAGER_FILE << "Received from router " << i << ": ready" << endl;
	}
	MANAGER_FILE << endl;
}

void send_instruction_to_router(int source_id, string destination_id){
	int source_socket = ROUTER_SOCKETS.at(source_id);
	struct packet instruction = {};
	for(unsigned int i = 0; i < destination_id.size(); i++){
		instruction.message[i] = destination_id.at(i);
	}
	send(source_socket, reinterpret_cast<char*>(&instruction), sizeof(instruction), 0);
}

void send_router_instructions(){

	MANAGER_FILE << "Sending instructions to routers..." << endl << endl;
	bool end_of_instructions = false;
	int instruction_number = 0;
	while(end_of_instructions == false){
		char connection[MAX_CONNECTION_LENGTH];
		TOPOLOGY_FILE.getline(connection, MAX_CONNECTION_LENGTH);
		if(connection[0] == '-'){ end_of_instructions = true; }
		else{
			string line(connection);
			trim(line);
			vector<string> elements = split(line, ' ');			
			int source_id = stoi(elements.at(0));
			string destination_id = elements.at(1);
			instruction_number = instruction_number + 1;
			send_instruction_to_router(source_id, destination_id);

			if(DEBUG){ cout << "Instruction " << instruction_number << ": " << source_id << " " << destination_id << endl; }
			MANAGER_FILE << "Sent instruction to router " << source_id << ": " << "destination " << destination_id << endl;
			
		}
	}
}

void connect_to_routers(int manager_socket){
	pid_t pid;
	for(int i = 0; i < NUM_NODES; i++){
		pid = fork();
		
		if(pid == 0) { // child process
			system("./router");
			_exit(0);
		} else if(pid > 0) { // parent_process
			int accept_socket = accept_router_connection(manager_socket);
			if(accept_socket == -1) return;
			MANAGER_FILE << "Connected to router " << i << endl;
			ROUTER_SOCKETS.push_back(accept_socket);
			receive_router_packet(accept_socket, i);
			send_message_to_router(accept_socket, i);
		} else {
			cout << "fork failed" << endl;
			exit(-1);
		}
	}

	for(int i = 0; i < NUM_NODES; i++){
		confirm_router_ready(i);
	}
	
	send_router_instructions();
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
	MANAGER_FILE << "Listening for routers..." << endl << endl;

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

	// create the output file
	string output_file = "manager.out";
	remove(output_file.c_str()); // clear the file if it already exists
	MANAGER_FILE.open(output_file.c_str(), ios::out | ios::app);
	chmod(output_file.c_str(), 0666);
	MANAGER_FILE << "-----------------------MANAGER-----------------------" << endl << endl;
	
	// read topology file
	string filename = argv[1];
	if(DEBUG){ cout << "Reading topology file..." << endl; }
	vector<string> topology = read_topology_file(&filename);
	if(DEBUG){ print_topology(NUM_NODES, &topology); }
	print_topology_to_file(NUM_NODES, &topology);

	// start listening for routers
	MANAGER_SOCKET = start_listening();

	// connect to and run routers
	connect_to_routers(MANAGER_SOCKET);

	// send instructions to routers
	
	int status;
	int tempN = NUM_NODES;
	while(tempN > 0){
		wait(&status);
		tempN--;
	}

	// close manager file
	MANAGER_FILE.close();

	// close router connections
	close_router_connections();
	
	// close manager 
	close(MANAGER_SOCKET);
	
  exit(0);
}
