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
                @display("p=354,245,c,90");
            gates:
            	exec[N];
        }
		//decido di fare delete del msg direttamente nelle queue
        //sink: Sink {
        //    parameters:
        //        @display("p=634,245");
        //}
    connections:
		for i=0..N-1, for j=0..E-1 {
            // gen[i].out --> queue.in++;
            gen[i].user[j] <--> machine[j].exec[i];

        }


}
