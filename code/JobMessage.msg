// we define a new msg type with several fields
packet JobMessage 
{
    unsigned int RelativeJobId; // represents the relative identificator respect to the OriginalExecId, 
                                // number of new jobs arrived to this executor up to now
    unsigned int ClientId;      //Client identification number relative to the client that generates the job
    unsigned int OriginalExecId; // executor that has received the job from the client
    unsigned int ActualExecId; // executor that processes the job
                               // (can be different from original exec due to load balancing)
    simtime_t JobComplexity;  // computation time for that job
    int QueueLength;	// contains the jobQueue length of the executor during the load balancing process
    bool StatusRequest; // true for during the status process
    bool Probing;  // true during load balancing process
    bool ReRouted; //true for the jobs that have been reRouted due to load balancing
    bool Ack;      // true if the message represents a repsonse to a previous request
    bool NewJob; // true if the job hasn't assigned a jobId yet
    bool IsEnded; // true if the job has been processed
   	bool ReBoot; // true when the executor is in the Reboot Mode
    bool NewJobsQueue; // true if the job is inside the newJobsQueue
    bool JobQueue; // true if the job is inside the jobQueue
    bool ReRoutedJobQueue; // true if the job is inside the reRoutedJobs
    bool BackupComplete; // true if the backup process from the storage is over
    bool CompletedQueue; // true if the job is inside the completedJob
    bool Duplicate; // true if the job is found in one of the queues of the executor
    simtime_t StartingTime; // time instant when the job is sent to the executor for the first time
    simtime_t EndingTime; // time instant when the job is received as processed in the original client
};