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
  simtime_t timeoutAck;
  simtime_t timeoutStatus;
  simtime_t channelDelay;

  bool startCheckJobStatus;
  cArray workInProgress;
  void jobStatusHandler();
  void selfMessage(msg_check *msg);

protected:
  virtual void initialize();
  virtual void handleMessage(cMessage *cmsg);
  simsignal_t avgSendingRateSignal;
  simsignal_t avgComplexitySignal;


public:
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
    simtime_t interArrivalTime;

    channelDelay= par("channelDelay");
    timeoutAck=par("timeoutAck")+2*channelDelay; //the channelDelay should be considered twice in the timeouts:one for the send and one for the reply(2 accesses to the channel)
    timeoutStatus=par("timeoutStatus")+2*channelDelay;
    EV<<"ack "<<timeoutAck<<" status "<<timeoutStatus<<endl;
    E = par("E"); //non volatile parameters --once defined they never change
    N = par("N");
    interArrivalTime=exponential(par("interArrivalTime").doubleValue()); //simulate an exponential generation of packets

    avgSendingRateSignal = registerSignal("avgSendingRate");
    avgComplexitySignal = registerSignal("avgComplexity");

    emit(avgSendingRateSignal,interArrivalTime);



    sendNewJob = new msg_check("sendNewJob");
    timeoutAckNewJob = new msg_check("timeoutAckNewJob");
    checkJobStatus = new msg_check("checkJobStatus");
    nbGenMessages=0;
    sourceID=getId()-1;   //defines the Priority-ID of the message that each source will transmit(different sources send different priorities messages)
    //EV<<"Client ID "<<sourceID<<endl;
    scheduleAt(simTime() + timeoutStatus, checkJobStatus);
    scheduleAt(simTime() + interArrivalTime, sendNewJob);




}

void Client::handleMessage(cMessage *cmsg) {
    int destinationMachine;
    simtime_t interArrivalTime;
    // Casting from cMessage to msg_check
    msg_check *msg = check_and_cast<msg_check *>(cmsg);
    const char  *jobId;
    jobId = msg->getName();
   if(msg->isSelfMessage())
       selfMessage(msg);
        else
            if(msg->getStatusRequest()==true){
                if(msg->getAck()==true){
                    if(msg->getIsEnded()==true){
                        EV<<"Completed: "<<jobId<<endl;
                        destinationMachine = msg->getOriginalExecId();
                        send(msg->dup(),"user$o",destinationMachine);
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
                        emit(avgSendingRateSignal,interArrivalTime);
                        EV<<"Interarrival time "<<interArrivalTime<<endl;
                        //re-start the timer for new jobs
                        scheduleAt(simTime()+interArrivalTime, sendNewJob);
                    }
                }


}

void Client::jobStatusHandler(){
    int i;
    msg_check *message;
    cObject *obj;
    int destinationMachine;
    for (i = 0;i < workInProgress.size();i++){
         obj = workInProgress.get(i);
         if(obj!=nullptr){
            message = check_and_cast<msg_check *>(obj);
            message = message->dup();
            message->setStatusRequest(true);
            destinationMachine = message->getOriginalExecId();
            EV<<"Asking the status of: "<<message->getOriginalExecId()<<"-"<<message->getRelativeJobId()<<endl;
            send(message,"user$o",destinationMachine);
        }
    }
    scheduleAt(simTime() + timeoutStatus, checkJobStatus);
}

void Client::selfMessage(msg_check *msg){
    int destinationMachine;

    msg_check *message;
    simtime_t jobComplexity;

    if (msg == sendNewJob){
        char msgname[20];
        ++nbGenMessages; //Total number of packets sent by a specific source(thus with the same priority) up to now
        sprintf(msgname, "message%d-#%d", sourceID, nbGenMessages);

        //select the executor among a set of uniform values
        destinationMachine=rand() % E;
        //destinationMachine=uniform(N+2,N+E+1);

        message = new msg_check(msgname);
        message->setStatusRequest(false);
        message->setProbing(false);
        jobComplexity=exponential(par("jobComplexity").doubleValue());
        message->setJobComplexity(jobComplexity);
        emit(avgComplexitySignal,jobComplexity);
        EV<<"job complexity "<<jobComplexity<<endl;
        message->setRelativeJobId(0); //will be useful for computing the per class extended service time
        message->setClientId(sourceID);  //initialize to 0 the time when a packet goes for he first time to service(useful for extended per class service time)
        message->setActualExecId(destinationMachine);
        message->setOriginalExecId(destinationMachine);
        message->setQueueLength(0);
        message->setReRouted(false);
        message->setIsEnded(false);
        message->setAck(false);
        message->setNewJob(true);
        message->setReBoot(false);
        EV<<"msg sent to machine "<<destinationMachine<<endl;
        msg_to_ack=message->dup();
        send(message,"user$o",destinationMachine);  //send the message to the queue
        scheduleAt(simTime()+timeoutAck, timeoutAckNewJob);//waiting ack

    }
    else
        //if the message is a timeout event the message it is re-sent to the executor
        if (msg==timeoutAckNewJob) {
            EV << "Timeout expired, re-sending message and restarting timer\n";
            send(msg_to_ack->dup(),"user$o",msg_to_ack->getOriginalExecId());
            //start the timeout for the re-transmission
            scheduleAt(simTime()+timeoutAck, timeoutAckNewJob);
            }
        else
           if(msg == checkJobStatus){
             jobStatusHandler();
           }
}
