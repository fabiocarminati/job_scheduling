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
                executorGate[E];
        }
        
        executor[E]: Executor {
            parameters:
                @display("p=327,191,ri"); //display executors of the cluster as a circle
            gates:
                clientGate[C];
                storageGate;
                executorGate[E];
        }

        storage[E]: Storage {
            parameters:
                @display("p=150,90,m,90");
            gates:
                executorGate;
        }
        
    connections:
        //create the links amid clients and executors considering the transmission time over the channel
        for i=0..C-1, for j=0..E-1 {
            client[i].executorGate[j] <--> {delay = channelDelay;} <--> executor[j].clientGate[i];
        }
        
        //create the links amid storage and executor considering the transmission time over the storage channel
        for k=0..E-1 {
            executor[k].storageGate <--> {delay = channelStorageDelay;} <--> storage[k].executorGate;
        }
        
        //create the links amid executors considering the transmission time over the channel
        for k=0..E-1 {
              executor[k].executorGate$o[k] --> {delay = channelDelay;} --> executor[k].executorGate$i[k];
        }
        
        for k=0..E-1 , for r=k+1..E-1 {
              executor[k].executorGate[r] <--> {delay = channelDelay;} <--> executor[r].executorGate[k];
        }

}
