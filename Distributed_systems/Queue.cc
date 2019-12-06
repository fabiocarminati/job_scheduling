#include <string.h>
#include <omnetpp.h>
#include <msg_check_m.h>  //fare parsing
using namespace omnetpp;
class Queue : public cSimpleModule {
private:
    msg_check *msgServiced; // message being served
    msg_check *endServiceMsg;
    simtime_t defServiceTime;
    simtime_t expPar;
    int src_id,E,N,port_id;
    double job_id;
protected:
    cQueue queue;
virtual void initialize() override;
virtual void handleMessage(cMessage *msg) override;
};
Define_Module(Queue);

void Queue::initialize() {
    int i;
    E = par("E"); //non volatile parameters --once defined they never change
    N= par("N");
 // The exponential value is computed in the handleMessage; Set the service time as exponential
    expPar=exponential(par("defServiceTime").doubleValue());
    msgServiced = endServiceMsg = nullptr;
    endServiceMsg = new msg_check("end-service");
   // for(i=0; i<E; i++){
      //  qname="executor_id=";
     //   qname.append(std::to_string(i).c_str());
   //     queue[i].setName(std::to_string(i).c_str());
//



        // Average (single) queue length
        // Save the name of the i-th signal in the array of avg queue lengths
        //sprintf(avgQueueLength, "avgQueueLength%d", i);
        // Register the signal with the name set above
        //simsignal_t avgQueueLengthSignal = registerSignal(avgQueueLength);
        // Save the relative template properties into *statisticTemplate
       // cProperty *statisticTemplateAQL = getProperties()->get("statisticTemplate", "avgQueueLengthTemplate");
        // Adds result recording listeners for the given signal on the given component (see addResultRecorders())
       // getEnvir()->addResultRecorders(this, avgQueueLengthSignal, avgQueueLength,  statisticTemplateAQL);
        // Assign the signal to the i-th cell of the vector of corresponding signals
       // avgQueueLengthSignals[i] = avgQueueLengthSignal;
    //}
}
void Queue::handleMessage(cMessage *cmsg) {
    // Casting from cMessage to msg_check
   msg_check *msg = check_and_cast<msg_check *>(cmsg);
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
         EV<<"Empty queue, the machine "<<msgServiced->getExecId()<<" goes IDLE"<<endl;
         msgServiced = nullptr;
        //emit(busySignal, false);    // Magari puo servire
         }
              else{ // at least one queue contains users
                  // i has the value of the highest priority
                  msgServiced = (msg_check *)queue.pop(); //remove the first element of that queue(FIFO policy)
                  //workDone = msgServiced->getAlreadyDone(); //recover the partial service time execution previously done
                  msgServiced->setResidualTime(expPar);  //attenzione che e volatile magari in schedule at e ricalcolato con un diverso valore
                  job_id= msgServiced->getJobId();
                  src_id=msgServiced->getSourceId();
                  EV<<"Starting service of "<<job_id<<" coming from user ID "<<src_id<<" from the queue of the machine "<<msgServiced->getExecId()<<endl;
                  //serviceTime = exponential(expPar); //defines the service time
                  scheduleAt(simTime()+expPar, endServiceMsg);
                  //EV<<"serviceTime= "<<expPar<<endl;
              }
       }

   else{
       if (!msgServiced){      // Server is IDLE, there's no message in service:execute the one that has arrived right now or put it in the queue
           EV<<"EMPTY queue immediate service "<<endl;
           // Direct service
           msgServiced = msg; //given that the server is idle the arrived message is immediately served despite his priority
           msgServiced->setResidualTime(expPar);
           // save the time when the packet has been served for the first time (useful for per class extended service time)
           job_id= msgServiced->getJobId();
           src_id=msgServiced->getSourceId();
           EV<<"Starting service of "<<job_id<<" coming from user ID "<<src_id<<endl;
           //serviceTime = exponential(expPar); //defines the service time
           scheduleAt(simTime()+expPar, endServiceMsg);
           //EV<<"serviceTime= "<<expPar<<endl;
           }
           else{
               queue.insert(msg);  //ovviamente non ce load balncing tra le varie machine ancora
               EV<<"QUEUE msg ID "<<job_id<<" coming from user ID "<<src_id<<" goes in the queue of the machine ID"<<msg->getExecId()<<endl;
               }


      //queueingTime = msgServiced->getStartingTime() - msgServiced->getTimestamp() - msgServiced->getAlreadyDone();
     // emit(queueingTimeSignals[priority], queueingTime);
     // emit(genericQueueingTimeSignal, queueingTime);
     // EV<<"queueingTime = "<<queueingTime<<endl;
       }



}
