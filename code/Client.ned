simple Client
{
    parameters:
        @display("i=block/source");
        volatile double sendingTime @unit(s); 
        volatile double jobComplexity @unit(s);
        double channelDelay @unit(s);
        double timeoutAck @unit(s);
        double timeoutStatus @unit(s);
        int E;
        int maxRetry;
        
        // statistics for the sending rate
        @signal[avgSendingRate](type="simtime_t");
        @statistic[avgSendingRate](title="SENDING RATE";record=vector, timeavg;interpolationmode=sample-hold);
      	
      	// statistics for the service time client side
      	@signal[realTime](type="simtime_t");
        @statistic[realTime](title="REAL COMPUTATION TIME";record=vector, timeavg;interpolationmode=sample-hold);
      	
      	// statistics for execution time for the jobs
  	    @signal[avgComplexity](type="simtime_t");
        @statistic[avgComplexity](title="JOB COMPLEXITY";record=vector, timeavg;interpolationmode=sample-hold);
      	
    gates:
         // E bidirectional links which each of the executors
        inout links[];
}