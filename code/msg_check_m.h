//
// Generated file, do not edit! Created by nedtool 5.5 from msg_check.msg.
//

#ifndef __MSG_CHECK_M_H
#define __MSG_CHECK_M_H

#if defined(__clang__)
#  pragma clang diagnostic ignored "-Wreserved-id-macro"
#endif
#include <omnetpp.h>

// nedtool version check
#define MSGC_VERSION 0x0505
#if (MSGC_VERSION!=OMNETPP_VERSION)
#    error Version mismatch! Probably this file was generated by an earlier version of nedtool: 'make clean' should help.
#endif



/**
 * Class generated from <tt>msg_check.msg:2</tt> by nedtool.
 * <pre>
 * packet msg_check //name of the new type of message 
 * {
 *     int RelativeJobId;
 *     int ClientId;
 *     int OriginalExecId;
 *     int ActualExecId;
 *     simtime_t JobComplexity;
 *     int QueueLength;
 *     bool StatusRequest;
 *     bool Probing;
 *     bool ReRouted;
 *     bool Ack;
 *     bool NewJob;
 *     bool IsEnded;
 *     bool ReBoot;
 *     bool NewJobsQueue;
 *     bool JobQueue;
 *     bool ReRoutedJobQueue;
 *     bool BackupComplete;
 *     bool CompletedQueue;
 *     bool Duplicate;
 *     simtime_t StartingTime;
 *     simtime_t EndingTime;
 * }
 * </pre>
 */
class msg_check : public ::omnetpp::cPacket
{
  protected:
    int RelativeJobId;
    int ClientId;
    int OriginalExecId;
    int ActualExecId;
    ::omnetpp::simtime_t JobComplexity;
    int QueueLength;
    bool StatusRequest;
    bool Probing;
    bool ReRouted;
    bool Ack;
    bool NewJob;
    bool IsEnded;
    bool ReBoot;
    bool NewJobsQueue;
    bool JobQueue;
    bool ReRoutedJobQueue;
    bool BackupComplete;
    bool CompletedQueue;
    bool Duplicate;
    ::omnetpp::simtime_t StartingTime;
    ::omnetpp::simtime_t EndingTime;

  private:
    void copy(const msg_check& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const msg_check&);

  public:
    msg_check(const char *name=nullptr, short kind=0);
    msg_check(const msg_check& other);
    virtual ~msg_check();
    msg_check& operator=(const msg_check& other);
    virtual msg_check *dup() const override {return new msg_check(*this);}
    virtual void parsimPack(omnetpp::cCommBuffer *b) const override;
    virtual void parsimUnpack(omnetpp::cCommBuffer *b) override;

    // field getter/setter methods
    virtual int getRelativeJobId() const;
    virtual void setRelativeJobId(int RelativeJobId);
    virtual int getClientId() const;
    virtual void setClientId(int ClientId);
    virtual int getOriginalExecId() const;
    virtual void setOriginalExecId(int OriginalExecId);
    virtual int getActualExecId() const;
    virtual void setActualExecId(int ActualExecId);
    virtual ::omnetpp::simtime_t getJobComplexity() const;
    virtual void setJobComplexity(::omnetpp::simtime_t JobComplexity);
    virtual int getQueueLength() const;
    virtual void setQueueLength(int QueueLength);
    virtual bool getStatusRequest() const;
    virtual void setStatusRequest(bool StatusRequest);
    virtual bool getProbing() const;
    virtual void setProbing(bool Probing);
    virtual bool getReRouted() const;
    virtual void setReRouted(bool ReRouted);
    virtual bool getAck() const;
    virtual void setAck(bool Ack);
    virtual bool getNewJob() const;
    virtual void setNewJob(bool NewJob);
    virtual bool getIsEnded() const;
    virtual void setIsEnded(bool IsEnded);
    virtual bool getReBoot() const;
    virtual void setReBoot(bool ReBoot);
    virtual bool getNewJobsQueue() const;
    virtual void setNewJobsQueue(bool NewJobsQueue);
    virtual bool getJobQueue() const;
    virtual void setJobQueue(bool JobQueue);
    virtual bool getReRoutedJobQueue() const;
    virtual void setReRoutedJobQueue(bool ReRoutedJobQueue);
    virtual bool getBackupComplete() const;
    virtual void setBackupComplete(bool BackupComplete);
    virtual bool getCompletedQueue() const;
    virtual void setCompletedQueue(bool CompletedQueue);
    virtual bool getDuplicate() const;
    virtual void setDuplicate(bool Duplicate);
    virtual ::omnetpp::simtime_t getStartingTime() const;
    virtual void setStartingTime(::omnetpp::simtime_t StartingTime);
    virtual ::omnetpp::simtime_t getEndingTime() const;
    virtual void setEndingTime(::omnetpp::simtime_t EndingTime);
};

inline void doParsimPacking(omnetpp::cCommBuffer *b, const msg_check& obj) {obj.parsimPack(b);}
inline void doParsimUnpacking(omnetpp::cCommBuffer *b, msg_check& obj) {obj.parsimUnpack(b);}


#endif // ifndef __MSG_CHECK_M_H

