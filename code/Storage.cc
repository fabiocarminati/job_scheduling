#include <string.h>
#include <omnetpp.h>
#include <JobMessage_m.h>
#include <map>
using namespace omnetpp;

/*
This class represents a secure storage(his content will never be lost).
There is one secure storage for each executor.
It can send and receive messages only to/from the correspondent executor
It contains four distinct map functions which have the job ID as key and the job as value:
     ->The map function newJobsQueue: the executor hasn't decided yet how to handle these messages. Several actions are possible:
     1. Send the job to another executor in order to perform load balancing
     2. Keep the job and execute it either immediately if idle or in the future
     ->The map function jobQueue that contains all the messages waiting to be processed by that executor with a FIFO logic
     ->The map function reRoutedQueue: keeps track of all the jobs sent to other executors due to load balancing.
     ->The map function completedJobQueue: contains all the jobs whose execution is ended

For the first three maps we are interesting in the evolution over time of their length so we define for them 3 signals.
For the completedJobQueue we don't consider statistics because for how is defined in the Executor.cc his length is related not only to the
number of completed jobs but also to the number and to frequency of the status requests made by the clients.
Anyway in order to have an idea about the total number of completed jobs by each executor at the end of the simulation we print
that value.

N.B.In this document we use job,message,packet as synonyms
    In this document we refer to original executor as the executor to which the job is sent by the client
    In this document we refer to actual executor as the executor that processes the job
    Actual exec!=Orignal exec if reRouting is performed due to load balancing
    Actual exec==Orignal exec if load balancing isn't performed
    Anyway only the original executor can communicate with the client and not the actual executor
 */

class Storage : public cSimpleModule {
private:

    std::map<std::string, JobMessage *> newJobsQueue;
    std::map<std::string, JobMessage *> jobQueue;
    std::map<std::string, JobMessage *> reRoutedQueue;
    std::map<std::string, JobMessage *> completedJobQueue;

    void searchMessage(std::string jobId,  JobMessage *msg, std::map<std::string, JobMessage *> *storedMap);
    void executorReboot(std::string jobId,std::map<std::string, JobMessage *> *storedMap);



protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *cmsg) override;
    virtual void finish() override;
    simsignal_t JobSignal;
    simsignal_t NewSignal;
    simsignal_t ReRoutedSignal;


public:

  Storage();
 ~Storage();


};

Define_Module(Storage);

Storage::Storage()
{

}
Storage::~Storage()
{
    std::map<std::string, JobMessage *>::iterator search;

    for (search = newJobsQueue.begin();search != newJobsQueue.end(); ++search)
         delete search->second;
    for (search = jobQueue.begin();search != jobQueue.end(); ++search)
         delete search->second;
    for (search = reRoutedQueue.begin();search != reRoutedQueue.end(); ++search)
         delete search->second;
    for (search = completedJobQueue.begin();search != completedJobQueue.end(); ++search)
         delete search->second;
}

/*
 * INITIALIZE
At the beginning of the simulation all the various queues are empty so we set equal to 0 the first emitted value for the signals representing
the length of the jobQueue,newJobsQueue and reRouted
 */

void Storage::initialize() {
    JobSignal = registerSignal("Job");
    NewSignal = registerSignal("NewJobs");
    ReRoutedSignal = registerSignal("ReRouted");
    emit(JobSignal, 0);
    emit(NewSignal, 0);
    emit(ReRoutedSignal, 0);
}

/*
 * HANDLE MESSAGE
Different actions according to the type of message received by the executor:
    ->Message notifying that the executor has ended the failure mode:the storage will reply sending all the jobs contained in his four maps
    followed by a final message with the flag BackupComplete=true so that the executor understands that the backup process is over.
    ->Message notifying that a job must be put(removed) to(from) one of the four maps. After this operation the length of the map function
    changes so an emit is done.

Finally the incoming msg is deleted
 */

void Storage::handleMessage(cMessage *cmsg) {
   // Casting from cMessage to JobMessage
   JobMessage *msg = check_and_cast<JobMessage *>(cmsg);
   JobMessage *msgBackup;
   std::string jobId;

   double jobIdLength,newJobIdLength,reRoutedJobIdLength,completedJobIdLength;

   jobId.append(std::to_string(msg->getOriginalExecId()));
   jobId.append("-");
   jobId.append(std::to_string(msg->getRelativeJobId()));

   if(msg->getReBoot()==true){
       EV<<"The failure of executor "<<msg->getOriginalExecId()<<" ends:start the backup process from the storage"<<endl;
       bubble("Start sending backup copies to the executor");
       executorReboot(jobId,&jobQueue);
       executorReboot(jobId,&newJobsQueue);
       executorReboot(jobId,&reRoutedQueue);
       executorReboot(jobId,&completedJobQueue);

       EV<<"JOB "<<jobQueue.size()<<" NEW "<<newJobsQueue.size()<<" REROUTED "<<reRoutedQueue.size() <<" ENDED "<<completedJobQueue.size()<<" job id "<<jobId<<endl;



       msgBackup = new JobMessage("End recover backup");
       msgBackup->setBackupComplete(true);
       msgBackup->setReBoot(true);
       send(msgBackup,"executorGate$o");
       EV<<"Notify end of backup process to executor"<<endl;

   }else if(msg->getNewJobsQueue()==true){
           msg->setNewJobsQueue(true);
           searchMessage(jobId,msg,&newJobsQueue);
           EV<<"working on NEWJOB map for: "<<jobId<<endl;
           newJobIdLength=newJobsQueue.size();
           emit(NewSignal,newJobIdLength);
         }else if(msg->getJobQueue()==true){
                   msg->setJobQueue(true);
                   searchMessage(jobId,msg,&jobQueue);
                   EV<<"working on JOBID map for: "<<jobId<<endl;
                   jobIdLength=jobQueue.size();
                   emit(JobSignal,jobIdLength);
                }else if(msg->getReRoutedJobQueue()==true){
                           msg->setReRoutedJobQueue(true);
                           searchMessage(jobId,msg,&reRoutedQueue);
                           EV<<"working on REROUTED map for: "<<jobId<<endl;
                           reRoutedJobIdLength=reRoutedQueue.size();
                           emit(ReRoutedSignal,reRoutedJobIdLength);
                       }else if(msg->getCompletedQueue()==true){
                                   msg->setCompletedQueue(true);
                                   searchMessage(jobId,msg,&completedJobQueue);
                                   EV<<"working on ENDED JOBS map for: "<<jobId<<endl;
                                   completedJobIdLength=completedJobQueue.size();
           }

   delete msg;
}

/*
 * EXECUTOR REBOOT
Invoked during the backup process for each map function.
First it makes a copy of all the jobs in that map and then sends it to the executor with flag ReBoot=true
*/

void Storage::executorReboot(std::string jobId,std::map<std::string, JobMessage *> *storedMap){

    JobMessage *msgBackup;
    std::map<std::string, JobMessage *>::iterator search;

    for (search = storedMap->begin();search != storedMap->end(); ++search){
        msgBackup=search->second->dup();
        msgBackup->setReBoot(true);
        send(msgBackup,"executorGate$o");
    }
}

/*
 * SEARCH MESSAGE
Within the jobs coming from the executor there isn't any clue about whether that job is already present in that map or not.
Therefore we must check this at the storage:
    ->A found means that the job has already been inserted in that map;so we delete it because the only reason why the
    executor will send the same job twice to his own storage is for a remove event
    ->In case that job isn't found we add it in the map
*/

void Storage::searchMessage(std::string jobId, JobMessage *msg, std::map<std::string, JobMessage *> *storedMap){
    std::map<std::string, JobMessage *>::iterator search;

    search=storedMap->find(jobId);
    if (search != storedMap->end()){  //a key is found(the message has already been inserted in that map)
        delete search->second;
        storedMap->erase(jobId);
        EV<<"Erase "<<jobId<<" from storage"<<endl;
    }
    else{

        storedMap->insert(std::pair<std::string ,JobMessage *>(jobId, msg->dup()));
        EV<<"New element with ID "<<jobId<< " added in the secure storage in the map "<<endl;
        EV<<"JOB "<<jobQueue.size()<<" NEW "<<newJobsQueue.size()<<" REROUTED "<<reRoutedQueue.size() <<" ENDED "<<completedJobQueue.size()<<" job id "<<jobId<<endl;
    }
}

/*
 * FINISH
At the end of the simulation print the length of all the four maps
*/

void Storage::finish()
{
    EV<<"Map lengths at the END of the simulation"<<endl;
    EV<<"JOB "<<jobQueue.size()<<" NEW "<<newJobsQueue.size()<<" REROUTED "<<reRoutedQueue.size() <<" ENDED "<<completedJobQueue.size()<<endl;

}
