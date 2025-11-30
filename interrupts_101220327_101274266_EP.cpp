#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <thread>
#include <chrono>

//Obj-PCB
struct PCB{
    int pid;
    int memorySize; 
    int arrivalTime;
    int totalTime;
    int frequency;
    int duration;
    int cooldown;
    int eachRunTime;
    std::string state;
    int priority;
};

//Obj-Partition
struct Partition{
    int space;
    int occupied;
};

//Obj-ExecutionLog
struct ExecutionLog{
    int timeOfTransition;
    int pid; 
    std::string oldState;
    std::string newState;
};

//Obj-MemoryStatusLog
struct MemoryStatusLog{
    int timeOfEvent;
    int memoryUsed;
    std::string partitionsState;
    int totalFreeMemory;
    int usableFreeMemory;
};

//Func-Read input data
std::vector<PCB> readInputDataFile(const std::string& filename){
    std::vector<PCB> tempPCBs;
    std::ifstream infile(filename);
    std::string line;
    while (std::getline(infile, line)){
        PCB tempPCB;

        std::string pidStr;
        std::string memorySizeStr;
        std::string arrivalTimeStr;
        std::string totalTimeStr;
        std::string frequencyStr;
        std::string durationStr;

        std::stringstream ss(line);
        std::getline(ss, pidStr, ',');
        std::getline(ss, memorySizeStr, ',');
        std::getline(ss, arrivalTimeStr, ',');
        std::getline(ss, totalTimeStr, ',');
        std::getline(ss, frequencyStr, ',');
        std::getline(ss, durationStr, ',');
        
        tempPCB.pid = std::stoi(pidStr);
        tempPCB.memorySize = std::stoi(memorySizeStr);
        tempPCB.arrivalTime = std::stoi(arrivalTimeStr);
        tempPCB.totalTime = std::stoi(totalTimeStr);
        tempPCB.frequency = std::stoi(frequencyStr);
        if (tempPCB.frequency == 0){tempPCB.frequency = 99999;}
        tempPCB.duration = std::stoi(durationStr);
        tempPCB.cooldown = 0;
        tempPCB.eachRunTime = 0;
        tempPCB.state = "NEW";
        tempPCB.priority = tempPCB.pid;

        tempPCBs.push_back(tempPCB);
    }
    infile.close();
    return tempPCBs;
}

//Func-Execution Log Maker
void eLog(std::vector<ExecutionLog>& executionLogs, const int& tick, const PCB& pcb){
    ExecutionLog tempLog;
    tempLog.timeOfTransition = tick;
    tempLog.pid = pcb.pid;
    std::string oldState = "NEW";
    for(const auto& eachLog : executionLogs){
        if(eachLog.pid == pcb.pid){
            oldState = eachLog.newState;
        }
    }
    tempLog.oldState = oldState;
    tempLog.newState = pcb.state;
    executionLogs.push_back(tempLog);
}

//Func-Memory Status Log Maker
void mLog(std::vector<MemoryStatusLog>& memoryStatusLogs, const int& tick, const std::vector<PCB>& PCBStack, const std::vector<Partition>& partitions){
    MemoryStatusLog tempLog;

    tempLog.timeOfEvent = tick;
    tempLog.memoryUsed = 0;
    for(const auto& eachPartition : partitions){
        if (eachPartition.occupied != -1) {
            for (const auto& eachPCB : PCBStack){
                if (eachPCB.pid == eachPartition.occupied) {
                    tempLog.memoryUsed += eachPCB.memorySize;
                }
            }
        }
    }
    tempLog.partitionsState = "";
    for (const auto& eachPartition : partitions) {
        tempLog.partitionsState += std::to_string(eachPartition.occupied) + ", ";
    }
    tempLog.partitionsState.pop_back();
    tempLog.partitionsState.pop_back();
    tempLog.totalFreeMemory = 100-tempLog.memoryUsed;
    tempLog.usableFreeMemory = 0;
    for (const auto& eachPartition : partitions){
        if (eachPartition.occupied == -1) tempLog.usableFreeMemory += eachPartition.space;
    }
    memoryStatusLogs.push_back(tempLog);
}

//Func-Visualization Log Maker
void vLog(std::vector<std::string>& visualizationLogs, const std::vector<PCB>& PCBStack){
    for(int i = 0; i < PCBStack.size(); i++){
        if(PCBStack[i].state == "NEW"){
            visualizationLogs[i] += " ";
        }
        else if(PCBStack[i].state == "WAITING"){
            visualizationLogs[i] += "-";
        }
        else if(PCBStack[i].state == "READY"){
            visualizationLogs[i] += "=";
        }
        else if(PCBStack[i].state == "RUNNING"){
            visualizationLogs[i] += "#";
        }
        else if(PCBStack[i].state == "TERMINATED"){
            visualizationLogs[i] += "";
        }
    }
}

//Func-Partitions initalizer
std::vector<Partition> partitionsInitalizer(){
    std::vector<Partition> tempPartitions;
    Partition tempPartition;

    tempPartition.occupied = -1;

    tempPartition.space = 40;
    tempPartitions.push_back(tempPartition);

    tempPartition.space = 25;
    tempPartitions.push_back(tempPartition);

    tempPartition.space = 15;
    tempPartitions.push_back(tempPartition);

    tempPartition.space = 10;
    tempPartitions.push_back(tempPartition);

    tempPartition.space = 8;
    tempPartitions.push_back(tempPartition);

    tempPartition.space = 2;
    tempPartitions.push_back(tempPartition);

    return tempPartitions;
}

//Func-Partition allocator
bool partitionAllocator(const PCB& pcb, std::vector<Partition>& partitions){
    for (int i = 5; i >= 0; i--) {
        if (partitions[i].occupied == -1 && partitions[i].space >= pcb.memorySize) {
            partitions[i].occupied = pcb.pid;
            return true;
        }
    }
    return false;
}

//Func-Partition releaser
void partitionReleaser(const PCB& pcb, std::vector<Partition>& partitions){
    for (auto& eachPartition : partitions){
        if (eachPartition.occupied == pcb.pid){
            eachPartition.occupied = -1;
        }
    }
}

//Func-Update waiting stack
void updateWaitingStack(std::vector<MemoryStatusLog>& memoryStatusLogs, const int& tick, std::vector<PCB>& PCBStack, std::vector<Partition>& partitions){
    for(auto& eachPCB : PCBStack){
        if(eachPCB.state == "NEW" && eachPCB.arrivalTime <= 0 && partitionAllocator(eachPCB, partitions)){
            mLog(memoryStatusLogs, tick, PCBStack, partitions);
            eachPCB.state = "WAITING";
        }
    }
}

//Func-Update ready stack
void updateReadyStack(std::vector<ExecutionLog>& executionLogs, const int& tick, std::vector<PCB>& PCBStack){
    for(auto& eachPCB : PCBStack){
        if(eachPCB.state == "WAITING" && eachPCB.cooldown <= 0){
            eachPCB.state = "READY";
            eLog(executionLogs, tick, eachPCB);
            eachPCB.eachRunTime = eachPCB.frequency;
            eachPCB.cooldown = eachPCB.duration;
        }
    }
}

//Func-Get the next running pcb
void getNextRunningPCB(std::vector<PCB>& PCBStack, PCB*& runningSlot){
    for(auto& eachPCB : PCBStack){
        if(!runningSlot && eachPCB.state == "READY"){
            runningSlot = &eachPCB;
        }
        else if(runningSlot && eachPCB.state == "READY" && eachPCB.priority < runningSlot->priority){
            runningSlot = &eachPCB;
        }
    }
}

//Func-Update running slot
void updateRunningSlot(std::vector<MemoryStatusLog>& memoryStatusLogs, std::vector<ExecutionLog>& executionLogs, int& tick, std::vector<PCB>& PCBStack, PCB*& runningSlot, std::vector<Partition>& partitions){
    if(runningSlot){
        if((runningSlot->totalTime == 0) || runningSlot->totalTime <= -1){
            runningSlot->state = "TERMINATED";
            partitionReleaser(*runningSlot, partitions);
            eLog(executionLogs, tick, *runningSlot);
            mLog(memoryStatusLogs, tick, PCBStack, partitions);
            runningSlot = nullptr;
        }
        else if(runningSlot->totalTime >= 0 && runningSlot->eachRunTime <= 0){
            runningSlot->state = "WAITING";
            eLog(executionLogs, tick, *runningSlot);
            runningSlot = nullptr;
        }
    }
    if(!runningSlot){
        getNextRunningPCB(PCBStack, runningSlot);
        if(runningSlot){
            runningSlot->state = "RUNNING";
            eLog(executionLogs, tick, *runningSlot);
        }
    }
}

//Func-Add space for string
std::string addSpace(const std::string& s, const int& n){
    std::string tempString = "";
    for(int i = 0; i < n; i++){
        tempString += s;
    }
    return tempString;
}

//Func-Generate "execution.txt"
void generateExecutionLogFile(const std::vector<ExecutionLog>& executionLogs){
    std::ofstream outfile("execution.txt");

    int timeOfTransitionML = 18;
    int pidML = 3;
    int oldStateML = 9;
    int newStateML = 9;

    for(const auto& eachLog : executionLogs){
        if(std::to_string(eachLog.timeOfTransition).length() > timeOfTransitionML){
            timeOfTransitionML = std::to_string(eachLog.timeOfTransition).length();
        }
        if(std::to_string(eachLog.pid).length() > pidML){
            pidML = std::to_string(eachLog.pid).length();
        }
        if(eachLog.oldState.length() > oldStateML){
            oldStateML = eachLog.oldState.length();
        }
        if(eachLog.newState.length() > newStateML){
            newStateML = eachLog.newState.length();
        }
    }

    outfile << "+" << addSpace("-", timeOfTransitionML + pidML + oldStateML + newStateML - 39) << "--------------------------------------------------+" << std::endl;
    outfile << "| " << addSpace(" ", timeOfTransitionML - 18) << "Time of Transition | " << addSpace(" ", pidML - 3) << "PID | " << addSpace(" ", oldStateML - 9) << "Old State | " << addSpace(" ", newStateML - 9) << "New State |" << std::endl;
    outfile << "+" << addSpace("-", timeOfTransitionML + pidML + oldStateML + newStateML - 39) << "--------------------------------------------------+" << std::endl;

    for(const auto& eachLog : executionLogs){
        std::string tempTimeOfTransition = "| ";
        std::string tempPid;
        std::string tempOldState;
        std::string tempNewState;
        
        for(int i = 0; i < timeOfTransitionML-std::to_string(eachLog.timeOfTransition).length(); i++){
            tempTimeOfTransition += " ";
        }
        tempTimeOfTransition += std::to_string(eachLog.timeOfTransition) + " | ";

        for(int i = 0; i < pidML-std::to_string(eachLog.pid).length(); i++){
            tempPid += " ";
        }
        tempPid += std::to_string(eachLog.pid) + " | ";

        for(int i = 0; i < oldStateML-eachLog.oldState.length(); i++){
            tempOldState += " ";
        }
        tempOldState += eachLog.oldState + " | ";

        for(int i = 0; i < newStateML-eachLog.newState.length(); i++){
            tempNewState += " ";
        }
        tempNewState += eachLog.newState + " |";

        outfile << tempTimeOfTransition << tempPid << tempOldState << tempNewState << std::endl;
    }

    outfile << "+" << addSpace("-", timeOfTransitionML + pidML + oldStateML + newStateML - 39) << "--------------------------------------------------+" << std::endl;
    outfile.close();
}

//Func-Generate "memory_status.txt"
void generateMemoryStatusLogFile(const std::vector<MemoryStatusLog>& memoryStatusLogs){
    std::ofstream outfile("memory_status.txt");

    int timeOfEventML = 13;
    int memoryUsedML = 11;
    int partitionsStateML = 16;
    int totalFreeMemoryML = 17;
    int usableFreeMemoryML = 18;
    
    for(const auto& eachLog : memoryStatusLogs){
        if(std::to_string(eachLog.timeOfEvent).length() > timeOfEventML){
            timeOfEventML = std::to_string(eachLog.timeOfEvent).length();
        }
        if(std::to_string(eachLog.memoryUsed).length() > memoryUsedML){
            memoryUsedML = std::to_string(eachLog.memoryUsed).length();
        }
        if(eachLog.partitionsState.length() > partitionsStateML){
            partitionsStateML = eachLog.partitionsState.length();
        }
        if(std::to_string(eachLog.totalFreeMemory).length() > totalFreeMemoryML){
            totalFreeMemoryML = std::to_string(eachLog.totalFreeMemory).length();
        }
        if(std::to_string(eachLog.usableFreeMemory).length() > usableFreeMemoryML){
            usableFreeMemoryML = std::to_string(eachLog.usableFreeMemory).length();
        }
    }

    outfile << "+" << addSpace("-", timeOfEventML + memoryUsedML + partitionsStateML + totalFreeMemoryML + usableFreeMemoryML - 75) << "-----------------------------------------------------------------------------------------+" << std::endl;
    outfile << "| " << addSpace(" ", timeOfEventML - 13) << "Time of Event | "<< addSpace(" ", memoryUsedML - 11) <<"Memory Used | " << addSpace(" ", partitionsStateML - 16) << "Partitions State | " << addSpace(" ", totalFreeMemoryML - 17) << "Total Free Memory | " << addSpace(" ", usableFreeMemoryML - 18) << "Usable Free Memory |" << std::endl;
    outfile << "+" << addSpace("-", timeOfEventML + memoryUsedML + partitionsStateML + totalFreeMemoryML + usableFreeMemoryML - 75) << "-----------------------------------------------------------------------------------------+" << std::endl;

    for(const auto& eachLog : memoryStatusLogs){
        std::string tempTimeOfEvent = "| ";
        std::string tempMemoryUsed;
        std::string tempPartitionsState;
        std::string tempTotalFreeMemory;
        std::string tempUsableFreeMemory;
        
        for(int i = 0; i < timeOfEventML-std::to_string(eachLog.timeOfEvent).length(); i++){
            tempTimeOfEvent += " ";
        }
        tempTimeOfEvent += std::to_string(eachLog.timeOfEvent) + " | ";

        for(int i = 0; i < memoryUsedML-std::to_string(eachLog.memoryUsed).length(); i++){
            tempMemoryUsed += " ";
        }
        tempMemoryUsed += std::to_string(eachLog.memoryUsed) + " | ";

        for(int i = 0; i < partitionsStateML-eachLog.partitionsState.length(); i++){
            tempPartitionsState += " ";
        }
        tempPartitionsState += eachLog.partitionsState + " | ";

        for(int i = 0; i < totalFreeMemoryML-std::to_string(eachLog.totalFreeMemory).length(); i++){
            tempTotalFreeMemory += " ";
        }
        tempTotalFreeMemory += std::to_string(eachLog.totalFreeMemory) + " | ";

        for(int i = 0; i < usableFreeMemoryML-std::to_string(eachLog.usableFreeMemory).length(); i++){
            tempUsableFreeMemory += " ";
        }
        tempUsableFreeMemory += std::to_string(eachLog.usableFreeMemory) + " |";

        outfile << tempTimeOfEvent << tempMemoryUsed << tempPartitionsState << tempTotalFreeMemory << tempUsableFreeMemory << std::endl;
    }
    outfile << "+" << addSpace("-", timeOfEventML+memoryUsedML+partitionsStateML+totalFreeMemoryML+usableFreeMemoryML-75) << "-----------------------------------------------------------------------------------------+" << std::endl;
    outfile.close();
}

//Func-Generate "visualization_logs.txt"
void generateVisualizationLogFile(const std::vector<std::string>& visualizationLogs, const int& tick, const std::vector<PCB>& PCBStack){
    std::ofstream outfile("visualization_log.txt");

    int pidML = 0;
    std::vector<int> waitingTimes(PCBStack.size(), 0);
    int totalWaitingTime = 0;
    int totalTurnaroundTime = 0;

    for(const auto& eachPCB : PCBStack){
        if(std::to_string(eachPCB.pid).length() > pidML){
            pidML = std::to_string(eachPCB.pid).length();
        }
    }

    for(int i = 0; i < visualizationLogs.size(); i++){
        outfile << addSpace(" ", pidML - std::to_string(PCBStack[i].pid).length()) << PCBStack[i].pid << " : " << visualizationLogs[i] << std::endl;
    }

    for(int i = 0; i < PCBStack.size(); i++){
        waitingTimes[i] += std::count(visualizationLogs[i].begin(), visualizationLogs[i].end(), '-');
        waitingTimes[i] += std::count(visualizationLogs[i].begin(), visualizationLogs[i].end(), '=');
        totalWaitingTime += waitingTimes[i];
    }

    for(int i = 0; i < PCBStack.size(); i++){
        totalTurnaroundTime += std::count(visualizationLogs[i].begin(), visualizationLogs[i].end(), '-');
        totalTurnaroundTime += std::count(visualizationLogs[i].begin(), visualizationLogs[i].end(), '=');
        totalTurnaroundTime += std::count(visualizationLogs[i].begin(), visualizationLogs[i].end(), '#');
    }

    outfile << "Throughput: " << std::to_string(static_cast<double>(PCBStack.size())/tick) << std::endl;
    outfile << "Average turnaround time: " << std::to_string(static_cast<double>(totalTurnaroundTime)/PCBStack.size()) << std::endl;
    outfile << "Average Waiting Time: " << std::to_string(static_cast<double>(totalWaitingTime)/PCBStack.size()) << std::endl;
    outfile.close();
}

//Func-Run one tick
void runOneTick(int& tick, std::vector<PCB>& PCBStack){
    tick++;

    for(auto& eachPCB : PCBStack){
        if(eachPCB.state == "RUNNING"){
            eachPCB.eachRunTime--;
            eachPCB.totalTime--;
        }
        else if(eachPCB.state == "WAITING"){
            eachPCB.cooldown--;
        }
        else if(eachPCB.state == "NEW"){
            eachPCB.arrivalTime--;
        }
    }
}

//Func-False if all Terminated
bool ifAllTerminated(std::vector<PCB>& PCBStack){
    for(auto& eachPCB : PCBStack){
        if(eachPCB.state != "TERMINATED"){
            return false;
        }
    }
    return true;
}

void simulate(std::vector<MemoryStatusLog>& memoryStatusLogs, std::vector<ExecutionLog>& executionLogs, int& tick, std::vector<PCB>& PCBStack, PCB*& runningSlot, std::vector<Partition>& partitions, std::vector<std::string>& visualizationLogs){
    mLog(memoryStatusLogs, tick, PCBStack, partitions);
    while(!ifAllTerminated(PCBStack)){
        updateWaitingStack(memoryStatusLogs, tick, PCBStack, partitions);
        updateReadyStack(executionLogs, tick, PCBStack);
        updateRunningSlot(memoryStatusLogs, executionLogs, tick, PCBStack, runningSlot, partitions);
        runOneTick(tick, PCBStack);

        vLog(visualizationLogs, PCBStack);
    }
}

int main(int argc, char* argv[]){
    std::vector<PCB> PCBStack = readInputDataFile(argv[1]);
    std::vector<Partition> partitions = partitionsInitalizer();
    PCB* runningSlot = nullptr;
    int tick = 0;
    std::vector<ExecutionLog> executionLogs;
    std::vector<MemoryStatusLog> memoryStatusLogs;
    std::vector<std::string> visualizationLogs(PCBStack.size());

    simulate(memoryStatusLogs, executionLogs, tick, PCBStack, runningSlot, partitions, visualizationLogs);

    generateExecutionLogFile(executionLogs);
    generateMemoryStatusLogFile(memoryStatusLogs);
    generateVisualizationLogFile(visualizationLogs, tick, PCBStack);
    
    return 0;
}
/*
██████╗ ██╗   ██╗ █████╗ ███╗   ██╗ ██████╗ ███████╗ █████╗ ███████╗ █████╗ 
██╔══██╗██║   ██║██╔══██╗████╗  ██║██╔════╝ ██╔════╝██╔══██╗██╔════╝██╔══██╗
██████╔╝██║   ██║███████║██╔██╗ ██║██║  ███╗█████╗  ███████║█████╗  ███████║
██╔══██╗██║   ██║██╔══██║██║╚██╗██║██║   ██║██╔══╝  ██╔══██║██╔══╝  ██╔══██║
██║  ██║╚██████╔╝██║  ██║██║ ╚████║╚██████╔╝██║     ██║  ██║██║     ██║  ██║
╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═══╝ ╚═════╝ ╚═╝     ╚═╝  ╚═╝╚═╝     ╚═╝  ╚═╝                                                                                                                                       
*/                                                                   