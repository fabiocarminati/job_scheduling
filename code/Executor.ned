simple Executor
{    
    parameters:

		@display("i=block/queue");
		int N;
        int E;
        double probEvent;
        double probCrashDuringExecution;
		double channelDelay @unit(s);
		double timeoutLoad @unit(s);
        double timeoutFailure @unit(s);
        double timeoutEndActual @unit(s);
// GENERAL STATISTICS

		// Busy signal
       // @signal[busy](type="bool");
        //@statistic[busy](title="Server busy state";record=vector?,timeavg;interpolationmode=sample-hold);

		// Average queueing 
       // @signal[genericQueueingTime](type="simtime_t");
      //  @statistic[genericQueueingTime](title="Overall queueing time";unit=s;record=vector,mean,max;interpolationmode=none);
        
		// Average response time 
    //    @signal[genericResponseTime](type="simtime_t");
    //    @statistic[genericResponseTime](title="Overall response time at end of service";unit=s;record=vector,mean,max;interpolationmode=none);

        // Average queue length
    //    @signal[genericAvgQueueLength](type="long");
    //    @statistic[genericAvgQueueLength](title="Overall average queue length";record=vector, timeavg;interpolationmode=sample-hold);
        
// PER-CLASS STATISTICS

    	// Average queueing time
   // 	@signal[queueingTime*](type="simtime_t");
    //	@statisticTemplate[queueingTimeTemplate](title="Queueing time at the queue";unit=s;record=vector,mean;interpolationmode=none);
    	
    	// Average response time
    //	@signal[responseTime*](type="simtime_t");
    	//@statisticTemplate[responseTimeTemplate](title="Response time at end of service";unit=s;record=vector,mean,max;interpolationmode=none);

    	// Extended average service time
    	// When messages are inserted back in the queue in the case of preemption, it still counts as service time 
    	//@signal[extendedServiceTime*](type="simtime_t");
    //	@statisticTemplate[extendedServiceTimeTemplate](title="Extended service time"; unit=s;record=vector,mean,max;interpolationmode=none);
    	
    	// Average queue length
    //    @signal[avgQueueLength*](type="long");
    //    @statisticTemplate[avgQueueLengthTemplate](title="Average queue length";record=vector,timeavg,max;interpolationmode=sample-hold);

    gates:
        inout exec[];
        inout backup_send;
        input load_receive[];
        output load_send[];
}