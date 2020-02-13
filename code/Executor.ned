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
        double timeoutFailure @unit(s);

    gates:
        inout clientGate[]; // C bidirectional links with the clients
        inout storageGate; // a bidirectional link with the storage
        // E-1 links from and to the other executors of the cluster
        inout executorGate[];
}