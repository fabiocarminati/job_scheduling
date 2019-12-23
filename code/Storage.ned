simple Storage
{    
    parameters:
	//	string schedulingPolicy;
		//volatile double defServiceTime  @unit(s); 
		    @display("i=block/sink");
		//int N;
        int E;
		
        // Average queue length
        @signal[avgJobQueueLength](type="long");
        @statistic[avgJobQueueLength](title="JOB QUEUE average queue length";record=vector, timeavg;interpolationmode=sample-hold);
      
      	@signal[avgNewJobQueueLength](type="long");
        @statistic[avgNewJobQueueLength](title="NEW JOB QUEUE average queue length";record=vector, timeavg;interpolationmode=sample-hold);
        
        @signal[avgReRoutedJobQueueLength](type="long");
        @statistic[avgReRoutedJobQueueLength](title=" REROUTED JOB QUEUE average queue length";record=vector, timeavg;interpolationmode=sample-hold);
        
        @signal[avgCompletedJobQueueLength](type="long");
        @statistic[avgCompletedJobQueueLength](title="COMPLETED JOB QUEUE average queue length";record=vector, timeavg;interpolationmode=sample-hold);
    gates:
        inout backup_rec;
}