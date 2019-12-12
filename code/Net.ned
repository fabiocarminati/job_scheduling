network Net
{
    parameters:
        int N;
        int E;
        @display("bgb=718,496");
        @figure[description](type=text; pos=5,20; font=,,bold; text="Distributed job scheduling");

    submodules:
       	client[N]: Client {
            parameters:
                @display("p=68,157,c,70");
            gates:
                user[E];

        }

       executor[E]: Executor {
            parameters:
                @display("p=354,60,c,90;q");
            gates:
            	exec[N];
            	backup_send;
            	load_receive[E];
        		load_send[E];
        }
		//decido di fare delete del msg direttamente nelle queue
        //sink: Sink {
        //    parameters:
        //        @display("p=634,245");
        //}
        backup[E]:Storage{
            parameters:
                @display("p=600,80,c,90");
            gates:
            	backup_rec;
        
        }
    connections:
		for i=0..N-1, for j=0..E-1 {
            // client[i].out --> queue.in++;
            client[i].user[j] <--> executor[j].exec[i];

        }
        for k=0..E-1 {
            // client[i].out --> queue.in++;
            executor[k].backup_send <--> backup[k].backup_rec;

        }
        for k=0..E-1,for r=0..E-1{
            // client[i].out --> queue.in++;

            executor[k].load_send[r]-->  executor[r].load_receive[k];

        }

}
