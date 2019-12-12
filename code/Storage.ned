simple Storage
{    
    parameters:
	//	string schedulingPolicy;
		//volatile double defServiceTime  @unit(s); 
		    @display("i=block/sink");
		//int N;
        int E;

    gates:
        inout backup_rec;
}