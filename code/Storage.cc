#include <string.h>
#include <omnetpp.h>
#include <msg_check_m.h>  //fare parsing
//#include <msg_backup_m.h>
#include <map>
//#include <iostream>
#define SIZE 1000000

struct msg_info{
    int source_id,actual_exec,original_exec;
    const char * job_id;
    bool ended,rerouted;
    //simtime_t residual_time;

};

using namespace omnetpp;
class Storage : public cSimpleModule {
private:
    //msg_check *msgCopy; // send message copy in case of failure
    msg_check *saveMsg;
   // msg_backup *storedMsg;
    msg_info coming_msg;

    std::map<const char *,msg_info> storedMsg;
    //map<int,int,int,int,int,simtime_t,bool> storedMsg;
    //std::map<int,int,int,int,int,simtime_t,bool> storedMsg;  //key is an int type, the value referenced by the key is a msg_backup message
    std::map<const char *,msg_info>::iterator search;  //INIZIALIZZARLo!!!!
    simtime_t expPar,residual_time;
    int E;
    //job_id,source_id,actual_exec,original_exec;
    double occupation;
    bool ended;



protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *cmsg) override;
};
Define_Module(Storage);

void Storage::initialize() {
    occupation=0;
    //storedMsg=new msg_backup[SIZE];
    E = par("E"); //non volatile parameters --once defined they never change
 // The exponential value is computed in the handleMessage; Set the service time as exponential
    //expPar=exponential(par("defServiceTime").doubleValue());
    //msgServiced = endServiceMsg = nullptr;
    //endServiceMsg = new msg_check("end-service");
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
void Storage::handleMessage(cMessage *cmsg) {
    // Casting from cMessage to msg_check
   msg_check *msg = check_and_cast<msg_check *>(cmsg);
   coming_msg.job_id=msg->getJobId();
   coming_msg.source_id=msg->getSourceId();
   coming_msg.actual_exec=msg->getActualExecId();
   coming_msg.original_exec=msg->getOriginalExecId();
   //residual_time=msg->getResidualTime();
   coming_msg.ended=msg->getHasEnded();
   coming_msg.rerouted=msg->getReRouted();
  // int,int,int,int,int,simtime_t,bool
   //for(i=0;i<occupation;i++){
   //auto search=storedMsg.find(job_id);
   search=storedMsg.find(coming_msg.job_id);
   if (search != storedMsg.end()){ //a key is found(the msg has already been inserted in the storage)
   //if(job_id==i->getJobId){
       storedMsg.erase(coming_msg.job_id);
       EV<<"job id "<<coming_msg.job_id<<endl;
       if(coming_msg.rerouted==true){
           storedMsg.insert({coming_msg.job_id,coming_msg});
           EV<<"Due to load balancing a change in the machine is performed for "<<coming_msg.job_id<< " and this is notified to the secure storage "<<endl;
           EV<<"Original  "<<coming_msg.original_exec<<" and new actual "<<coming_msg.actual_exec<<endl;

           //storedMsg.insert({job_id,source_id,actual_exec,original_exec,residual_time,ended});
           //load balancing performed
           //storedMsg.insert( std::array<int,int,int,int,int,simtime_t,bool>(job_id,source_id,actual_exec,original,exec,residual_time,ended));
           //storedMsg.at(job_id)=msg; //replace the message(basically update actual exec)
           }

       }
   else{
       storedMsg.insert({coming_msg.job_id,coming_msg});
       EV<<"New element with ID "<<coming_msg.job_id<< " added in the secure storage "<<endl;

       //storedMsg.insert( std::array<int,int,int,int,int,simtime_t,bool>(job_id,source_id,actual_exec,original,exec,residual_time,ended));
       //storedMsg.insert(struct<int,msg_backup> (job_id,msg)); //wrong syntax
       //storedMsg.insert( std::pair<int,msg_backup>(job_id,msg) );
       }
   /*
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
       send(msg,"backup_send$o");//send a copy of backup to the storage to cope with possible failures
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

*/
   delete msg;
}
