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
    std::map<std::string, msg_check *> jobQueue;
    std::map<std::string, msg_check *> reRoutedQueue;

    void searchMessage(std::string jobId,  msg_check *msg, std::map<std::string, msg_check *> *storedMap);
    void executorReboot(std::string jobId, msg_check *msg,std::map<std::string, msg_check *> *storedMap,bool jobQueue,bool newJobQueue,bool ReRouted  );



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
   std::string jobId;


   jobId.append(std::to_string(msg->getOriginalExecId()));
   jobId.append("-");
   jobId.append(std::to_string(msg->getRelativeJobId()));

   if(msg->getReBoot()==true){
       EV<<"the failure of executor "<<msg->getOriginalExecId()<<" is detected by the storage"<<endl;

       executorReboot(jobId,msg,&jobQueue,true,false,false);

       executorReboot(jobId,msg,&newJobsQueue,false,true,false);

       executorReboot(jobId,msg,&reRoutedQueue,false,false,true);
       msgBackup = new msg_check("End recover backup");
       msgBackup->setBackupComplete(true);
       msgBackup->setReBoot(true);
       send(msgBackup,"backup_rec$o");
       EV<<"notify end of backup to executor"<<endl;
/*
       for (auto searchJobQueue = jobQueue.begin();searchJobQueue != jobQueue.end(); ++searchJobQueue){
           EV<<"Recover JOB"<<searchJobQueue->first<<endl;
           msgBackup=searchJobQueue->second->dup();
           msgBackup->setName("send to the executor backup copy JOBQUEUE ");
           msgBackup->setJobQueue(true);
           send(msgBackup,"backup_rec$o");
           EV<<"send the backup copy for "<<jobId<<endl;
       }
       for (auto searchNewJobsQueue = newJobsQueue.begin();searchNewJobsQueue != newJobsQueue.end(); ++searchNewJobsQueue){
           EV<<"Recover NEWJOB"<<searchNewJobsQueue->first<<endl;
           msgBackup=searchNewJobsQueue->second->dup();
           msgBackup->setName("send to the executor backup copy NEWJOBQUEUE ")
           msgBackup->setNewJobsQueue(true);
           send(msgBackup,"backup_rec$o");
           EV<<"send the backup copy for "<<jobId<<endl;
       }
       for (auto searchReRoutedQueue = reRoutedQueue.begin();searchReRoutedQueue != reRoutedQueue.end(); ++searchReRoutedQueue){
           EV<<"Recover REROUTED "<< searchReRoutedQueue->first<<endl;
           msgBackup=searchReRoutedQueue->second->dup();
           msgBackup->setName("send to the executor backup copy REROUTED");
           msgBackup->setReRoutedJobQueue(true);
           send(msgBackup,"backup_rec$o");
           EV<<"send the backup copy for "<<jobId<<endl;
       }
*/

   }else if(msg->getNewJobsQueue()==true){
       searchMessage(jobId,msg,&newJobsQueue);
       EV<<"working on NEWJOB map for: "<<jobId<<endl;

   }else if(msg->getJobQueue()==true){
       searchMessage(jobId,msg,&jobQueue);
       EV<<"working on JOBID map for: "<<jobId<<endl;
   }else if(msg->getReRoutedJobQueue()==true){
       searchMessage(jobId,msg,&reRoutedQueue);
       EV<<"working on REROUTED map for: "<<jobId<<endl;
   }

   delete msg;
}

void Storage::executorReboot(std::string jobId, msg_check *msg,std::map<std::string, msg_check *> *storedMap,bool jobQueue,bool newJobQueue,bool ReRouted ){

    msg_check *msgBackup;
    std::map<std::string, msg_check *>::iterator search;

    for (search = storedMap->begin();search != storedMap->end(); ++search){
        msgBackup=search->second->dup();
        msgBackup->setName("send backup copy");
        msgBackup->setReBoot(true);
        //msgBackup->setJobQueue(jobQueue);
       // msgBackup->setNewJobsQueue(newJobQueue);
       // msgBackup->setReRoutedJobQueue(ReRouted);
        send(msgBackup,"backup_rec$o");
        EV<<"send the backup copy for "<<jobId<<endl;
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

        EV<<"Erase "<<jobId<<"from storage"<<endl;

        //if the job has not ended the computation, re-insert it with the modified field
        if(msg->getReRouted()==true && msg->getHasEnded()==false){
            storedMap->insert(std::pair<std::string ,msg_check *>(jobId, msg->dup()));
            EV<<"Due to load balancing a change in the machine is performed for "<<jobId<< " and this is notified to the secure storage "<<endl;
            EV<<"Original  "<<msg->getOriginalExecId()<<" and new actual "<<msg->getActualExecId()<<endl;
        }
    }
    else{
        //No job found, insert it has new job
        storedMap->insert(std::pair<std::string ,msg_check *>(jobId, msg->dup()));
        EV<<"New element with ID "<<jobId<< " added in the secure storage in the map "<<endl;
    }


}
