packet msg_check //name of the new type of message 
{
    int RelativeJobId;
    int ClientId;
    int OriginalExecId;
    int ActualExecId;
    simtime_t JobComplexity;
    int QueueLength;
    bool HasEnded;
    bool Probing;
    bool ReRouted;
    bool Ack;
    bool NewJob;  
    bool checkPercentageWork;
   	bool ReBoot;
    bool NewJobsQueue;
    bool JobQueue;
    bool ReRoutedJobQueue;
    bool BackupComplete;
};