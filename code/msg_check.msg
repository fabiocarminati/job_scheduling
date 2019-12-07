packet msg_check //name of the new type of message 
{
    string JobId;
    int SourceId;
    int OriginalExecId;
    int ActualExecId;
    simtime_t ResidualTime;
    bool HasEnded;
    int QueueLength;
    bool Probing;
    bool Probed;
};

//packet msg_backup extends msg_check //name of the new type of message 
//{
   
//};