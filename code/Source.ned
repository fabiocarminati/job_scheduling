simple Source
{
    parameters:
        //volatile double pr=default(0);   //col default posso cambiare il parametro durante l'esecuzione
        volatile double interArrivalTime @unit(s);
        @display("i=block/source");
        int N;
        int E;
    gates:
        inout user[];
}