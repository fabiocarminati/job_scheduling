simple Storage
{    
    parameters:
		@display("i=block/sink");
		
        int E;
		double channelDelay @unit(s);
		
        // Average map length
        @signal[Job](type="double");
        @statistic[Job](title="JOB QUEUE";record=vector,timeavg;interpolationmode=sample-hold);
 
     	@signal[NewJobs] (type="double");
        @statistic[NewJobs](title="NEW JOB QUEUE";record=vector, timeavg;interpolationmode=sample-hold);
        
        @signal[ReRouted](type="double");
     	@statistic[ReRouted](title=" REROUTED JOB QUEUE";record=vector, timeavg;interpolationmode=sample-hold);
        
    gates:
        inout backup_rec;
}