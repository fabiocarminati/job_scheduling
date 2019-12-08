#include <string.h>
#include <omnetpp.h>
#include <msg_check_m.h>
#include <map>

//#include <msg_backup_m.h>
using namespace omnetpp;
class Source : public cSimpleModule {
private:
  int id, nbGenMessages,dst,N,E,output;
  msg_check *sendMessageEvent;
  std::map<const char *,int> workInProgress;
protected:
  virtual void initialize();
  virtual void handleMessage(cMessage *cmsg);
public:
  simtime_t interArrivalTime;

};
Define_Module(Source);
void Source::initialize() {
    sendMessageEvent = new msg_check("sendMessageEvent");
    nbGenMessages=0;
    id=getId()-1;   //defines the Priority-ID of the message that each source will transmit(different sources send different priorities messages)
    EV<<"Source ID "<<id<<endl;
    scheduleAt(simTime(), sendMessageEvent); //generates the first packet with priority I from that source I
    E = par("E"); //non volatile parameters --once defined they never change
    N= par("N");


    }
void Source::handleMessage(cMessage *cmsg) {

    msg_check *msg = check_and_cast<msg_check *>(cmsg); // Casting from cMessage to msg_check
    //ASSERT(msg == sendMessageEvent);  //if(msg=sendMessageEvent):.... else if(msg=controllo a che punto sono).... lo faro dopo
    if (msg == sendMessageEvent){
        char msgname[20];
        ++nbGenMessages; //Total number of packets sent by a specific source(thus with the same priority) up to now
        sprintf(msgname, "message%d-#%d", id, nbGenMessages);

        msg_check *message = new msg_check(msgname);
        interArrivalTime=exponential(par("interArrivalTime").doubleValue()); //collect the interarrival time as parameter
        //EV<<"interArrivalTime "<<id<<" = "<<interArrivalTime<<endl;
        message->setHasEnded(false);  //set message priority
        message->setProbing(false);
        message->setProbed(false);
        message->setResidualTime(SIMTIME_ZERO); //initialize to the partial elaboration done of a packet; will be useful for server utilization signal and preemptive resume
        //message->setWaitingTime(SIMTIME_ZERO);
        message->setJobId(0); //will be useful for computing the per class extended service time
        message->setSourceId(id);  //initialize to 0 the time when a packet goes for he first time to service(useful for extended per class service time)
        dst=uniform(N+2,N+E+1);
        //EV<<" dst "<<dst<<endl ;
        output=dst-N-2;
        message->setActualExecId(output);
        message->setOriginalExecId(output);
        message->setQueueLength(0);
        message->setReRouted(false);
        EV<<"destination machine "<<output<<" output port of the user"<<output<<endl;
        send(message,"user$o",output);  //send the message to the queue
        scheduleAt(simTime()+interArrivalTime, sendMessageEvent);  //self call that the i-th source makes to generate a new packet with the same priority of the previous ones

    }
    else{
        if(msg->getHasEnded()==true){//end of processing
            EV<<"end of computation "<<msg->getJobId()<<endl;
            workInProgress.erase(msg->getJobId()); //delete the job id
            delete msg;
            }

        else{ //notify to the user the jobid
            workInProgress.insert({msg->getJobId(),msg->getOriginalExecId()});
            EV<<"As a user "<<id<<" "<<msg->getJobId() <<" from "<<workInProgress.at(msg->getJobId())<<endl;

            }
    }

    //else if (end of processing)... else if(msg=controllo a che punto sono).... lo faro dopo
}