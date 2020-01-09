#include <string.h>
#include <omnetpp.h>
#include <msg_check_m.h>
#include <map>

using namespace omnetpp;

class Client : public cSimpleModule {
private:
  int clientId;
  unsigned int nbGenMessages;
  int E;
  int maxRetry;
  msg_check *sendNewJob;
  msg_check *timeoutAckNewJob;
  msg_check *checkJobStatus;
  msg_check *msg_to_ack;
  simtime_t timeoutAck;
  simtime_t timeoutStatus;
  simtime_t channelDelay;

  bool startCheckJobStatus;
  cArray notComputed;
  cArray noStatusInfo;
  void jobStatusHandler();
  void selfMessage(msg_check *msg);

protected:
  virtual void initialize();
  virtual void handleMessage(cMessage *cmsg);
  simsignal_t avgSendingRateSignal;
  simsignal_t avgComplexitySignal;
  simsignal_t realTimeSignal;

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
    cancelAndDelete(checkJobStatus);
}

void Client::initialize() {
    //initializing variables
    simtime_t interArrivalTime;

    channelDelay= par("channelDelay");
    timeoutAck=par("timeoutAck")+2*channelDelay; //the channelDelay should be considered twice in the timeouts:one for the send and one for the reply(2 accesses to the channel)
    timeoutStatus=par("timeoutStatus")+2*channelDelay;
    EV<<"ack "<<timeoutAck<<" status "<<timeoutStatus<<endl;
    E = par("E"); //non volatile parameters --once defined they never change
    interArrivalTime=exponential(par("interArrivalTime").doubleValue()); //simulate an exponential generation of packets
    maxRetry=par("maxRetry");

    avgSendingRateSignal = registerSignal("avgSendingRate");
    avgComplexitySignal = registerSignal("avgComplexity");
    realTimeSignal = registerSignal("realTime");

    emit(avgSendingRateSignal,interArrivalTime);


    sendNewJob = new msg_check("sendNewJob");
    timeoutAckNewJob = new msg_check("timeoutAckNewJob");
    checkJobStatus = new msg_check("checkJobStatus");
    nbGenMessages=0;
    clientId=getIndex();

    scheduleAt(simTime() + timeoutStatus, checkJobStatus);
    scheduleAt(simTime() + interArrivalTime, sendNewJob);
}

void Client::handleMessage(cMessage *cmsg) {
    unsigned int executor;
    cObject *obj;
    msg_check *message;
    simtime_t interArrivalTime,realComputationTime;
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
                        EV<<"Current Status Completed for "<<jobId<<endl;
                        executor = msg->getOriginalExecId();
                        realComputationTime=msg->getEndingTime()-msg->getStartingTime();
                        send(msg,"user$o",executor);
                        //delete the job from the list of the job currently in processing
                        EV<<"real computation time measured "<<realComputationTime<<endl;
                        emit(realTimeSignal,realComputationTime);
                        if(noStatusInfo.exist(jobId))
                            delete noStatusInfo.remove(jobId);
                        else
                            if(notComputed.exist(jobId)){
                                delete notComputed.remove(jobId);
                            }
                    }
                    else{
                        EV<<"Not completed: "<<jobId<<endl;

                        obj = noStatusInfo.remove(jobId);//if doesn't exist?
                        if (obj!=nullptr){
                           message = check_and_cast<msg_check *>(obj);
                           EV << "Job not processed yet:we will reAask his status later "<<message->getName()<<endl;
                           notComputed.add(message);
                        }
                        /* questo ramo else non ho ben capito cosa faccia, se l'executor manda al client
                         * che il job non è completato come mai lo rimuoviamo da  notComputed?
                         *else{
                            obj = notComputed.remove(jobId);
                            if (obj!=nullptr)
                                EV<<"pkt whose status should be reasked in the future will be removed from the client"<<endl;
                            else
                                EV << "FATAL ERROR::pkt isn't in any of the client queues"<<jobId<<endl;
                        }*/
                        delete msg;
                    }
                }else
                    delete msg;
            }
            else //received the ack from the executor, the job was received correctly
                if(msg->getNewJob()){
                     if(msg->getAck()==true){
                        msg->setNewJob(false);
                        msg->setAck(false);
                        notComputed.add(msg);
                        EV << "ACK received for "<<jobId<<endl;
                        cancelEvent(timeoutAckNewJob);
                        delete msg_to_ack;
                        //simulate an exponential generation of packets
                        interArrivalTime=exponential(par("interArrivalTime").doubleValue());
                        emit(avgSendingRateSignal,interArrivalTime);
                        EV<<"Interarrival time "<<interArrivalTime<<endl;
                        //re-start the timer for new jobs
                        scheduleAt(simTime()+interArrivalTime, sendNewJob);
                    }else
                        delete msg;
                }else
                    delete msg;

}



void Client::jobStatusHandler(){
  /*  int i;
    msg_check *message,*msgStore;
    cObject *obj;
    int executor;

    for (i = 0;i < notComputed.size();i++){
        obj = notComputed.remove(i);
        if (obj!=nullptr){
           message = check_and_cast<msg_check *>(obj);
           //EV << "Remove from notCompleted cArray "<<message->getRelativeJobId()<<endl;
           msgStore = message->dup();
           noStatusInfo.add(msgStore);
        }
        else
           EV << "FATAL ERROR: Erasing in executor from the notComputed queue: "<<endl;

        message->setStatusRequest(true);
        executor = message->getOriginalExecId();
        EV<<"Asking the status of: "<<message->getName()<<endl;
        send(message,"user$o",executor);

    }
    scheduleAt(simTime() + timeoutStatus, checkJobStatus);*/
}


void Client::selfMessage(msg_check *msg){
    int executor;

    msg_check *message;
    simtime_t jobComplexity;

    if (msg == sendNewJob){
        maxRetry=par("maxRetry");
        //EV<<"maxRetry in normal mode "<<maxRetry<<endl;
        char msgname[20];
        ++nbGenMessages; //Total number of packets sent by a specific source(thus with the same priority) up to now
        sprintf(msgname, "message%d-#%d", clientId, nbGenMessages);

        //select the executor among a set of uniform values

        executor=rand() % E;


        message = new msg_check(msgname);
        message->setStatusRequest(false);
        message->setProbing(false);
        jobComplexity=exponential(par("jobComplexity").doubleValue());
        message->setJobComplexity(jobComplexity);
        emit(avgComplexitySignal,jobComplexity);
        EV<<"job complexity "<<jobComplexity<<endl;
        message->setRelativeJobId(0); //will be useful for computing the per class extended service time
        message->setClientId(clientId);  //initialize to 0 the time when a packet goes for he first time to service(useful for extended per class service time)
        message->setActualExecId(executor);
        message->setOriginalExecId(executor);
        message->setQueueLength(0);
        message->setReRouted(false);
        message->setIsEnded(false);
        message->setAck(false);
        message->setNewJob(true);
        message->setReBoot(false);
        message->setStartingTime(simTime());
        message->setEndingTime(SIMTIME_ZERO);
        EV<<"msg sent to machine "<<executor<<endl;
        msg_to_ack=message->dup();
        send(message,"user$o",executor);  //send the message to the queue
        scheduleAt(simTime()+timeoutAck, timeoutAckNewJob);//waiting ack
    }
    else
        //if the message is a timeout event the message it is re-sent to the executor
        if (msg==timeoutAckNewJob) {

            if(maxRetry)
                maxRetry--;
            else{
               maxRetry=par("maxRetry");

               executor=rand() % E;

               while(executor==msg_to_ack->getOriginalExecId())
                   executor=rand() % E;
               msg_to_ack->setActualExecId(executor);
               msg_to_ack->setOriginalExecId(executor);
               EV<<"Change destination executor "<<endl;
            }
            //EV<<"maxretry when ack expired "<<maxRetry<<endl;
            EV << "Timeout expired, re-sending message and restarting timer\n";
            send(msg_to_ack->dup(),"user$o",msg_to_ack->getOriginalExecId());
            //start the timeout for the re-transmission
            scheduleAt(simTime()+timeoutAck, timeoutAckNewJob);
        }
        else
           if(msg == checkJobStatus){
            // jobStatusHandler();
           }
}
