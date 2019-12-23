//
// Generated file, do not edit! Created by nedtool 5.5 from msg_check.msg.
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
#include "msg_check_m.h"

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

Register_Class(msg_check)

msg_check::msg_check(const char *name, short kind) : ::omnetpp::cPacket(name,kind)
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
}

msg_check::msg_check(const msg_check& other) : ::omnetpp::cPacket(other)
{
    copy(other);
}

msg_check::~msg_check()
{
}

msg_check& msg_check::operator=(const msg_check& other)
{
    if (this==&other) return *this;
    ::omnetpp::cPacket::operator=(other);
    copy(other);
    return *this;
}

void msg_check::copy(const msg_check& other)
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
}

void msg_check::parsimPack(omnetpp::cCommBuffer *b) const
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
}

void msg_check::parsimUnpack(omnetpp::cCommBuffer *b)
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
}

int msg_check::getRelativeJobId() const
{
    return this->RelativeJobId;
}

void msg_check::setRelativeJobId(int RelativeJobId)
{
    this->RelativeJobId = RelativeJobId;
}

int msg_check::getClientId() const
{
    return this->ClientId;
}

void msg_check::setClientId(int ClientId)
{
    this->ClientId = ClientId;
}

int msg_check::getOriginalExecId() const
{
    return this->OriginalExecId;
}

void msg_check::setOriginalExecId(int OriginalExecId)
{
    this->OriginalExecId = OriginalExecId;
}

int msg_check::getActualExecId() const
{
    return this->ActualExecId;
}

void msg_check::setActualExecId(int ActualExecId)
{
    this->ActualExecId = ActualExecId;
}

::omnetpp::simtime_t msg_check::getJobComplexity() const
{
    return this->JobComplexity;
}

void msg_check::setJobComplexity(::omnetpp::simtime_t JobComplexity)
{
    this->JobComplexity = JobComplexity;
}

int msg_check::getQueueLength() const
{
    return this->QueueLength;
}

void msg_check::setQueueLength(int QueueLength)
{
    this->QueueLength = QueueLength;
}

bool msg_check::getStatusRequest() const
{
    return this->StatusRequest;
}

void msg_check::setStatusRequest(bool StatusRequest)
{
    this->StatusRequest = StatusRequest;
}

bool msg_check::getProbing() const
{
    return this->Probing;
}

void msg_check::setProbing(bool Probing)
{
    this->Probing = Probing;
}

bool msg_check::getReRouted() const
{
    return this->ReRouted;
}

void msg_check::setReRouted(bool ReRouted)
{
    this->ReRouted = ReRouted;
}

bool msg_check::getAck() const
{
    return this->Ack;
}

void msg_check::setAck(bool Ack)
{
    this->Ack = Ack;
}

bool msg_check::getNewJob() const
{
    return this->NewJob;
}

void msg_check::setNewJob(bool NewJob)
{
    this->NewJob = NewJob;
}

bool msg_check::getIsEnded() const
{
    return this->IsEnded;
}

void msg_check::setIsEnded(bool IsEnded)
{
    this->IsEnded = IsEnded;
}

bool msg_check::getReBoot() const
{
    return this->ReBoot;
}

void msg_check::setReBoot(bool ReBoot)
{
    this->ReBoot = ReBoot;
}

bool msg_check::getNewJobsQueue() const
{
    return this->NewJobsQueue;
}

void msg_check::setNewJobsQueue(bool NewJobsQueue)
{
    this->NewJobsQueue = NewJobsQueue;
}

bool msg_check::getJobQueue() const
{
    return this->JobQueue;
}

void msg_check::setJobQueue(bool JobQueue)
{
    this->JobQueue = JobQueue;
}

bool msg_check::getReRoutedJobQueue() const
{
    return this->ReRoutedJobQueue;
}

void msg_check::setReRoutedJobQueue(bool ReRoutedJobQueue)
{
    this->ReRoutedJobQueue = ReRoutedJobQueue;
}

bool msg_check::getBackupComplete() const
{
    return this->BackupComplete;
}

void msg_check::setBackupComplete(bool BackupComplete)
{
    this->BackupComplete = BackupComplete;
}

bool msg_check::getCompletedQueue() const
{
    return this->CompletedQueue;
}

void msg_check::setCompletedQueue(bool CompletedQueue)
{
    this->CompletedQueue = CompletedQueue;
}

class msg_checkDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertynames;
  public:
    msg_checkDescriptor();
    virtual ~msg_checkDescriptor();

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

Register_ClassDescriptor(msg_checkDescriptor)

msg_checkDescriptor::msg_checkDescriptor() : omnetpp::cClassDescriptor("msg_check", "omnetpp::cPacket")
{
    propertynames = nullptr;
}

msg_checkDescriptor::~msg_checkDescriptor()
{
    delete[] propertynames;
}

bool msg_checkDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<msg_check *>(obj)!=nullptr;
}

const char **msg_checkDescriptor::getPropertyNames() const
{
    if (!propertynames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
        const char **basenames = basedesc ? basedesc->getPropertyNames() : nullptr;
        propertynames = mergeLists(basenames, names);
    }
    return propertynames;
}

const char *msg_checkDescriptor::getProperty(const char *propertyname) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? basedesc->getProperty(propertyname) : nullptr;
}

int msg_checkDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? 18+basedesc->getFieldCount() : 18;
}

unsigned int msg_checkDescriptor::getFieldTypeFlags(int field) const
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
    };
    return (field>=0 && field<18) ? fieldTypeFlags[field] : 0;
}

const char *msg_checkDescriptor::getFieldName(int field) const
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
    };
    return (field>=0 && field<18) ? fieldNames[field] : nullptr;
}

int msg_checkDescriptor::findField(const char *fieldName) const
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
    return basedesc ? basedesc->findField(fieldName) : -1;
}

const char *msg_checkDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldTypeString(field);
        field -= basedesc->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "int",
        "int",
        "int",
        "int",
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
    };
    return (field>=0 && field<18) ? fieldTypeStrings[field] : nullptr;
}

const char **msg_checkDescriptor::getFieldPropertyNames(int field) const
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

const char *msg_checkDescriptor::getFieldProperty(int field, const char *propertyname) const
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

int msg_checkDescriptor::getFieldArraySize(void *object, int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldArraySize(object, field);
        field -= basedesc->getFieldCount();
    }
    msg_check *pp = (msg_check *)object; (void)pp;
    switch (field) {
        default: return 0;
    }
}

const char *msg_checkDescriptor::getFieldDynamicTypeString(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldDynamicTypeString(object,field,i);
        field -= basedesc->getFieldCount();
    }
    msg_check *pp = (msg_check *)object; (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string msg_checkDescriptor::getFieldValueAsString(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldValueAsString(object,field,i);
        field -= basedesc->getFieldCount();
    }
    msg_check *pp = (msg_check *)object; (void)pp;
    switch (field) {
        case 0: return long2string(pp->getRelativeJobId());
        case 1: return long2string(pp->getClientId());
        case 2: return long2string(pp->getOriginalExecId());
        case 3: return long2string(pp->getActualExecId());
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
        default: return "";
    }
}

bool msg_checkDescriptor::setFieldValueAsString(void *object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->setFieldValueAsString(object,field,i,value);
        field -= basedesc->getFieldCount();
    }
    msg_check *pp = (msg_check *)object; (void)pp;
    switch (field) {
        case 0: pp->setRelativeJobId(string2long(value)); return true;
        case 1: pp->setClientId(string2long(value)); return true;
        case 2: pp->setOriginalExecId(string2long(value)); return true;
        case 3: pp->setActualExecId(string2long(value)); return true;
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
        default: return false;
    }
}

const char *msg_checkDescriptor::getFieldStructName(int field) const
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

void *msg_checkDescriptor::getFieldStructValuePointer(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldStructValuePointer(object, field, i);
        field -= basedesc->getFieldCount();
    }
    msg_check *pp = (msg_check *)object; (void)pp;
    switch (field) {
        default: return nullptr;
    }
}


