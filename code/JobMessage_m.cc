//
// Generated file, do not edit! Created by nedtool 5.5 from JobMessage.msg.
//

// Disable warnings about unused variables, empty switch stmts, etc:
#ifdef _MSC_VER
#  pragma warning(disable:4101)
#  pragma warning(disable:4065)
#endif

#if defined(__clang__)
#  pragma clang diagnostic ignored "-Wshadow"
#  pragma clang diagnostic ignored "-Wconversion"
#  pragma clang diagnostic ignored "-Wunused-parameter"
#  pragma clang diagnostic ignored "-Wc++98-compat"
#  pragma clang diagnostic ignored "-Wunreachable-code-break"
#  pragma clang diagnostic ignored "-Wold-style-cast"
#elif defined(__GNUC__)
#  pragma GCC diagnostic ignored "-Wshadow"
#  pragma GCC diagnostic ignored "-Wconversion"
#  pragma GCC diagnostic ignored "-Wunused-parameter"
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#  pragma GCC diagnostic ignored "-Wsuggest-attribute=noreturn"
#  pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif

#include <iostream>
#include <sstream>
#include "JobMessage_m.h"

namespace omnetpp {

// Template pack/unpack rules. They are declared *after* a1l type-specific pack functions for multiple reasons.
// They are in the omnetpp namespace, to allow them to be found by argument-dependent lookup via the cCommBuffer argument

// Packing/unpacking an std::vector
template<typename T, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::vector<T,A>& v)
{
    int n = v.size();
    doParsimPacking(buffer, n);
    for (int i = 0; i < n; i++)
        doParsimPacking(buffer, v[i]);
}

template<typename T, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::vector<T,A>& v)
{
    int n;
    doParsimUnpacking(buffer, n);
    v.resize(n);
    for (int i = 0; i < n; i++)
        doParsimUnpacking(buffer, v[i]);
}

// Packing/unpacking an std::list
template<typename T, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::list<T,A>& l)
{
    doParsimPacking(buffer, (int)l.size());
    for (typename std::list<T,A>::const_iterator it = l.begin(); it != l.end(); ++it)
        doParsimPacking(buffer, (T&)*it);
}

template<typename T, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::list<T,A>& l)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i=0; i<n; i++) {
        l.push_back(T());
        doParsimUnpacking(buffer, l.back());
    }
}

// Packing/unpacking an std::set
template<typename T, typename Tr, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::set<T,Tr,A>& s)
{
    doParsimPacking(buffer, (int)s.size());
    for (typename std::set<T,Tr,A>::const_iterator it = s.begin(); it != s.end(); ++it)
        doParsimPacking(buffer, *it);
}

template<typename T, typename Tr, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::set<T,Tr,A>& s)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i=0; i<n; i++) {
        T x;
        doParsimUnpacking(buffer, x);
        s.insert(x);
    }
}

// Packing/unpacking an std::map
template<typename K, typename V, typename Tr, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::map<K,V,Tr,A>& m)
{
    doParsimPacking(buffer, (int)m.size());
    for (typename std::map<K,V,Tr,A>::const_iterator it = m.begin(); it != m.end(); ++it) {
        doParsimPacking(buffer, it->first);
        doParsimPacking(buffer, it->second);
    }
}

template<typename K, typename V, typename Tr, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::map<K,V,Tr,A>& m)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i=0; i<n; i++) {
        K k; V v;
        doParsimUnpacking(buffer, k);
        doParsimUnpacking(buffer, v);
        m[k] = v;
    }
}

// Default pack/unpack function for arrays
template<typename T>
void doParsimArrayPacking(omnetpp::cCommBuffer *b, const T *t, int n)
{
    for (int i = 0; i < n; i++)
        doParsimPacking(b, t[i]);
}

template<typename T>
void doParsimArrayUnpacking(omnetpp::cCommBuffer *b, T *t, int n)
{
    for (int i = 0; i < n; i++)
        doParsimUnpacking(b, t[i]);
}

// Default rule to prevent compiler from choosing base class' doParsimPacking() function
template<typename T>
void doParsimPacking(omnetpp::cCommBuffer *, const T& t)
{
    throw omnetpp::cRuntimeError("Parsim error: No doParsimPacking() function for type %s", omnetpp::opp_typename(typeid(t)));
}

template<typename T>
void doParsimUnpacking(omnetpp::cCommBuffer *, T& t)
{
    throw omnetpp::cRuntimeError("Parsim error: No doParsimUnpacking() function for type %s", omnetpp::opp_typename(typeid(t)));
}

}  // namespace omnetpp


// forward
template<typename T, typename A>
std::ostream& operator<<(std::ostream& out, const std::vector<T,A>& vec);

// Template rule which fires if a struct or class doesn't have operator<<
template<typename T>
inline std::ostream& operator<<(std::ostream& out,const T&) {return out;}

// operator<< for std::vector<T>
template<typename T, typename A>
inline std::ostream& operator<<(std::ostream& out, const std::vector<T,A>& vec)
{
    out.put('{');
    for(typename std::vector<T,A>::const_iterator it = vec.begin(); it != vec.end(); ++it)
    {
        if (it != vec.begin()) {
            out.put(','); out.put(' ');
        }
        out << *it;
    }
    out.put('}');
    
    char buf[32];
    sprintf(buf, " (size=%u)", (unsigned int)vec.size());
    out.write(buf, strlen(buf));
    return out;
}

Register_Class(JobMessage)

JobMessage::JobMessage(const char *name, short kind) : ::omnetpp::cPacket(name,kind)
{
    this->RelativeJobId = 0;
    this->ClientId = 0;
    this->OriginalExecId = 0;
    this->ActualExecId = 0;
    this->JobComplexity = 0;
    this->QueueLength = 0;
    this->StatusRequest = false;
    this->Probing = false;
    this->ReRouted = false;
    this->Ack = false;
    this->NewJob = false;
    this->IsEnded = false;
    this->ReBoot = false;
    this->NewJobsQueue = false;
    this->JobQueue = false;
    this->ReRoutedJobQueue = false;
    this->BackupComplete = false;
    this->CompletedQueue = false;
    this->Duplicate = false;
    this->StartingTime = 0;
    this->EndingTime = 0;
}

JobMessage::JobMessage(const JobMessage& other) : ::omnetpp::cPacket(other)
{
    copy(other);
}

JobMessage::~JobMessage()
{
}

JobMessage& JobMessage::operator=(const JobMessage& other)
{
    if (this==&other) return *this;
    ::omnetpp::cPacket::operator=(other);
    copy(other);
    return *this;
}

void JobMessage::copy(const JobMessage& other)
{
    this->RelativeJobId = other.RelativeJobId;
    this->ClientId = other.ClientId;
    this->OriginalExecId = other.OriginalExecId;
    this->ActualExecId = other.ActualExecId;
    this->JobComplexity = other.JobComplexity;
    this->QueueLength = other.QueueLength;
    this->StatusRequest = other.StatusRequest;
    this->Probing = other.Probing;
    this->ReRouted = other.ReRouted;
    this->Ack = other.Ack;
    this->NewJob = other.NewJob;
    this->IsEnded = other.IsEnded;
    this->ReBoot = other.ReBoot;
    this->NewJobsQueue = other.NewJobsQueue;
    this->JobQueue = other.JobQueue;
    this->ReRoutedJobQueue = other.ReRoutedJobQueue;
    this->BackupComplete = other.BackupComplete;
    this->CompletedQueue = other.CompletedQueue;
    this->Duplicate = other.Duplicate;
    this->StartingTime = other.StartingTime;
    this->EndingTime = other.EndingTime;
}

void JobMessage::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::omnetpp::cPacket::parsimPack(b);
    doParsimPacking(b,this->RelativeJobId);
    doParsimPacking(b,this->ClientId);
    doParsimPacking(b,this->OriginalExecId);
    doParsimPacking(b,this->ActualExecId);
    doParsimPacking(b,this->JobComplexity);
    doParsimPacking(b,this->QueueLength);
    doParsimPacking(b,this->StatusRequest);
    doParsimPacking(b,this->Probing);
    doParsimPacking(b,this->ReRouted);
    doParsimPacking(b,this->Ack);
    doParsimPacking(b,this->NewJob);
    doParsimPacking(b,this->IsEnded);
    doParsimPacking(b,this->ReBoot);
    doParsimPacking(b,this->NewJobsQueue);
    doParsimPacking(b,this->JobQueue);
    doParsimPacking(b,this->ReRoutedJobQueue);
    doParsimPacking(b,this->BackupComplete);
    doParsimPacking(b,this->CompletedQueue);
    doParsimPacking(b,this->Duplicate);
    doParsimPacking(b,this->StartingTime);
    doParsimPacking(b,this->EndingTime);
}

void JobMessage::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::omnetpp::cPacket::parsimUnpack(b);
    doParsimUnpacking(b,this->RelativeJobId);
    doParsimUnpacking(b,this->ClientId);
    doParsimUnpacking(b,this->OriginalExecId);
    doParsimUnpacking(b,this->ActualExecId);
    doParsimUnpacking(b,this->JobComplexity);
    doParsimUnpacking(b,this->QueueLength);
    doParsimUnpacking(b,this->StatusRequest);
    doParsimUnpacking(b,this->Probing);
    doParsimUnpacking(b,this->ReRouted);
    doParsimUnpacking(b,this->Ack);
    doParsimUnpacking(b,this->NewJob);
    doParsimUnpacking(b,this->IsEnded);
    doParsimUnpacking(b,this->ReBoot);
    doParsimUnpacking(b,this->NewJobsQueue);
    doParsimUnpacking(b,this->JobQueue);
    doParsimUnpacking(b,this->ReRoutedJobQueue);
    doParsimUnpacking(b,this->BackupComplete);
    doParsimUnpacking(b,this->CompletedQueue);
    doParsimUnpacking(b,this->Duplicate);
    doParsimUnpacking(b,this->StartingTime);
    doParsimUnpacking(b,this->EndingTime);
}

unsigned int JobMessage::getRelativeJobId() const
{
    return this->RelativeJobId;
}

void JobMessage::setRelativeJobId(unsigned int RelativeJobId)
{
    this->RelativeJobId = RelativeJobId;
}

unsigned int JobMessage::getClientId() const
{
    return this->ClientId;
}

void JobMessage::setClientId(unsigned int ClientId)
{
    this->ClientId = ClientId;
}

unsigned int JobMessage::getOriginalExecId() const
{
    return this->OriginalExecId;
}

void JobMessage::setOriginalExecId(unsigned int OriginalExecId)
{
    this->OriginalExecId = OriginalExecId;
}

unsigned int JobMessage::getActualExecId() const
{
    return this->ActualExecId;
}

void JobMessage::setActualExecId(unsigned int ActualExecId)
{
    this->ActualExecId = ActualExecId;
}

::omnetpp::simtime_t JobMessage::getJobComplexity() const
{
    return this->JobComplexity;
}

void JobMessage::setJobComplexity(::omnetpp::simtime_t JobComplexity)
{
    this->JobComplexity = JobComplexity;
}

int JobMessage::getQueueLength() const
{
    return this->QueueLength;
}

void JobMessage::setQueueLength(int QueueLength)
{
    this->QueueLength = QueueLength;
}

bool JobMessage::getStatusRequest() const
{
    return this->StatusRequest;
}

void JobMessage::setStatusRequest(bool StatusRequest)
{
    this->StatusRequest = StatusRequest;
}

bool JobMessage::getProbing() const
{
    return this->Probing;
}

void JobMessage::setProbing(bool Probing)
{
    this->Probing = Probing;
}

bool JobMessage::getReRouted() const
{
    return this->ReRouted;
}

void JobMessage::setReRouted(bool ReRouted)
{
    this->ReRouted = ReRouted;
}

bool JobMessage::getAck() const
{
    return this->Ack;
}

void JobMessage::setAck(bool Ack)
{
    this->Ack = Ack;
}

bool JobMessage::getNewJob() const
{
    return this->NewJob;
}

void JobMessage::setNewJob(bool NewJob)
{
    this->NewJob = NewJob;
}

bool JobMessage::getIsEnded() const
{
    return this->IsEnded;
}

void JobMessage::setIsEnded(bool IsEnded)
{
    this->IsEnded = IsEnded;
}

bool JobMessage::getReBoot() const
{
    return this->ReBoot;
}

void JobMessage::setReBoot(bool ReBoot)
{
    this->ReBoot = ReBoot;
}

bool JobMessage::getNewJobsQueue() const
{
    return this->NewJobsQueue;
}

void JobMessage::setNewJobsQueue(bool NewJobsQueue)
{
    this->NewJobsQueue = NewJobsQueue;
}

bool JobMessage::getJobQueue() const
{
    return this->JobQueue;
}

void JobMessage::setJobQueue(bool JobQueue)
{
    this->JobQueue = JobQueue;
}

bool JobMessage::getReRoutedJobQueue() const
{
    return this->ReRoutedJobQueue;
}

void JobMessage::setReRoutedJobQueue(bool ReRoutedJobQueue)
{
    this->ReRoutedJobQueue = ReRoutedJobQueue;
}

bool JobMessage::getBackupComplete() const
{
    return this->BackupComplete;
}

void JobMessage::setBackupComplete(bool BackupComplete)
{
    this->BackupComplete = BackupComplete;
}

bool JobMessage::getCompletedQueue() const
{
    return this->CompletedQueue;
}

void JobMessage::setCompletedQueue(bool CompletedQueue)
{
    this->CompletedQueue = CompletedQueue;
}

bool JobMessage::getDuplicate() const
{
    return this->Duplicate;
}

void JobMessage::setDuplicate(bool Duplicate)
{
    this->Duplicate = Duplicate;
}

::omnetpp::simtime_t JobMessage::getStartingTime() const
{
    return this->StartingTime;
}

void JobMessage::setStartingTime(::omnetpp::simtime_t StartingTime)
{
    this->StartingTime = StartingTime;
}

::omnetpp::simtime_t JobMessage::getEndingTime() const
{
    return this->EndingTime;
}

void JobMessage::setEndingTime(::omnetpp::simtime_t EndingTime)
{
    this->EndingTime = EndingTime;
}

class JobMessageDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertynames;
  public:
    JobMessageDescriptor();
    virtual ~JobMessageDescriptor();

    virtual bool doesSupport(omnetpp::cObject *obj) const override;
    virtual const char **getPropertyNames() const override;
    virtual const char *getProperty(const char *propertyname) const override;
    virtual int getFieldCount() const override;
    virtual const char *getFieldName(int field) const override;
    virtual int findField(const char *fieldName) const override;
    virtual unsigned int getFieldTypeFlags(int field) const override;
    virtual const char *getFieldTypeString(int field) const override;
    virtual const char **getFieldPropertyNames(int field) const override;
    virtual const char *getFieldProperty(int field, const char *propertyname) const override;
    virtual int getFieldArraySize(void *object, int field) const override;

    virtual const char *getFieldDynamicTypeString(void *object, int field, int i) const override;
    virtual std::string getFieldValueAsString(void *object, int field, int i) const override;
    virtual bool setFieldValueAsString(void *object, int field, int i, const char *value) const override;

    virtual const char *getFieldStructName(int field) const override;
    virtual void *getFieldStructValuePointer(void *object, int field, int i) const override;
};

Register_ClassDescriptor(JobMessageDescriptor)

JobMessageDescriptor::JobMessageDescriptor() : omnetpp::cClassDescriptor("JobMessage", "omnetpp::cPacket")
{
    propertynames = nullptr;
}

JobMessageDescriptor::~JobMessageDescriptor()
{
    delete[] propertynames;
}

bool JobMessageDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<JobMessage *>(obj)!=nullptr;
}

const char **JobMessageDescriptor::getPropertyNames() const
{
    if (!propertynames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
        const char **basenames = basedesc ? basedesc->getPropertyNames() : nullptr;
        propertynames = mergeLists(basenames, names);
    }
    return propertynames;
}

const char *JobMessageDescriptor::getProperty(const char *propertyname) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? basedesc->getProperty(propertyname) : nullptr;
}

int JobMessageDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? 21+basedesc->getFieldCount() : 21;
}

unsigned int JobMessageDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldTypeFlags(field);
        field -= basedesc->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
    };
    return (field>=0 && field<21) ? fieldTypeFlags[field] : 0;
}

const char *JobMessageDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldName(field);
        field -= basedesc->getFieldCount();
    }
    static const char *fieldNames[] = {
        "RelativeJobId",
        "ClientId",
        "OriginalExecId",
        "ActualExecId",
        "JobComplexity",
        "QueueLength",
        "StatusRequest",
        "Probing",
        "ReRouted",
        "Ack",
        "NewJob",
        "IsEnded",
        "ReBoot",
        "NewJobsQueue",
        "JobQueue",
        "ReRoutedJobQueue",
        "BackupComplete",
        "CompletedQueue",
        "Duplicate",
        "StartingTime",
        "EndingTime",
    };
    return (field>=0 && field<21) ? fieldNames[field] : nullptr;
}

int JobMessageDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    int base = basedesc ? basedesc->getFieldCount() : 0;
    if (fieldName[0]=='R' && strcmp(fieldName, "RelativeJobId")==0) return base+0;
    if (fieldName[0]=='C' && strcmp(fieldName, "ClientId")==0) return base+1;
    if (fieldName[0]=='O' && strcmp(fieldName, "OriginalExecId")==0) return base+2;
    if (fieldName[0]=='A' && strcmp(fieldName, "ActualExecId")==0) return base+3;
    if (fieldName[0]=='J' && strcmp(fieldName, "JobComplexity")==0) return base+4;
    if (fieldName[0]=='Q' && strcmp(fieldName, "QueueLength")==0) return base+5;
    if (fieldName[0]=='S' && strcmp(fieldName, "StatusRequest")==0) return base+6;
    if (fieldName[0]=='P' && strcmp(fieldName, "Probing")==0) return base+7;
    if (fieldName[0]=='R' && strcmp(fieldName, "ReRouted")==0) return base+8;
    if (fieldName[0]=='A' && strcmp(fieldName, "Ack")==0) return base+9;
    if (fieldName[0]=='N' && strcmp(fieldName, "NewJob")==0) return base+10;
    if (fieldName[0]=='I' && strcmp(fieldName, "IsEnded")==0) return base+11;
    if (fieldName[0]=='R' && strcmp(fieldName, "ReBoot")==0) return base+12;
    if (fieldName[0]=='N' && strcmp(fieldName, "NewJobsQueue")==0) return base+13;
    if (fieldName[0]=='J' && strcmp(fieldName, "JobQueue")==0) return base+14;
    if (fieldName[0]=='R' && strcmp(fieldName, "ReRoutedJobQueue")==0) return base+15;
    if (fieldName[0]=='B' && strcmp(fieldName, "BackupComplete")==0) return base+16;
    if (fieldName[0]=='C' && strcmp(fieldName, "CompletedQueue")==0) return base+17;
    if (fieldName[0]=='D' && strcmp(fieldName, "Duplicate")==0) return base+18;
    if (fieldName[0]=='S' && strcmp(fieldName, "StartingTime")==0) return base+19;
    if (fieldName[0]=='E' && strcmp(fieldName, "EndingTime")==0) return base+20;
    return basedesc ? basedesc->findField(fieldName) : -1;
}

const char *JobMessageDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldTypeString(field);
        field -= basedesc->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "unsigned int",
        "unsigned int",
        "unsigned int",
        "unsigned int",
        "simtime_t",
        "int",
        "bool",
        "bool",
        "bool",
        "bool",
        "bool",
        "bool",
        "bool",
        "bool",
        "bool",
        "bool",
        "bool",
        "bool",
        "bool",
        "simtime_t",
        "simtime_t",
    };
    return (field>=0 && field<21) ? fieldTypeStrings[field] : nullptr;
}

const char **JobMessageDescriptor::getFieldPropertyNames(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldPropertyNames(field);
        field -= basedesc->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

const char *JobMessageDescriptor::getFieldProperty(int field, const char *propertyname) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldProperty(field, propertyname);
        field -= basedesc->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

int JobMessageDescriptor::getFieldArraySize(void *object, int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldArraySize(object, field);
        field -= basedesc->getFieldCount();
    }
    JobMessage *pp = (JobMessage *)object; (void)pp;
    switch (field) {
        default: return 0;
    }
}

const char *JobMessageDescriptor::getFieldDynamicTypeString(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldDynamicTypeString(object,field,i);
        field -= basedesc->getFieldCount();
    }
    JobMessage *pp = (JobMessage *)object; (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string JobMessageDescriptor::getFieldValueAsString(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldValueAsString(object,field,i);
        field -= basedesc->getFieldCount();
    }
    JobMessage *pp = (JobMessage *)object; (void)pp;
    switch (field) {
        case 0: return ulong2string(pp->getRelativeJobId());
        case 1: return ulong2string(pp->getClientId());
        case 2: return ulong2string(pp->getOriginalExecId());
        case 3: return ulong2string(pp->getActualExecId());
        case 4: return simtime2string(pp->getJobComplexity());
        case 5: return long2string(pp->getQueueLength());
        case 6: return bool2string(pp->getStatusRequest());
        case 7: return bool2string(pp->getProbing());
        case 8: return bool2string(pp->getReRouted());
        case 9: return bool2string(pp->getAck());
        case 10: return bool2string(pp->getNewJob());
        case 11: return bool2string(pp->getIsEnded());
        case 12: return bool2string(pp->getReBoot());
        case 13: return bool2string(pp->getNewJobsQueue());
        case 14: return bool2string(pp->getJobQueue());
        case 15: return bool2string(pp->getReRoutedJobQueue());
        case 16: return bool2string(pp->getBackupComplete());
        case 17: return bool2string(pp->getCompletedQueue());
        case 18: return bool2string(pp->getDuplicate());
        case 19: return simtime2string(pp->getStartingTime());
        case 20: return simtime2string(pp->getEndingTime());
        default: return "";
    }
}

bool JobMessageDescriptor::setFieldValueAsString(void *object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->setFieldValueAsString(object,field,i,value);
        field -= basedesc->getFieldCount();
    }
    JobMessage *pp = (JobMessage *)object; (void)pp;
    switch (field) {
        case 0: pp->setRelativeJobId(string2ulong(value)); return true;
        case 1: pp->setClientId(string2ulong(value)); return true;
        case 2: pp->setOriginalExecId(string2ulong(value)); return true;
        case 3: pp->setActualExecId(string2ulong(value)); return true;
        case 4: pp->setJobComplexity(string2simtime(value)); return true;
        case 5: pp->setQueueLength(string2long(value)); return true;
        case 6: pp->setStatusRequest(string2bool(value)); return true;
        case 7: pp->setProbing(string2bool(value)); return true;
        case 8: pp->setReRouted(string2bool(value)); return true;
        case 9: pp->setAck(string2bool(value)); return true;
        case 10: pp->setNewJob(string2bool(value)); return true;
        case 11: pp->setIsEnded(string2bool(value)); return true;
        case 12: pp->setReBoot(string2bool(value)); return true;
        case 13: pp->setNewJobsQueue(string2bool(value)); return true;
        case 14: pp->setJobQueue(string2bool(value)); return true;
        case 15: pp->setReRoutedJobQueue(string2bool(value)); return true;
        case 16: pp->setBackupComplete(string2bool(value)); return true;
        case 17: pp->setCompletedQueue(string2bool(value)); return true;
        case 18: pp->setDuplicate(string2bool(value)); return true;
        case 19: pp->setStartingTime(string2simtime(value)); return true;
        case 20: pp->setEndingTime(string2simtime(value)); return true;
        default: return false;
    }
}

const char *JobMessageDescriptor::getFieldStructName(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldStructName(field);
        field -= basedesc->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    };
}

void *JobMessageDescriptor::getFieldStructValuePointer(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldStructValuePointer(object, field, i);
        field -= basedesc->getFieldCount();
    }
    JobMessage *pp = (JobMessage *)object; (void)pp;
    switch (field) {
        default: return nullptr;
    }
}


