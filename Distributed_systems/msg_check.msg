packet msg_check //name of the new type of message 
{
    int JobId;
    int SourceId;
    int ExecId;
    simtime_t ResidualTime;
    bool HasEnded;
};