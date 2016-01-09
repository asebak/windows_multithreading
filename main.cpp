//By Ahmad Sebak

#include "stdafx.h"
#include <windows.h>
#include <process.h>
#include <iostream>
#include <sstream>
#include <time.h>
#include <fstream>
#include <vector>
#include <map>
using namespace std;
#define TIMEQUANTUM 1000//Time Quantum
static CRITICAL_SECTION Mutex;//For locking the api calls
int TotalOSProcesses=0;//for the scheduler thread
ofstream output("Output.txt");//output file
ofstream vm( "vm.txt" ); //virtual memory file
enum State{Started,Resumed,Paused,Finished};
int currentCommand=0;
struct Storage
{
string variableid;
unsigned int value;
int commandID;
};

struct Release
{
string variableid;
int commandID;
};

struct Lookup
{
string variableid;
int commandID;

};

struct Process
{
State ProcessState;
int ReadyTime;//when the process is ready
int ServiceTime;//process duration
int ServiceTimeDuplicate;//the actualy amount of service time remaining
int ThreadID;//process identifier
bool hasCPU1;
bool hasCPU2;
bool markedAsStarted;
bool markedAsFinished;
};
bool Schedule=true;//for the main thread 
int MainMemory=0;
bool schedulingprocesses=true;//to continue the scheduler until all processes are done
bool flag=true;
vector<Process> ProcessList;// all the processes stored in a vector
vector<Storage> StorageList;// storage commands
vector<Release> ReleaseList;// release commands
vector<Lookup> LookupList;// lookup commands
map<string,int> MemoryMap;// mainmemory map
Process *currentProcess;
vector<Process> P;

void store(string variableid,unsigned int value)
	{

		if(MemoryMap.size()<MainMemory)//check if the size is smaller than the main memory size
		{
		MemoryMap[variableid]=value;//place it on the map
		cout << "Clock: "<< clock() <<", Process: "<< currentProcess->ThreadID <<"," << " Store: Variable "<< variableid <<", Value: " << value << endl;
		output << "Clock: "<< clock() <<", Process: "<<currentProcess->ThreadID <<"," << " Store: Variable "<< variableid <<", Value: " << value << endl;
		}
		else//MEMORY FULL HERE store in VM.TXT
		{
			cout << "Clock: "<< clock() <<", Process: "<< currentProcess->ThreadID <<"," << " Store: Variable "<< variableid <<", Value: " << value << " (In Virtual Memory)"<<endl;
			output << "Clock: "<< clock() <<", Process: "<< currentProcess->ThreadID <<"," << " Store: Variable "<< variableid <<", Value: " << value << " (In Virtual Memory)"<<endl;
			vm << variableid << " " << value << endl;
		}
	}

void release(string variableid)
	{
		std::map<string,int>::iterator it;
		it=MemoryMap.find(variableid);//find the key in the map
		if(it!=MemoryMap.end())// if found, remove it and output the state
		{
			cout << "Clock: "<< clock() <<", Process: "<< currentProcess->ThreadID<<"," << " Release: Variable "<< it->first << endl;
			output << "Clock: "<< clock() <<", Process: "<< currentProcess->ThreadID<<"," << " Release: Variable "<< it->first << endl;
		MemoryMap.erase(it);
		}
		else//not found in memorymap search virtual memory instead
		{
		vector<Storage> tempStorage;
		string path="vm.txt";
		string line;
		string ParsedData;
		Storage * s=new Storage();
		bool invm=false;
		int counter=0;
		ifstream file(path,ifstream::in);//get file
		while(getline(file,line))//gets all lines in the text file
		{
			istringstream currentLine(line);// the current line
			while(getline(currentLine,ParsedData,' '))// gets current line seperates it passed on the delimiter ' '
			{   
				switch (counter)
				{
				case 0:
					if(variableid!=ParsedData)//if the variable id is not in the text file then add it to the vector
					{
					s->variableid=ParsedData;
					counter++;
					}
					else//if found put random counter value, goes to default for the value then resets to 0;
					{
						invm=true;
						counter=3;
					}
				break;
				case 1:
					s->value=atoi(ParsedData.c_str());
					tempStorage.push_back(*s);
					s=new Storage();
					counter=0;
				break;
				default:
					counter=0;
					break;
				}
			}
		}
		delete s;
		if(invm)//if found reconstruct the vm text file without that variable
		{
		cout << "Clock: "<< clock() <<", Process: "<< currentProcess->ThreadID<<"," << " Release: Variable "<< variableid <<" (In Virtual Memory)" << endl;
		output << "Clock: "<< clock() <<", Process: "<< currentProcess->ThreadID<<"," << " Release: Variable "<< variableid <<" (In Virtual Memory)" << endl;
		remove("vm.txt");
		for(Storage s:tempStorage)//reconstruct text file
		{
			vm << s.variableid << " " << s.value << endl;
		}
		}
		else
		{
		cout << "Clock: "<< clock() <<", Process: "<< currentProcess->ThreadID<<"," << " Release: Variable "<< variableid <<" (Not In Main or Virtual Memory)" << endl;
		output << "Clock: "<< clock() <<", Process: "<< currentProcess->ThreadID<<"," << " Release: Variable "<< variableid <<" (Not In Main or Virtual Memory)" << endl;
		}
		}
	}

int lookup(string variableid)
	{
		int doesnotexist=-1;
		std::map<string,int>::iterator it;
		it=MemoryMap.find(variableid);
		if(it!=MemoryMap.end())//if found in mainmemory
		{
			cout << "Clock: "<< clock() <<", Process: "<< currentProcess->ThreadID <<", " << "LookUp: Variable "<< variableid <<", Value: " << it->second<< endl;
			output << "Clock: "<< clock() <<", Process: "<< currentProcess->ThreadID <<", " << "LookUp: Variable "<< variableid <<", Value: " << it->second<< endl;
			return (it->second);
		}
		else//doesnt exist in main memory check the virtual memory
		{

		//check vm if the variable exists
		//if it does not exist return a -1
		//if it exists check if map size is less than memorysize
		//if it is put it in memorymap without doing anything, 
		//if memorymap is full swap the variableid and value with the oldest accessed one(smallest clock size)

		vector<Storage> tempStorage;
		string path="vm.txt";
		string line;
		string ParsedData;
		Storage * s=new Storage();
		Storage valuefound;
		bool invm=false;
		int counter=0;
		ifstream file(path,ifstream::in);//get file
		while(getline(file,line))//gets all lines in the text file
		{
			istringstream currentLine(line);// the current line
			while(getline(currentLine,ParsedData,' '))// gets current line seperates it passed on the delimiter ' '
			{   
				switch (counter)
				{
				case 0:
					if(variableid!=ParsedData)//if the variable id is not in the text file then add it to the vector
					{
					s->variableid=ParsedData;
					counter++;
					}
					else//if found put random counter value, goes to default for the value then resets to 0;
					{
						valuefound.variableid=ParsedData;
						invm=true;
						counter=3;
					}
				break;
				case 1:
					s->value=atoi(ParsedData.c_str());
					tempStorage.push_back(*s);
					s=new Storage();
					counter=0;
				break;
				default:
					valuefound.value=atoi(ParsedData.c_str());
					counter=0;
					break;
				}
			}
		}
		delete s;

		if(invm)
		{
		remove("vm.txt");
		for(Storage s:tempStorage)//reconstruct text file without that variable
		{
			vm << s.variableid << " " << s.value << endl;
		}
		if(MemoryMap.size()<MainMemory)
		{
			MemoryMap[valuefound.variableid]=valuefound.value;
			output << "Clock: "<< clock() <<", Process: "<< currentProcess->ThreadID <<", " << "LookUp: Variable "<< valuefound.variableid <<", Value: " << valuefound.value << endl;
			cout << "Clock: "<< clock() <<", Process: "<< currentProcess->ThreadID <<", " << "LookUp: Variable "<< valuefound.variableid <<", Value: " << valuefound.value << endl;
		}
		else//remove the oldest element which is the first value in the map
		{
			  map<string, int>::iterator it;
			  it=MemoryMap.begin();
			  output << "Clock: " << clock() <<", Memory Manager" <<", SWAP: " <<"Variable "<< valuefound.variableid <<" with " << "Variable "<<it->first <<endl ;
			  cout << "Clock: " << clock() <<", Memory Manager" <<", SWAP: " <<"Variable "<< valuefound.variableid <<" with " << "Variable "<<it->first <<endl ;
			  MemoryMap.erase(it);
			  MemoryMap[valuefound.variableid]=valuefound.value;
			  output << "Clock: "<< clock() <<", Process: "<< currentProcess->ThreadID <<", " << "LookUp: Variable "<< valuefound.variableid<<", Value: " << valuefound.value<< endl;
			  cout << "Clock: "<< clock() <<", Process: "<< currentProcess->ThreadID <<", " << "LookUp: Variable "<< valuefound.variableid<<", Value: " << valuefound.value<< endl;
		}
		return valuefound.value;
		}
		}
		
		return doesnotexist;
		
	}

void ReadProcessesTxTFile(string filepath)
{
	string path=filepath;
	string line;
	string ParsedData;
	//intialization
	Process* CurrentProcess=new Process();
	int ProcessesDone=0;
	int counter=-1;
	int UserThreadCount=1;
	//
		ifstream file(path,ifstream::in);//get file
		while(getline(file,line))//gets all lines in the text file
		{
			istringstream currentLine(line);// the current line
			while(getline(currentLine,ParsedData,' '))// gets current line seperates it passed on the delimiter ' '
			{   
				switch (counter)
				{
				case -1:
					counter++;
					break;
				case 0:
					CurrentProcess->ReadyTime=atoi(ParsedData.c_str())*1000;//the read time * 1000
					counter++;
				break;
				case 1:
					CurrentProcess->ServiceTime=atoi(ParsedData.c_str())*1000; // Service time * 1000
					CurrentProcess->ServiceTimeDuplicate=atoi(ParsedData.c_str())*1000;
					counter=0;
					//next lines are just intializations
					CurrentProcess->ThreadID=UserThreadCount;
					UserThreadCount++;
					CurrentProcess->hasCPU1=false;
					CurrentProcess->hasCPU2=false;
					CurrentProcess->ProcessState=(State)1;//set the state to anything but started or finished
					CurrentProcess->markedAsFinished=false;
					CurrentProcess->markedAsStarted=false;
					ProcessList.push_back(*CurrentProcess);//push in the vector the current process
					CurrentProcess=new Process();//reset everything
					TotalOSProcesses++;
				break;
				}
			}
		}
		delete CurrentProcess;//release the memory
}

void ReadMemConfigTxTFile(string filepath)
{
	string path=filepath;
	string line;
	ifstream file(path,ifstream::in);//get file
		while(getline(file,line))//gets all lines in the text file
		{
			MainMemory=atoi(line.c_str());
		}
}

void ReadCommandsTxTFile(string filepath)
{
	string path=filepath;
	string line;
	string ParsedData;
	Lookup * l=new Lookup();
	Release * r=new Release();
	Storage * s=new Storage();
	bool firststorage=false;
	int thiscommand=0;
	int counter=0;
		ifstream file(path,ifstream::in);//get file
		while(getline(file,line))//gets all lines in the text file
		{
			istringstream currentLine(line);// the current line
			while(getline(currentLine,ParsedData,' '))// gets current line seperates it passed on the delimiter ' '
			{   
				switch (counter)
				{
				case 0://the actual string of the message
					if(ParsedData=="Store")
					{
						counter=1;
					}
					else if (ParsedData=="Release")
					{
						counter=2;
					}
					else if(ParsedData=="Lookup")
					{
						counter=3;
					}
				break;
				case 1://for store
					if(!firststorage)
					{
						s->variableid=ParsedData;
						firststorage=true;
					}
					else
					{
						s->value=atoi(ParsedData.c_str());
						s->commandID=thiscommand;
						StorageList.push_back(*s);
						firststorage=false;
						s=new Storage();
						thiscommand++;
						counter=0;
					}
				break;
				case 2://for release
					r->variableid=ParsedData;
					r->commandID=thiscommand;
					ReleaseList.push_back(*r);
					r=new Release();
					thiscommand++;
					counter=0;
					break;
				case 3://for lookup
					l->variableid=ParsedData;
					l->commandID=thiscommand;
					LookupList.push_back(*l);
					l=new Lookup();
					thiscommand++;
					counter=0;
					break;

				}
			}
		}
		delete l;
		delete r;
		delete s;
}

void WriteOutput(int ProcessID,State ProcessState)
{
	string theProcessState;
	switch (ProcessState)//check the enum state
	{
	case Started:
		theProcessState="Started";
		break;
	case Resumed:
		theProcessState="Resumed";
		break;
	case Paused:
		theProcessState="Paused";
		break;
	case Finished:
		theProcessState="Finished";
		break;
	default:
		break;
	}
	int clocktime=clock()/1000;
	clocktime*=1000;
	output << "Clock: " << clocktime << ", " <<"Process: " << ProcessID << ", "  << theProcessState << endl;
	cout << "Clock: " << clocktime  << ", " <<"Process: " << ProcessID << ", "  << theProcessState << endl;
}

unsigned int __stdcall ProcessThread(void* a)
{	
	Process* info = (Process*)a;
	long executionDuration = 0;
	//Continue until total execution reaches the duration of the thread
	while(executionDuration < info->ServiceTime)
	{
		//Wait until get time slice from the scheduler
		while(!info->hasCPU1 && !info->hasCPU2){}
		//Calculate current cycle execution time 

		long executionStart = clock();
		long cycleDuration = 0;
		//for api , delay before a api call then choose a random api to do.
		srand(clock());//seed
		int randomwaitperiod= 300 + (rand() % (int)(1000 - 300 + 1));
		Sleep(randomwaitperiod);

		EnterCriticalSection(&Mutex);
		currentProcess=info;
		for(Storage s:StorageList)
		{
			if(s.commandID==currentCommand)
			{
				store(s.variableid,s.value);
				goto EXIT;
			}
			else
				continue;
		}
		for(Release r:ReleaseList)
		{
			if(r.commandID==currentCommand)
			{
				release(r.variableid);
				goto EXIT;
			}
			else
				continue;
		}
		for(Lookup l:LookupList)
		{
			if(l.commandID==currentCommand)
			{
				lookup(l.variableid);
				goto EXIT;
			}
			else
				continue;
		}
		EXIT:
		currentCommand++;
		LeaveCriticalSection(&Mutex);

		while((info->hasCPU1||info->hasCPU2) && executionDuration + cycleDuration < info->ServiceTime)
		{
			cycleDuration = clock() - executionStart;	
		}
		executionDuration += cycleDuration;
	}
	//Set the thread as finished
	info->ProcessState=Finished;
	return 0;
}

unsigned int __stdcall SchedulerThreadCPU1(void* AllProcesses)
{
	vector<Process>* ProcessMultithreading=(vector<Process>*)AllProcesses;
	 P=*ProcessMultithreading;//transfer pointer
	int CurrentTime;//the current time
	int totalprocesses=TotalOSProcesses;
	int lowestServiceTime;
	Sleep(1000);//delay this thread to start at a clock of 1 second
	int leastIndex=0;//the index with the least remaining service time
	while(schedulingprocesses)
	{
		CurrentTime=clock();
		lowestServiceTime=100000000000;//random very large value to compare the service time

		for(size_t i=0;i!=P.size();++i)
		{
			if(P[i].ReadyTime<=CurrentTime)//check if the process is ready
			{
				if(P[i].ServiceTimeDuplicate<lowestServiceTime && P[i].ProcessState!=Finished&&!P[i].hasCPU2)//check if the Current Service time of the process is lower than the lowest service time
				{
					lowestServiceTime=P[i].ServiceTimeDuplicate;//switch
					leastIndex=i;//get that index
				}
			}
		}

		if(P.at(leastIndex).ProcessState!=Started && !P.at(leastIndex).markedAsStarted)//check if it has not started then start a new thread for it
		{
			P.at(leastIndex).ProcessState=Started;
			WriteOutput(P.at(leastIndex).ThreadID,P.at(leastIndex).ProcessState);
			_beginthreadex(NULL, 0, ProcessThread, &P.at(leastIndex), 0, 0);//start a new thread for that process
			P.at(leastIndex).markedAsStarted=true;
		}

		if(P.at(leastIndex).markedAsStarted && P.at(leastIndex).ProcessState!=Finished && !P.at(leastIndex).hasCPU2)//if a thread has started go here
		{
		P.at(leastIndex).hasCPU1=true;
		P.at(leastIndex).ProcessState=Resumed;
		WriteOutput(P.at(leastIndex).ThreadID,P.at(leastIndex).ProcessState);
		int startClockDelay = clock();//from round robin example
		while(P.at(leastIndex).ProcessState!=Finished && clock()-startClockDelay <=TIMEQUANTUM){}//from round robin example
		P.at(leastIndex).ServiceTimeDuplicate-=TIMEQUANTUM;
		P.at(leastIndex).hasCPU1=false;
		if(P.at(leastIndex).ProcessState!=Finished)//if it's not finished it becomes paused
		{
			P.at(leastIndex).ProcessState=Paused;
			WriteOutput(P.at(leastIndex).ThreadID,P.at(leastIndex).ProcessState);
		}
		}

		if(P.at(leastIndex).ProcessState==Finished&&!P.at(leastIndex).markedAsFinished)//check to see if it is finished and subtract 1 process
		{
				WriteOutput(P.at(leastIndex).ThreadID,P.at(leastIndex).ProcessState);
				P.at(leastIndex).markedAsFinished=true;
				TotalOSProcesses--;
		}
		 flag=true;
		
		 for(size_t i=0;i!=P.size();++i)
		{
			if(P[i].ProcessState==Finished)
				continue;
			else
				flag=false;
		}
		 if(flag)
		schedulingprocesses=false;
		if(TotalOSProcesses==0)
		   schedulingprocesses=false;//break from the scheduler thread	

		}

	
Schedule=false;
return 0;
}

unsigned int __stdcall SchedulerThreadCPU2(void* AllProcesses)
{
	vector<Process>* ProcessMultithreading=(vector<Process>*)AllProcesses;
	P=*ProcessMultithreading;//transfer pointer
	int CurrentTime;//the current time
	int totalprocesses=TotalOSProcesses;
	int lowestServiceTime;
	Sleep(1100);//delay this thread to start at a clock of 1 second
	int leastIndex=0;//the index with the least remaining service time
	int secondleastIndex=0;
	int min1=P[0].ServiceTimeDuplicate;
	int min2=0;
	while(schedulingprocesses)
	{
		CurrentTime=clock();
		lowestServiceTime=100000000000;//random very large value to compare the service time
		for(size_t i=0;i!=P.size();++i)
		{
			if(P[i].ReadyTime<=CurrentTime)//check if the process is ready
			{
				if(P[i].ServiceTimeDuplicate<lowestServiceTime && P[i].ProcessState!=Finished&&!P[i].hasCPU1)//check if the Current Service time of the process is lower than the lowest service time
				{
					secondleastIndex=leastIndex;
					min2=min1;
					min1=P[i].ServiceTimeDuplicate;
					lowestServiceTime=P[i].ServiceTimeDuplicate;//switch
					leastIndex=i;//get that index
				}
			}
		}
		if(P.at(secondleastIndex).markedAsStarted && !P.at(secondleastIndex).ProcessState!=Finished && !P.at(secondleastIndex).hasCPU1)//if a thread has started go here
		{
		P.at(secondleastIndex).hasCPU2=true;
		int startClockDelay = clock();//from round robin example
		while(P.at(secondleastIndex).ProcessState!=Finished && clock()-startClockDelay <=TIMEQUANTUM){}//from round robin example
		P.at(secondleastIndex).ServiceTimeDuplicate-=TIMEQUANTUM;
		P.at(secondleastIndex).hasCPU2=false;
		if(!P.at(secondleastIndex).ProcessState!=Finished &&!P.at(secondleastIndex).markedAsFinished)
			WriteOutput(P.at(secondleastIndex).ThreadID,P.at(secondleastIndex).ProcessState);
		}



	

		
	}
return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
InitializeCriticalSection(&Mutex);
ReadProcessesTxTFile("processes.txt");
ReadMemConfigTxTFile("memconfig.txt");
ReadCommandsTxTFile("commands.txt");
remove("vm.txt");
_beginthreadex(NULL, 0, SchedulerThreadCPU1,&ProcessList, 0, 0);//start a new thread for the scheduler
_beginthreadex(NULL, 0, SchedulerThreadCPU2,&ProcessList, 0, 0);//start a new thread for the scheduler
while(Schedule){}
	return 0;
}

