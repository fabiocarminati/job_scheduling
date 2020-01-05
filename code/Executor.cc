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

    simtime_t channelDelay;
    simtime_t timeoutLoad;
    simtime_t timeoutFailure;
    simtime_t timeoutEndActual;

    int E,C,nArrived,myId,granularity,skipLoad,probeResponse;
    double probEvent,probCrashDuringExecution,jobCompleted;
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
    bool checkDuplicate(msg_check *msg);
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
    C = par("C");
    E = par("E"); //non volatile parameters --once defined they never change
    granularity = par("granularity");
    probeResponse = par("probeResponse");
    skipLoad=granularity;
    channelDelay = par("channelDelay");
    timeoutLoad = par("timeoutLoad")+2*channelDelay; //the channelDelay should be considered twice in the timeouts:one for the send and one for the reply(2 accesses to the channel)
    timeoutFailure = par("timeoutFailure")+2*channelDelay;
    timeoutEndActual = par("timeoutEndActual")+2*channelDelay;
    EV<<"load "<<timeoutLoad<<" failure "<<timeoutFailure<<" end "<<timeoutEndActual<<endl;
    myId=getId()-2-C;
    nArrived=0;
    jobCompleted=0;
    probEvent=par("probEvent");
    probCrashDuringExecution=par("probCrashDuringExecution");
    failure=false;


    timeoutJobComputation = new msg_check("Job completed");
    timeoutLoadBalancing = new msg_check("timeoutLoadBalancing");
    timeoutReRouted = new msg_check("timeoutReRouted");
    timeoutFailureEnd = new msg_check("timeoutFailureEnd");
    //timeoutEndOfReRoutedExecution = new msg_check("timeout actual exec notifies end of execution to original exec");




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


   failureEvent(probEvent);
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
       else if(msg->getReBoot()==true){
                rePopulateQueues(msg);
             }
           else{
               EV<<"The executor isn't in normal mode and this is not a msg coming from the backup:ignore it"<<endl;
               if(!msg->isSelfMessage())
                   delete msg;
           }
   }
   else{
         if(msg->isSelfMessage()){
               selfMessage(msg);
         }else{
             if(msg->getNewJob()==true){
                           newJob(msg);
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

void Executor::failureEvent(double prob){
    if(!failure){
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
    msg_check *msgSend;
    int portId;
    if(msg->getBackupComplete()==false){
        msg->setReBoot(false);
        if(msg->getNewJobsQueue()==true){
            msg->setNewJobsQueue(false);
            newJobsQueue.insert(msg);
            EV<<"BACKUP In the new_job_queue"<<endl;
        }else if(msg->getJobQueue()==true){
                  msg->setJobQueue(false);
                  if(msg->getReRouted()==true){
                      jobQueue.insert(msg);//reRouted=true lo lascio
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

        EV<<"check completed cArray"<<endl;

        for (cArray::Iterator it(completedJob); !it.end(); ++it) {
            cObject *obj = *it;
            msg_check *tmp,*duplicate;

            duplicate = check_and_cast<msg_check *>(obj);
            tmp=duplicate->dup();
            if(tmp->getOriginalExecId()==myId)
            {
                tmp->setStatusRequest(true);
                tmp->setAck(true);
                tmp->setIsEnded(true);

                EV<<"During Reboot Found JobId in my completed jobs(original): "<<tmp->getRelativeJobId()<<endl;
                portId=tmp->getClientId()-1;
                send(tmp,"exec$o",portId);
            }
            else{
                tmp->setStatusRequest(true);
                tmp->setAck(true);
                tmp->setIsEnded(true);

                EV << "During Reboot Sending the status to original exec: "<<tmp->getRelativeJobId()<<endl;
                send(tmp,"load_send",tmp->getOriginalExecId());
            }


        }

        EV<<"The backup process is over, executor is now in normal execution mode"<<endl<<".........."<<endl;
        bubble("Normal mode");
        failure=false;//until I have recovered all the backup message I will still ignore all the other messages
        restartNormalMode();
        delete msg;
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
        else {
                tmp = check_and_cast<msg_check *>(newJobsQueue.front());
                balancedJob(tmp);
            }


    }
    else{
         msgServiced = check_and_cast<msg_check *>(jobQueue.front());
         jobId = msgServiced->getRelativeJobId();
         portId = msgServiced->getClientId()-1;
         timeoutJobComplexity = msgServiced->getJobComplexity();
         scheduleAt(simTime()+timeoutJobComplexity, timeoutJobComputation);
         EV<<"Starting service of "<<jobId<<" coming from user ID "<<portId<<" from the queue of the machine "<<msgServiced->getOriginalExecId()<<endl;

    }

}

void Executor::balancedJob(msg_check *msg){
    msg_check *msgSend;
    int i;
    int src_id;
    src_id=msg->getClientId();
    EV<<"msg ID "<<msg->getRelativeJobId()<<" coming from user ID "<<src_id-1<<" trying to distribute probing mode "<<probingMode<<endl;
    if(!probingMode){  //==-1 means that the coming msg has been forwarded by another machine after load balancing
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
                 EV<<"crash when sending load balancing requests "<<endl;
                 return;
             }
             send(msgSend,"load_send",i);
             EV<<"Asking the load to machine "<<msgSend->getActualExecId()<<endl;
             }
         //TO DO:ADD ONE TIMEOUT FOR ALL
        }
        scheduleAt(simTime()+timeoutLoad, timeoutLoadBalancing);
    }
}

void Executor::probeHandler(msg_check *msg){
    msg_check *tmp;
    int greaterLength;
    switch(msg->getAck()){
        case false:  //sono stato interrogato per sapere lo stato della mia coda---setAck=true---salvo da quaclhe parte il minimo poi decido se reinstradare poi i da E diventa 0 cosi posso rifare questo giochino

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

            failureEvent(probCrashDuringExecution);
            if(failure){
                EV<<"crash in the middle of sending probing response "<<endl;
                return;
            }


            switch(probeResponse)
            {
            case 1:
                if(msg->getDuplicate() || jobQueue.getLength() < msg->getQueueLength()){
                   msg->setQueueLength(jobQueue.getLength());
                   send(msg,"load_send",msg->getOriginalExecId());
                   EV<<"normal reply to probe"<<endl;
                }
                else
                    delete msg;
                break;

            default:
                greaterLength=jobQueue.getLength()+probeResponse;
                if(msg->getDuplicate() ||  greaterLength<msg->getQueueLength()){
                       msg->setQueueLength(jobQueue.getLength());
                       send(msg,"load_send",msg->getOriginalExecId());
                       EV<<"suboptimal load balancing reply to probe"<<endl;
                 }
                else
                     delete msg;



            }
            break;
            case true:
            /*
             D)The Original exec has received the reply from another executor:
                 ->It will extract the QueueLength field of the packet and store it
             * */
                if(!newJobsQueue.isEmpty()&&strcmp(check_and_cast<msg_check *>(newJobsQueue.front())->getName(),msg->getName())==0){
                   msg->setProbing(false);
                   msg->setAck(false);

                   if(msg->getDuplicate()){
                       cancelEvent(timeoutLoadBalancing);
                       tmp = check_and_cast<msg_check *>(newJobsQueue.pop()); //remove
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
               break;
    }
}

void Executor::reRoutedHandler(msg_check *msg){
    msg_check *msgSend,*tmp;
    int actualExecId;
    simtime_t timeoutJobComplexity;
    switch(msg->getAck()){
    case false: //the actual exec has received the packet;notifies the original exec
        jobQueue.insert(msg->dup());//not in new job otherwise will redo load balancing:wrong

        msgSend=msg->dup();
        msgSend->setJobQueue(true);
        send(msgSend,"backup_send$o");

        failureEvent(probCrashDuringExecution);
        if(failure){
            EV<<"crash in the middle of notify the packet received due to load balancing;TIMEOUT WILL EXPIRE:RESTART LOAD BALANCING "<<endl;
            //the packet can be executed twice if is not resend after the new load balancing to this entity
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
        break;
    case true:
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
        if(newJobsQueue.getLength()>0){
            tmp = check_and_cast<msg_check *>(newJobsQueue.front());
            balancedJob(tmp);
        }
        EV<<"ack received from actual exec "<< actualExecId <<endl;
        delete msg;
        break;
    }
}

void Executor::statusRequestHandler(msg_check *msg){
    msg_check *tmp, *msgEnded;
    cObject *obj;
    int portId;
    const char *jobId;
    jobId = msg->getName();
    if(msg->getAck()==true){
      EV << "ACK received for "<<jobId<<endl;
      if(msg->getReRouted()==true){
         if(msg->getOriginalExecId()==myId){
             if(msg->getIsEnded()){
                 obj = reRoutedJobs.remove(jobId);
                 if(obj!=nullptr){
                     tmp = check_and_cast<msg_check *>(obj);
                     EV << "Erasing from the rerouted jobs the job: "<<msg->getOriginalExecId() <<"-"<<msg->getRelativeJobId()<<endl;
                     tmp->setReRoutedJobQueue(true);
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
    delete msg;
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

    if(jobQueue.isEmpty()){
        jobQueue.insert(msg->dup());

        timeoutJobComplexity = msg->getJobComplexity();
        msgSend=msg;
        msgSend->setJobQueue(true);
        send(msgSend,"backup_send$o");

        if(!timeoutJobComputation->isScheduled()){
            EV<<"New message arrived with idle server:execute it immediately:Job Id "<<id<<endl;
            scheduleAt(simTime()+timeoutJobComplexity, timeoutJobComputation);
        }
    }else
        switch(granularity){
        case 1:
                msgSend=msg->dup();
                msgSend->setNewJobsQueue(true);
                send(msgSend,"backup_send$o");//send a copy of backup to the storage to cope with possible failure

                newJobsQueue.insert(msg->dup());
                balancedJob(msg);
                EV<<"Do optimal load balancing"<<endl;
                break;

        default:
                if(!skipLoad){
                    msgSend=msg->dup();
                    msgSend->setNewJobsQueue(true);
                    send(msgSend,"backup_send$o");//send a copy of backup to the storage to cope with possible failure
                    EV<<"Perform load this time; partial "<<skipLoad<<" versus granularity "<<granularity<<endl;
                    newJobsQueue.insert(msg->dup());
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
        send(tmp->dup(),"load_send", actualExec);
        scheduleAt(simTime()+timeoutLoad, timeoutReRouted);
    }
    EV<<"Final executor "<<actualExec<<endl;
    EV<<"jobQueue after timeout load balancing "<<jobQueue.getLength()<<endl;
}

void Executor::timeoutJobExecutionHandler(){

    const char *jobId;
    int portId;
    simtime_t timeoutJobComplexity;
    msg_check *msgServiced,*msgSend;
    // Get the source_id of the message that just finished service

    msgServiced = check_and_cast<msg_check *>(jobQueue.pop());
    jobId = msgServiced->getName();
    portId = msgServiced->getClientId()-1;  //Source id-1=portId
    EV<<"Completed job: "<<jobId<<" creating by the user ID "<<portId<<endl;
    jobCompleted++;

    msgSend = msgServiced->dup();
    msgSend->setJobQueue(true);
    send(msgSend,"backup_send$o");

    msgSend = msgServiced->dup();
    msgSend->setEndingTime(simTime());
    completedJob.add(msgSend);


    msgSend= msgServiced;
    msgSend->setCompletedQueue(true);
    send(msgSend,"backup_send$o");


    /*
    B)Then the executor decides which packet it should process next:
      ->If his queue is empty no computation is performed:the executor goes idle until a new packet comes
      ->If the queue is not empty:takes the first packet in the queue(FIFO approach) and starts to execute it
     */
    if(jobQueue.isEmpty())
      EV<<"Empty queue, the machine "<<myId<<" goes IDLE"<<endl;
    else{
         msgServiced = check_and_cast<msg_check *>(jobQueue.front());
         jobId = msgServiced->getName();
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
           probingMode = false;
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
void Executor::finish()
{
    EV<<"completed jobs "<<jobCompleted<<endl;
    }
