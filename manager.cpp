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

int NUM_NODES;
int DEBUG = 1;
int MAX_CONNECTION_LENGTH = 8;

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

    return 0;
}
