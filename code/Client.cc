#include <string.h>
#include <omnetpp.h>
#include <msg_check_m.h>
#include <map>

using namespace omnetpp;

class Client : public cSimpleModule {
private:
  int sourceID;
  int nbGenMessages;
  int N,E;
  msg_check *sendNewJob;
  msg_check *timeoutAckNewJob;
  msg_check *checkJobStatus;
  msg_check *msg_to_ack;
  simtime_t timeout;
  simtime_t timeoutStatus;
  simtime_t jobComplexity;
  bool startCheckJobStatus;
  cArray workInProgress;
  void jobStatusHandler();

protected:
  virtual void initialize();
  virtual void handleMessage(cMessage *cmsg);

public:
  simtime_t interArrivalTime;
  Client();
  ~Client();
};

Define_Module(Client);

Client::Client()
{
    checkJobStatus = timeoutAckNewJob = sendNewJob = msg_to_ack = nullptr;
}

Client::~Client()
{
    cancelAndDelete(timeoutAckNewJob);
    cancelAndDelete(sendNewJob);
}

void Client::initialize() {
    //initializing variables
    sendNewJob = new msg_check("sendNewJob");
    timeoutAckNewJob = new msg_check("timeoutAckNewJob");
    checkJobStatus = new msg_check("checkJobStatus");
    nbGenMessages=0;
    timeout=0.5;
    timeoutStatus=1;
    sourceID=getId()-1;   //defines the Priority-ID of the message that each source will transmit(different sources send different priorities messages)
    //EV<<"Client ID "<<sourceID<<endl;
    scheduleAt(simTime() + timeoutStatus, checkJobStatus);
    scheduleAt(simTime() + timeout, sendNewJob); //generates the first packet
    E = par("E"); //non volatile parameters --once defined they never change
    N = par("N");
    jobComplexity = par("jobComplexity");
    EV<<"job complexity"<<jobComplexity<<endl;

}

void Client::handleMessage(cMessage *cmsg) {
    int destinationPort;
    int destinationMachine;
    msg_check *message;
    // Casting from cMessage to msg_check
    msg_check *msg = check_and_cast<msg_check *>(cmsg);
    const char  *jobId;
    jobId = msg->getName();
    if (msg == sendNewJob){
        char msgname[20];
        ++nbGenMessages; //Total number of packets sent by a specific source(thus with the same priority) up to now
        sprintf(msgname, "message%d-#%d", sourceID, nbGenMessages);

        //select the executor among a set of uniform values
        //destinationMachine=3;
        destinationMachine=uniform(N+2,N+E+1);
        destinationPort=destinationMachine-N-2;

        message = new msg_check(msgname);
        message->setStatusRequest(false);
        message->setProbing(false);
        message->setJobComplexity(0.2); //initialize to the partial elaboration done of a packet; will be useful for server utilization signal and preemptive resume
        message->setRelativeJobId(0); //will be useful for computing the per class extended service time
        message->setClientId(sourceID);  //initialize to 0 the time when a packet goes for he first time to service(useful for extended per class service time)
        message->setActualExecId(destinationPort);
        message->setOriginalExecId(destinationPort);
        message->setQueueLength(0);
        message->setReRouted(false);
        message->setIsEnded(false);
        message->setAck(false);
        message->setNewJob(true);
        message->setReBoot(false); //PHIL:questo viene settato come true da un executor dopo aver crashato ed essersi ripreso. In questo
        //modo quando lo storage vede questo flag a true capisce che deve mandare all'executor tutti i suoi messaggi nella sua map jobQueue(non newJobQueue)

        interArrivalTime=exponential(par("interArrivalTime").doubleValue()); //simulate an exponential generation of packets

        EV<<"msg sent to machine "<<destinationMachine<<" with user-output port "<<destinationPort<<endl;
        msg_to_ack=message->dup();
        send(message,"user$o",destinationPort);  //send the message to the queue
        scheduleAt(simTime()+timeout, timeoutAckNewJob);//waiting ack

    }
    else{
        //if the message is a timeout event the message it is re-sent to the executor
        if (msg==timeoutAckNewJob) {
            EV << "Timeout expired, re-sending message and restarting timer\n";

            send(msg_to_ack->dup(),"user$o",msg_to_ack->getOriginalExecId());
            //start the timeout for the re-transmission
            scheduleAt(simTime()+timeout, timeoutAckNewJob);
            }
        else
            //end of the processing
            if(msg->getStatusRequest()==true){
                if(msg->getAck()==true){
                    if(msg->getIsEnded()==true){
                        EV<<"Completed: "<<jobId<<endl;
                        destinationPort = msg->getOriginalExecId();
                        send(msg->dup(),"user$o",destinationPort);

                        //delete the job from the list of the job currently in processing
                        delete workInProgress.remove(jobId);
                    }
                    else{
                        EV<<"Not completed: "<<jobId<<endl;
                    }
                }
                delete msg;
            }
            else //received the ack from the executor, the job was received correctly
                if(msg->getNewJob()){
                     if(msg->getAck()==true){
                        msg->setNewJob(false);
                        msg->setAck(false);
                        workInProgress.add(msg);
                        EV << "ACK received for "<<jobId<<endl;
                        cancelEvent(timeoutAckNewJob);
                        delete msg_to_ack;
                        //simulate an exponential generation of packets
                        interArrivalTime=exponential(par("interArrivalTime").doubleValue());
                        //re-start the timer for new jobs
                        scheduleAt(simTime()+interArrivalTime, sendNewJob);
                    }
                }
                 else
                    if(msg == checkJobStatus){
                      jobStatusHandler();
                    }
    }
}

void Client::jobStatusHandler(){
    int i;
    msg_check *message;
    cObject *obj;
    int destinationPort;
    for (i = 0;i < workInProgress.size();i++){
         obj = workInProgress.get(i);
         if(obj!=nullptr){
            message = check_and_cast<msg_check *>(obj);
            message = message->dup();
            message->setStatusRequest(true);
            destinationPort = message->getOriginalExecId();
            EV<<"Asking the status of: "<<message->getOriginalExecId()<<"-"<<message->getRelativeJobId()<<endl;
            send(message,"user$o",destinationPort);
        }
    }
    scheduleAt(simTime() + timeoutStatus, checkJobStatus);
}
