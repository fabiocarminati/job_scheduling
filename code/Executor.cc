#include <string.h>
#include <omnetpp.h>
#include <msg_check_m.h>  //fare parsing
#include <map>
#define CLUSTER_SIZE 1000
#define QUEUE_SIZE 10000000
using namespace omnetpp;
class Executor : public cSimpleModule {
private:

    msg_check *timeoutJobComputation;
    msg_check *timeoutLoadBalancing;
    msg_check *timeoutReRouted;
    msg_check *timeoutFailureEnd;

    simtime_t timeoutLoad;
    simtime_t timeoutOriginal;
    simtime_t defServiceTime;
    simtime_t expPar;
    simtime_t timeoutFailure;
    int E,N,nArrived,myId;
    double probEvent;
    bool probingMode,failure;

    cArray completedJob;
    cQueue newJobsQueue;
    cQueue jobQueue;
    cQueue balanceResponses;
    cArray reRoutedJobs;

    void selfMessage(msg_check *msg);
    void newJob(msg_check *msg);
    void balancedJob(msg_check *msg);
    void probeHandler(msg_check *msg);
    void ReRoutedJobEnd(msg_check *msg);
    void reRoutedHandler(msg_check *msg);
    void timeoutLoadBalancingHandler();
    void timeoutJobExecutionHandler();
    void failureEvent(msg_check *msg,double probEvent);
    void rePopulateQueues(msg_check *msg);
protected:
    virtual void initialize();// override;
    virtual void handleMessage(cMessage *cmsg);// override;

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

void Executor::initialize() {
    probingMode = false;
    myId=getId()-2-N;
    nArrived=0;
    E = par("E"); //non volatile parameters --once defined they never change
    N= par("N");
    probEvent=par("probEvent");
    failure=false;
 // The exponential value is computed in the handleMessage; Set the service time as exponential
    expPar=exponential(par("defServiceTime").doubleValue());
    //forward_backup = new msg_check("sending the message for backup");
    //forward_id = new msg_check("notify the user about job id");
    //probe = new msg_check("Ask the load");
    //reply = new msg_check("Reply with the queue value");
    timeoutJobComputation = new msg_check("Job completed");
    timeoutLoadBalancing = new msg_check("timeoutLoadBalancing");
    timeoutReRouted = new msg_check("timeoutReRouted");
    timeoutFailureEnd = new msg_check("timeoutFailureEnd");

    timeoutLoad=0.1;
    timeoutOriginal=0.2;
    timeoutFailure=0.1;
}
void Executor::handleMessage(cMessage *cmsg) {

   int i;
   // Casting from cMessage to msg_check
   msg_check *msg = check_and_cast<msg_check *>(cmsg);
   msg_check *msgSend;
   std::string jobb_id;
   std::string received;
   const char * id,* job_id;
   int machine;

   failureEvent(msg,probEvent);
   if(failure){
       if(msg==timeoutFailureEnd) {
            msgSend = new msg_check("Failure end");
            msgSend->setReBoot(true); //problema:poi rientro in selfEvents
            //msgSend->setHasEnded(false);
            //msgSend->setProbing(false);
            //msgSend->setJobComplexity(0.2); //initialize to the partial elaboration done of a packet; will be useful for server utilization signal and preemptive resume
            //msgSend->setRelativeJobId(0); //will be useful for computing the per class extended service time
           // msgSend->setClientId(-1);  //initialize to 0 the time when a packet goes for he first time to service(useful for extended per class service time)
            msgSend->setActualExecId(myId);
            msgSend->setOriginalExecId(myId);
           // msgSend->setQueueLength(0);
            /*msgSend->setReRouted(false);
            msgSend->setAck(false);
            msgSend->setNewJob(false);
            msgSend->setReBoot(false);
            msgSend->setJobQueue(false);
            msgSend->setNewJobsQueue(false);
            msgSend->setReRoutedJobQueue(false);*/
            EV<<"Reboot of the executor starts"<<endl;
            send(msgSend,"backup_send$o");
            failure=false;
       }
       else
         //delete msg;
         return;
   }
   else if(msg->isSelfMessage()){
               selfMessage(msg);
         }else if(msg->getReBoot()==true){
                     rePopulateQueues(msg);
                 }else if(msg->getNewJob()==true){
                           newJob(msg);
                       }else if(msg->getProbing()==true){
                                 probeHandler(msg);
                             }else if(msg->getHasEnded()==true){
                                       ReRoutedJobEnd(msg);
                                   }else if(msg->getReRouted()==true){
                                             reRoutedHandler(msg);
                                         }

       //delete msg;

}
void Executor::rePopulateQueues(msg_check *msg){
    if(msg->getNewJobsQueue()==true){
        newJobsQueue.insert(msg->dup());
        balancedJob(msg);
    }else if(msg->getJobQueue()==true){
          if(msg->getReRouted()==true)
              jobQueue.insert(msg->dup());
          else{
              newJobsQueue.insert(msg->dup());
              balancedJob(msg);
          }
    }else if(msg->getReRoutedJobQueue()==true){
              reRoutedJobs.add(msg->dup());
    }

}

void Executor::failureEvent(msg_check *msg,double probEvent){
    msg_check *msgSend;
 if(!failure)
 {
   if(uniform(0,1)<probEvent)
     failure=true;
   if(failure){
     //outGoingPacket.clear();
     newJobsQueue.clear();
     jobQueue.clear();
     balanceResponses.clear();
     reRoutedJobs.clear();
     EV<<"ERASE NEW "<<newJobsQueue.getLength()<<" JOB "<<jobQueue.getLength()<<" BALANCED"<<balanceResponses.getLength()<<" RE ROUTED "<<reRoutedJobs.size();
     cancelEvent(timeoutReRouted);
     cancelEvent(timeoutJobComputation);
     cancelEvent(timeoutLoadBalancing);
     //cancelEvent(timeoutFailureEnd);
     scheduleAt(simTime()+timeoutFailure, timeoutFailureEnd);
     EV<<"A failure has happened"<<endl;

   }
 }
 else
         EV<<"Due to Failure message is lost "<<endl;

}

void Executor::balancedJob(msg_check *msg){
    msg_check *msgSend;
    int i;
    int src_id;
    src_id=msg->getClientId();
    EV<<"msg ID "<<msg->getRelativeJobId()<<" coming from user ID "<<src_id-1<<" trying to distribute"<<endl;
    if(!probingMode){  //==-1 means that the coming msg has been forwarded by another machine after load balancing
        msg->setHasEnded(false);
        msg->setProbing(true);
        msg->setAck(false);
        msg->setReRouted(false);
        msg->setQueueLength(jobQueue.getLength());
        for(i=0;i<E;i++){
         if(i!=msg->getOriginalExecId()){
             msgSend = msg->dup();
             msgSend->setActualExecId(i);
             send(msgSend,"load_send",i); //in caso di guasti???
             EV<<"Asking the load to machine "<<msgSend->getActualExecId()<<endl;
             }
         //TO DO:ADD ONE TIMEOUT FOR ALL
        }
        probingMode = true;
        scheduleAt(simTime()+timeoutLoad, timeoutLoadBalancing);
    }
}

void Executor::probeHandler(msg_check *msg){
    msg_check *msgSend;
    if(msg->getAck()==false)  {//sono stato interrogato per sapere lo stato della mia coda---setAck=true---salvo da quaclhe parte il minimo poi decido se reinstradare poi i da E diventa 0 cosi posso rifare questo giochino
        //NO IS WRONG THIS THINKING+1 is needed in order to avoid ping pong:exe machine 5 has queue length =3 while machine 1 has length  =2... without +1 5-2 and 1-3 but if a
        //an arrival comes in 3
        msgSend = msg->dup();
        msgSend->setAck(true);
        msgSend->setHasEnded(false);  //set message priority
        msgSend->setProbing(true);
        msgSend->setReRouted(false);
        EV<<"The machine "<<msgSend->getOriginalExecId()<<" has a queue "<<msgSend->getQueueLength()<<" than the one in this machine which has queue length="<<jobQueue.getLength()<<endl;
        msgSend->setQueueLength(jobQueue.getLength());
        send(msgSend,"load_send",msgSend->getOriginalExecId());
        }
    else{
        /*
         D)The Original exec has received the reply from another executor:
             ->It will extract the QueueLength field of the packet and store it

         * */
        balanceResponses.insert(msg->dup());
        EV<<"store load reply from "<< msg->getActualExecId()<<endl;
    }
}

void Executor::reRoutedHandler(msg_check *msg){
    msg_check *msgSend,*tmp;
    simtime_t timeoutJobComplexity;
    if(msg->getAck()==false)  {//sono stato interrogato per sapere lo stato della mia coda---setAck=true---salvo da quaclhe parte il minimo poi decido se reinstradare poi i da E diventa 0 cosi posso rifare questo giochino
        //NO IS WRONG THIS THINKING+1 is needed in order to avoid ping pong:exe machine 5 has queue length =3 while machine 1 has length  =2... without +1 5-2 and 1-3 but if a
        //an arrival comes in 3
        msgSend = msg->dup();
        msgSend->setAck(true);
        msgSend->setHasEnded(false);  //set message priority
        msgSend->setProbing(false);
        msgSend->setReRouted(true);
        send(msgSend,"load_send",msgSend->getOriginalExecId());

        jobQueue.insert(msg->dup());
        EV<<"Insert jobQueue in storage"<<endl;
        msgSend=msg->dup();
        msgSend->setJobQueue(true);
        send(msgSend,"backup_send$o");

        if(!timeoutJobComputation->isScheduled()){
             timeoutJobComplexity = msgSend->getJobComplexity();
             EV<<"The new executor is idle:it starts executing immediately the packet"<<endl;
             scheduleAt(simTime()+timeoutJobComplexity, timeoutJobComputation);
        }
        EV<<"new job in the queue, actual length: "<< jobQueue.getLength()<<endl;
   }
    else{
        tmp = check_and_cast<msg_check *>(newJobsQueue.pop());
        msgSend=tmp->dup();
        msgSend->setNewJobsQueue(true);
        EV<<"Send to the backup newjobqueue pop event "<<endl;
        send(msgSend,"backup_send$o");


        msgSend=tmp->dup();
        msgSend->setReRoutedJobQueue(true);
        EV<<"Send to the backup reroutedjobqueue add event "<<endl;
        send(msgSend,"backup_send$o");

        reRoutedJobs.add(tmp);
        probingMode = false;
        if(newJobsQueue.getLength()>0){
            tmp = check_and_cast<msg_check *>(newJobsQueue.front());
            balancedJob(tmp);
        }
        cancelEvent(timeoutReRouted);
        EV<<"ack received from actual exec "<< msg->getActualExecId()<<endl;
    }
}

void Executor::ReRoutedJobEnd(msg_check *msg){
    /*When the actual exec receives that ACK:
               ->It will stop the timeout event timeoutReSendOriginal for the ACK
               ->Notify his secure storage to delete the local copy of the previously processed packet
           */
    msg_check *ackToActualExec;
    if(msg->getAck()==true){
      EV << "ACK received from original exec "<<msg->getOriginalExecId() <<" for "<<msg->getRelativeJobId()<<endl;
      ackToActualExec=msg->dup();
      send(ackToActualExec, "backup_send$o");
      //cancelEvent(timeoutReSendOriginal);
    }
    else{
        /*When the original exec receives the msg notifying the end of processing by actual exec:
              ->Sends the ACK to the actual exec
              ->Notifies his secure storage to delete the local copy of the previously forwarded packet to the actual exec
              ->Notifies the client the end of the processing
              */
        EV << "Send the ACK to the actual exec "<<msg->getActualExecId() <<" for "<<msg->getRelativeJobId()<<endl;
        ackToActualExec=msg->dup();
        ackToActualExec->setAck(true);
        send(ackToActualExec, "load_send",ackToActualExec->getActualExecId());
        ackToActualExec=msg->dup();
        send(ackToActualExec, "backup_send$o"); //the original exec notifies his own storage to delete the entry
        ackToActualExec=msg->dup();
        send(ackToActualExec, "exec$o",msg->getClientId()-1);
        //timeout has been removed:to add,remove from rerouted jobs

    }
      //delete the copy in the local storage
}

void Executor::newJob(msg_check *msg){
    msg_check *msgSend;
    const char *id;
    std::string jobId;
    int machine, port_id;
    simtime_t timeoutJobComplexity;
    nArrived++;
    //Create the JobId
    machine=msg->getOriginalExecId();
    //jobId="Job ID: ";
    jobId.append(std::to_string(machine));
    jobId.append("-");
    jobId.append(std::to_string(nArrived));
    id=jobId.c_str();
    //save the message to the stable storage
    msg->setRelativeJobId(nArrived);
    msgSend=msg->dup();
    //msgSend->setActualExecId(msg->getActualExecId());
    msgSend->setNewJobsQueue(true);
    EV<<"Send to the backup info about original "<<msgSend->getOriginalExecId()<<" and actual "<<msgSend->getActualExecId()<<" of the "<<id<<endl;
    EV<<"ADD NEWJOBSQUEUE"<<endl;
    send(msgSend,"backup_send$o");//send a copy of backup to the storage to cope with possible failure

    EV<<"NEW JOB QUEUE LENGTH NEW ARRIVAL "<<newJobsQueue.getLength();
    //Reply to the client
    port_id=msg->getClientId()-1;
    msgSend=msg->dup();
    msgSend->setAck(true);
    send(msgSend->dup(),"exec$o",port_id);
    EV<<"First time the packet is in the cluster:define his "<<id<<endl;
    msg->setNewJob(false);

/*
    if(!jobQueue.getLength()){
        jobQueue.insert(msg->dup());
        EV<<"IInsert job queue in storage"<<endl;
        msgSend=msg->dup();
        msgSend->setJobQueue(true);
        send(msgSend,"backup_send$o");
        */

    if(!jobQueue.getLength()){
        //tmp = check_and_cast<msg_check *>(newJobsQueue.pop());
        jobQueue.insert(msg->dup());
        EV<<"IInsert job queue in storage"<<endl;
        msgSend=msg->dup();
        msgSend->setJobQueue(true);
        send(msgSend,"backup_send$o");
        //delete tmp;

        if(!timeoutJobComputation->isScheduled()){
            timeoutJobComplexity = msg->getJobComplexity();
            EV<<"New message arrived with idle server:execute it immediately:Job Id "<<id<<endl;
            scheduleAt(simTime()+timeoutJobComplexity, timeoutJobComputation);
        }
    }
    else{
        newJobsQueue.insert(msg->dup());
        balancedJob(msg);

    }
}

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
        EV<<"NO one has a better queue than me "<<newJobsQueue.getLength()<<endl;
        tmp = check_and_cast<msg_check *>(newJobsQueue.pop());
        msgSend=tmp->dup();
            //msgSend->setActualExecId(msg->getActualExecId());
        msgSend->setNewJobsQueue(true);

        EV<<"Send to the backup newjobqueue pop event "<<endl;
        EV<<"NEW QUEUE NO LOAD "<<newJobsQueue.getLength()<<endl;
        send(msgSend,"backup_send$o");//send a copy of backup to the storage to cope with possible failure
        msgSend=tmp->dup();
        jobQueue.insert(tmp);
        EV<<"IIInsert job queue in storage giben that no load balancing is performed"<<endl;
        msgSend->setJobQueue(true);
        send(msgSend,"backup_send$o");
        //chi lo dice che sono idle?nisuno
        if(!timeoutJobComputation->isScheduled()){
            timeoutJobComplexity = tmp->getJobComplexity();
            EV<<"load balancing is useless and i am idle indeeed jobQueue length "<<jobQueue.getLength()<<endl;
            scheduleAt(simTime()+timeoutJobComplexity, timeoutJobComputation);
        }
        probingMode = false;
         if(newJobsQueue.getLength()>0){
             tmp = check_and_cast<msg_check *>(newJobsQueue.front());
             balancedJob(tmp);
         }
    }
    else{
        tmp->setHasEnded(false);
        tmp->setProbing(false);
        tmp->setAck(false);
        tmp->setReRouted(true);
        tmp->setQueueLength(-1);
        tmp->setActualExecId(actualExec);

        EV<<"Send to the machine "<<actualExec<<" that has a lower queue the "<<tmp->getRelativeJobId()<<endl;
        send(tmp->dup(),"load_send", actualExec); //tmp rimane dentro i miei newjobs???Non va nei rerouted?si lo fa alla ricezione dell ack
        scheduleAt(simTime()+timeoutLoad, timeoutReRouted);
    }
    EV<<"Final executor "<<actualExec<<endl;
    EV<<"jobQueue after timeout load balancing "<<jobQueue.getLength()<<endl;
}

void Executor::timeoutJobExecutionHandler(){
    int jobId;
    int portId;
    int originalExecutor;
    simtime_t timeoutJobComplexity;
    msg_check *msgServiced,*msgSend;
    // Get the source_id of the message that just finished service

    msgServiced = check_and_cast<msg_check *>(jobQueue.pop());
    originalExecutor = msgServiced->getOriginalExecId();
    jobId = msgServiced->getRelativeJobId();
    portId = msgServiced->getClientId()-1;  //Source id-1=portId
    EV<<"Completed service of "<<originalExecutor<<"with ID: "<<jobId<<" creating by the user ID "<<portId<<endl;
    msgServiced->setHasEnded(true);

    EV<<"Notify end of execution to storage"<<endl;
    msgSend= msgServiced->dup();
    msgSend->setJobQueue(true);
    send(msgSend,"backup_send$o");
    completedJob.add(msgServiced);


    /*
    B)Then the executor decides which packet it should process next:
      ->If his queue is empty no computation is performed:the executor goes idle until a new packet comes
      ->If the queue is not empty:takes the first packet in the queue(FIFO approach) and starts to execute it
     */
    if(jobQueue.isEmpty())
      EV<<"Empty queue, the machine "<<msgServiced->getActualExecId()<<" goes IDLE"<<endl;
    else{
         msgServiced = check_and_cast<msg_check *>(jobQueue.front());
         originalExecutor = msgServiced->getOriginalExecId();
         jobId = msgServiced->getRelativeJobId();
         portId = msgServiced->getClientId()-1;
         timeoutJobComplexity = msgServiced->getJobComplexity();
         scheduleAt(simTime()+timeoutJobComplexity, timeoutJobComputation);
         EV<<"Starting service of "<<jobId<<" coming from user ID "<<portId<<" from the queue of the machine "<<originalExecutor<<endl;
    }

}

//called in the self message function

void Executor::selfMessage(msg_check *msg){
    int portId;
    msg_check *msgServiced;

    if(msg==timeoutReRouted){
       if(newJobsQueue.getLength()>0){
           msgServiced = check_and_cast<msg_check *>(newJobsQueue.front());
           balancedJob(msgServiced);
       }
    }
    else
       /* SELF-MESSAGE HAS ARRIVED
       The timeout(timeoutLoadBalancing) started when an executor queries the other executors in order to know their queue length(and thus
       eventually perform load balancing) has ended:
       1)The responses arrived up to now are checked in order to understand whether load balancing should be performed or not according
        to the different queue length values
       */
       if(msg==timeoutLoadBalancing){
          timeoutLoadBalancingHandler();
       }else

       /* SELF-MESSAGE HAS ARRIVED
        The executor finished serving a packet
        A)The executor can notify the end of the computation to different entities:
           1)If it's the executor that has received a packet due to load balancing:
              ->It will notify the original exec and waiting up to timeoutOriginal for an ACK from that executor.
           2)If the actual and original executor are the same(no load balancing is performed):
              ->It will notify the storage(that will delete the entry in his map function referring to that job id)
              ->It will notify the client the end of the computation.

       */

           if (msg == timeoutJobComputation){
               timeoutJobExecutionHandler();
           }
}
