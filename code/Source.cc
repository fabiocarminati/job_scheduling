#include <string.h>
#include <omnetpp.h>
#include <msg_check_m.h>
#include <map>

using namespace omnetpp;

class Source : public cSimpleModule {
private:
  int sourceID;
  int nbGenMessages;
  int N,E;
  msg_check *sendMessageEvent;
  msg_check *timeoutEvent;
  msg_check *msg_to_ack;
  simtime_t timeout;
  std::map<std::string,int> workInProgress;

protected:
  virtual void initialize();
  virtual void handleMessage(cMessage *cmsg);

public:
  simtime_t interArrivalTime;
  Source();
  ~Source();
};

Define_Module(Source);

Source::Source()
{
    timeoutEvent = sendMessageEvent = msg_to_ack = nullptr;
}

Source::~Source()
{
    cancelAndDelete(timeoutEvent);
    cancelAndDelete(sendMessageEvent);
}

void Source::initialize() {
    //initializing variables
    sendMessageEvent = new msg_check("sendMessageEvent");
    timeoutEvent = new msg_check("timeoutEvent");
    nbGenMessages=0;
    timeout=0.5;
    sourceID=getId()-1;   //defines the Priority-ID of the message that each source will transmit(different sources send different priorities messages)

    EV<<"Source ID "<<sourceID<<endl;
    scheduleAt(simTime(), sendMessageEvent); //generates the first packet
    E = par("E"); //non volatile parameters --once defined they never change
    N= par("N");

   // timeoutEvent = msg_to_ack =nullptr;

}

void Source::handleMessage(cMessage *cmsg) {
    int destinationPort;
    int destinationMachine;
    msg_check *message;
    // Casting from cMessage to msg_check
    msg_check *msg = check_and_cast<msg_check *>(cmsg);

    if (msg == sendMessageEvent){
        char msgname[20];
        ++nbGenMessages; //Total number of packets sent by a specific source(thus with the same priority) up to now
        sprintf(msgname, "message%d-#%d", sourceID, nbGenMessages);

        //select the executor among a set of uniform values
        destinationMachine=uniform(N+2,N+E+1);
        destinationPort=destinationMachine-N-2;

        message = new msg_check(msgname);
        message->setHasEnded(false);
        message->setProbing(false);
        message->setProbed(false);
        message->setResidualTime(SIMTIME_ZERO); //initialize to the partial elaboration done of a packet; will be useful for server utilization signal and preemptive resume
        //message->setWaitingTime(SIMTIME_ZERO);
        message->setJobId(0); //will be useful for computing the per class extended service time
        message->setSourceId(sourceID);  //initialize to 0 the time when a packet goes for he first time to service(useful for extended per class service time)
        message->setActualExecId(destinationPort);
        message->setOriginalExecId(destinationPort);
        message->setQueueLength(0);
        message->setReRouted(false);
        //simulate an exponential generation of packets
        interArrivalTime=exponential(par("interArrivalTime").doubleValue()); //collect the interarrival time as parameter

        EV<<"msg sent to machine "<<destinationMachine<<" with user-output port "<<destinationPort<<endl;
        msg_to_ack=message->dup();
        delete message;
        send(msg_to_ack->dup(),"user$o",destinationPort);  //send the message to the queue
        scheduleAt(simTime()+timeout, timeoutEvent);//waiting ack

    }
    else{
        //if the message is a timeout event the message it is re-sent to the executor
        if (msg==timeoutEvent) {
            EV << "Timeout expired, resending message and restarting timer\n";

            send(msg_to_ack->dup(),"user$o",msg_to_ack->getOriginalExecId());
            // non ho ben capito perchè crei un messaggio cosi, faccio alcune modifiche
            // se pensi che non vadano benne dimmi pure: msg_to_ack=new msg_check("copy");
            //start the timeout for the re-transmission
            scheduleAt(simTime()+timeout, timeoutEvent);
            }
        else{
            //end of the processing
            if(msg->getHasEnded()==true){
                EV<<"end of computation "<<msg->getJobId()<<endl;
                //delete the job from the list of the job currently in processing
                workInProgress.erase(msg->getJobId());
                delete msg;
                }
            else{ //received the ack from the executor, the job was received correctly
                workInProgress.insert(std::pair<std::string, int>(msg->getJobId(),msg->getOriginalExecId()));
                EV << "ACK received for "<<msg->getJobId() <<" from "<<workInProgress.at(msg->getJobId())<<endl;
                cancelEvent(timeoutEvent);
                //delete msg;
                //simulate an exponential generation of packets
                interArrivalTime=exponential(par("interArrivalTime").doubleValue());
                //re-start the timer for new jobs
                scheduleAt(simTime()+interArrivalTime, sendMessageEvent);
                }
            }
    }
    //else if (end of processing)... else if(msg=controllo a che punto sono).... lo faro dopo
}
