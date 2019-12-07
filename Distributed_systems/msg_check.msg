packet msg_check //name of the new type of message 
{
    int JobId;
    int SourceId;
    int OriginalExecId;
    int ActualExecId;
    simtime_t ResidualTime;
    bool HasEnded;
};

//packet msg_backup extends msg_check //name of the new type of message 
//{
   
//};