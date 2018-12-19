#include<iostream>
#include <unistd.h>
#include <sys/wait.h>
#include<sys/msg.h>
#include<signal.h>
#include<string>
#include <sys/ipc.h>
#include<string>
#include<vector>
#include<fstream>
#include<string.h>
using namespace std;

#define DOWNQUE 13572468
#define UPQUE 24681357

void printLineError(bool error, string messageIfError);
void sendTerminateAndWait();

const long BORN_MTYPE = 1;
const long TERMINATE_MTYPE = 6;

using namespace std;
struct msgbuff
{
	long mtype;
	char mtext[64];//index 0 for operation type
	
};
struct message {
	int time;
	//char operation[3];
	string operation;
	char message[64];
};

int processClock = 0;
int id = getpid();
vector<message> input;

void handleCLock(int signum) {
	processClock++;
    cout << "----------------- clk #" << processClock << "-----------------" << endl;
}
void readInput(char* inputFileName) {
	ifstream inputFile;
	inputFile.open(inputFileName);
	message temp;
	while (!inputFile.eof()) {
		inputFile >> temp.time;
		inputFile >> temp.operation;
		inputFile >> temp.message;
		//// cout<<temp.operation<<"   "<<temp.message<<"  "<<temp.time<<endl;   
		input.push_back(temp);
	}
	inputFile.close();
}

int downQueue, upQueue;
int main(int argc, char*argv[]) {
	int x; 
	signal(SIGUSR2, handleCLock);
	downQueue = msgget(DOWNQUE, IPC_CREAT | 0644);
	upQueue = msgget(UPQUE, IPC_CREAT | 0644);
	cout << "up queue id: " << upQueue << endl;
	cout << "down queue id " << downQueue << endl;
	if (downQueue == -1 || upQueue == -1) {
		cout << "Error in creating Queues" << endl;
		exit(-1);
	}
	//send my id to the kernel
	msgbuff mess;
	mess.mtype = BORN_MTYPE;
	//memcpy(mess.mtext, &id, sizeof(id));
	
	to_string(id).copy(mess.mtext, to_string(id).size());
	cout<<mess.mtext<<"  "<<id<<" "<<mess.mtype<<sizeof(mess.mtext)<<argv[1]<<endl;
	msgsnd(upQueue, &mess, sizeof(mess.mtext), IPC_NOWAIT);
	
	
	cout<<"sent my id to kernel"<<endl;	
	
	readInput(argv[1]);
	for(int i=0;i<input.size()-1;i++){
		cout<<"i= "<<i<<" input operation "<<input[i].operation<<" input time "<<input[i].time<<" input message "<<input[i].message<<endl;
	}
	cout<<"readInput ended"<<endl;
	
	mess.mtype = id;
	cout<<"set mtype=id"<<mess.mtype<<endl;

	bool isError;	
	while (!input.empty()) {
		
		if (processClock >= input[0].time) {
			strcpy(mess.mtext + 1, input[0].message);
			if (input[0].operation == "ADD") {
				mess.mtext[0] = 'A';
			}
			else {
				mess.mtext[0] = 'D';
			}			
			cout << "sending to kernel " << mess.mtext<<" with mType: "<< mess.mtype<< endl;
			isError = msgsnd(upQueue, &mess, sizeof(mess.mtext), !IPC_NOWAIT) == -1;
			printLineError(isError, "can't send to kernal");
			cout<<"operation sent to kernel" << endl;
			do isError = msgrcv(downQueue, &mess, sizeof(mess.mtext), mess.mtype, !IPC_NOWAIT) == -1;
			while(isError && errno == 4);
			printLineError(isError, "can't recieve from kernel");
			cout<<"message received: "<< mess.mtext << endl;
			if (mess.mtext[0] == '0') cout << "Successfull add\n";
			else if (mess.mtext[0] == '1') cout << "Successfull Delete\n";
			else if (mess.mtext[0] == '2') cout << "unable to Add\n";
			else if (mess.mtext[0] == '3') cout << "unable to delete\n";
			input.erase(input.begin());
		}
	}
	//while(1);
	sendTerminateAndWait();
	cout << "Terminated :(" << endl;
}


void sendTerminateAndWait(){
	msgbuff mess;
	mess.mtype = TERMINATE_MTYPE;	
	to_string(id).copy(mess.mtext, to_string(id).size());
	msgsnd(upQueue, &mess, sizeof(mess.mtext), IPC_NOWAIT);

	bool isError;
	mess.mtype = id;
	do isError = msgrcv(downQueue, &mess, sizeof(mess.mtext), mess.mtype, !IPC_NOWAIT) == -1;
	while(isError && errno == 4);
}

void printLineError(bool error, string messageIfError) {
	if (error) {
		cout << messageIfError << endl;
		cout << "System says: " << errno;
		// exit(-1);
	}
}