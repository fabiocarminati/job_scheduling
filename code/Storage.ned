simple Storage
{    
    parameters:
	//	string schedulingPolicy;
		//volatile double defServiceTime  @unit(s); 
		    @display("i=block/sink");
		//int N;
        int E;
		
        // Average queue length
        @signal[avgJob](type="double");
        @statistic[avgJob](title="JOB QUEUE";record=vector,timeavg;interpolationmode=sample-hold);
        
                // Average queue length
        @signal[whyNotWork](type="double");
        @statistic[whyNotWork](title="JOB QUEUE";record=vector,timeavg;interpolationmode=sample-hold);
        
      
     	@signal[avgNew] (type="double");
        @statistic[avgNew](title="NEW JOB QUEUE";record=vector, timeavg;interpolationmode=sample-hold);
        
        //@signal[avgReRouted](type="double");
     //	@statistic[avgReRouted](title=" REROUTED JOB QUEUE";record=vector, timeavg;interpolationmode=sample-hold);
        
   //     @signal[avgCompleted](type="double");
  //     	@statistic[avgCompleted](title="COMPLETED JOB QUEUE";record=vector, timeavg;interpolationmode=sample-hold);
    gates:
        inout backup_rec;
}