network Net
{
    parameters:
        int C;
        int E;
        double channelDelay @unit(s);
        double channelStorageDelay @unit(s);
      
        @display("bgb=718,496");
        @figure[description](type=text; pos=5,20; font=,,bold; text="Distributed job scheduling");

    submodules:
        client[C]: Client {
            parameters:
                @display("p=100,200,c;");
            gates:
                links[E];
        }
        
        executor[E]: Executor {
            parameters:
                @display("p=327,191,ri"); //display executors of the cluster as a circle
            gates:
                exec[C];
                backup_send;
                load_receive[E];
                load_send[E];
        }

        backup[E]: Storage {
            parameters:
                @display("p=150,90,m,90");
            gates:
                backup_rec;
        }
        
    connections:
        //create the links amid clients and executors considering the transmission time over the channel
        for i=0..C-1, for j=0..E-1 {
            client[i].links[j] <--> {delay = channelDelay;} <--> executor[j].exec[i];
        }
        
        //create the links amid storage and executor considering the transmission time over the storage channel
        for k=0..E-1 {
            executor[k].backup_send <--> {delay = channelStorageDelay;} <--> backup[k].backup_rec;
        }
        
        //create the links amid executors considering the transmission time over the channel
        for k=0..E-1, for r=0..E-1{
            executor[k].load_send[r] --> {delay = channelDelay;} --> executor[r].load_receive[k];
        }

}
