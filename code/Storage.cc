#include <string.h>
#include <omnetpp.h>
#include <msg_check_m.h>
#include <map>
#define SIZE 1000000

using namespace omnetpp;

class Storage : public cSimpleModule {
private:

    std::map<std::string, msg_check *> newJobsQueue;
    std::map<std::string, msg_check *> jobQueue;
    std::map<std::string, msg_check *> reRoutedQueue;
    std::map<std::string, msg_check *> completedJobQueue;

    void searchMessage(std::string jobId,  msg_check *msg, std::map<std::string, msg_check *> *storedMap);
    void executorReboot(std::string jobId, msg_check *msg,std::map<std::string, msg_check *> *storedMap);



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
     ->The map function newJobsQueue that contains all the messages(msg_check) arrived to an executor that has not
     already either processed or queued them or forwarded them to other executors due to load balancing
     ->The map function jobQueue that contains all the messages waiting to be processed by that executor
 */
void Storage::initialize() {

}
void Storage::handleMessage(cMessage *cmsg) {
   // Casting from cMessage to msg_check
   msg_check *msg = check_and_cast<msg_check *>(cmsg);
   msg_check *msgBackup;
   std::string jobId;


   jobId.append(std::to_string(msg->getOriginalExecId()));
   jobId.append("-");
   jobId.append(std::to_string(msg->getRelativeJobId()));

   if(msg->getReBoot()==true){
       EV<<"the failure of executor "<<msg->getOriginalExecId()<<" is detected by the storage"<<endl;
       executorReboot(jobId,msg,&jobQueue);
       executorReboot(jobId,msg,&newJobsQueue);
       executorReboot(jobId,msg,&reRoutedQueue);
       executorReboot(jobId,msg,&completedJobQueue);

       msgBackup = new msg_check("End recover backup");
       msgBackup->setBackupComplete(true);
       msgBackup->setReBoot(true);
       send(msgBackup,"backup_rec$o");
       EV<<"notify end of backup to executor"<<endl;

   }else if(msg->getNewJobsQueue()==true){
           msg->setNewJobsQueue(true);
           searchMessage(jobId,msg,&newJobsQueue);
           EV<<"working on NEWJOB map for: "<<jobId<<endl;
         }else if(msg->getJobQueue()==true){
                   msg->setJobQueue(true);
                   searchMessage(jobId,msg,&jobQueue);
                   EV<<"working on JOBID map for: "<<jobId<<endl;
                }else if(msg->getReRoutedJobQueue()==true){
                           msg->setReRoutedJobQueue(true);
                           searchMessage(jobId,msg,&reRoutedQueue);
                           EV<<"working on REROUTED map for: "<<jobId<<endl;
                       }else if(msg->getCompletedQueue()==true){
                                   msg->setCompletedQueue(true);
                                   searchMessage(jobId,msg,&completedJobQueue);
                                   EV<<"working on ENDED JOBS map for: "<<jobId<<endl;
           }

   delete msg;
}

void Storage::executorReboot(std::string jobId, msg_check *msg,std::map<std::string, msg_check *> *storedMap){

    msg_check *msgBackup;
    std::map<std::string, msg_check *>::iterator search;

    for (search = storedMap->begin();search != storedMap->end(); ++search){
        msgBackup=search->second->dup();
        msgBackup->setName("send backup copy");
        msgBackup->setReBoot(true);
        EV<<"JOB "<<jobQueue.size()<<" NEW "<<newJobsQueue.size()<<" REROUTED "<<reRoutedQueue.size() <<" ENDED "<<completedJobQueue.size()<<" job id "<<jobId<<endl;
        send(msgBackup,"backup_rec$o");
    }
}
void Storage::searchMessage(std::string jobId, msg_check *msg, std::map<std::string, msg_check *> *storedMap){
    std::map<std::string, msg_check *>::iterator search;
    //Search if the job is already present
    search=storedMap->find(jobId);
    if (search != storedMap->end()){  //a key is found(the msg has already been inserted in that map)
        //Delete the old job entry
        delete search->second;
        storedMap->erase(jobId);

        EV<<"Erase "<<jobId<<" from storage"<<endl;
    }
    else{
        //No job found, insert it has new job
        storedMap->insert(std::pair<std::string ,msg_check *>(jobId, msg->dup()));
        EV<<"New element with ID "<<jobId<< " added in the secure storage in the map "<<endl;
        EV<<"JOB "<<jobQueue.size()<<" NEW "<<newJobsQueue.size()<<" REROUTED "<<reRoutedQueue.size() <<" ENDED "<<completedJobQueue.size()<<" job id "<<jobId<<endl;
    }


}
