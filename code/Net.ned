network Net
{
    parameters:
        int N;
        int E;
        double channelDelay @unit(s);
      
        @display("bgb=718,496");
        @figure[description](type=text; pos=5,20; font=,,bold; text="Distributed job scheduling");

    submodules:
        client[N]: Client {
            parameters:
                @display("p=100,200,c;");
            gates:
                user[E];

        }

        executor[E]: Executor {
            parameters:
                @display("p=327,191,ri"); //display elements in circle(see section 24 of the manual for the details
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
        backup[E]: Storage {
            parameters:
                @display("p=150,90,m,90");
            gates:
                backup_rec;

        }
    connections:
        for i=0..N-1, for j=0..E-1 {
            // client[i].out --> queue.in++;
            client[i].user[j] <--> {delay = channelDelay;} <--> executor[j].exec[i];

        }
        for k=0..E-1 {
            // client[i].out --> queue.in++;
            executor[k].backup_send <--> {delay = channelDelay;} <--> backup[k].backup_rec;

        }
        for k=0..E-1, for r=0..E-1 {
            executor[k].load_send[r] --> {delay = channelDelay;} --> executor[r].load_receive[k];

        }

}
