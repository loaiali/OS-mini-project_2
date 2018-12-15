/**
* The disk receive messages from kernel only and respond to it
* it tries to receive the command iff it is not busy. if it is not busy wait(block) till something comes to it
* ? this valid only if the SIGUSR2 still working while the process is blocked OR signals is buffered somewhere and called by order
* after it finished it sends to the queue and tell that is successfully
*/
#include<iostream>
#include <unistd.h>
#include <sys/wait.h>
#include<sys/msg.h>
#include<signal.h>
#include<string>
#include <sys/ipc.h>
#include<string>
using namespace std;

#define DOWNQUE 12345678
#define UPQUE 87654321
#define MESSAGE_SIZE 64
#define SLOTS_NUM 10


struct MessageBuffer {
	long mtype;
	/* type of message */
	char message[MESSAGE_SIZE];
	/* message text */
};

int ctoi(char num);
void printLineError(bool error, string messageIfError);


long mType;
const long BORN_MTYPE = 1;
const long STATUS_MTYPE = 2;
const long NORMAL_MTYPE = 3;
int downQueId, upQueId;

struct Disk{
	string* slots;
	bool* freeSlots;
    int clk = 0;

	Disk() : addLatency(3), deleteLatency(1) {
		this->clk = 0;
		operationReminder = 0;
		slots = new string[SLOTS_NUM];
		freeSlots = new bool[SLOTS_NUM];
	}

	void inc() {
		clk += 1;
		if (operationReminder == 1) {
			cout << "added successfully" << endl;
			cout << "sending to up queue the feedback" << endl;
			MessageBuffer msgBuffer;
			msgBuffer.mtype = NORMAL_MTYPE;
			msgBuffer.message[0] = '1';
			bool isError = msgsnd(upQueId, &msgBuffer, sizeof(msgBuffer.message), !IPC_NOWAIT) == -1;
			if (isError) {
				cout << "can't send feedback" << endl;
				exit(-1);
			}
			cout << "sent to up queue successfully" << endl;
		}
		operationReminder = operationReminder != 0 ? operationReminder - 1 : 0;
	}
	~Disk() {
		delete[] slots;
		delete[] freeSlots;
	}

	string diskStatus() {
		string status = "";
		for (int i = 0; i<SLOTS_NUM; i++)
			status += freeSlots[i] ? "1" : "0";
		return status;
	}

	int countFreeSlots() {
		int free = 0;
		for (int i = 0; i<SLOTS_NUM; i++)
			free += freeSlots[i];
		return free;
	}

	bool add(string message) {
		if (operationReminder != 0) {
			cerr << "Error! writing into disk while its busy: " << operationReminder << " clks needed to end operation";
			return false;
		}
		for (int i = 0; i < SLOTS_NUM; i++) {
			if (freeSlots[i]) {
				slots[i] = message;
				freeSlots[i] = false;
				cout << "message '" << message << "' is being written in the disk in slot #" << i << endl;
				operationReminder = addLatency;
				return true;
			}
		}
	}

	bool isBusy() { return operationReminder != 0; }
	bool del(int slot) {
		if (operationReminder != 0) {
			cerr << "Error! deleting from disk while its busy: " << operationReminder << " clks needed to end operation";
			return false;
		}
		freeSlots[slot] = true;
		cout << "message ' " << slots[slot] << "' in slot #" << slot << " is being deleted" << endl;
		operationReminder = deleteLatency;
	}
private:
	const int addLatency = 3, deleteLatency = 1;
	int operationReminder; //* I can use clk only instead of this var, as here in the disk i didn't use it never, but in real world they should be clk, and count is only for sim

}disk;

void handleClk(int signalnum) {
	// cout << "handleClk\n";
    //? should i put the code here of receiving from queue -> i think yes
    cout << "----------------- clk #" << disk.clk << "-----------------" << endl;
	disk.inc();
}


void handleMessage(char* message, int size) {
	char op = message[0];
	char* actualMes = &message[2];
	if (op == 'A') {
		disk.add(actualMes);
	}
	else if (op == 'D') {
		disk.del(ctoi(actualMes[0]));
	}
	else {
		cerr << "Error!: BAD FORMAT '" << message << "'\n";
	}
	
}

void handleStatus(int signalnum) {
	cout << "handleStatus\n";
	string num = to_string(disk.countFreeSlots());
	MessageBuffer msgBuffer;
	msgBuffer.mtype = STATUS_MTYPE;
	num.copy(msgBuffer.message, 64);
	bool isError = msgsnd(upQueId, &msgBuffer, sizeof(msgBuffer.message), !IPC_NOWAIT) == -1;
	if (isError) {
		cout << "can't send status to the caller";
		exit(-1);
	}
	cout << "status sent successfully" << endl;
}


void sendBornStatus(int id) {
	MessageBuffer msgBuffer;
	msgBuffer.mtype = BORN_MTYPE;
	to_string(id).copy(msgBuffer.message, to_string(id).size());
	bool isError = msgsnd(upQueId, &msgBuffer, sizeof(msgBuffer.message), !IPC_NOWAIT) == -1;
	if (isError) {
		cout << "can't send my id to process" << endl;
		exit(-1);
	}
	cout << "my id is sent to the queue" << endl;
}
int main(int argc, char const *argv[])
{
	// todo: send my id to the kernel
	int id = getpid();
	signal(SIGUSR2, handleClk);
	signal(SIGUSR1, handleStatus);

	downQueId = msgget(DOWNQUE, IPC_CREAT | 0644);
	upQueId = msgget(UPQUE, IPC_CREAT | 0644);
	cout << "up queue id: " << upQueId << endl;
	cout << "down queue id " << downQueId << endl;

	sendBornStatus(id);


	while (true) {
        // todo: block here and move the following code to the handleclk function
		if (disk.isBusy()) continue;
		//? not busy, so block for new command --> can't block as sigusr1 when come it returns from it with errno 4 (Interrupted system call)
		MessageBuffer receivedMessage;
		bool isError = msgrcv(downQueId, &receivedMessage, sizeof(receivedMessage.message), -2, IPC_NOWAIT) == -1; //? normal_mtype, or 0
		if (isError) {
			//cerr << "can't recieve\n";
            //cout << "system says: " << errno << endl;
			// exit(-1);
            continue;
		}
		handleMessage(receivedMessage.message, 64);
	}

	return 0;
}




void printLineError(bool error, string messageIfError) {
	if (error) {
		cout << messageIfError << endl;
		cout << "System says: " << errno;
		exit(-1);
	}
}

int ctoi(char num) {
	return stoi(string(1, num));
}