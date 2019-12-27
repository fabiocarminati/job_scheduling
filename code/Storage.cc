#include <string.h>
#include <omnetpp.h>
#include <msg_check_m.h>
#include <map>


using namespace omnetpp;

class Storage : public cSimpleModule {
private:

    std::map<std::string, msg_check *> newJobsQueue;
    std::map<std::string, msg_check *> jobQueue;
    std::map<std::string, msg_check *> reRoutedQueue;
    std::map<std::string, msg_check *> completedJobQueue;
    double totale;
    void searchMessage(std::string jobId,  msg_check *msg, std::map<std::string, msg_check *> *storedMap);
    void executorReboot(std::string jobId, msg_check *msg,std::map<std::string, msg_check *> *storedMap);
    simtime_t channelDelay;



protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *cmsg) override;
    virtual void finish() override;
    simsignal_t JobSignal;
    simsignal_t NewSignal;
    //simsignal_t whyNotWorkSignal;
    simsignal_t ReRoutedSignal;
    simsignal_t CompletedSignal;

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
    std::map<std::string, msg_check *>::iterator search;

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
 This class represents a secure storage(his content will never be lost).
 There is one secure storage for each executor,
 Is useful because is the place where we put
     ->The map function newJobsQueue that contains all the messages(msg_check) arrived to an executor that has not
     already either processed or queued them or forwarded them to other executors due to load balancing
     ->The map function jobQueue that contains all the messages waiting to be processed by that executor
 */
void Storage::initialize() {
    JobSignal = registerSignal("Job");
    NewSignal = registerSignal("NewJobs");
    //whyNotWorkSignal = registerSignal("whyNotWork");
    ReRoutedSignal = registerSignal("ReRouted");
    CompletedSignal = registerSignal("Completed");
    emit(JobSignal, 0);
    emit(NewSignal, 0);
    //totale=0;
    //emit(whyNotWorkSignal,totale);
    emit(ReRoutedSignal, 0);
    emit(CompletedSignal, 0);
    channelDelay = par("channelDelay");

    //the channelDelay should be considered twice in the timeouts:one for the send and one for the reply(2 accesses to the channel)

}
void Storage::handleMessage(cMessage *cmsg) {
   // Casting from cMessage to msg_check
   msg_check *msg = check_and_cast<msg_check *>(cmsg);
   msg_check *msgBackup;
   std::string jobId;
   //totale++;
   //emit(whyNotWorkSignal,totale);
   double jobIdLength,newJobIdLength,reRoutedJobIdLength,completedJobIdLength;

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
           newJobIdLength=newJobsQueue.size();
           emit(NewSignal,newJobIdLength);
           EV<<"   "<<newJobIdLength<<endl;
         }else if(msg->getJobQueue()==true){
                   msg->setJobQueue(true);
                   searchMessage(jobId,msg,&jobQueue);
                   EV<<"working on JOBID map for: "<<jobId<<endl;
                   jobIdLength=jobQueue.size();
                   emit(JobSignal,jobIdLength);
                   EV<<"   "<<jobIdLength<<endl;
                }else if(msg->getReRoutedJobQueue()==true){
                           msg->setReRoutedJobQueue(true);
                           searchMessage(jobId,msg,&reRoutedQueue);
                           EV<<"working on REROUTED map for: "<<jobId<<endl;
                           reRoutedJobIdLength=reRoutedQueue.size();
                           emit(ReRoutedSignal,reRoutedJobIdLength);
                           EV<<"   "<<reRoutedJobIdLength<<endl;
                       }else if(msg->getCompletedQueue()==true){
                                   msg->setCompletedQueue(true);
                                   searchMessage(jobId,msg,&completedJobQueue);
                                   EV<<"working on ENDED JOBS map for: "<<jobId<<endl;
                                   completedJobIdLength=completedJobQueue.size();
                                   emit(CompletedSignal,completedJobIdLength);
                                   EV<<"   "<<completedJobIdLength<<endl;
           }

   delete msg;
}

void Storage::executorReboot(std::string jobId, msg_check *msg,std::map<std::string, msg_check *> *storedMap){

    msg_check *msgBackup;
    std::map<std::string, msg_check *>::iterator search;

    for (search = storedMap->begin();search != storedMap->end(); ++search){
        msgBackup=search->second->dup();
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
        delete search->second;
        storedMap->erase(jobId);
        EV<<"Erase "<<jobId<<" from storage"<<endl;
    }
    else{
        //No job found, insert it as new job
        storedMap->insert(std::pair<std::string ,msg_check *>(jobId, msg->dup()));
        EV<<"New element with ID "<<jobId<< " added in the secure storage in the map "<<endl;
        EV<<"JOB "<<jobQueue.size()<<" NEW "<<newJobsQueue.size()<<" REROUTED "<<reRoutedQueue.size() <<" ENDED "<<completedJobQueue.size()<<" job id "<<jobId<<endl;
    }
}
void Storage::finish()
{
    EV<<"END"<<endl;
    EV<<"JOB "<<jobQueue.size()<<" NEW "<<newJobsQueue.size()<<" REROUTED "<<reRoutedQueue.size() <<" ENDED "<<completedJobQueue.size()<<endl;

}
