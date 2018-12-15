#include<iostream>
#include <unistd.h>
#include <sys/wait.h>
#include<sys/msg.h>
#include<signal.h>
#include<string>
#include <sys/ipc.h>
#include <sys/types.h>
#include<vector>
#include<queue>
#include<map>

#define PROCESS_DOWNQUE 13572468
#define PROCESS_UPQUE 24681357
#define DISK_UPQUEUE 87654321
#define DISK_DOWNQUEUE 12345678

using namespace std;
int ctoi(char num);
void printLineError(bool error, string messageIfError);
void handleDelete(int signalnum);

struct MessageBuffer {
	long mtype;
	/* type of message */
	char message[64];
	/* message text */
};
struct ProcessRequest {
	long who;
	char operation;
	ProcessRequest(long who, char op) {
		this->who = who;
		this->operation = op;
	}
};

const long BORN_MTYPE = 1;
const long STATUS_MTYPE = 2;
const long NORMAL_MTYPE = 3;
int downQueId_process, upQueId_process, downQueId_disk, upQueId_disk;
int diskId = -1;
int clk = 0;
map<long, bool> liveProcesses;
queue<ProcessRequest> requests;


void clkSendAll() {
    int ret;
    if(diskId != -1){
	    ret = kill(diskId, SIGUSR2);
	    printLineError(ret == -1, "can't send SIGUSR2 to disk process");
    }
	for (auto const & p : liveProcesses)
	{
		if (!p.second) continue;
		long id = p.first;
		ret = kill(id, SIGUSR2);
		printLineError(ret == -1, "can't send SIGUSR2 to disk process");
	}
}


void kernalOneClk() {
	bool isError;

	// try to recieve from disk
	MessageBuffer receivedMessage;
	auto bytes = msgrcv(upQueId_disk, &receivedMessage, sizeof(receivedMessage.message), 0/*todo: disk mtype*/, IPC_NOWAIT);
	if (bytes == -1) {
		cout << "can't recieve from disk" << endl;
        cout << errno << endl;
		// exit(-1);
	}
	else if (bytes == 0) {
		cout << "no disk message in the clk" << endl;;
	}
	else if (receivedMessage.mtype == BORN_MTYPE) {
		// disk is born
        cout << "Disk is born.. hello baby disk" << endl;
		diskId = stoi(receivedMessage.message);
	}
	else if (receivedMessage.mtype == NORMAL_MTYPE) {
		int status = ctoi(receivedMessage.message[0]);
		cout << "status: " << status << " from disk\n";
		ProcessRequest requestedProcess = requests.front(); requests.pop();
		receivedMessage.mtype = requestedProcess.who;
		cout << "sending to process queue with mtype " << receivedMessage.mtype << endl;
		isError = msgsnd(downQueId_process, &receivedMessage, sizeof(receivedMessage.message), !IPC_NOWAIT) == -1;
		printLineError(isError, "can't send data to process");
		cout << "sent success\n";
	}


    if(clk == 20){
        // simulate the process

        return;
    }

	// try to recieve a message from any processes
	bytes = msgrcv(upQueId_process, &receivedMessage, sizeof(receivedMessage.message), 0, IPC_NOWAIT);
	if (bytes == -1) {
		cout << "can't recieve from process" << endl;
		// exit(-1);
	}
	else if (bytes == 0) {
		cout << "no process message in the clk" << endl;;
	}
	else if (receivedMessage.mtype == BORN_MTYPE) {
		cout << "A new process is born";
		long newProcessId = stol(receivedMessage.message);
		cout << " (Id = " << newProcessId << ")" << endl;
		//? should i call here incClk for this process
		liveProcesses[newProcessId] = true;
	}
	else {
		// there is a command in this clk
		cout << "command found from " << receivedMessage.mtype << ": " << receivedMessage.message;
		/*char* mes = receivedMessage.message; long mType = receivedMessage.mtype;*/
		int ret = kill(diskId, SIGUSR1); // read the status
										 // wait till the disk send the status
		MessageBuffer statusMessageBfr; statusMessageBfr.mtype = STATUS_MTYPE;
		bool isError = msgrcv(upQueId_disk, &statusMessageBfr, sizeof(statusMessageBfr.message), STATUS_MTYPE, !IPC_NOWAIT) == -1;
		printLineError(isError, "can't recieve disk status");
		int freeSlots = stoi(statusMessageBfr.message);
		char operation = receivedMessage.message[0];
		if (freeSlots == 0 && operation == 'A') {
			// add to full disk, send unsucessful add
			receivedMessage.message[0] = '2';
			isError = msgsnd(downQueId_process, &receivedMessage, sizeof(receivedMessage.message), !IPC_NOWAIT) == -1;
			printLineError(isError, "can't send feedback to the process queue");
		}
		else if (freeSlots == 10 && receivedMessage.message[0] == 'D') {
			// del from empty disk, send unsucessful del
			receivedMessage.message[0] = '3';
			isError = msgsnd(downQueId_process, &receivedMessage, sizeof(receivedMessage.message), !IPC_NOWAIT) == -1;
			printLineError(isError, "can't send feedback to the process queue");
		}
		else if (operation == 'A') {
			requests.push(ProcessRequest(receivedMessage.mtype, 'A'));
			receivedMessage.mtype = 1; // 1 is add
			isError = msgsnd(downQueId_disk, &receivedMessage, sizeof(receivedMessage.message), !IPC_NOWAIT) == -1;
			printLineError(isError, "can't send command add to the disk queue");
		}
		else if (operation == 'D') {
			receivedMessage.mtype = 2; // 2 is del
			isError = msgsnd(downQueId_disk, &receivedMessage, sizeof(receivedMessage.message), !IPC_NOWAIT) == -1;
			printLineError(isError, "can't send command del to the disk queue");
		}
		else {
			cerr << "invalid operation" << endl;
		}
	}
}

int main(int argc, char const *argv[])
{
	signal(SIGINT, handleDelete);
	downQueId_process = msgget(PROCESS_DOWNQUE, IPC_CREAT | 0644);
	cout << "process down queue id " << downQueId_process << endl;
	upQueId_process = msgget(PROCESS_UPQUE, IPC_CREAT | 0644);
	cout << "process up queue id: " << upQueId_process << endl;

	downQueId_disk = msgget(DISK_DOWNQUEUE, IPC_CREAT | 0644);
	cout << "disk down queue id " << downQueId_disk << endl;
	upQueId_disk = msgget(DISK_UPQUEUE, IPC_CREAT | 0644);
	cout << "disk up queue id: " << upQueId_disk << endl;

	while (true) {
		sleep(1);
		clk += 1;
		clkSendAll();
		cout << "--------------- Clk #" << clk << " -------------------" << endl;

		kernalOneClk();
		
		cout << endl;
	}
	return 0;
}




void handleDelete(int signalnum) {
	cout << "i will delete";
	msgctl(downQueId_process, IPC_RMID, (struct msqid_ds *) 0);
	msgctl(upQueId_process, IPC_RMID, (struct msqid_ds *) 0);
	msgctl(downQueId_disk, IPC_RMID, (struct msqid_ds *) 0);
	msgctl(upQueId_disk, IPC_RMID, (struct msqid_ds *) 0);
	cout << "deleted";
	exit(0);
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