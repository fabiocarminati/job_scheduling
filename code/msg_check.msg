packet msg_check //name of the new type of message 
{
    string JobId;
    int ClientId;
    int OriginalExecId;
    int ActualExecId;
    simtime_t ResidualTime;
    int QueueLength;
    bool HasEnded;
    bool Probing;
    bool ReRouted;
    bool Ack;
    bool NewJob;
    bool ReBoot;
    bool checkPercentageWork;
};