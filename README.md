# DISTRIBUTED JOB SCHEDULING

We have used Omnet++ to implement and model an infrastructure to manage jobs submitted to a cluster of Executors.

## Assumptions:
The channels between the various entities are bi-directional and it’s assumed they are reliable. In particular, it’s assumed TCP like connections when there is a channel
So, unicast communication is possible between entities.

All the various parameter of the simulation (job complexity, timeouts, delay in the channels, ...) can be modified by the omnetpp.ini file. 		
An exponential distribution is considered for the sending rate, timeout failure and job complexity in order to avoid deterministic values such that a more realistic behaviour of the system can be obtained.
Thus for these 3 parameters only the mean is specified.

## Entities:
There are mainly four different entities in the system implemented with different classes:
<ol>
<li>Job </li>
<li>Executor </li>
<li>Storage </li>
<li>Client </li>
</ol>

### Jobs:
The Jobs are represented in the system by the JobMessage.
JobMessages are exchanged amid the various entities in the system and contains all the essential information to represent a Job:
<ul>
<li> RelativeJobId  </li>
<li> ClientId   </li>
<li> OriginalExecId   </li> 
<li> ActualExecId </li>
<li> JobComplexity </li>
<li> ReRouted </li>
<li> NewJob </li>  
<li> IsEnded </li>
<li> StartingTime </li>
<li> EndingTime </li>
</ul>


### Executor:
The Executors execute the received jobs from the Clients in a distributed way.

The Executors communicate with:
<ul>
<li> Clients (status and job requests)  </li>
<li> Storages (send and receive internal status)  </li>
<li> Others Executors (load balancing operations and status)  </li>
</ul>

The Executor is the only entity that can fail in the system. 
Only crash are allowed (losing the partial computation of a job and losing internal state),No Byzantine and timing failures.

Can be in three main different modes:
<ol>
<li> Normal Mode 
<li> Failure Mode:Ignoring all the incoming packets until timeoutFailureDuration expires </li> 
<li> Reboot Mode: Ignoring all the incoming packets except those from storage </li> 
</ol>

The Executors can crash (with different probabilities) either at the reception of each message or in the middle of computation.


### Storage:
Each Executor has its own Storage connected.
Each storage save permanently and in a reliable way the internal status of the executor attached to it.

The Storage CANNOT crash.
The Storages save the jobs in four different std::map with a simple protocol:	
<ol>
<li> Insert operation: If a job arrives for a specific map and it doesn’t exist on it, the storage saves the job in the specified map </li>
<li> Delete operation: If a job already exists in a specific map, the storage delete the job from that map </li>
</ol>

### Client:

Clients communicate only with the Executors.
Send new jobs periodically (Executor is random selected) and then they are added to notComputed  cArray.
Request info about the status of the Jobs (completed or not completed).

Each Client is completely unaware if load balancing is performed or not  for each Job (e.g. who is the the effective Executor, ActualExecId).

If an Executor doesn’t respond to a new Job request with a JobId for a given period the job is resent for a given amount of times, after that another Executor is selected for that Job.

Periodically the Clients ask the status of the Jobs to the respective Executors (OriginalExecId), also they move all the Jobs to noStatusInfo  cArray (assuming that there was a crash processing that job).

When there is a status response, if the Job is completed it is removed and notified, otherwise it is added to notComputed such that it will be re-asked later.

## Statistics
In order to both understand whether the system behaves as expected and to monitor system performances we introduce some signals:
In the Client : 
<ol>
<li> avgSendingRateSignal for the sending rate of the Jobs </li>
<li> avgComplexitySignal for the processing time of a Job </li>
<li> realTimeSignal for service time for each Job. From the moment in which the Job is sent to original the executor for the first time up to the time in which the client receives the completed status message </li>
</ol>

In the Storage :
<ol>
<li> JobSignal for the evolution of the jobQueue length </li>
<li> NewSignal for the evolution over time of the newJobsQueue length </li>
<li> ReRoutedSignal for the evolution over time of the reRoutedQueue length </li>	
</ol>

## Results
Considering these simulation parameters:

We obtain the following results:

# License
This project is licensed under the MIT License - see the LICENSE.md file for details
