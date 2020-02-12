simple Executor
{    
    parameters:
		@display("i=block/queue");
        int E;
        int granularity;
        int probeResponse;
        double probEvent;
        double probCrashDuringExecution;
		double channelDelay @unit(s);
		double timeoutLoad @unit(s);
        volatile double timeoutFailure @unit(s);

    gates:
        inout exec[]; // C bidirectional links with the clients
        inout backup_send; // a bidirectional link with the storage
        // E-1 links from and to the other executors of the cluster
        input load_receive[];
        output load_send[];
}