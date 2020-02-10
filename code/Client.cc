#include <string.h>
#include <omnetpp.h>
#include <msg_check_m.h>
#include <map>

using namespace omnetpp;

/*
 This class represents a client.

It can communicate only with the E executors
It contains two fundamental elements:
     ->The cArray notComputed: contains all the jobs whose status must be asked to the executors. When a job is inside this cArray?
         ->After the original executor has acknowledged the job to the client
         ->If the original executor replies to a previous status request with a NON COMPLETED status reply for that job
     For these jobs status requests are sent every timeoutStatus to the proper original executor
     ->The cArray noStatusInfo: contains all the jobs whose status requests haven't been fulfilled yet by the original executor.
     For these jobs the client won't send any other status request but will simply waits for a status response

In the client we define three signals given that we are interested in the evolution over time of:
    ->Sending rate of the jobs:avgSendingRateSignal signal
    ->Given that we assume that the time needed to compute the jobs is defined by the client (and then communicate to the executor) we
    introduce the avgComplexitySignal signal
    ->Service time for each job:from the moment in which the job is sent to the executor for the first time(selfMessage->timeoutAckNewJob)
    up to the time in which the client receives the COMPLETED status from the original executor. The realTimeSignal signal is used

N.B.In this document we use job,message,packet as synonyms
    In this document we refer to original executor as the executor to which the job is sent by the client
    In this document we refer to actual executor as the executor that processes the job
    Actual exec!=Orignal exec if reRouting is performed due to load balancing
    Actual exec==Orignal exec if load balancing has not been performed
    Anyway only the original executor can communicate with the client and not the actual executor
 */

class Client : public cSimpleModule {
private:
  int clientId;
  int E;
  int maxRetry;
  msg_check *sendNewJob;
  msg_check *timeoutAckNewJob;
  msg_check *checkJobStatus;
  msg_check *msgToAck;
  simtime_t timeoutAck;
  simtime_t timeoutStatus;

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
    checkJobStatus = timeoutAckNewJob = sendNewJob = msgToAck = nullptr;
}

Client::~Client()
{
    cancelAndDelete(timeoutAckNewJob);
    cancelAndDelete(sendNewJob);
    cancelAndDelete(checkJobStatus);
    if(msgToAck != nullptr)
        delete msgToAck;
}

/*
 * INITIALIZE
At the beginning of the simulation we:
    ->Recover all the parameters of the module Client
    ->Schedule the generation of a job after at time equal to sendingTime and emit the respective avgSendingRateSignal signal
    ->Schedule the after at time equal to timeoutStatus the sending of the status requests for the jobs inside the cArray notComputed
*/

void Client::initialize() {
    simtime_t sendingTime,channelDelay;

    channelDelay= par("channelDelay");
    //the channelDelay should be considered twice in the timeouts:one for the send and one for the reply(2 accesses to the channel)
    timeoutAck = par("timeoutAck")+2*channelDelay;
    timeoutStatus = par("timeoutStatus")+2*channelDelay;
    E = par("E");
    sendingTime = exponential(par("sendingTime").doubleValue());
    maxRetry = par("maxRetry");

    avgSendingRateSignal = registerSignal("avgSendingRate");
    avgComplexitySignal = registerSignal("avgComplexity");
    realTimeSignal = registerSignal("realTime");

    emit(avgSendingRateSignal,sendingTime);

    sendNewJob = new msg_check("sendNewJob");
    timeoutAckNewJob = new msg_check("timeoutAckNewJob");
    checkJobStatus = new msg_check("checkJobStatus");
    clientId=getIndex();

    scheduleAt(simTime() + timeoutStatus, checkJobStatus);
    scheduleAt(simTime() + sendingTime, sendNewJob);
}

/*
 * HANDLE MESSAGE
Different actions according to the type of message received by the client:
    ->Self messages are handled through the selfMessage() function
    ->Status messages:
        ->COMPLETED:Emit the realTimeSignal + remove the correspondent job from the proper cArray(either notComputed or noStatusInfo)
        ->NOT COMPLETED:The job is put again in the notCompleted cArray such that a new status request will be forwarded to the executor
    ->Ack message from the original executor containing the JobId:
        ->Cancel the self event timeoutAckNewJob
        ->Put the message in notComputed such that a status request for that job will be generated
        ->Schedule the generation of a new job after at time equal to sendingTime and emit the respective avgSendingRateSignal signal
 */

void Client::handleMessage(cMessage *cmsg) {
    unsigned int executor;
    cObject *obj;
    msg_check *message;
    simtime_t sendingTime, realComputationTime;
    msg_check *msg = check_and_cast<msg_check *>(cmsg);
    const char  *jobId;

    jobId = msg->getName();
    if(msg->isSelfMessage())
       selfMessage(msg);
        else
            if(msg->getStatusRequest()==true){
                if(msg->getAck()==true){
                    if(msg->getIsEnded()==true){
                        EV<<"COMPLETED status info received for: "<<jobId<<endl;
                        executor = msg->getOriginalExecId();
                        realComputationTime=msg->getEndingTime()-msg->getStartingTime();
                        send(msg,"links$o",executor);
                        if(noStatusInfo.exist(jobId)){
                            EV<<"Total Service time client side: "<<realComputationTime<<endl;
                            emit(realTimeSignal,realComputationTime);
                            delete noStatusInfo.remove(jobId);
                        }else
                            if(notComputed.exist(jobId)){
                                EV<<"Total Service time client side: "<<realComputationTime<<endl;
                                emit(realTimeSignal,realComputationTime);
                                delete notComputed.remove(jobId);
                            }
                    }
                    else{
                        EV<<"NOT COMPLETED status info received for: "<<jobId<<endl;
                        obj = noStatusInfo.remove(jobId);
                        if (obj!=nullptr){
                           message = check_and_cast<msg_check *>(obj);
                           EV << "Put the job "<<jobId<<" into notComputed cArray such that the client will reAask his status later"<<endl;
                           notComputed.add(message);
                        }
                        delete msg;
                    }
                }else
                    delete msg;
            }
            else
                if(msg->getNewJob()){
                     if(msg->getAck()==true){
                        msg->setNewJob(false);
                        msg->setAck(false);
                        notComputed.add(msg);
                        EV << "ACK received for "<<jobId<<endl;
                        cancelEvent(timeoutAckNewJob);
                        delete msgToAck;
                        msgToAck = nullptr;
                        sendingTime=exponential(par("sendingTime").doubleValue());
                        emit(avgSendingRateSignal,sendingTime);
                        scheduleAt(simTime()+sendingTime, sendNewJob);
                    }else
                        delete msg;
                }else
                    delete msg;
}

/*
 * JOBSTATUSHANDLER
Every timeoutStatus sends the status requests for each job inside the notComputed cArray to the respective original executor.

N.B
    For how the executor is designed if a status reply isn't received by the client it means that either the original or the actual executor
    are in failure mode at that moment. Thus if sending a new status request to them will be useless and will only add traffic on the links.
    Thus the jobs are moved from notComputed to noStatusInfo such that we don't send another status request for the same job before the
    original executor replies to the first one(and this will happen after the executor comes back from a failure
    --->see executor->updateJobsStatus().)
*/

void Client::jobStatusHandler(){
    int i;
    msg_check *message,*msgStore;
    cObject *obj;
    int executor;

    for (i = 0;i < notComputed.size();i++){
        obj = notComputed.remove(i);
        if (obj!=nullptr){
           message = check_and_cast<msg_check *>(obj);
           msgStore = message->dup();
           noStatusInfo.add(msgStore);
           message->setStatusRequest(true);
           executor = message->getOriginalExecId();
           EV<<"Asking the status of: "<<message->getName()<<endl;
           send(message,"links$o",executor);
        }
        else
           EV << "FATAL ERROR during erase in notComputed cArray of the client: "<<clientId<<endl;
    }
    scheduleAt(simTime() + timeoutStatus, checkJobStatus);
}

/*
 * SELFMESSAGE
Handles the self messages:
    ->sendNewJob:A new job is generated:
        ->choose the original executor randomly
        ->send the job through a message with the proper fields initialised
        ->Before generating any new job the client waits for a response message with the jobId from the original executor up to timeoutAck
    ->timeoutAckNewJob:the timeoutAck has expired(the original executor hasn't replied with the jobId)
        ->the client reTries to transmit the same job to the same executor up to maxRetry times. If the the original executor still
        doesn't reply for maxRetry times the client assumes that it isn't available and thus sends the job to a new executor.
        N.B The starting time field of the job isn't changed
    ->checkJobStatus: invoke the function that sends the status requests messages (jobStatusHandler())
 */

void Client::selfMessage(msg_check *msg){
    int executor;
    msg_check *message;
    simtime_t jobComplexity;

    if (msg == sendNewJob){
        maxRetry=par("maxRetry");
        executor=rand() % E;

        message = new msg_check(" ");
        message->setStatusRequest(false);
        message->setProbing(false);
        jobComplexity=exponential(par("jobComplexity").doubleValue());//no default
        message->setJobComplexity(jobComplexity);
        emit(avgComplexitySignal,jobComplexity);
        message->setRelativeJobId(0);
        message->setClientId(clientId);
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
        EV<<"Job sent to original executor "<<executor<<endl;
        msgToAck=message->dup();
        send(message,"links$o",executor);
        scheduleAt(simTime()+timeoutAck, timeoutAckNewJob);
    }
    else
        if (msg == timeoutAckNewJob) {
            if(maxRetry)
                maxRetry--;
            else{
               maxRetry=par("maxRetry");
               executor=rand() % E;
               while(executor==msgToAck->getOriginalExecId())
                   executor=rand() % E;
               msgToAck->setActualExecId(executor);
               msgToAck->setOriginalExecId(executor);
               EV<<"Change destination executor: "<<executor<<endl;
            }
            EV << "timeoutAck expired, re-sending the same job and restarting timer"<<endl;
            send(msgToAck->dup(),"links$o",msgToAck->getOriginalExecId());
            scheduleAt(simTime()+timeoutAck, timeoutAckNewJob);
        }
        else
           if(msg == checkJobStatus){
             jobStatusHandler();
           }
}
