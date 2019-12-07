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
    simtime_t defServiceTime;
    simtime_t expPar;
    int src_id,E,N,port_id,nReceived;


protected:
    cQueue queue;
    virtual void initialize() override;
    virtual void handleMessage(cMessage *cmsg) override;
};
Define_Module(Queue);

void Queue::initialize() {

    nReceived=0;
    E = par("E"); //non volatile parameters --once defined they never change
    N= par("N");
 // The exponential value is computed in the handleMessage; Set the service time as exponential
    expPar=exponential(par("defServiceTime").doubleValue());
    msgServiced = endServiceMsg = nullptr;
    forward_backup = forward_id = probe = nullptr;
    endServiceMsg = new msg_check("end-service");
    forward_backup = new msg_check("sending the message for backup");
    forward_id = new msg_check("notify the user about job id");
    probe = new msg_check("Perform load balancing");

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
      src_id=msgServiced->getSourceId();
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
       if(getProbing==false){
           nReceived++;
           machine=msg->getOriginalExecId();
           jobb_id="Job ID: ";
           jobb_id.append(std::to_string(machine).c_str());
           received="-";
           jobb_id.append(received);
           jobb_id.append(std::to_string(nReceived).c_str());
           id=jobb_id.c_str();
           msg->setJobId(id);
           forward_backup=msg->dup();
           port_id=msg->getSourceId()-1;
           send(forward_backup,"exec$o",port_id);
           forward_id=msg->dup();
           send(forward_id,"backup_send$o");//send a copy of backup to the storage to cope with possible failures

           //forward_id = nullptr;
         //  forward_backup = nullptr;

           if (!msgServiced){      // Server is IDLE, there's no message in service:execute the one that has arrived right now or put it in the queue
               EV<<"EMPTY queue immediate service "<<endl;
               // Direct service
               msgServiced = msg; //given that the server is idle the arrived message is immediately served despite his priority
               msgServiced->setResidualTime(expPar);
               // save the time when the packet has been served for the first time (useful for per class extended service time)
               src_id=msgServiced->getSourceId();
               EV<<"Starting service of "<<id<<" coming from user ID "<<src_id<<endl;
               //serviceTime = exponential(expPar); //defines the service time
               scheduleAt(simTime()+expPar, endServiceMsg);
               //EV<<"serviceTime= "<<expPar<<endl;
               }
               else{
                   //ovviamente non ce load balancing tra le varie machine ancora
                   probe->setHasEnded(false);  //set message priority
                   probe->setProbing(true);
                   probe->setProbed(false);
                   probe->setResidualTime(SIMTIME_ZERO); //initialize to the partial elaboration done of a packet; will be useful for server utilization signal and preemptive resume
                   probe->setJobId(0); //will be useful for computing the per class extended service time
                   probe->setSourceId(0);  //initialize to 0 the time when a packet goes for he first time to service(useful for extended per class service time)
                   probe->setActualExecId(msg->getOriginalExecId());
                   probe->setOriginalExecId(msg->getOriginalExecId());
                   for(i=0;i<E;i++){
                       msg_check *query= probe->dup();
                       send(query,"load_send",i); //in caso di guasti???
                       }
                   probe = nullptr;
                   //io sono stato interrogato qui---posso mettere i pacchetti in coda e aspettare le risposte senza fare altro
                   EV<<"QUEUE msg ID "<<id<<" coming from user ID "<<src_id<<" goes in the queue of the machine ID"<<msg->getOriginalExecId()<<endl;
                   queue.insert(msg);
               }
       }
       else{//sono stato interrogato per sapere lo stato della mia coda---setProbed=true---salvo da quaclhe parte il minimo poi decido se reinstradare poi i da E diventa 0 cosi posso rifare questo giochino

       }

   }



}
