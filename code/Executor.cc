#include <string.h>
#include <omnetpp.h>
#include <msg_check_m.h>
#include <map>
using namespace omnetpp;

/*
 This class represents an executor.

It can communicate both with the C clients and with the other E-1 executors
It contains five fundamental elements:
     ->The cArray completedJob:contains all the jobs whose execution is ended.
     A completed job stays in this cArray until the client asks for his status.
     ->The cQueue newJobsQueue: the executor hasn't decided yet how to handle these messages.Several actions are possible:
     1. Send the job to another executor in order to perform load balancing
     2. Keep the job and execute it either immediately if idle or in the future
     ->The cQueue jobQueue: that contains all the messages waiting to be processed by that executor with a FIFO logic.
     ->The cQueue balanceResponses:here there are all the messages containing the jobQueue length values of the other executors(useful
     during the load balancing process)
     ->The cArray reRoutedJobs:keeps track of all the jobs sent to other executors due to load balancing.

The total number of completed jobs by each executor at the end of the simulation is printed at the end of the simulation.

Please notice that in this document we use job,message,packet as synonyms.
 */

class Executor : public cSimpleModule {
private:

    msg_check *timeoutJobComputation;
    msg_check *timeoutLoadBalancing;
    msg_check *timeoutReRouted;
    msg_check *timeoutFailureEnd;

    simtime_t channelDelay;
    simtime_t timeoutLoad;
    simtime_t timeoutFailure;

    int E,myId,granularity,skipLoad,probeResponse;
    double probEvent,probCrashDuringExecution;
    bool probingMode,failure;

    unsigned int jobCompleted;
    unsigned int nNewJobArrived;

    cArray completedJob;
    cQueue newJobsQueue;
    cQueue jobQueue;
    cQueue balanceResponses;
    cArray reRoutedJobs;

    void selfMessage(msg_check *msg);
    void newJobHandler(msg_check *msg);
    void balancedJob(msg_check *msg);
    void probeHandler(msg_check *msg);
    void statusRequestHandler(msg_check *msg);
    void reRoutedHandler(msg_check *msg);
    void timeoutLoadBalancingHandler();
    void timeoutJobExecutionHandler();
    void failureEvent(double probEvent);
    void rePopulateQueues(msg_check *msg);
    void restartNormalMode();
    bool checkDuplicate(msg_check *msg);
    void updateJobsStatus();

protected:
    virtual void initialize();// override;
    virtual void handleMessage(cMessage *cmsg);// override;
    virtual void finish();

public:
  simtime_t interArrivalTime;
  Executor();
  ~Executor();

};
Define_Module(Executor);
Executor::Executor()
{
    timeoutLoadBalancing = timeoutFailureEnd = nullptr;
    timeoutJobComputation = timeoutReRouted = nullptr;
}

Executor::~Executor()
{
    cancelAndDelete(timeoutLoadBalancing);
    cancelAndDelete(timeoutJobComputation);
    cancelAndDelete(timeoutReRouted);
    cancelAndDelete(timeoutFailureEnd);
}

/*
INITIALIZE
At the beginning of the simulation we recover all the parameters of the module Executor and initialize to 0 or to false all
the other variables that we will need.
*/

void Executor::initialize() {
    probingMode = false;
    E = par("E"); //non volatile parameters --once defined they never change
    granularity = par("granularity");
    probeResponse = par("probeResponse");
    skipLoad=granularity;
    channelDelay = par("channelDelay");
    timeoutLoad = par("timeoutLoad")+2*channelDelay; //the channelDelay should be considered twice in the timeouts:one for the send and one for the reply(2 accesses to the channel)
    timeoutFailure = par("timeoutFailure");
    myId=getIndex();
    nNewJobArrived=0;
    jobCompleted=0;
    probEvent=par("probEvent");
    probCrashDuringExecution=par("probCrashDuringExecution");
    failure=false;

    timeoutJobComputation = new msg_check("Job completed");
    timeoutLoadBalancing = new msg_check("timeoutLoadBalancing");
    timeoutReRouted = new msg_check("timeoutReRouted");
    timeoutFailureEnd = new msg_check("timeoutFailureEnd");
}

/*
HANDLE MESSAGE
Each executor can be in three different states:
1. Normal mode:the executor processes packets or fails either immediately after the arrival of a packet or during his processing
2. Failure mode:the executor discards any incoming packet a part from the self message notifying the end of the failure phase
3. Reboot mode: the executor recovers from a failure. During this phase only the messages coming from the backup are accepted

During normal mode the executor treats the incoming packets in different ways according to their flags.
*/

void Executor::handleMessage(cMessage *cmsg) {
  // Casting from cMessage to msg_check
   msg_check *msg = check_and_cast<msg_check *>(cmsg);
   msg_check *msgSend;

   failureEvent(probEvent);
   if(failure){
       if(msg==timeoutFailureEnd) {
            //After the expiration of the timeoutFailure a message is sent to the storage such that the backup process can start.
            msgSend = new msg_check("Failure end");
            msgSend->setReBoot(true);
            msgSend->setActualExecId(myId);
            msgSend->setOriginalExecId(myId);
            EV<<"Reboot phase starts"<<endl<<"....."<<endl;
            send(msgSend,"backup_send$o");
            bubble("Reboot mode");

       }
       else if(msg->getReBoot()==true){
                rePopulateQueues(msg);
             }
           else{
               EV<<"The executor isn't in normal mode and this is not a msg coming from the backup:ignore it"<<endl;
               if(!msg->isSelfMessage())
                   delete msg;
           }
   }
   else{//Normal Mode
         if(msg->isSelfMessage()){
               selfMessage(msg);
         }else{
             if(msg->getNewJob()==true){
                           newJobHandler(msg);
                       }else if(msg->getProbing()==true){
                                probeHandler(msg);
                             }else
                                 if(msg->getStatusRequest()==true){
                                     statusRequestHandler(msg);
                                   }else if(msg->getReRouted()==true){
                                           reRoutedHandler(msg);
                                         }
         }
   }
}

/*
 * CHECK DUPLICATE---Normal Mode
It is designed for a specific situation during the probing process: when a packet is sent to another executor due to load balancing
the original executor will move the job from the newJobsQueue to the reRoutedJobs only after having received of the acknowledgement from the
actual executor. But if the original executor crashes before receiving this ack?
The consequence is that after the failure it will reDo the load balancing process for the same packet given that is still in the newJobsQueue;
so two (or more in case this situation repeats) executors may compute the same packet.
One way to avoid this is when a probe request is received to check(through the checkDuplicate) whether that job is found in one of his
queue (and so the load balancing for that job must immediately ends) or not found(the load balancing keeps going).
*/

bool Executor::checkDuplicate(msg_check *msg){
    bool isInJobQueue;
    bool isInNewJobQueue;
    bool isInReRoutedJob;
    bool isInCompletedJob;
    const char *jobId;

    jobId = msg->getName();
    isInJobQueue=false;
    for (cQueue::Iterator it(jobQueue);!isInJobQueue&&!it.end(); ++it) {
        if(strcmp((*it)->getName(), jobId)==0)
            isInJobQueue=true;
    }
    isInNewJobQueue=false;
    for (cQueue::Iterator it(newJobsQueue);!isInNewJobQueue&&!it.end(); ++it) {
        if(strcmp((*it)->getName(), jobId)==0)
            isInNewJobQueue=true;
    }
    isInReRoutedJob=reRoutedJobs.exist(jobId);
    isInCompletedJob=completedJob.exist(jobId);
    EV<<"bool job queue "<<isInJobQueue<<" bool new job queue "<<isInNewJobQueue
            <<" bool rerouted job"<<isInReRoutedJob<<" bool completed job"<<isInCompletedJob<<endl;

    if(isInJobQueue||isInNewJobQueue||isInReRoutedJob||isInCompletedJob){
        EV<<"Duplicate found"<<endl;
        return true;
    }
    return false;
}

/*
 * FAILURE EVENT---Normal Mode->Failure Mode
A failure can happen in two situations:
    ->Immediately after receiving a packet with probability probEvent
    ->In the middle of the processing with probability probCrashDuringExecution.
In the latter case we consider that a failure can happen only after the storage is notified about the processing and before
the same response is sent to the proper entity(executor/client).

With this function we can handle both the two cases:
->if the executor is already in the FAILURE OR RECOVERY MODE this function will do nothing
->if the executor is in NORMAL MODE: we check whether a failure occurs using a uniform function and the probability value given as
parameter. In case the failure occurs we made some assumptions on how it should be represented:
     ->The jobs inside newJobsQueue,jobQueue,balanceResponses,reRoutedJobs,completedJob are lost
     ->The executor goes in FAILURE MODE
     ->The executor stops to wait for load balancing responses(probingMode=false)
     ->Any timeout that will generate a self event in the future is interrupted
Then the timeoutFailure is started and until his expiration any other incoming message will be discarded by the executor(failure mode).
*/

void Executor::failureEvent(double prob){
    if(!failure){
        if(uniform(0,1)<prob){
         failure=true;
         newJobsQueue.clear();
         jobQueue.clear();
         balanceResponses.clear();
         reRoutedJobs.clear();
         completedJob.clear();
         probingMode = false;

         cancelEvent(timeoutReRouted);
         cancelEvent(timeoutJobComputation);
         cancelEvent(timeoutLoadBalancing);
         cancelEvent(timeoutFailureEnd);
         scheduleAt(simTime()+timeoutFailure, timeoutFailureEnd);
         EV<<"A failure has happened:start failure phase "<<"......"<<endl;
         bubble("Failure");
        }
    }
}

/*
 * REPOPULATE QUEUES---Reboot Mode:1->Normal Mode
    1)The storage is sending its copies of the jobs to the executor with the flag ReBoot=true.
    2)According to the flag inside the message the executor will store the jobs in the proper queue.
    Be careful that in case of a message with JobQueue=true there are two possible actions:
        ->If the job has been received by another executor due to load balancing(ReRouted=true) we store it inside the jobQueue of the
        executor
        ->If the job hasn't been rerouted we put it inside the newJobsQueue and not into the jobQueue given that in the time during which
        the executor was in failure mode the load of the other executors may have been reduced. Therefore redoing the load balancing process
        for that job can decrease his queueing time and so improve the performances of the cluster.
    3)1,2 are repeated until a message with BackupComplete=true is received. This message notifies the end of the backup process:the executor
    moves into the Normal Mode.
*/

void Executor::rePopulateQueues(msg_check *msg){
    msg_check *msgSend;
    if(msg->getBackupComplete()==false){
        msg->setReBoot(false);
        if(msg->getNewJobsQueue()==true){
            msg->setNewJobsQueue(false);
            newJobsQueue.insert(msg);
            EV<<"BACKUP In the new_job_queue"<<endl;
        }else if(msg->getJobQueue()==true){
                  msg->setJobQueue(false);
                  if(msg->getReRouted()==true){
                      jobQueue.insert(msg);
                      EV<<"BACKUP in new because REROUTED"<<endl;
                  }else{
                      msgSend=msg->dup();
                      msgSend->setNewJobsQueue(true);
                      send(msgSend,"backup_send$o");
                      msgSend=msg->dup();
                      msgSend->setJobQueue(true);
                      send(msgSend,"backup_send$o");
                      newJobsQueue.insert(msg);
                      EV<<"BACKUP In the new job because has not been re routed"<<endl;
                  }
              }else if(msg->getReRoutedJobQueue()==true){
                      msg->setReRoutedJobQueue(false);
                      reRoutedJobs.add(msg);
                      EV<<"BACKUP In the rerouted_queue"<<endl;
                     }else if(msg->getCompletedQueue()==true){
                                  msg->setCompletedQueue(false);
                                  completedJob.add(msg);
                                  EV<<"BACKUP in completed jobs"<<endl;
                           }
    }
    else{
        EV<<"The backup process is over, executor is now in normal execution mode"<<endl<<".........."<<endl;
        bubble("Normal mode");
        failure=false;//until I have recovered all the backup message I will still ignore all the other messages
        restartNormalMode();
        delete msg;
    }
}

/*
 * RESTART NORMAL MODE---Reboot Mode:2->Normal Mode
The executor once has recovered its queues it will look for meaningful job status to send either to the client(in case no reRouting has
been performed) or to the original executor(see updateJobsStatus()).
Now the executor starts to process the jobs into the jobQueue or the probing process if it is empty.
If both jobQueue and newJobsQueue are empty the executor goes idle.
*/

void Executor::restartNormalMode(){
    msg_check *msgServiced;
    simtime_t timeoutJobComplexity;
    int jobId, clientId;
    EV<<"After recover Show me JOB "<<jobQueue.getLength()<<endl;
    EV<<"Show me NEW "<<newJobsQueue.getLength()<<endl;
    EV<<"Show me REROURTED "<<reRoutedJobs.size()<<endl;
    EV<<"Show me ENDED "<<completedJob.size()<<endl;
    updateJobsStatus();
    if(jobQueue.isEmpty()){
        if(newJobsQueue.isEmpty())
            EV<<"JOB and NEWJOB queue are idle, the machine goes IDLE"<<endl;
        else{
            msgServiced = check_and_cast<msg_check *>(newJobsQueue.front());
            balancedJob(msgServiced);
        }
    }
    else{
         msgServiced = check_and_cast<msg_check *>(jobQueue.front());
         jobId = msgServiced->getRelativeJobId();
         clientId = msgServiced->getClientId();
         timeoutJobComplexity = msgServiced->getJobComplexity();
         scheduleAt(simTime()+timeoutJobComplexity, timeoutJobComputation);
         EV<<"Starting service of "<<jobId<<" coming from Client ID "<<clientId<<" from the queue of the machine "<<msgServiced->getOriginalExecId()<<endl;
    }
}

/*
 * UPDATEJOBSTATUS---Reboot Mode:3->Normal Mode
As we already know in the time during which an executor is in failure mode it will ignore all the incoming messages including the status
requests/response coming from either other executors or from the clients.
In order to mitigate these losses after the reboot phase the executor will look inside his queues and generate proper status
messages such as:
    ->Notify the end of the computation for the jobs inside the completedJob either to the original executor in case of load balancing
    or to the client.
    ->Ask the status of the jobs inside the reRoutedJobs to their actual executor
    ->Notify that the execution isn't over yet for the jobs inside the jobQueue either to the original executor in case of load balancing
    or to the client.
    ->Notify that the execution isn't over yet for the jobs inside the newJobsQueue to the client
*/

void Executor::updateJobsStatus(){
    msg_check *tmp, *msgSend;
    cObject *obj;
    int clientId;
    int executorId;
    EV<<"check completed cArray"<<endl;
    for (cArray::Iterator it(completedJob); !it.end(); ++it) {
        obj = *it;
        tmp = check_and_cast<msg_check *>(obj);
        msgSend=tmp->dup();
        if(msgSend->getOriginalExecId()==myId)
        {
            msgSend->setStatusRequest(true);
            msgSend->setAck(true);
            msgSend->setIsEnded(true);

            EV<<"During Reboot Found JobId in my completed jobs(original): "<<msgSend->getName()<<endl;
            clientId=msgSend->getClientId();
            send(msgSend,"exec$o",clientId);
        }
        else{
            msgSend->setStatusRequest(true);
            msgSend->setAck(true);
            msgSend->setIsEnded(true);
            msgSend->setReRouted(true);
            EV << "During Reboot Sending the COMPLETED status to original exec: "<<msgSend->getName()<<endl;
            send(msgSend,"load_send",msgSend->getOriginalExecId());
        }
    }

    for (cArray::Iterator it(reRoutedJobs); !it.end(); ++it) {
        obj = *it;

        tmp = check_and_cast<msg_check *>(obj);
        msgSend=tmp->dup();
        executorId = msgSend->getActualExecId();

        msgSend->setStatusRequest(true);
        msgSend->setAck(false);
        msgSend->setReRouted(true);
        msgSend->setIsEnded(false);

        EV << "During Reboot Sending the STATUS request for the REROUTED JOB "<<msgSend->getName()<<" to the actual executor "<<executorId<<endl;
        send(msgSend,"load_send",executorId);
    }

    for (cQueue::Iterator it(jobQueue);!it.end(); ++it) {
        obj = *it;
        tmp = check_and_cast<msg_check *>(obj);

        if(tmp->getOriginalExecId()==myId)
        {
            msgSend=tmp->dup();
            msgSend->setStatusRequest(true);
            msgSend->setAck(true);
            msgSend->setIsEnded(false);

            EV<<"During Reboot Found JobId in my jobQueue(original): "<<msgSend->getName()<<endl;
            clientId=msgSend->getClientId();

            send(msgSend,"exec$o",clientId);
        }
        else{
            msgSend=tmp->dup();
            msgSend->setStatusRequest(true);
            msgSend->setAck(true);
            msgSend->setIsEnded(false);
            msgSend->setReRouted(true);
            executorId = msgSend->getOriginalExecId();

            EV << "During Reboot Sending the NOT COMPLETED status to original exec: "<<msgSend->getName()<<endl;
            send(msgSend,"load_send",executorId);
        }
    }
    for (cQueue::Iterator it(newJobsQueue);!it.end(); ++it) {
        obj = *it;
        tmp = check_and_cast<msg_check *>(obj);
        if(tmp->getOriginalExecId()==myId)
        {
            msgSend=tmp->dup();
            msgSend->setStatusRequest(true);
            msgSend->setAck(true);
            msgSend->setIsEnded(false);

            EV<<"During Reboot Found JobId in my newJobsQueue(original): "<<msgSend->getName()<<endl;
            clientId=msgSend->getClientId();
            send(msgSend,"exec$o",clientId);
        }
        else
            EV<<"this packet is in the wrong queue"<<endl;
    }
}

/*
 * BALANCEDJOB---Normal Mode
This function is invoked for each job inside the newJobsQueue:
    ->The probing process starts: the executor will send a copy of the job to each of the others executors
    (Optional)->A failure in the middle of the probing process can happen with probability probCrashDuringExecution
    ->The executor waits before taking any decision for a period equal to timeoutLoad
*/

void Executor::balancedJob(msg_check *msg){
    msg_check *msgSend;
    int i;
    int clientId;
    clientId=msg->getClientId();
    EV<<"msg ID "<<msg->getRelativeJobId()<<" coming from Client ID "<<clientId<<" trying to distribute probing mode "<<probingMode<<endl;
    if(!probingMode){
        probingMode = true;
        msg->setStatusRequest(false);
        msg->setProbing(true);
        msg->setAck(false);
        msg->setReRouted(false);
        msg->setQueueLength(jobQueue.getLength());
        for(i=0;i<E;i++){
         if(i!=msg->getOriginalExecId()){
             msgSend = msg->dup();
             msgSend->setActualExecId(i);
             failureEvent(probCrashDuringExecution);
             if(failure){
                 EV<<"crash when sending probing messages"<<endl;
                 delete msgSend;
                 return;
             }
             send(msgSend,"load_send",i);
             EV<<"Asking the load to machine "<<msgSend->getActualExecId()<<endl;
             }
        }
        scheduleAt(simTime()+timeoutLoad, timeoutLoadBalancing);
    }
}

/*
 * PROBEHANDLER---Normal Mode
Every time an executor receives a probing requests:
    ->It will check whether that job is already inside one of his queues in order to avoid duplicated packets. If no duplicate is found:
        ->No response is sent if his jobQueue length isn't adequate(***)
        ->A response is sent if his jobQueue length is adequate(***)

(***)
In order to reduce the signaling traffic we decide to not send a reply to a probing message if that executor cannot be useful for the load
balancing.
Moreover we decide to introduce a simulation parameter probeResponse which defines the granularity of the load balancing that is how
must be shorter the jobQueue length of that executor with respect to one in the original executor such that we can to reduce the computation
cost for the load balancing. Of course this result in a less efficient share of the load.
So there is a tradeoff amid the complexity of the load balancing process and the efficiency of the sharing of the load.
Another parameter for the load balancing complexity is the granularity(see newJobsHandler())
(***)

Every time the original executor receives a reply:
    ->If the flag Duplicate=true:stop immediately the load balancing process and put the job inside reRoutedJobs
    ->Otherwise store the reply in balanceResponses. No decision is taken until the timeoutLoad ends
 */

void Executor::probeHandler(msg_check *msg){
    msg_check *tmp;
    int minQueueLength;
    if(msg->getAck()==false){
        msg->setAck(true);
        msg->setStatusRequest(false);
        msg->setProbing(true);
        msg->setReRouted(false);

        if(checkDuplicate(msg)){
            msg->setDuplicate(true);
            EV<<"During load balancing I found a duplicate"<<endl;
        }else
            msg->setDuplicate(false);

        EV<<"The original executor "<<msg->getOriginalExecId()<<" has queue length equal to "<<msg->getQueueLength()<<" while this one: "<<jobQueue.getLength()<<endl;

        minQueueLength=jobQueue.getLength()+probeResponse;
        if(msg->getDuplicate() ||  minQueueLength <= msg->getQueueLength()){
               msg->setQueueLength(jobQueue.getLength());
               send(msg,"load_send",msg->getOriginalExecId());
               EV<<"load balancing reply with granularity: "<<probeResponse<<endl;
         }
        else
             delete msg;
    }else{
            if(!newJobsQueue.isEmpty()&&strcmp(check_and_cast<msg_check *>(newJobsQueue.front())->getName(),msg->getName())==0){
               msg->setProbing(false);
               msg->setAck(false);

               if(msg->getDuplicate()){
                   cancelEvent(timeoutLoadBalancing);
                   tmp = check_and_cast<msg_check *>(newJobsQueue.pop());
                   tmp->setNewJobsQueue(true);
                   send(tmp,"backup_send$o");

                   reRoutedJobs.add(msg->dup());
                   msg->setDuplicate(false);
                   msg->setReRoutedJobQueue(true);
                   send(msg,"backup_send$o");

                   probingMode = false;
                   balanceResponses.clear();
                   if(newJobsQueue.getLength()>0){
                       tmp = check_and_cast<msg_check *>(newJobsQueue.front());
                       balancedJob(tmp);
                   }
               }
               else{
                   EV<<"store load reply from "<< msg->getActualExecId()<<endl;
                   balanceResponses.insert(msg);
               }
           }
           else
              delete msg;
    }
}

/*
 * REROUTEDHANDLER---Normal Mode
The actual executor has received the job due to load balancing:
    ->Insert it inside the jobQueue(not inside the newJobsQueue otherwise it will undergo again the load balancing process) and notify
    the storage
    (Optional)->A failure in the middle processing can happen with probability probCrashDuringExecution
    ->Notify the original executor that the job has been received successfully
    ->Execute the job immediately only if the jobQueue length is 1

The original executor has received the confirmation from the actual executor:
    ->Move the job from newJobsQueue into reRoutedJobs
    ->For the first packet(if any) inside the newJobsQueue we redo the load balancing process.
 */

void Executor::reRoutedHandler(msg_check *msg){
    msg_check *msgSend,*tmp;
    int actualExecId;
    simtime_t timeoutJobComplexity;
    if(msg->getAck()==false){
        jobQueue.insert(msg->dup());

        msgSend=msg->dup();
        msgSend->setJobQueue(true);
        send(msgSend,"backup_send$o");

        failureEvent(probCrashDuringExecution);
        if(failure){
            EV<<"crash in the middle of notify the packet received due to load balancing;TIMEOUT WILL EXPIRE:RESTART LOAD BALANCING "<<endl;
            delete msg;
            return;
        }
        msgSend = msg;
        msgSend->setAck(true);
        send(msgSend,"load_send",msgSend->getOriginalExecId());

        if(!timeoutJobComputation->isScheduled()){
             timeoutJobComplexity = msgSend->getJobComplexity();
             EV<<"The new executor is idle:it starts executing immediately the packet"<<endl;
             scheduleAt(simTime()+timeoutJobComplexity, timeoutJobComputation);
        }
        EV<<"new job in the queue, actual length: "<< jobQueue.getLength()<<endl;
    }else{
        cancelEvent(timeoutReRouted);
        tmp = check_and_cast<msg_check *>(newJobsQueue.pop());
        msgSend=tmp->dup();
        msgSend->setNewJobsQueue(true);
        send(msgSend,"backup_send$o");
        actualExecId = msg->getActualExecId();
        tmp->setActualExecId(actualExecId);

        msgSend=tmp->dup();
        msgSend->setReRoutedJobQueue(true);
        send(msgSend,"backup_send$o");

        reRoutedJobs.add(tmp);
        probingMode = false;
        EV<<"ack received from actual exec "<< actualExecId <<endl;
        delete msg;
        if(newJobsQueue.getLength()>0){
            tmp = check_and_cast<msg_check *>(newJobsQueue.front());
            balancedJob(tmp);
        }
    }
}

/*
 *STATUSREQUESTHANDLER---Normal Mode
This function handles the status requests and responses:
    ->In case a status request is received from the client:
        ->if the job is inside one of the queues(but not in reRoutedJobs) of the executor:immediately reply to the client
        ->if the job is inside the reRoutedJobs:forward the request to the actual  executor
    ->In case a status request is received from the original executor:
        ->notify the status only to the original executor
    ->In case the original executor receives a status notification from the actual executor for a job:
         ->in case the job hasn't been completed yet simply notify the client
         ->in case of completed jobs:notify the client(*) + notify the actual executor that will
         remove it from his completedJob queue
    ->(*)Then the client will acknowledge the status message to the executor that will remove the job from the his reRoutedJobs
 */

void Executor::statusRequestHandler(msg_check *msg){
    msg_check *tmp, *msgEnded;
    cObject *obj;
    int portId;
    const char *jobId;
    jobId = msg->getName();
    if(msg->getAck()==true){
      EV << "ACK received for "<<jobId<<endl;
      if(msg->getReRouted()==true){
         if(msg->getOriginalExecId() == myId){
             if(msg->getIsEnded()){
                 if(msg->getRelativeJobId() <= nNewJobArrived){
                     obj = reRoutedJobs.remove(jobId);
                     if(obj!=nullptr){
                         tmp = check_and_cast<msg_check *>(obj);
                         EV << "Erasing from the reRouted jobs the job: "<<jobId<<endl;
                         tmp->setReRoutedJobQueue(true);
                         send(tmp,"backup_send$o");
                     }
                     portId = msg->getActualExecId();
                     tmp = msg->dup();
                     tmp->setReRouted(false);
                     send(tmp,"load_send",portId);
                 }else{
                     EV << "FATAL ERROR: Erasing in executor from the re-routed job queue: "<<jobId<<endl;
                 }
             }
             portId = msg->getClientId();
             tmp = msg->dup();
             tmp->setReRouted(false);
             tmp->setActualExecId(tmp->getOriginalExecId());
             send(tmp,"exec$o",portId);
         }
      }else{//because the client will reply to the status reply
          if(msg->getIsEnded()){
                   obj = completedJob.remove(jobId);
                   if (obj!=nullptr){
                       tmp = check_and_cast<msg_check *>(obj);
                       EV << "Erasing in executor from the completed job queue: "<<jobId<<endl;
                       tmp->setCompletedQueue(true);
                       send(tmp,"backup_send$o");//notify erase in completed job queue to the storage
                   }
                   else{
                       EV << "FATAL ERROR: Erasing in executor from the completed job queue: "<<jobId<<endl;
                   }
          }
      }
    }
    else{
        obj = completedJob.get(jobId);
        if(msg->getReRouted()==true){ //the actual exec has received the forwarded status request from the original exec
            if(obj!=nullptr){
                tmp = check_and_cast<msg_check *>(obj);
                tmp = tmp->dup();
                tmp->setStatusRequest(true);
                tmp->setAck(true);
                tmp->setIsEnded(true);
            }else{
                  tmp = msg->dup();
                  tmp->setIsEnded(false);
                  tmp->setAck(true);
            }
            EV << "Sending the status to original exec: "<<jobId<<endl;
            portId = tmp->getOriginalExecId();
            send(tmp,"load_send",portId);
        }else{
            if(obj!=nullptr){
                tmp = check_and_cast<msg_check *>(obj);
                tmp = tmp->dup();
                tmp->setStatusRequest(true);
                tmp->setAck(true);
                tmp->setIsEnded(true);
                EV<<"Found JobId in my completed jobs(original): "<<jobId<<endl;
                portId=tmp->getClientId();
                tmp->setActualExecId(tmp->getOriginalExecId());
                send(tmp,"exec$o",portId);
            }else{
                obj = reRoutedJobs.get(jobId);
                if(obj!=nullptr){
                    tmp = check_and_cast<msg_check *>(obj);
                    tmp = tmp->dup();
                    tmp->setStatusRequest(true);
                    tmp->setAck(false);
                    tmp->setReRouted(true);
                    tmp->setIsEnded(false);
                    portId=tmp->getActualExecId();
                    send(tmp,"load_send",portId);
                    EV << "Asking the status to actual exec: "<<tmp->getOriginalExecId() <<"-"<<tmp->getRelativeJobId()
                            <<"to "<<tmp->getActualExecId()<<endl;
                }else{
                    tmp = msg->dup();
                    tmp->setStatusRequest(true);
                    tmp->setAck(true);
                    tmp->setIsEnded(false);
                    portId=tmp->getClientId();
                    tmp->setActualExecId(tmp->getOriginalExecId());
                    send(tmp,"exec$o",portId);
                }
            }
        }
    }
    delete msg;
}

/*
 * NEWJOBHANDLER---Normal Mode
Every time a new job is received from a client:
    ->Assign a jobId as #executorID-#numberOfNewJobsArrivedToThisExecutorUpToNow and notify it to the client
    ->If jobQueue is empty immediately process the job
    ->Otherwise
        ->Put the job inside the newJobsQueue and start the probing process(if possible) if skipLoad=1(***)
        ->Put the job inside jobQueue even if it isn't empty that is don't do the load balancing process for that job if skipLoad!=1(***)

(***)
skipLoad is a variable derived from the granularity parameter that specifies the number of new jobs that must skip the load balancing process
(even if they should theoretically undergo it).For example granularity=2 means that 1 out of 2 incoming new jobs must undergo load balancing;
granularity=1 means that any incoming new job undergoes the process.
Thus the greater is the value of the granularity the lower is the computation cost for the load balancing. Of course this result in a less
efficient share of the load. So there is a tradeoff amid the complexity of the load balancing process and the efficiency of the sharing of
the load.
Another parameter for the load balancing complexity is the probeResponse(see probeHandler())
(***)
*/

void Executor::newJobHandler(msg_check *msg){
    msg_check *msgSend;
    const char *id;
    std::string jobId;
    int machine, clientId;
    simtime_t timeoutJobComplexity;
    nNewJobArrived++;
    //Create the JobId
    machine=msg->getOriginalExecId();
    jobId.append(std::to_string(machine));
    jobId.append("-");
    jobId.append(std::to_string(nNewJobArrived));
    id=jobId.c_str();
    //save the message to the stable storage
    msg->setRelativeJobId(nNewJobArrived);

    //Reply to the client
    clientId=msg->getClientId();
    msg->setName(id);
    msgSend=msg->dup();
    msgSend->setAck(true);
    send(msgSend,"exec$o",clientId);
    EV<<"First time the packet is in the cluster:define his Job Id"<<id<<endl;
    msg->setNewJob(false);
    if(jobQueue.isEmpty()){
        jobQueue.insert(msg->dup());
        timeoutJobComplexity = msg->getJobComplexity();
        msgSend = msg;
        msgSend->setJobQueue(true);
        send(msgSend,"backup_send$o");
        EV<<"New message arrived with idle serve: execute it immediately. Job Id "<<id<<endl;
        scheduleAt(simTime()+timeoutJobComplexity, timeoutJobComputation);
    }else{
        if(skipLoad==1){
            msgSend=msg->dup();
            msgSend->setNewJobsQueue(true);
            send(msgSend,"backup_send$o");//send a copy of backup to the storage to cope with possible failure
            EV<<"Perform load this time; granularity: "<<granularity<<endl;
            newJobsQueue.insert(msg);
            skipLoad=granularity;
            balancedJob(msg);
        }
        else{
            skipLoad--;
            jobQueue.insert(msg->dup());
            EV<<"partial "<<skipLoad<<" versus granularity "<<granularity<<endl;
            msgSend=msg;
            msgSend->setJobQueue(true);
            send(msgSend,"backup_send$o");
        }
    }
}

/*
 *TIMEOUTLOADBALANCINGHANDLER---Normal Mode
After the expiration of the timeoutLoad the executor decides whether to perform load balancing or not according to the set of responses
received from the others executors.The actual executor will be the one with the lower jobQueue:
    ->In case no executor can be found:don't do load balancing + move the job into the jobQueue
    ->In case the executor is found: send the job to that executor + start a timeout.
    The timeout is stopped in case the actual executor replies to the original executor(reRoutedHandler()).
    Otherwise a timeout expiration means that the actual executor is now unavailable and thus the same job must undergo again the
    load balancing process(selfMessage()->timeoutReRouted).
*/

void Executor::timeoutLoadBalancingHandler(){
    int i;
    int actualExec = -1;
    msg_check *tmp,*msgSend;
    bool processing = true;
    simtime_t timeoutJobComplexity;
    int minLength = jobQueue.getLength();
    if(balanceResponses.getLength()>0){
        tmp = check_and_cast<msg_check *>(balanceResponses.front());
        actualExec = tmp->getOriginalExecId();
        while(!balanceResponses.isEmpty()){
          tmp = check_and_cast<msg_check *>(balanceResponses.pop());
          if(tmp->getQueueLength()<minLength){
              actualExec=tmp->getActualExecId();
              processing = false;
              minLength = tmp->getQueueLength();
          }
          delete tmp;
        }
    }
    tmp = check_and_cast<msg_check *>(newJobsQueue.front());
    if(processing){
        EV<<"NO one has a better queue than me "<<jobQueue.getLength()<<endl;
        bubble("No load balancing");
        tmp = check_and_cast<msg_check *>(newJobsQueue.pop());
        msgSend=tmp->dup();
        msgSend->setNewJobsQueue(true);
        send(msgSend,"backup_send$o");

        msgSend=tmp->dup();
        msgSend->setJobQueue(true);
        send(msgSend,"backup_send$o");
        if(!timeoutJobComputation->isScheduled()){
            timeoutJobComplexity = tmp->getJobComplexity();
            EV<<"load balancing is useless and i am idle indeed jobQueue length "<<jobQueue.getLength()<<endl;
            scheduleAt(simTime()+timeoutJobComplexity, timeoutJobComputation);
        }
        jobQueue.insert(tmp);
        probingMode = false;
        if(newJobsQueue.getLength()>0){
            tmp = check_and_cast<msg_check *>(newJobsQueue.front());
            balancedJob(tmp);
        }
    }
    else{
        tmp->setStatusRequest(false);
        tmp->setProbing(false);
        tmp->setAck(false);
        tmp->setReRouted(true);
        tmp->setQueueLength(-1);
        tmp->setActualExecId(actualExec);
        bubble("Load balancing");
        EV<<"Send to the machine "<<actualExec<<" that has a lower queue the "<<tmp->getRelativeJobId()<<endl;
        failureEvent(probCrashDuringExecution);
        if(failure){
            EV<<"crash in the middle of sending load balancing:I will keep it myself "<<endl;
            return;
        }
        send(tmp->dup(),"load_send", actualExec);
        scheduleAt(simTime()+timeoutLoad, timeoutReRouted);
    }
    EV<<"Final executor "<<actualExec<<endl;
}

/*
 *TIMEOUTJOBEXECUTIONHANDLER---Normal Mode
When the computation of a job ends:
    ->Move the job from jobQueue into completedJob notifying the storage
    ->Then the executor decides which packet it should process next:
        ->If jobQueue and is empty no computation is performed:the executor goes idle until a new packet comes
        ->If jobQueue is not empty:take the first job in the queue(FIFO approach) and execute it
*/

void Executor::timeoutJobExecutionHandler(){
    const char *jobId;
    int clientId;
    simtime_t timeoutJobComplexity;
    msg_check *msgServiced,*msgSend;

    // Retrieve the source_id of the message that just finished service
    msgServiced = check_and_cast<msg_check *>(jobQueue.pop());
    jobId = msgServiced->getName();
    clientId = msgServiced->getClientId();
    EV<<"Completed job: "<<jobId<<" creating by the Client ID "<<clientId<<endl;
    jobCompleted++;

    msgSend = msgServiced->dup();
    msgSend->setJobQueue(true);
    send(msgSend,"backup_send$o");

    msgServiced->setEndingTime(simTime());
    msgSend = msgServiced->dup();
    msgSend->setCompletedQueue(true);
    send(msgSend,"backup_send$o");

    completedJob.add(msgServiced);

    if(jobQueue.isEmpty())
      EV<<"Empty queue, the machine "<<myId<<" goes IDLE"<<endl;
    else{
         msgServiced = check_and_cast<msg_check *>(jobQueue.front());
         jobId = msgServiced->getName();
         clientId = msgServiced->getClientId();
         timeoutJobComplexity = msgServiced->getJobComplexity();
         scheduleAt(simTime()+timeoutJobComplexity, timeoutJobComputation);
         EV<<"Starting job"<<jobId<<" coming from Client ID "<<clientId<<endl;
    }
}

/*
 * SELFMESSAGE
Handles self messages:
    ->timeoutReRouted:during the load balancing process the executor to which the job has been sent is unavailable.
    The same job must undergo again the load balancing process
    ->timeoutLoadBalancing:timeoutLoad started when an executor queries the other executors in order to know their jobQueue length(and thus
       eventually perform load balancing).The various responses arrived up to now are checked in order to understand whether load balancing
       should be performed or not(timeoutLoadBalancingHandler())
    ->timeoutJobComputation:The executor finished processing a packet(timeoutJobExecutionHandler())
 */

void Executor::selfMessage(msg_check *msg){
    msg_check *msgServiced;

    if(msg==timeoutReRouted){
       if(newJobsQueue.getLength()>0){
           EV<<"The load balancing receiver is down;load balancing non performed correctly"<<endl;
           msgServiced = check_and_cast<msg_check *>(newJobsQueue.front());
           probingMode = false;
           balancedJob(msgServiced);
       }
    }
    else
       if(msg==timeoutLoadBalancing){
          timeoutLoadBalancingHandler();
       }else

       /* SELF-MESSAGE HAS ARRIVED


       */
           if (msg == timeoutJobComputation){
              timeoutJobExecutionHandler();
           }
}

/*
 * FINISH
At the end of the simulation print the length of all the four maps
*/

void Executor::finish()
{
    EV<<"completed jobs "<<jobCompleted<<endl;
}
