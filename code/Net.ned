network Net
{
    parameters:
        int N;
        int E;
        @display("bgb=718,496");
        @figure[description](type=text; pos=5,20; font=,,bold; text="Distributed job scheduling");

    submodules:
        gen[N]: Source {
            parameters:
                @display("p=68,157,c,70");
            gates:
                user[E];

        }

       machine[E]: Queue {
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
        backup:Storage{
            parameters:
                @display("p=600,80,c,90");
            gates:
            	backup_rec[E];
        
        }
    connections:
		for i=0..N-1, for j=0..E-1 {
            // gen[i].out --> queue.in++;
            gen[i].user[j] <--> machine[j].exec[i];

        }
        for k=0..E-1 {
            // gen[i].out --> queue.in++;
            machine[k].backup_send <--> backup.backup_rec[k];

        }
        for k=0..E-1,for r=0..E-1{
            // gen[i].out --> queue.in++;

            machine[k].load_send[r]-->  machine[r].load_receive[k];

        }

}
