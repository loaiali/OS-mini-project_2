#include<iostream>
#include<vector>
#include<fstream>
#include<string.h>
using namespace std;

struct message {
	int time = 0;
	//char operation[3];
	string operation = "Add";
	char message[64];
};


vector<message> input;

void readInput(string inputFileName) {
	ifstream inputFile;
	inputFile.open(inputFileName);
	message temp;
    int i = 0;
	while (!inputFile.eof()) {
        cout << i++;
		inputFile >> temp.time;
		inputFile >> temp.operation;
		inputFile >> temp.message;
		//// cout<<temp.operation<<"   "<<temp.message<<"  "<<temp.time<<endl;   
		input.push_back(temp);
	}
	inputFile.close();

}

main(int argc, char const *argv[])
{
    readInput("input.txt");
    //input.push_back(message());
    // input.erase(input.begin());
    return 0;
}
