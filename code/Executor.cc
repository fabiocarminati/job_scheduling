#include <string.h>
#include <omnetpp.h>
#include <msg_check_m.h>  //fare parsing
#include <map>
#define CLUSTER_SIZE 1000
#define QUEUE_SIZE 10000000
using namespace omnetpp;
class Executor : public cSimpleModule {
private:
    msg_check *msgServiced; // message being served
    msg_check *endServiceMsg;
    //msg_check *probe;
    // msg_check *reply;
    msg_check *notifyOriginalExec;
    msg_check *ackToActualExec;
    msg_check *end;
    msg_check *timeoutLoadBalancing;
    msg_check *timeoutReSendOriginal;
    msg_check *sentMsg;
    simtime_t timeoutLoad;
    simtime_t timeoutOriginal;
    simtime_t defServiceTime;
    simtime_t expPar;
    int E,N,nArrived,thisLength,myId;
    bool probingMode;

    cQueue outgoingPacket;
    cQueue newJobsQueue;
    cQueue jobQueue;
    cQueue balanceResponses;
    std::map<std::string,int> ProbeMsg;
    std::map<std::string,int>::iterator search;

    void selfMessage(msg_check *msg);
    void newJob(msg_check *msg);
    void balancedJob(msg_check *msg);
    void probeHandler(msg_check *msg);
    void ReRoutedJobEnd(msg_check *msg);
    void timeoutLoadBalancingHandler();
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
    timeoutLoadBalancing = msgServiced = endServiceMsg = timeoutReSendOriginal = sentMsg = ackToActualExec = nullptr;
    notifyOriginalExec = nullptr;
    //probe = reply = nullptr;

}

Executor::~Executor()
{
    delete msgServiced;
    cancelAndDelete(timeoutLoadBalancing);
    cancelAndDelete(timeoutReSendOriginal);
    cancelAndDelete(endServiceMsg);
   // cancelAndDelete(msgToSend);
}

void Executor::initialize() {
    probingMode = false;
    myId=getId()-2-N;
    nArrived=0;
    E = par("E"); //non volatile parameters --once defined they never change
    N= par("N");
 // The exponential value is computed in the handleMessage; Set the service time as exponential
    expPar=exponential(par("defServiceTime").doubleValue());
    //msgServiced = endServiceMsg = end = nullptr;

    endServiceMsg = new msg_check("end-service");
    //forward_backup = new msg_check("sending the message for backup");
    //forward_id = new msg_check("notify the user about job id");
    //probe = new msg_check("Ask the load");
    //reply = new msg_check("Reply with the queue value");
    notifyOriginalExec = new msg_check("Notify original exec about end of the computation");
    end = new msg_check("Notify to the backup the delete");
    timeoutLoadBalancing = new msg_check("timeoutEventLoad");
    timeoutReSendOriginal = new msg_check("timeoutReSendOriginal");
    sentMsg=new msg_check("sentMsg");
    timeoutLoad=0.1;
    timeoutOriginal=0.2;
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
   if(msg->isSelfMessage())
      selfMessage(msg);
   else{
       if(msg->getNewJob()==true){
          newJob(msg);
       }else if(msg->getProbing()==true){
               probeHandler(msg);
             }
             else if(msg->getHasEnded()==true){
                     ReRoutedJobEnd(msg);
             }

       delete msg;
   }
}

void Executor::balancedJob(msg_check *msg){
    msg_check *msgSend;
    int i;
    int src_id;
    src_id=msg->getClientId();
    if (msgServiced!=nullptr){      // Server is IDLE, there's no message in service:execute the one that has arrived right now or put it in the queue
       EV<<"EMPTY queue immediate service "<<endl;
       // Direct service
       msgServiced = msg->dup(); //given that the server is idle the arrived message is immediately served despite his priority
       msgServiced->setResidualTime(expPar);
       // save the time when the packet has been served for the first time (useful for per class extended service time)
       EV<<"Starting service of "<<msgServiced->getJobId()<<" coming from user ID "<<src_id-1<<endl;
       //serviceTime = exponential(expPar); //defines the service time
       scheduleAt(simTime()+expPar, endServiceMsg);
       //EV<<"serviceTime= "<<expPar<<endl;
    }
    else{
         EV<<"QUEUE msg ID "<<msg->getJobId()<<" coming from user ID "<<src_id<<" goes in the queue of the machine ID"<<msg->getActualExecId()<<endl;
         if(!probingMode){  //==-1 means that the coming msg has been forwarded by another machine after load balancing
             msg->setHasEnded(false);
             msg->setProbing(true);
             msg->setAck(false);
             msg->setReRouted(false);
             msg->setQueueLength(jobQueue.getLength());
             msg->setResidualTime(SIMTIME_ZERO); //initialize to the partial elaboration done of a packet; will be useful for server utilization signal and preemptive resume
             //msg->setJobId(msg->getJobId()); //will be useful for computing the per class extended service time
             for(i=1;i<=E;i++){
                 if(i-1!=msg->getOriginalExecId()){
                     msg->setActualExecId(i-1);//attento!
                     msgSend = msg->dup();
                     send(msgSend,"load_send",i-1); //in caso di guasti???
                     //EV<<"Queue length"<<query->getQueueLength()<<endl;
                     EV<<"Asking the load to machine "<<msg->getActualExecId()<<endl;
                     }
                 //TO DO:ADD ONE TIMEOUT FOR ALL
                 }
             probingMode = true;
             scheduleAt(simTime()+timeoutLoad, timeoutLoadBalancing);
         }
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
        msgSend->setResidualTime(SIMTIME_ZERO); //initialize to the partial elaboration done of a packet; will be useful for server utilization signal and preemptive resume
        if(msgSend->getQueueLength()>jobQueue.getLength())   //thisLength+1
            EV<<"The machine "<<msgSend->getQueueLength()<<" has a lower queue="<<thisLength<<" than the one in this machine which has queue length="<<jobQueue.getLength()<<endl;
        else
            EV<<"The machine "<<msgSend->getQueueLength()<<" has a greater or equal queue="<<thisLength<<" than the one in this machine which has queue length="<<jobQueue.getLength()<<endl;
        msgSend->setQueueLength(thisLength);
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

void Executor::ReRoutedJobEnd(msg_check *msg){
    /*When the actual exec receives that ACK:
               ->It will stop the timeout event timeoutReSendOriginal for the ACK
               ->Notify his secure storage to delete the local copy of the previously processed packet
           */
    if(msg->getAck()==true){
      EV << "ACK received from original exec "<<msg->getOriginalExecId() <<" for "<<msg->getJobId()<<endl;
      ackToActualExec=msg->dup();
      send(ackToActualExec, "backup_send");
      cancelEvent(timeoutReSendOriginal);
    }
    else{
        /*When the original exec receives the msg notifying the end of processing by actual exec:
              ->Sends the ACK to the actual exec
              ->Notifies his secure storage to delete the local copy of the previously forwarded packet to the actual exec
              ->Notifies the client the end of the processing
              */
        EV << "Send the ACK to the actual exec "<<msg->getActualExecId() <<" for "<<msg->getJobId()<<endl;
        ackToActualExec=msg->dup();
        ackToActualExec->setAck(true);
        send(ackToActualExec, "load_send",ackToActualExec->getActualExecId());
        ackToActualExec=msg->dup();
        send(ackToActualExec, "backup_send"); //the original exec notifies his own storage to delete the entry
        ackToActualExec=msg->dup();
        send(ackToActualExec, "exec$o",msg->getClientId()-1);
    }
      //delete the copy in the local storage
}

void Executor::newJob(msg_check *msg){
    msg_check *msgSend;
    std::string jobb_id;
    const char *id;
    std::string received;
    int machine, port_id;
    nArrived++;
    //Create the JobId
    machine=msg->getOriginalExecId();
    jobb_id="Job ID: ";
    jobb_id.append(std::to_string(machine));
    received="-";
    jobb_id.append(received);
    jobb_id.append(std::to_string(nArrived));
    id=jobb_id.c_str();
    //save the message to the stable storage
    msgSend=msg->dup();
    msgSend->setActualExecId(msg->getActualExecId());
    msgSend->setJobId(id);
    EV<<"Send to the backup info about original "<<msgSend->getOriginalExecId()<<" and actual "<<msgSend->getActualExecId()<<" of the "<<msgSend->getJobId()<<endl;
    send(msgSend,"backup_send$o");//send a copy of backup to the storage to cope with possible failure
    //Reply to the client
    port_id=msg->getClientId()-1;
    msgSend=msg->dup();
    msgSend->setJobId(id);
    msgSend->setAck(true);
    send(msgSend,"exec$o",port_id);
    EV<<"First time the packet is in the cluster:define his "<<msgSend->getJobId()<<endl;
    msg->setNewJob(false);
    msg->setJobId(id);
    newJobsQueue.insert(msg->dup());
    balancedJob(msg);
}

void Executor::timeoutLoadBalancingHandler(){
    int i;
    int actualExec;
    msg_check *tmp;
    bool processing = true;

    if(balanceResponses.getLength()>0){
        while(!balanceResponses.isEmpty()){
          tmp = check_and_cast<msg_check *>(balanceResponses.pop());
          if(tmp->getQueueLength()<jobQueue.getLength()){
              actualExec=tmp->getActualExecId();
              processing = false;
          }
          delete tmp;
        }
    }

    tmp = check_and_cast<msg_check *>(newJobsQueue.pop());
    if(processing)
        jobQueue.insert(tmp);
    else{
        tmp->setHasEnded(false);
        tmp->setProbing(false);
        tmp->setAck(false);
        tmp->setReRouted(true);
        tmp->setQueueLength(-1); //modify service time when coming
        EV<<"Final executor "<<actualExec<<endl;
        tmp->setActualExecId(actualExec);

        ProbeMsg.insert(std::pair<std::string, int>(tmp->getJobId(),actualExec)); //will overwrite the existing value?
        EV<<"Send to the machine "<<actualExec<<" that has a lower queue the "<<tmp->getJobId()<<endl;
        send(tmp,"load_send", actualExec);//send to the backup
    }
    probingMode = false;
    if(newJobsQueue.getLength()>0){
        tmp = check_and_cast<msg_check *>(newJobsQueue.front());
        balancedJob(tmp);
    }
    EV<<"queue of jobs "<<jobQueue.getLength()<<endl;
}




void Executor::selfMessage(msg_check *msg){
    int i;
    const char * id,* job_id;
    int actualExec, src_id, port_id;
    msg_check *msgToSend = new msg_check("Perform load balancing");
    msg_check *msgServiced;
    /* SELF-MESSAGE HAS ARRIVED
    The timeout(timeoutOriginal) started when the actual exec notifies the end of the computation to the original exec has expired:
    1)The actual exec understands that the original exec has crashed/is currently unavailable
    2)The actual exec will retry to notify the original exec about the end of the computation starting a new timeout
    These steps will be repeated by the actual executor until the original exec is available again and thus will reply to the message
    */

    if(msg==timeoutReSendOriginal){

            EV<<"The original executor is not responding;retry to reach it"<<endl;
            notifyOriginalExec=sentMsg->dup();
            send(notifyOriginalExec, "load_send",notifyOriginalExec->getOriginalExecId());
            //send(sentMsg->dup(), "backup_send$o");
            scheduleAt(simTime()+timeoutOriginal,timeoutReSendOriginal);

       }
       else
           /* SELF-MESSAGE HAS ARRIVED
           The timeout(timeoutLoad) started when an executor queries the other executors in order to know their queue length(and thus
           eventually perform load balancing) has ended:
           1)TO DO --- The executors which hasn't responded up to now are considered as failed and thus the storage is notified
           2)The responses arrived up to now are checked in order to understand whether load balancing should be performed or not according
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

               if (msg == endServiceMsg){
                  // Get the source_id of the message that just finished service
                  job_id = msgServiced->getJobId();
                  src_id = msgServiced->getClientId();  //Source id-1=port_id
                  port_id = src_id-1;

                  EV<<"Completed service of "<<job_id<<" coming from user ID "<<src_id<<endl;
                  msgServiced->setHasEnded(true);

                  if (msgServiced->getReRouted()==true){
                      notifyOriginalExec=msgServiced->dup();
                      send(notifyOriginalExec, "load_send",notifyOriginalExec->getOriginalExecId());
                      sentMsg=msgServiced->dup();
                      scheduleAt(simTime()+timeoutOriginal,timeoutReSendOriginal);
                  }
                  else{
                     // end=msgServiced->dup();
                      send(msgServiced->dup(), "backup_send$o");
                      send(msgServiced->dup(), "exec$o",port_id); //send(msgServiced->dup(), "exec$o",port_id);
                  }
                  /*
                   B)Then the executor decides which packet it should process next:
                       ->If his queue is empty no computation is performed:the executor goes idle until a new packet comes
                       ->If the queue is not empty:takes the first packet in the queue(FIFO approach) and starts to execute it
                   */
                  if(newJobsQueue.isEmpty()){
                     EV<<"Empty queue, the machine "<<msgServiced->getOriginalExecId()<<" goes IDLE"<<endl;
                    //emit(busySignal, false);    // Magari puo servire
                     }
                          else{ // at least one queue contains users
                              //msgServiced = (msg_check *)newJobsQueue.pop(); //remove the first element of that queue(FIFO policy)
                              thisLength--;
                              msgServiced->setResidualTime(expPar);  //attenzione che e volatile magari in schedule at e ricalcolato con un diverso valore
                              job_id= msgServiced->getJobId();
                              src_id=msgServiced->getClientId();
                              EV<<"Starting service of "<<job_id<<" coming from user ID "<<src_id-1<<" from the queue of the machine "<<msgServiced->getOriginalExecId()<<endl;
                              //serviceTime = exponential(expPar); //defines the service time
                              scheduleAt(simTime()+expPar, endServiceMsg);
                              //EV<<"serviceTime= "<<expPar<<endl;
                          }
             }
}
