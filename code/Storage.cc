#include <string.h>
#include <omnetpp.h>
#include <msg_check_m.h>
#include <map>
#define SIZE 1000000

using namespace omnetpp;
class Storage : public cSimpleModule {
private:

    //STD::map used to store the message relative to a job. The job are searched using the JobId field in the message.
    std::map<std::string, msg_check *> storedMsg;
    std::map<std::string, msg_check *>::iterator search;

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *cmsg) override;
};
Define_Module(Storage);

void Storage::initialize() {

}
void Storage::handleMessage(cMessage *cmsg) {
   // Casting from cMessage to msg_check
   msg_check *msg = check_and_cast<msg_check *>(cmsg);
   EV<<"receiving:"<<msg->getJobId()<<endl;

   //Search if the job is already present
   search=storedMsg.find(msg->getJobId());

   if (search != storedMsg.end()){
       //a key is found(the msg has already been inserted in the storage

       //Delete the old job entry
       delete search->second;
       storedMsg.erase(msg->getJobId());

       EV<<"Job id "<<msg->getJobId()<<endl;

       //if the job has not ended the computation, re-insert it with the modified field
       if(msg->getReRouted()==true&&msg->getHasEnded()==false){
           storedMsg.insert(std::pair<std::string ,msg_check *>(msg->getJobId(), msg->dup()));
           EV<<"Due to load balancing a change in the machine is performed for "<<msg->getJobId()<< " and this is notified to the secure storage "<<endl;
           EV<<"Original  "<<msg->getOriginalExecId()<<" and new actual "<<msg->getActualExecId()<<endl;
       }
   }
   else{
       //No job found, insert it has new job
       storedMsg.insert(std::pair<std::string ,msg_check *>(msg->getJobId(), msg->dup()));
       EV<<"New element with ID "<<msg->getJobId()<< " added in the secure storage "<<endl;
   }

   delete msg;
}
