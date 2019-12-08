packet msg_check //name of the new type of message 
{
    string JobId;
    int SourceId;
    int OriginalExecId;
    int ActualExecId;
    simtime_t ResidualTime;
    int QueueLength;
    bool HasEnded;
    bool Probing;
    bool Probed;
    bool ReRouted;
};

//packet msg_backup extends msg_check //name of the new type of message 
//{
   
//};