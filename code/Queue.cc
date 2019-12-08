#include <string.h>
#include <omnetpp.h>
#include <msg_check_m.h>  //fare parsing
#include <map>
using namespace omnetpp;
class Queue : public cSimpleModule {
private:
    msg_check *msgServiced; // message being served
    msg_check *endServiceMsg;
    msg_check *forward_backup;
    msg_check *forward_id;
    msg_check *probe;
    msg_check *msgToSend;
    msg_check *reply;
    simtime_t defServiceTime;
    simtime_t expPar;
    int src_id,E,N,port_id,nProcessed,thisLength,minLength;
    std::map<const char *,int> ProbeMsg;
    //map<int,int,int,int,int,simtime_t,bool> storedMsg;
    //std::map<int,int,int,int,int,simtime_t,bool> storedMsg;  //key is an int type, the value referenced by the key is a msg_backup message
    std::map<const char *,int>::iterator search;


protected:
    cQueue queue;
    virtual void initialize() override;
    virtual void handleMessage(cMessage *cmsg) override;
};
Define_Module(Queue);

void Queue::initialize() {

    nProcessed=0;
    thisLength=0;
    minLength=0;
    E = par("E"); //non volatile parameters --once defined they never change
    N= par("N");
 // The exponential value is computed in the handleMessage; Set the service time as exponential
    expPar=exponential(par("defServiceTime").doubleValue());
    msgServiced = endServiceMsg = nullptr;
    forward_backup = forward_id = probe = msgToSend = reply =nullptr;
    endServiceMsg = new msg_check("end-service");
    forward_backup = new msg_check("sending the message for backup");
    forward_id = new msg_check("notify the user about job id");
    probe = new msg_check("Ask the load");
    msgToSend = new msg_check("Perform load balancing");
    reply = new msg_check("Reply with a lower queue");

}
void Queue::handleMessage(cMessage *cmsg) {
    // Casting from cMessage to msg_check
   int i;
   msg_check *msg = check_and_cast<msg_check *>(cmsg);
   std::string jobb_id;
   std::string received;
   const char * id,* job_id;
   int machine;
   if (msg == endServiceMsg){      // SELF-MESSAGE HAS ARRIVED - the server finished serving a message
      // Get the source_id of the message that just finished service
      job_id= msgServiced->getJobId();
      src_id=msgServiced->getSourceId();  //Source id-1=port_id
      port_id=src_id-1;

      EV<<"Completed service of "<<job_id<<" coming from user ID "<<src_id<<endl;
      msgServiced->setHasEnded(true);
      // Notify the end of the computation to the input user
      send(msgServiced, "exec$o",port_id);//is it correct id as output? think about it;---thinks about :end of computing+element in the queue:msgserviced=msgfrom the queue
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
                  src_id=msgServiced->getSourceId();
                  EV<<"Starting service of "<<job_id<<" coming from user ID "<<src_id<<" from the queue of the machine "<<msgServiced->getOriginalExecId()<<endl;
                  //serviceTime = exponential(expPar); //defines the service time
                  scheduleAt(simTime()+expPar, endServiceMsg);
                  //EV<<"serviceTime= "<<expPar<<endl;
              }
       }
   else{
       if(msg->getProbing()==false){
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
               port_id=msg->getSourceId()-1;
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
               src_id=msgServiced->getSourceId();
               EV<<"Starting service of "<<msgServiced->getJobId()<<" coming from user ID "<<src_id<<endl;
               //serviceTime = exponential(expPar); //defines the service time
               scheduleAt(simTime()+expPar, endServiceMsg);
               //EV<<"serviceTime= "<<expPar<<endl;
               }
               else{
                   if(msg->getQueueLength()!=-1){  //==-1 means that the coming msg has been forwarded by another machine after load balancing
                       //no need to redo load balancing
                   thisLength++;
                   //ovviamente non ce load balancing tra le varie machine ancora
                   probe->setHasEnded(false);  //set message priority
                   probe->setProbing(true);
                   probe->setProbed(false);
                   probe->setReRouted(false);
                   probe->setQueueLength(thisLength);
                   probe->setResidualTime(SIMTIME_ZERO); //initialize to the partial elaboration done of a packet; will be useful for server utilization signal and preemptive resume
                   probe->setJobId(msg->getJobId()); //will be useful for computing the per class extended service time
                   probe->setSourceId(msg->getSourceId());  //initialize to 0 the time when a packet goes for he first time to service(useful for extended per class service time)
                   //probe->setActualExecId(msg->getOriginalExecId());
                   probe->setOriginalExecId(msg->getOriginalExecId());
                   probe->setActualExecId(msg->getOriginalExecId());
                   for(i=0;i<E;i++){
                       if(i!=msg->getOriginalExecId()){
                           msg_check *query= probe->dup();
                           query->setActualExecId(i);//attento!
                           send(query,"load_send",i); //in caso di guasti???
                           EV<<"Asking the load to machine "<<query->getActualExecId()<<endl;
                           }
                       }
                   //ProbeMsg.insert({msg->getJobId(),probe});

                   //probe = nullptr;
                   }


                   //io sono stato interrogato qui---posso mettere i pacchetti in coda e aspettare le risposte senza fare altro
                   EV<<"QUEUE msg ID "<<id<<" coming from user ID "<<src_id<<" goes in the queue of the machine ID"<<msg->getOriginalExecId()<<endl;
                   queue.insert(msg);


               }
       }
       else{
           if(msg->getProbing()==true && msg->getProbed()==false)  {//sono stato interrogato per sapere lo stato della mia coda---setProbed=true---salvo da quaclhe parte il minimo poi decido se reinstradare poi i da E diventa 0 cosi posso rifare questo giochino
               if(msg->getQueueLength()>thisLength){   //thisLength+1
                   //NO IS WRONG THIS THINKING+1 is needed in order to avoid ping pong:exe machine 5 has queue length =3 while machine 1 has length  =2... without +1 5-2 and 1-3 but if a
                   //an arrival comes in 3
                   reply->setProbed(true);
                   reply->setHasEnded(false);  //set message priority
                   reply->setProbing(true);
                   reply->setReRouted(false);
                   reply->setResidualTime(SIMTIME_ZERO); //initialize to the partial elaboration done of a packet; will be useful for server utilization signal and preemptive resume
                   reply->setJobId(msg->getJobId());
                   reply->setSourceId(msg->getSourceId());  //initialize to 0 the time when a packet goes for he first time to service(useful for extended per class service time)
                   reply->setOriginalExecId(msg->getOriginalExecId());
                   reply->setQueueLength(thisLength);
                   reply->setActualExecId(msg->getActualExecId());

                   send(reply,"load_send",msg->getOriginalExecId());
                   EV<<"The machine "<<msg->getActualExecId()<<" has a lower queue="<<thisLength<<" than the one in machine "<<msg->getOriginalExecId()<<" which has queue length="<<msg->getQueueLength()<<endl;

                   }
               else
                   delete msg;
               }
           minLength=msg->getQueueLength();
           if(queue.isEmpty()) //maybe in the emantime the queue has been emptied:no need to perform load balancing(also I will try to accesso an empty queue:segmentation fault)
               delete msg;
           else{
               search=ProbeMsg.find(msg->getJobId());
               if (search == ProbeMsg.end()){
                   msgToSend = (msg_check *)queue.pop();
                   msgToSend->setHasEnded(false);
                   msgToSend->setProbing(false);
                   msgToSend->setProbed(false);
                   msgToSend->setReRouted(true);
                   msgToSend->setQueueLength(-1); //modify service time when coming
                   msgToSend->setActualExecId(msg->getActualExecId());
                   thisLength--;
                  // nProcessed--;
                   ProbeMsg.insert({msg->getJobId(),msgToSend->getActualExecId()});
                   //msgNotify=msgToSend->dup();
                   EV<<"Send to the machine "<<msg->getActualExecId()<<" that has a lower queue the "<<msg->getJobId()<<endl;
                   EV<<"Send  queue "<<msg->getQueueLength()<<endl;
                   send(msgToSend,"load_send",msg->getActualExecId());//send to the backup

               }
               else //that message has been already forwarded somewhere:no sense to forward in another place too
                   delete msg;

               }



           }
   }
}
