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

void parentProcess(pid_t pid, int fd){
	int pidParent = getpid();
	string pidVal = to_string(pidParent);
	string temp = "parent process " + pidVal + "\n";
	write(fd, temp.c_str(), temp.size());
}

void childProcess(pid_t pid, int fd){
	int pidChild = getpid();
	string pidVal = to_string(pidChild);
	string temp = "child process " + pidVal + "\n";
	write(fd, temp.c_str(), temp.size());
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

	// create the output file
	int fd = open("output.txt", O_RDWR | O_CREAT);
	chmod("output.txt", 0666);

	pid_t pids[NUM_NODES];
	cout << "hello world parent" << endl;
	parentProcess(getpid(), fd);

	for(int i = 0; i < NUM_NODES; i++){
		pids[i] = fork();
		if(pids[i] == 0){
			cout << "hello world child" << endl;
			childProcess(pids[i], fd);
			_exit(0);
		}
		else if(pids[i] < 0){
			cout << "fork failed" << endl;
			exit(-1);
		}
	}

	int status;
	int tempN = NUM_NODES;
	while(tempN > 0){
		wait(&status);
		tempN--;
	}		

	close(fd);

    return 0;
}
