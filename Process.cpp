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
using namespace std;
struct msgbuff
{
   long mtype;
   char mtext[65];//index 0 for operation type
};
struct message{
    int time;
    char operation[3];
    char message[64];
};

int processClock=0;
int id=getpid();
vector<message> input;

void handleCLock(int signum){
    processClock++;
    
}
void readInput(char* inputFileName){
    ifstream inputFile;
    inputFile.open(inputFileName);
    message temp;
    while(!inputFile.eof()){
        inputFile>>temp.time;
        inputFile>>temp.operation;
        inputFile>>temp.message;
        input.push_back(temp);
    }
}

int main(int argc,char*argv[]){
    
    signal (SIGUSR2, handleCLock);
    int downQueue= msgget (  555, IPC_CREAT|0644 );
	int upQueue=msgget( 666 ,IPC_CREAT|0644);
	if(downQueue==-1|| upQueue==-1){
		printf("Error in creating Queues");	
	}
    //send my id to the kernel
    msgbuff mess;
    mess.mtype =1;
    memcpy(mess.mtext,&id,sizeof(int));
    msgsnd(upQueue,&mess,sizeof(mess.mtext),!IPC_NOWAIT);
    
    mess.mtype=id;
    while(!input.empty()){
        if(processClock>=input[0].time){
            
            if(input[0].operation=="ADD"){
                mess.mtext[0]='A';
                }
            else {
                mess.mtext[0]='D';
            }
            
            strcpy(mess.mtext+1,input[0].message);
            msgsnd(upQueue,&mess,sizeof(mess.mtext),!IPC_NOWAIT);
            msgrcv(downQueue, &mess, sizeof(mess.mtext), mess.mtype, !IPC_NOWAIT);
            
            if(mess.mtext=="0")cout<<"Successfull add";
            else if (mess.mtext=="1")cout<<"Successfull Delete";
            else if(mess.mtext=="2")cout<<"unable to Add";
            else if (mess.mtext=="3")cout<<"unable to delete";
            input.erase(input.begin());
        }



    }

}
