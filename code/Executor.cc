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
    msg_check *forward_backup;
    msg_check *forward_id;
    //msg_check *probe;
    msg_check *msgToSend;
   // msg_check *reply;
    msg_check *notifyOriginalExec;
    msg_check *end;
    msg_check *timeoutEventLoad;
    msg_check *timeoutReSendOriginal;
    msg_check *sentMsg;
    simtime_t timeoutLoad;
    simtime_t timeoutOriginal;
    simtime_t defServiceTime;
    simtime_t expPar;
    int src_id,E,N,port_id,nProcessed,thisLength,minLength,lengths[CLUSTER_SIZE],myId;

    std::map<std::string,int> ProbeMsg;
    //map<int,int,int,int,int,simtime_t,bool> storedMsg;
    //std::map<int,int,int,int,int,simtime_t,bool> storedMsg;  //key is an int type, the value referenced by the key is a msg_backup message
    std::map<std::string,int>::iterator search;


protected:
    cQueue queue;
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
    timeoutEventLoad = msgServiced =endServiceMsg = msgToSend = timeoutReSendOriginal = sentMsg = nullptr;
    forward_backup = forward_id = msgToSend = notifyOriginalExec =nullptr;
    //probe = reply = nullptr;

}

Executor::~Executor()
{
    delete msgServiced;
    cancelAndDelete(timeoutEventLoad);
    cancelAndDelete(timeoutReSendOriginal);
    cancelAndDelete(endServiceMsg);
    cancelAndDelete(msgToSend);
}

void Executor::initialize() {
    myId=getId()-2-N;
    nProcessed=0;
    thisLength=0;
    minLength=0;
    E = par("E"); //non volatile parameters --once defined they never change
    N= par("N");
 // The exponential value is computed in the handleMessage; Set the service time as exponential
    expPar=exponential(par("defServiceTime").doubleValue());
    //msgServiced = endServiceMsg = end = nullptr;

    endServiceMsg = new msg_check("end-service");
    forward_backup = new msg_check("sending the message for backup");
    forward_id = new msg_check("notify the user about job id");
    //probe = new msg_check("Ask the load");
    msgToSend = new msg_check("Perform load balancing");
    //reply = new msg_check("Reply with the queue value");
    notifyOriginalExec = new msg_check("Notify original exec about end of the computation");
    end = new msg_check("Notify to the backup the delete");
    timeoutEventLoad = new msg_check("timeoutEventLoad");
    timeoutReSendOriginal = new msg_check("timeoutReSendOriginal");
    sentMsg=new msg_check("sentMsg");
    lengths[0]=0;
    timeoutLoad=0.1;
    timeoutOriginal=0.2;
}
void Executor::handleMessage(cMessage *cmsg) {
    // Casting from cMessage to msg_check

   int i;
   //probe = new msg_check("Ask the load");
   //msgToSend = new msg_check("Activate load balancing");
   //reply = new msg_check("Reply with a lower queue");
   msg_check *msg = check_and_cast<msg_check *>(cmsg);
   std::string jobb_id;
   std::string received;
   const char * id,* job_id;
   int machine,actualExec,minLength;
   /* SELF-MESSAGE HAS ARRIVED
   The timeout(timeoutOriginal) started when the actual exec notifies the end of the computation to the original exec has expired:
   1)The actual exec understands that the original exec has crashed/is currently unavailable
   2)TO DO --- The original executor which hasn't responded is considered as failed and thus the storage is notified
   */
   if(msg==timeoutReSendOriginal){
       EV<<"The original executor is not responding;I will notify myself the storage and the user the end of the computation"<<endl;
       send(sentMsg->dup(), "backup_send$o");
       send(sentMsg, "exec$o",port_id); //send(msgServiced->dup(), "exec$o",port_id);

   }
   else{
       /* SELF-MESSAGE HAS ARRIVED
       The timeout(timeoutLoad) started when an executor queries the other executors in order to know their queue length(and thus
       eventually perform load balancing) has ended:
       1)TO DO --- The executors which hasn't responded up to now are considered as failed and thus the storage is notified
       2)The responses arrived up to now are checked in order to understand whether load balancing should be performed or not according
        to the different queue length values
       */
       if(msg==timeoutEventLoad){
           if(queue.isEmpty()){ //maybe in the meantime the queue has been emptied:no need to perform load balancing(also I will try to accesso an empty queue:segmentation fault)
               EV<<"EMPTY queue in the meantime:no load balancing"<<endl;
               delete msg;

           }
           else{
           //search=ProbeMsg.find(msg->getJobId());
          // if (search == ProbeMsg.end()){
               msgToSend = (msg_check *)queue.pop();
               EV<<"dddd"<<endl;
               msgToSend->setHasEnded(false);
               msgToSend->setProbing(false);
               msgToSend->setProbed(false);
               msgToSend->setReRouted(true);
               msgToSend->setQueueLength(-1); //modify service time when coming
               minLength=thisLength;  //must be put as a constraint for cqueue(I don't want infinite queue)
               for(i=1;i<=E;i++){
                   if(lengths[i]!=-1 && lengths[i]<minLength){
                       minLength=lengths[i];
                       actualExec=i-1;
                       }
                   }//I suppose that at least one machine(other than the one that has sent the probing msg) is active anytime
               if(minLength==thisLength){
                   EV<<"no one has a lower queue length than me"<<endl;
               }else{
                   EV<<"final executor "<<actualExec<<endl;
                   msgToSend->setActualExecId(actualExec);
                   thisLength--;//posso usare queue.getlength:non sembra funzionare:to do check
                  // nProcessed--;
                   ProbeMsg.insert(std::pair<std::string, int>(msgToSend->getJobId(),msgToSend->getActualExecId())); //will overwrite the existing value?
                   //msgNotify=msgToSend->dup();
                   EV<<"Send to the machine "<<msgToSend->getActualExecId()<<" that has a lower queue the "<<msgToSend->getJobId()<<endl;
                   EV<<"Send  queue "<<msgToSend->getQueueLength()<<endl;
                   send(msgToSend,"load_send",msgToSend->getActualExecId());//send to the backup
                   //msgToSend=nullptr;
                   }
               delete msg;
           }

           lengths[0]=0;
       }else{

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
          job_id= msgServiced->getJobId();
          src_id=msgServiced->getClientId();  //Source id-1=port_id
          port_id=src_id-1;

          EV<<"Completed service of "<<job_id<<" coming from user ID "<<src_id<<endl;
          msgServiced->setHasEnded(true);

          if (msgServiced->getReRouted()==true){

              notifyOriginalExec=msgServiced->dup();
              send(notifyOriginalExec, "load_send",notifyOriginalExec->getOriginalExecId());
              sentMsg=msgServiced->dup();
              scheduleAt(simTime()+timeoutOriginal,timeoutReSendOriginal);
          }
          else{
              end=msgServiced->dup();
              send(end, "backup_send$o");
              send(msgServiced, "exec$o",port_id); //send(msgServiced->dup(), "exec$o",port_id);

          }
          /*
           B)Then the executor decides which packet it should process next:
               ->If his queue is empty no computation is performed:the executor goes idle until a new packet comes
               ->If the queue is not empty:takes the first packet in the queue(FIFO approach) and starts to execute it
           */
          if(queue.isEmpty()){
             EV<<"Empty queue, the machine "<<msgServiced->getOriginalExecId()<<" goes IDLE"<<endl;
             msgServiced = nullptr;
            //emit(busySignal, false);    // Magari puo servire
             }
                  else{ // at least one queue contains users

                      msgServiced = (msg_check *)queue.pop(); //remove the first element of that queue(FIFO policy)
                      thisLength--;
                      msgServiced->setResidualTime(expPar);  //attenzione che e volatile magari in schedule at e ricalcolato con un diverso valore
                      job_id= msgServiced->getJobId();
                      src_id=msgServiced->getClientId();
                      EV<<"Starting service of "<<job_id<<" coming from user ID "<<src_id<<" from the queue of the machine "<<msgServiced->getOriginalExecId()<<endl;
                      //serviceTime = exponential(expPar); //defines the service time
                      scheduleAt(simTime()+expPar, endServiceMsg);
                      //EV<<"serviceTime= "<<expPar<<endl;
                  }
           delete msg;
           }
       else{
           /*
            When load balancing is performed the actual executor and the original executor are different:when the actual executor ends the
            computation it will send a notification to the original exec.
            A)When the original exec receives that message:
                ->It will notify the storage(that will delete the entry in his map function referring to that job id)
                ->It will send an ACK to the actual exec to confirm the proper reception of his message.
                ->It will notify the client the end of the computation.
             An executor receives this packet from another executor which is the actual executor of a packet
            */
           if(msg->getHasEnded()==true && msg->getReRouted()==true && msg->getActualExecId()!=myId ){
               ProbeMsg.erase(msg->getJobId());
               forward_backup=msg->dup();
               forward_backup->setActualExecId(msg->getActualExecId());
               send(forward_backup,"backup_send$o");
               send(msg->dup(),"load_send",msg->getActualExecId());
               send(msg, "exec$o",msg->getClientId()-1);

           }
           else {
               /*B)When the actual exec receives that ACK:
                    ->It will stop the timeout event timeoutReSendOriginal for the ACK
                    ->Delete the local copy of the previously processed packet
                */
               if(msg->getHasEnded()==true && msg->getReRouted()==true && msg->getActualExecId()==myId){
                   EV << "ACK received from original exec "<<msg->getOriginalExecId() <<" for "<<msg->getJobId()<<endl;
                   cancelEvent(timeoutReSendOriginal);
                   delete  sentMsg;


           }
           else{
       /* A packet arrives to an executor:
               A)Either from a client(first time the packet enters the cluster of Executors). That executor will:
                   ->Assign a Job Id to the packet with the following structure:executor_id-number of packets already arrived in that executor up to now
                   ->Send an ACK message containing the Job Id to the client(that will stop his timeout)
                   ->Notify the storage that will add the packet and the job id in his map function
               B)or from another Executor(due to load balancing).That executor will:
                    ->NOT Re-Assign a Job Id to the packet
                    ->NOT Send an ACK message to the client
                    ->Notify the storage that will update the actual executor id referring to that packet in his map function
               Despite the source of the packet then the executor will:
                   ->If idle Immediately execute the packet
                   ->If busy:
                           ->Insert the packet in the queue
                           ->Start probing all the other executors in order to obtain their queue length and thus understand whether load
                           balancing should be performed or not
                           ->For a specific interval (timeoutLoad) the executor will only receive and store the replies without processing them yet
        *
        *
        *
        *
        */
                   if(msg->getProbing()==false){ //either I receive a new msg from a src or I receive a packet due to load balancing
                   nProcessed++;
                   EV<<"new packet event with queue "<<msg->getQueueLength()<<" with original "<<msg->getOriginalExecId()<<" and actual "<<msg->getActualExecId()<<endl;
                   if(msg->getReRouted()==false) {
                       machine=msg->getOriginalExecId();
                       jobb_id="Job ID: ";
                       jobb_id.append(std::to_string(machine).c_str());
                       received="-";
                       jobb_id.append(received);
                       jobb_id.append(std::to_string(nProcessed).c_str());
                       id=jobb_id.c_str();
                       msg->setJobId(id);
                       port_id=msg->getClientId()-1;
                       forward_id=msg->dup();
                       send(forward_id,"exec$o",port_id);
                       EV<<"First time the packet is in the cluster:define his "<<msg->getJobId()<<endl;
                       }
                   EV<<"Send to the backup info about original "<<msg->getOriginalExecId()<<" and actual "<<msg->getActualExecId()<<" of the "<<msg->getJobId()<<endl;
                   forward_backup=msg->dup();
                   forward_backup->setActualExecId(msg->getActualExecId());
                   send(forward_backup,"backup_send$o");//send a copy of backup to the storage to cope with possible failures

                   //forward_id = nullptr;
                 //  forward_backup = nullptr;

                   if (!msgServiced){      // Server is IDLE, there's no message in service:execute the one that has arrived right now or put it in the queue
                       EV<<"EMPTY queue immediate service "<<endl;
                       // Direct service
                       msgServiced = msg; //given that the server is idle the arrived message is immediately served despite his priority
                       msgServiced->setResidualTime(expPar);
                       // save the time when the packet has been served for the first time (useful for per class extended service time)
                       src_id=msgServiced->getClientId();
                       EV<<"Starting service of "<<msgServiced->getJobId()<<" coming from user ID "<<src_id<<endl;
                       //serviceTime = exponential(expPar); //defines the service time
                       scheduleAt(simTime()+expPar, endServiceMsg);
                       //EV<<"serviceTime= "<<expPar<<endl;
                       }
                       else{
                           thisLength++;//se arrivano 2/3 pacchetti insieme la coda aumanta solo di 1 quando chiedo agli altri
                           EV<<"QUEUE msg ID "<<msg->getJobId()<<" coming from user ID "<<src_id<<" goes in the queue of the machine ID"<<msg->getOriginalExecId()<<endl;
                           queue.insert(msg->dup());
                           if(msg->getReRouted()==false && lengths[0]==0){  //==-1 means that the coming msg has been forwarded by another machine after load balancing
                               //no need to redo load balancing
                           //ovviamente non ce load balancing tra le varie machine ancora
                               /*probe->setHasEnded(false);  //set message priority
                               probe->setProbing(true);
                               probe->setProbed(false);
                               probe->setReRouted(false);
                               probe->setQueueLength(thisLength);
                               probe->setResidualTime(SIMTIME_ZERO); //initialize to the partial elaboration done of a packet; will be useful for server utilization signal and preemptive resume
                               probe->setJobId(msg->getJobId()); //will be useful for computing the per class extended service time
                               probe->setClientId(msg->getClientId());  //initialize to 0 the time when a packet goes for he first time to service(useful for extended per class service time)
                               //probe->setActualExecId(msg->getOriginalExecId());
                               probe->setOriginalExecId(msg->getOriginalExecId());
                               probe->setActualExecId(msg->getOriginalExecId());
                               */
                               msg->setHasEnded(false);  //set message priority
                               msg->setProbing(true);
                               msg->setProbed(false);
                               msg->setReRouted(false);
                               msg->setQueueLength(thisLength);

                               msg->setResidualTime(SIMTIME_ZERO); //initialize to the partial elaboration done of a packet; will be useful for server utilization signal and preemptive resume
                               //msg->setJobId(msg->getJobId()); //will be useful for computing the per class extended service time
                               //probe->setClientId(msg->getClientId());  //initialize to 0 the time when a packet goes for he first time to service(useful for extended per class service time)
                               //probe->setActualExecId(msg->getOriginalExecId());
                               //probe->setOriginalExecId(msg->getOriginalExecId());
                               //probe->setActualExecId(msg->getOriginalExecId());
                               for(i=1;i<=E;i++){
                                   if(i-1!=msg->getOriginalExecId()){
                                       msg_check *query= msg->dup();
                                       query->setActualExecId(i-1);//attento!
                                       send(query,"load_send",i-1); //in caso di guasti???
                                       //EV<<"Queue length"<<query->getQueueLength()<<endl;
                                       EV<<"Asking the load to machine "<<query->getActualExecId()<<endl;
                                       lengths[i]=-1;
                                       }
                                   //TO DO:ADD ONE TIMEOUT FOR ALL
                                   }
                               lengths[0]=1;
                               lengths[msg->getOriginalExecId()+1]=CLUSTER_SIZE;//OTHERWISE FOR FAULT DETECTION IT'S A MESS
                               //probe->setActualExecId(E-1);
                               //EV<<"Asking the load to the last machine "<<probe->getActualExecId()<<endl;
                               //send(probe,"load_send",probe->getActualExecId());
                           //ProbeMsg.insert({msg->getJobId(),probe});
                               //delete probe;
                               //probe = nullptr;

                               delete msg;
                               scheduleAt(simTime()+timeoutLoad, timeoutEventLoad);
                               }
                           else
                                 delete msg;

                          // delete msg;



                   }
           }
           else{
           /*
            C)An executor has been queried by another executor:
                ->Set the probed field of the packet to true
                ->It will set the packet field QueueLength to his actual value
                ->Send the reply to the original executor


           */

               if(msg->getProbing()==true && msg->getProbed()==false)  {//sono stato interrogato per sapere lo stato della mia coda---setProbed=true---salvo da quaclhe parte il minimo poi decido se reinstradare poi i da E diventa 0 cosi posso rifare questo giochino
                   //NO IS WRONG THIS THINKING+1 is needed in order to avoid ping pong:exe machine 5 has queue length =3 while machine 1 has length  =2... without +1 5-2 and 1-3 but if a
                   //an arrival comes in 3

                   msg->setProbed(true);
                   msg->setHasEnded(false);  //set message priority
                   msg->setProbing(true);
                   msg->setReRouted(false);
                   msg->setResidualTime(SIMTIME_ZERO); //initialize to the partial elaboration done of a packet; will be useful for server utilization signal and preemptive resume
                  // msg->setJobId(msg->getJobId());
                   //reply->setClientId(msg->getClientId());  //initialize to 0 the time when a packet goes for he first time to service(useful for extended per class service time)
                   //reply->setOriginalExecId(msg->getOriginalExecId());

                  // reply->setActualExecId(msg->getActualExecId());

                   //send(reply,"load_send",msg->getOriginalExecId());

                   if(msg->getQueueLength()>thisLength)   //thisLength+1
                       EV<<"The machine "<<msg->getActualExecId()<<" has a lower queue="<<thisLength<<" than the one in machine "<<msg->getOriginalExecId()<<" which has queue length="<<msg->getQueueLength()<<endl;
                   else
                       EV<<"The machine "<<msg->getActualExecId()<<" has a greater or equal queue="<<thisLength<<" than the one in machine "<<msg->getOriginalExecId()<<" which has queue length="<<msg->getQueueLength()<<endl;
                   msg->setQueueLength(thisLength);
                   send(msg,"load_send",msg->getOriginalExecId());
                   //delete msg;
                   }
               else{
                   /*
                    D)The Original exec has received the reply from another executor:
                        ->It will extract the QueueLength field of the packet and store it

                    * */
                   if(msg->getProbing()==true && msg->getProbed()==true)  {
                   lengths[msg->getActualExecId()+1]=msg->getQueueLength();
                   EV<<"store load reply from "<< msg->getActualExecId()<<endl;
                   delete msg;
                   }
                   /*//EV<<"bbbb"<<endl;
                   if(queue.isEmpty()) //maybe in the meantime the queue has been emptied:no need to perform load balancing(also I will try to accesso an empty queue:segmentation fault)
                       delete msg;
                   else{
                       lengths[msg->getActualExecId()+1]=msg->getQueueLength();
                       EV<<"store load reply"<<endl;
                       delete msg;
                       }
               */
                   }



               }
           }
           }
                                           }
   }
   }
}
