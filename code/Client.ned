simple Client
{
    parameters:
        //volatile double pr=default(0);   //col default posso cambiare il parametro durante l'esecuzione
        volatile double interArrivalTime @unit(s);
        volatile double jobComplexity @unit(s);
        double channelDelay @unit(s);
        double timeoutAck @unit(s);
        double timeoutStatus @unit(s);
        @display("i=block/source");
        int E;
        int maxRetry;
        @signal[avgSendingRate](type="simtime_t");
        @statistic[avgSendingRate](title="INTERARRIVAL RATE";record=vector, timeavg;interpolationmode=sample-hold);
      	
  	    @signal[avgComplexity](type="simtime_t");
        @statistic[avgComplexity](title="JOB COMPLEXITY";record=vector, timeavg;interpolationmode=sample-hold);
      	
    gates:
        inout user[];
}