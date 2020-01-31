//we define a new msg type with several fields
packet msg_check 
{
    unsigned int RelativeJobId;
    unsigned int ClientId;
    unsigned int OriginalExecId;
    unsigned int ActualExecId;
    simtime_t JobComplexity;
    int QueueLength;
    bool StatusRequest;
    bool Probing;
    bool ReRouted;
    bool Ack;
    bool NewJob;  
    bool IsEnded;
   	bool ReBoot;
    bool NewJobsQueue;
    bool JobQueue;
    bool ReRoutedJobQueue;
    bool BackupComplete;
    bool CompletedQueue;
    bool Duplicate;
    simtime_t StartingTime;
    simtime_t EndingTime;
};