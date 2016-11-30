#include "project3.h"

int NUM_NODES = 0;
int DEBUG = 1;
int MAX_CONNECTION_LENGTH = 8;
vector<int> ROUTER_SOCKETS;
map<int, struct router_node> ROUTERS;

int print_help_message() {
    cout << endl << "manager help:" << endl << endl;
    cout << "$manager <input file>" << endl;
    cout << "input file: network topology description" << endl;
	exit(EXIT_FAILURE);
    return -1;
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
		new_router.num_routers = NUM_NODES;
		new_router.udp_port = BASE_UDP_PORT + i; // All UDP ports = BASE_UDP_PORT + router id
		ROUTERS[i] = new_router;
	}
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
	
	create_routers();

	vector<string> topology;
	bool eof = false;
	while(eof == false){
		char connection[MAX_CONNECTION_LENGTH];
		file.getline(connection, MAX_CONNECTION_LENGTH);
		if(connection[0] == '-'){ eof = true; }
		else{
			string line(connection);
			vector<string> elements = split(line, ' ');
			
			int router_id = stoi(elements.at(0));
			int neighbor_id = stoi(elements.at(1));
			int cost = stoi(elements.at(2));
			
			ROUTERS[router_id].neighbors.push_back(neighbor(neighbor_id, cost, BASE_UDP_PORT + neighbor_id));
			
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

void send_message_to_router(int accept_socket, int router_id){
	struct router_node r = ROUTERS[router_id];
	int send_result = send(accept_socket, reinterpret_cast<char*>(&r), sizeof(r), 0);
	if(send_result == -1){
		cout << "Error: Could not send data to router." << endl;
	}
	
	struct packet_header pack_head = {};
	pack_head.num_neighbors = r.neighbors.size();
	send(accept_socket, reinterpret_cast<char*>(&pack_head), sizeof(pack_head), 0);
	
	for(unsigned int i = 0; i < r.neighbors.size(); i++) {
		struct neighbor n = r.neighbors.at(i);
		send(accept_socket, reinterpret_cast<char*>(&n), sizeof(n), 0);
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

void connect_to_routers(int manager_socket){
	for(int i = 0; i < NUM_NODES; i++){
		pid_t pid = fork();
		
		if(pid == 0) { // child process
			system("./router");
			cout << "FINISHED ROUTER" << endl;
			_exit(0);
		} else if(pid > 0) { // parent_process
			int accept_socket = accept_router_connection(manager_socket);
			if(accept_socket == -1) return;
			ROUTER_SOCKETS.push_back(accept_socket);
			receive_router_packet(accept_socket, i);
			send_message_to_router(accept_socket, i);
		} else {
			cout << "fork failed" << endl;
			exit(-1);
		}
	}
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
	int fd = open("outputFiles/manager.out", O_RDWR | O_CREAT);
	chmod("outputFiles/manager.out", 0666);

	// start listening for routers
	int manager_router = start_listening();

	// connect to routers
	connect_to_routers(manager_router);
	
	while(true){};
	
	int status;
	int tempN = NUM_NODES;
	while(tempN > 0){
		wait(&status);
		tempN--;
	}

	// close router connections
	close_router_connections();
	
	// close manager 
	close(manager_router);
	
	close(fd);
    exit(0);
}
