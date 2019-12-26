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
    //msg_check *timeoutEndOfReRoutedExecution;

    simtime_t timeoutLoad;
    simtime_t timeoutFailure;
    simtime_t timeoutEndActual;

    int E,N,nArrived,myId;
    double probEvent,probCrashDuringExecution;
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
    void statusRequestHandler(msg_check *msg);
    void reRoutedHandler(msg_check *msg);
    void timeoutLoadBalancingHandler();
    void timeoutJobExecutionHandler();
    void failureEvent(double probEvent);
    void rePopulateQueues(msg_check *msg);
    void restartNormalMode();
    void checkDuplicate(msg_check *msg);
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
    //timeoutEndOfReRoutedExecution = nullptr;
}

Executor::~Executor()
{
    cancelAndDelete(timeoutLoadBalancing);
    cancelAndDelete(timeoutJobComputation);
    cancelAndDelete(timeoutReRouted);
    cancelAndDelete(timeoutFailureEnd);
    //cancelAndDelete(timeoutEndOfReRoutedExecution);
}

void Executor::initialize() {
    probingMode = false;
    N= par("N");
    E = par("E"); //non volatile parameters --once defined they never change
    myId=getId()-2-N;
    nArrived=0;

    probEvent=par("probEvent");
    probCrashDuringExecution=par("probCrashDuringExecution");
    failure=false;


    timeoutJobComputation = new msg_check("Job completed");
    timeoutLoadBalancing = new msg_check("timeoutLoadBalancing");
    timeoutReRouted = new msg_check("timeoutReRouted");
    timeoutFailureEnd = new msg_check("timeoutFailureEnd");
    //timeoutEndOfReRoutedExecution = new msg_check("timeout actual exec notifies end of execution to original exec");

    timeoutLoad=0.1;
    timeoutFailure=0.1;
    timeoutEndActual=0.2;
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


   //failureEvent(probEvent);
   if(failure){
       if(msg==timeoutFailureEnd) {
            msgSend = new msg_check("Failure end");
            msgSend->setReBoot(true);
            msgSend->setActualExecId(myId);
            msgSend->setOriginalExecId(myId);
            EV<<"Reboot phase starts"<<endl<<"....."<<endl;
            send(msgSend,"backup_send$o");
            bubble("Reboot mode");

       }
       else if(!msg->isSelfMessage() && msg->getReBoot()==true){
                rePopulateQueues(msg);
                //delete(msg);
             }
           else{
               EV<<"The executor is reboot mode and this is not a msg coming from the backup:ignore it"<<endl;
               return;
           }
   }
   else{
         //checkDuplicate(msg,&completedJob);
         if(msg->isSelfMessage()){
               selfMessage(msg);
         }else{
             if(msg->getNewJob()==true){
                           newJob(msg);
                       }else if(msg->getProbing()==true){
                                 probeHandler(msg);
                             }else if(msg->getStatusRequest()==true){
                                      statusRequestHandler(msg);
                                   }else if(msg->getReRouted()==true){
                                             reRoutedHandler(msg);
                                         }

             delete msg;
         }
   }
}

void Executor::checkDuplicate(msg_check *msg){
    std::map<std::string, msg_check *>::iterator search;

    msg_check *tmp;
    bool isInJobQueue=false,isInNewJobQueue=false;
    int isInReRoutedJobQueue=-1;
    const char *jobId;

    isInJobQueue=jobQueue.contains(msg);
    isInNewJobQueue=newJobsQueue.contains(msg);
    isInReRoutedJobQueue=reRoutedJobs.find(msg); //index of first match;-1 if not found
    EV<<"bool job queue "<<isInJobQueue<<" bool new job queue "<<isInNewJobQueue<<" bool rerouted job queue "<<isInReRoutedJobQueue<<endl;
    //EV<<"Show me NEW "<<newJobsQueue.getLength();
    //EV<<"Show me REROURTED "<<reRoutedJobs.size();
    //EV<<"Show me ENDED "<<completedJob->size()<<endl;
    if(msg->getIsEnded())
    {
        /*jobId = msg->getName();
        search=completedJob->find(jobId);
        if (search != completedJob->end()){  //a key is found(the msg has already been inserted in that map)
            delete search->second;
            completedJob->erase(jobId);
            EV<<"Erase in exec"<<jobId<<" from queue"<<endl;
            msg->setCompletedQueue(true);
            send(msg->dup(),"backup_send$o");
        }
        else*/
            EV<<"New First time the packet arrives in this executor:process it"<<endl;
    }

    if(isInJobQueue)
    {
        tmp=check_and_cast<msg_check *>(jobQueue.remove(msg));
        send(tmp,"backup_send$o");
    }
    if(isInNewJobQueue)
    {

        tmp=check_and_cast<msg_check *>(newJobsQueue.remove(msg));
        send(tmp,"backup_send$o");
    }
    if(isInReRoutedJobQueue!=-1)
    {
        msg->setReRoutedJobQueue(true);
        send(msg->dup(),"backup_send$o");
        reRoutedJobs.remove(msg);
    }

    //EV<<"Show AFTER NEW "<<newJobsQueue.getLength();
    //EV<<"Show AFTER REROURTED "<<reRoutedJobs.size();
    //EV<<"Show AFTER ENDED "<<completedJob->size()<<endl;

}


void Executor::failureEvent(double prob){
    return ;
    std::map<std::string, msg_check *>::iterator search;
 if(!failure)
 {
   if(uniform(0,1)<prob){
     failure=true;
     //outGoingPacket.clear();
     newJobsQueue.clear();
     jobQueue.clear();
     balanceResponses.clear();
     reRoutedJobs.clear();
     completedJob.clear();
     probingMode = false;//if probingMode = true means that i am waiting for responses by other executors but if I fail I stop also the timeout
     //thus I will never be able in the future to do probing becauset probingMode=true forever

     cancelEvent(timeoutReRouted);
     cancelEvent(timeoutJobComputation);
     cancelEvent(timeoutLoadBalancing);
     //cancelEvent(timeoutEndOfReRoutedExecution);
     cancelEvent(timeoutFailureEnd);
     scheduleAt(simTime()+timeoutFailure, timeoutFailureEnd);
     EV<<"A failure has happened:start failure phase "<<"......"<<endl;
     bubble("Failure");
   }
 }


}

void Executor::rePopulateQueues(msg_check *msg){
    const char *jobId;
    if(msg->getBackupComplete()==false){
        msg->setReBoot(false);
        if(msg->getNewJobsQueue()==true){
            newJobsQueue.insert(msg->dup());
            EV<<"BACKUP In the new_job_queue"<<endl;
        }else if(msg->getJobQueue()==true){
                  if(msg->getReRouted()==true){
                      jobQueue.insert(msg->dup());
                      EV<<"BACKUP in new because REROUTED"<<endl;
                  }else{
                      newJobsQueue.insert(msg->dup());
                      EV<<"BACKUP In the new job because has not been re routed"<<endl;
                  }
              }else if(msg->getReRoutedJobQueue()==true){
                      reRoutedJobs.add(msg->dup());
                      EV<<"BACKUP In the rerouted_queue"<<endl;
                     }else if(msg->getCompletedQueue()==true){
                                  jobId = msg->getName();
                                  completedJob.add(msg->dup());
                                  EV<<"BACKUP in completed jobs"<<endl;

                           }
        delete msg;
    }
    else{
        EV<<"The backup process is over, executor is now in normal execution mode"<<endl<<".........."<<endl;
        bubble("Normal mode");
        failure=false;//until I have recovered all the backup message I will still ignore all the other messages
        restartNormalMode();
    }
}

void Executor::restartNormalMode(){
    msg_check *msgServiced,*tmp;
    simtime_t timeoutJobComplexity;
    int jobId,portId;
    EV<<"After recover Show me JOB "<<jobQueue.getLength()<<endl;
    EV<<"Show me NEW "<<newJobsQueue.getLength()<<endl;
    EV<<"Show me REROURTED "<<reRoutedJobs.size()<<endl;
    EV<<"Show me ENDED "<<completedJob.size()<<endl;
    if(jobQueue.isEmpty()){
        if(newJobsQueue.isEmpty())
            EV<<"JOB and NEWJOB queue are idle, the machine goes IDLE"<<endl;
        else if(newJobsQueue.getLength()>0){
                tmp = check_and_cast<msg_check *>(newJobsQueue.front());
                balancedJob(tmp);
            }


    }
    else{
         msgServiced = check_and_cast<msg_check *>(jobQueue.front());
         jobId = msgServiced->getRelativeJobId();
         portId = msgServiced->getClientId()-1;
         if(!timeoutJobComputation->isScheduled()){
             timeoutJobComplexity = msgServiced->getJobComplexity();
             scheduleAt(simTime()+timeoutJobComplexity, timeoutJobComputation);
             EV<<"Starting service of "<<jobId<<" coming from user ID "<<portId<<" from the queue of the machine "<<msgServiced->getOriginalExecId()<<endl;
         }
    }

}

void Executor::balancedJob(msg_check *msg){
    msg_check *msgSend;
    int i;
    int src_id;
    src_id=msg->getClientId();
    EV<<"msg ID "<<msg->getRelativeJobId()<<" coming from user ID "<<src_id-1<<" trying to distribute"<<endl;
    if(!probingMode){  //==-1 means that the coming msg has been forwarded by another machine after load balancing
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
                 EV<<"crash when sending load balancing requests "<<endl;
                 return;
             }
             send(msgSend,"load_send",i);
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
    if(msg->get()==false)  {//sono stato interrogato per sapere lo stato della mia coda---setAck=true---salvo da quaclhe parte il minimo poi decido se reinstradare poi i da E diventa 0 cosi posso rifare questo giochino
        //NO IS WRONG THIS THINKING+1 is needed in order to avoid ping pong:exe machine 5 has queue length =3 while machine 1 has length  =2... without +1 5-2 and 1-3 but if a
        //an arrival comes in 3
        msgSend = msg->dup();
        msgSend->setAck(true);
        msgSend->setStatusRequest(false);  //set message priority
        msgSend->setProbing(true);
        msgSend->setReRouted(false);
        EV<<"The machine "<<msgSend->getOriginalExecId()<<" has a queue "<<msgSend->getQueueLength()<<" than the one in this machine which has queue length="<<jobQueue.getLength()<<endl;
        msgSend->setQueueLength(jobQueue.getLength());
        failureEvent(probCrashDuringExecution);
        if(failure){
            EV<<"crash in the middle of sending probing response "<<endl;
            return;
        }
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
    int actualExecId;
    simtime_t timeoutJobComplexity;
    if(msg->getAck()==false)  {//the actual exec has received the packet;notifies the original exec
        msgSend = msg->dup();
        msgSend->setAck(true);
        msgSend->setStatusRequest(false);  //set message priority
        msgSend->setProbing(false);
        msgSend->setReRouted(true);
        failureEvent(probCrashDuringExecution);
        if(failure){
            EV<<"crash in the middle of notify the packet received due to load balancing;TIMEOUT WILL EXPIRE:RESTART LOAD BALANCING "<<endl;
            return;
        }
        send(msgSend,"load_send",msgSend->getOriginalExecId());

        jobQueue.insert(msg->dup());
        //EV<<"Insert jobQueue in storage"<<endl;
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
        //EV<<"Send to the backup newjobqueue pop event "<<endl;
        send(msgSend,"backup_send$o");
        actualExecId = msg->getActualExecId();
        tmp->setActualExecId(actualExecId);

        tmp->setReRoutedJobQueue(true);
        msgSend=tmp->dup();
        //EV<<"Send to the backup reroutedjobqueue add event "<<endl;
        send(msgSend,"backup_send$o");
        reRoutedJobs.add(tmp);
        if(newJobsQueue.getLength()>0){
            tmp = check_and_cast<msg_check *>(newJobsQueue.front());
            balancedJob(tmp);
        }
        //TO  Check: filippo
        cancelEvent(timeoutReRouted);
        EV<<"ack received from actual exec "<< actualExecId <<endl;
    }
}

void Executor::statusRequestHandler(msg_check *msg){
    msg_check *tmp, *msgEnded;
    cObject *obj;
    int portId;
    const char *jobId;
    jobId = msg->getName();
    EV<<"mi sta chiedendo "<<jobId<<endl;
    if(msg->getAck()==true){
      EV << "ACK received for "<<jobId<<endl;
      if(msg->getReRouted()==true){
         if(msg->getOriginalExecId()==myId){
             if(msg->getIsEnded()){
                 obj = reRoutedJobs.remove(jobId);
                 if(obj!=nullptr){
                     tmp = check_and_cast<msg_check *>(obj);
                     EV << "Erasing from the rerouted jobs the job: "<<msg->getOriginalExecId() <<"-"<<msg->getRelativeJobId()<<endl;
                     send(tmp,"backup_send$o");//notify erase in rerouted job queue to the storage
                 }
                 portId = msg->getActualExecId();
                 tmp = msg->dup();
                 tmp->setReRouted(false);
                 //EV<<"send remove rerouted after STATUS REQUEST"<<endl;

                 send(tmp,"load_send",portId);
             }
             portId = msg->getClientId()-1;
             tmp = msg->dup();
             tmp->setReRouted(false);
             send(tmp,"exec$o",portId);
         }
      }else{//because the client will reply to the status reply
          if(msg->getIsEnded()){
                   obj = completedJob.remove(jobId);
                   if (obj!=nullptr){
                       tmp = check_and_cast<msg_check *>(obj);
                       EV << "Erasing in executor from the completed job queue: "<<jobId<<endl;
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
        if(msg->getReRouted()==true){ //the actual exec has received the forwarded status request froom the original exec
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
                portId=tmp->getClientId()-1;
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
                }else{//original exec non ha fatto rerouted;il pkt si trova o in jobqueue o in newjobqueue
                    tmp = msg->dup();
                    tmp->setStatusRequest(true);
                    tmp->setAck(true);
                    tmp->setIsEnded(false);
                    portId=tmp->getClientId()-1;
                    send(tmp,"exec$o",portId);
                }
            }
        }
    }
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

    //Reply to the client
    port_id=msg->getClientId()-1;
    msg->setName(id);
    msgSend=msg->dup();
    msgSend->setAck(true);
    send(msgSend,"exec$o",port_id);
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
        //EV<<"IInsert job queue in storage"<<endl;
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
        msgSend=msg->dup();
        msgSend->setNewJobsQueue(true);
       // EV<<"Send to the backup info about original "<<msgSend->getOriginalExecId()<<" and actual "<<msgSend->getActualExecId()<<" of the "<<id<<endl;
        send(msgSend,"backup_send$o");//send a copy of backup to the storage to cope with possible failure
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
        EV<<"NO one has a better queue than me "<<jobQueue.getLength()<<endl;
        bubble("No load balancing");
        tmp = check_and_cast<msg_check *>(newJobsQueue.pop());
        msgSend=tmp->dup();
            //msgSend->setActualExecId(msg->getActualExecId());
        msgSend->setNewJobsQueue(true);

        //EV<<"Send to the backup newjobqueue pop event "<<endl;
        //EV<<"NEW QUEUE NO LOAD "<<newJobsQueue.getLength()<<endl;
        send(msgSend,"backup_send$o");//send a copy of backup to the storage to cope with possible failure

        msgSend=tmp->dup();
        //EV<<"IIInsert job queue in storage giben that no load balancing is performed"<<endl;
        msgSend->setJobQueue(true);
        send(msgSend,"backup_send$o");
        //chi lo dice che sono idle?nisuno
        if(!timeoutJobComputation->isScheduled()){
            timeoutJobComplexity = tmp->getJobComplexity();
            EV<<"load balancing is useless and i am idle indeeed jobQueue length "<<jobQueue.getLength()<<endl;
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
        send(tmp->dup(),"load_send", actualExec); //tmp rimane dentro i miei newjobs???Non va nei rerouted?si lo fa alla ricezione dell ack
        scheduleAt(simTime()+timeoutLoad, timeoutReRouted);
    }
    EV<<"Final executor "<<actualExec<<endl;
    EV<<"jobQueue after timeout load balancing "<<jobQueue.getLength()<<endl;
}

void Executor::timeoutJobExecutionHandler(){
    std::string jobId;
    int portId;
    simtime_t timeoutJobComplexity;
    msg_check *msgServiced,*msgSend;
    // Get the source_id of the message that just finished service

    msgServiced = check_and_cast<msg_check *>(jobQueue.pop());
    jobId.append(std::to_string(msgServiced->getOriginalExecId()));
    jobId.append("-");
    jobId.append(std::to_string(msgServiced->getRelativeJobId()));
    portId = msgServiced->getClientId()-1;  //Source id-1=portId
    EV<<"Completed job: "<<jobId<<" creating by the user ID "<<portId<<endl;
    msgServiced->setCompletedQueue(true);
    /*if(msgServiced->getReRouted()==true){
         msgSend= msgServiced->dup();
         send(msgSend,"load_send",msgSend->getOriginalExecId());
         scheduleAt(simTime()+timeoutEndActual,timeoutEndOfReRoutedExecution);
     }
*/
    //EV<<"Notify end of execution to storage"<<endl;
    msgSend= msgServiced->dup();
    msgSend->setJobQueue(true);//dovrebbe essere implicito
    send(msgSend,"backup_send$o");


    msgSend= msgServiced->dup();

    send(msgSend,"backup_send$o");

    completedJob.add(msgServiced);//also in original exec add it




    /*
    B)Then the executor decides which packet it should process next:
      ->If his queue is empty no computation is performed:the executor goes idle until a new packet comes
      ->If the queue is not empty:takes the first packet in the queue(FIFO approach) and starts to execute it
     */
    if(jobQueue.isEmpty())
      EV<<"Empty queue, the machine "<<myId<<" goes IDLE"<<endl;
    else{
         msgServiced = check_and_cast<msg_check *>(jobQueue.front());
         jobId.append(std::to_string(msgServiced->getOriginalExecId()));
         jobId.append("-");
         jobId.append(std::to_string(msgServiced->getRelativeJobId()));
         portId = msgServiced->getClientId()-1;
         timeoutJobComplexity = msgServiced->getJobComplexity();
         scheduleAt(simTime()+timeoutJobComplexity, timeoutJobComputation);
         EV<<"Starting job"<<jobId<<" coming from user ID "<<portId<<endl;
    }

}


//called in the self message function

void Executor::selfMessage(msg_check *msg){
    int portId;
    msg_check *msgServiced;

    if(msg==timeoutReRouted){
       if(newJobsQueue.getLength()>0){
           EV<<"The load balancing receiver is down;load balancing non performed correctly"<<endl;
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
           /*else
               if(msg==timeoutEndOfReRoutedExecution){
                   EV<<"Original exec is unavailable;I won't retry but the original exec once is active can ask me "<<endl;

               }*/
}
