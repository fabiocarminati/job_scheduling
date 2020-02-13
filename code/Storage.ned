simple Storage
{    
    parameters:
		@display("i=block/sink");
	
        // statistics for the jobQueue length
        @signal[Job](type="double");
        @statistic[Job](title="JOB QUEUE";record=vector,timeavg;interpolationmode=sample-hold);
 
 		// statistics for the newJobsQueue length
     	@signal[NewJobs] (type="double");
        @statistic[NewJobs](title="NEW JOB QUEUE";record=vector, timeavg;interpolationmode=sample-hold);
        
        // statistics for the reRoutedQueue length
        @signal[ReRouted](type="double");
     	@statistic[ReRouted](title=" REROUTED JOB QUEUE";record=vector, timeavg;interpolationmode=sample-hold);
        
    gates:
        // a single bidirectional gate from and to his executor
        inout executorGate; 
}