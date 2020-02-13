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
                executor[E];
        }
        
        executor[E]: Executor {
            parameters:
                @display("p=327,191,ri"); //display executors of the cluster as a circle
            gates:
                client[C];
                backup;
                executor[E];
        }

        backup[E]: Storage {
            parameters:
                @display("p=150,90,m,90");
            gates:
                executor;
        }
        
    connections:
        //create the links amid clients and executors considering the transmission time over the channel
        for i=0..C-1, for j=0..E-1 {
            client[i].executor[j] <--> {delay = channelDelay;} <--> executor[j].client[i];
        }
        
        //create the links amid storage and executor considering the transmission time over the storage channel
        for k=0..E-1 {
            executor[k].backup <--> {delay = channelStorageDelay;} <--> backup[k].executor;
        }
        
        //create the links amid executors considering the transmission time over the channel
        for k=0..E-1 {
              executor[k].executor$o[k] --> {delay = channelDelay;} --> executor[k].executor$i[k];
        }
        
        for k=0..E-1 , for r=k+1..E-1 {
              executor[k].executor[r] <--> {delay = channelDelay;} <--> executor[r].executor[k];
        }

}
