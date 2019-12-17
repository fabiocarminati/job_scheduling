#include <string.h>
#include <omnetpp.h>
#include <msg_check_m.h>
#include <map>
#define SIZE 1000000

using namespace omnetpp;

class Storage : public cSimpleModule {
private:

    //STD::map used to store the message relative to a job. The job are searched using the JobId field in the message.
    std::map<std::string, msg_check *> newJobsQueue;
    std::map<std::string, msg_check *>::iterator searchNewJobsQueue;

    std::map<std::string, msg_check *> jobQueue;
    std::map<std::string, msg_check *>::iterator searchJobQueue;

    std::map<std::string, msg_check *> reRoutedQueue;
    std::map<std::string, msg_check *>::iterator searchReRoutedQueue;

    void searchMessage(std::string jobId,  msg_check *msg, std::map<std::string, msg_check *> storedMap,std::map<std::string, msg_check *>::iterator search);
    void executorReboot(std::string jobId, std::map<std::string, msg_check *> storedMap,std::map<std::string, msg_check *>::iterator search);



protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *cmsg) override;

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

}
/*
 This class represents a secure storage(his content will never be lost).
 There is one secure storage for each executor,
 Is useful because is the place where we put
     ->The map function newJobsQueue that contains all the messages(msg_check) arrived to an executor that has not already either processed or
     queued them or forwarded them to other executors due to load balancing
     ->The map function jobQueue that contains all the messages waiting to be processed by that executor
 */
void Storage::initialize() {

}
void Storage::handleMessage(cMessage *cmsg) {
   // Casting from cMessage to msg_check
   msg_check *msg = check_and_cast<msg_check *>(cmsg);
   msg_check *msgBackup;
   std::string id,jobId;
   //const char *jobId;

   id.append(std::to_string(msg->getOriginalExecId()));
   id.append("-");
   id.append(std::to_string(msg->getRelativeJobId()));
   //jobId=id.c_str();
  // EV<<"reboot flag "<<msg->getReBoot()<<endl;
   //EV<<"message name "<<msg->getName()<<endl;
   if(msg->getReBoot()==true){
       EV<<"the failure of executor "<<msg->getOriginalExecId()<<" is detected by the storage"<<endl;
       /*msg->setJobQueue(true);
       executorReboot(msg,jobId,jobQueue,searchJobQueue);
       msg->setJobQueue(false);
       msg->setNewJobsQueue(true);
       executorReboot(msg,jobId,newJobsQueue,searchNewJobsQueue);
       msg->setJobQueue(false);
       msg->setNewJobsQueue(false);
       msg->setReRoutedJobQueue(true);
       executorReboot(msg,jobId,reRoutedQueue,searchReRoutedQueue);*/


       for (auto searchJobQueue = jobQueue.begin();searchJobQueue != jobQueue.end(); ++searchJobQueue){
           EV<<"Recove JOBr "<<searchJobQueue->first<<endl;
           msgBackup=searchJobQueue->second->dup();
           msgBackup->setName("send to the executor the backup copy of the jobs that he has to process");
           msgBackup->setJobQueue(true);
           send(msgBackup,"backup_rec");
           EV<<"send the backup copy for "<<jobId<<endl;
       }
       for (auto searchNewJobsQueue = newJobsQueue.begin();searchNewJobsQueue != newJobsQueue.end(); ++searchNewJobsQueue){
           EV<<"Recover NEWJOB"<<searchNewJobsQueue->first<<endl;
           msgBackup=searchNewJobsQueue->second->dup();
           msgBackup->setName("send to the executor the backup copy of the jobs that he has to process");
           msgBackup->setNewJobsQueue(true);
           send(msgBackup,"backup_rec");
           EV<<"send the backup copy for "<<jobId<<endl;
       }
       for (auto searchReRoutedQueue = reRoutedQueue.begin();searchReRoutedQueue != reRoutedQueue.end(); ++searchReRoutedQueue){
           EV<<"Recover REROUTED "<< searchReRoutedQueue->first<<endl;
           msgBackup=searchReRoutedQueue->second->dup();
           msgBackup->setName("send to the executor the backup copy of the jobs that he has to process");
           msgBackup->setReRoutedJobQueue(true);
           send(msgBackup,"backup_rec");
           EV<<"send the backup copy for "<<jobId<<endl;
       }


   }else if(msg->getNewJobsQueue()==true){
       searchMessage(jobId,msg,newJobsQueue,searchNewJobsQueue);
       EV<<"add NEWJOB element that is: "<<jobId<<endl;

   }else if(msg->getJobQueue()==true){
       searchMessage(jobId,msg,jobQueue,searchJobQueue);
       EV<<"add JOBID element that is: "<<jobId<<endl;
   }else if(msg->getReRoutedJobQueue()==true){
       searchMessage(jobId,msg,reRoutedQueue,searchReRoutedQueue);
       EV<<"add REROUTED element that is: "<<jobId<<endl;
   }

   delete msg;
}

void Storage::executorReboot(std::string jobId, std::map<std::string, msg_check *> storedMap ,std::map<std::string, msg_check *>::iterator search){
    msg_check *msgBackup;
    for (auto search = storedMap.begin();search != storedMap.end(); ++search){
        //EV<<"This is the job id "<<search->first<<endl;
        msgBackup=search->second->dup();
        msgBackup->setName("send to the executor the backup copy of the jobs that he has to process");
        send(msgBackup,"backup_rec");
        EV<<"send the backup copy for "<<jobId<<endl;
    }
}
void Storage::searchMessage(std::string jobId, msg_check *msg, std::map<std::string, msg_check *> storedMap ,std::map<std::string, msg_check *>::iterator search){
    //Search if the job is already present
    search=storedMap.find(jobId);

    if (search != storedMap.end()){
        //a key is found(the msg has already been inserted in the storage

        //Delete the old job entry
        delete search->second;
        storedMap.erase(jobId);

        EV<<"Erase "<<jobId<<"from storage"<<endl;

        //if the job has not ended the computation, re-insert it with the modified field
        if(msg->getReRouted()==true && msg->getHasEnded()==false){
            storedMap.insert(std::pair<std::string ,msg_check *>(jobId, msg->dup()));
            EV<<"Due to load balancing a change in the machine is performed for "<<jobId<< " and this is notified to the secure storage "<<endl;
            EV<<"Original  "<<msg->getOriginalExecId()<<" and new actual "<<msg->getActualExecId()<<endl;
        }
    }
    else{
        //No job found, insert it has new job
        storedMap.insert(std::pair<std::string ,msg_check *>(jobId, msg->dup()));
        EV<<"New element with ID "<<jobId<< " added in the secure storage in the map "<<endl;
    }

}
