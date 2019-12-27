simple Storage
{    
    parameters:
	//	string schedulingPolicy;
		//volatile double defServiceTime  @unit(s); 
		    @display("i=block/sink");
		//int N;
        int E;
		
        // Average queue length
        @signal[Job](type="double");
        @statistic[Job](title="JOB QUEUE";record=vector,timeavg;interpolationmode=sample-hold);
        
                // Average queue length
       // @signal[whyNotWork](type="double");
      //  @statistic[whyNotWork](title="JOB QUEUE";record=vector,timeavg;interpolationmode=sample-hold);
        
      
     	@signal[NewJobs] (type="double");
        @statistic[NewJobs](title="NEW JOB QUEUE";record=vector, timeavg;interpolationmode=sample-hold);
        
        @signal[ReRouted](type="double");
     	@statistic[ReRouted](title=" REROUTED JOB QUEUE";record=vector, timeavg;interpolationmode=sample-hold);
        
        @signal[Completed](type="double");
       	@statistic[Completed](title="COMPLETED JOB QUEUE";record=vector, timeavg;interpolationmode=sample-hold);
    gates:
        inout backup_rec;
}