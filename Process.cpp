#include<iostream>
#include<fstream>
#include<vector>
#include<algorithm>
#include<signal.h>
#include<stdio.h>
#include<unistd.h>
#include<sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include<string.h>

#define DOWNQUE 13572468
#define UPQUE 24681357

void setFakeInput();
void sendDeadStatus();

const long BORN_MTYPE = 1;
const long DEAD_MTYPE = 5;

using namespace std;
struct msgbuff
{
	long mtype;
	char mtext[64];//index 0 for operation type
};
struct message {
	int time;
	char operation[3];
	char message[64];
};

int processClock = 0;
int id = getpid();
vector<message> input;

void handleCLock(int signum) {
	processClock++;

}
void readInput(char* inputFileName) {
	ifstream inputFile;
	inputFile.open(inputFileName);
	message temp;
	while (!inputFile.eof()) {
		inputFile >> temp.time;
		inputFile >> temp.operation;
		inputFile >> temp.message;
		input.push_back(temp);
	}
}

int main(int argc, char*argv[]) {
	setFakeInput();
	signal(SIGUSR2, handleCLock);
	int downQueue = msgget(DOWNQUE, IPC_CREAT | 0644);
	int upQueue = msgget(UPQUE, IPC_CREAT | 0644);
	cout << "up queue id: " << upQueue << endl;
	cout << "down queue id " << downQueue << endl;
	if (downQueue == -1 || upQueue == -1) {
		cout << "Error in creating Queues" << endl;
		exit(-1);
	}
	
	//send my id to the kernel
	cout << "sending my id (" << id << ")..." << endl;
	msgbuff mess;
	mess.mtype = BORN_MTYPE;
	//// memcpy(mess.mtext, &id, sizeof(int));
	to_string(id).copy(mess.mtext, to_string(id).size());
	cout << "I will send a message of " << mess.mtext << endl;
	msgsnd(upQueue, &mess, sizeof(mess.mtext), !IPC_NOWAIT);
	cout << "sent to the queue successfully" << endl;
	mess.mtype = id;
	while (!input.empty()) {
		if (processClock >= input[0].time) {

			if (input[0].operation == "ADD") {
				mess.mtext[0] = 'A';
			}
			else {
				mess.mtext[0] = 'D';
			}

			strcpy(mess.mtext + 1, input[0].message);
			msgsnd(upQueue, &mess, sizeof(mess.mtext), !IPC_NOWAIT);
			msgrcv(downQueue, &mess, sizeof(mess.mtext), mess.mtype, !IPC_NOWAIT);

			if (mess.mtext == "0")cout << "Successfull add";
			else if (mess.mtext == "1")cout << "Successfull Delete";
			else if (mess.mtext == "2")cout << "unable to Add";
			else if (mess.mtext == "3")cout << "unable to delete";
			input.erase(input.begin());
		}
	}
	sendDeadStatus();
}


void sendDeadStatus(){
	message msg;
	msg.mtype = DEAD_MTYPE;
	to_string(id).copy(msgBuffer.message, to_string(id).size());
	bool isError = msgsnd(upQueId, &msg, sizeof(msg.message), !IPC_NOWAIT) == -1;
	if (isError) {
		cout << "can't send dead status" << endl;
		exit(-1);
	}
	cout << "my id is sent to the queue" << endl;
}

void setFakeInput(){
	//input.push_back(message())
}