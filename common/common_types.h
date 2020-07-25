#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <string>
#include <functional>

#include <microservice_common/communication/network_interface.h>
#include <microservice_common/common/ms_common_types.h>

// ---------------------------------------------------------------------------
// forwards
// ---------------------------------------------------------------------------
class SourceManagerFacade;
class AnalyticManagerFacade;
class StorageEngineFacade;
class SystemEnvironmentFacade;
class CommunicationGatewayFacadeDSS;
class CommunicationGatewayFacadeNode; // TODO: CommunicationGatewayFacade -> CommunicationGatewayFacadeBase

namespace common_types{

// ---------------------------------------------------------------------------
// global typedefs
// ---------------------------------------------------------------------------
using TNodeId = std::string;
using TUserId = std::string;


// ---------------------------------------------------------------------------
// enums
// ---------------------------------------------------------------------------
enum class ENodeStatus {
    INITED,
    PREPARING,
    READY,
    ACTIVE,
    IDLE,
    CRASHED,
    UNDEFINED
};


// ---------------------------------------------------------------------------
// simple ADT
// ---------------------------------------------------------------------------
struct SNodeState {
    SNodeState()
        : id("invalid_id") // common_vars::INVALID_NODE_ID
        , agentId("invalid_id") // common_vars::INVALID_NODE_ID
        , ctxId(0) // common_vars::INVALID_CONTEXT_ID
        , status(common_types::ENodeStatus::UNDEFINED)
        , busy(false)
        , lastPingAtMillisec(0)
    {}
    common_types::TNodeId id;
    common_types::TNodeId agentId;
    common_types::TContextId ctxId;
    common_types::ENodeStatus status;
    bool busy;
    int64_t lastPingAtMillisec;
    std::string lastError;
};

struct SNodeWorkerSimulationState : SNodeState {

};

struct SNodeWorkerRealState : SNodeState {
    std::string realObjectName;
    std::string caps;
};

struct SNodeWorkerDumpState : SNodeState {

};

struct SNodeAgentState {
    SNodeAgentState()
        : nodeAgentId("invalid_id")
        , lastPingAtMillisec(0)
    {}
    TNodeId nodeAgentId;
    int64_t lastPingAtMillisec;
    std::string lastError;
};

struct SNodeAgentSimulateState : SNodeAgentState {
    std::vector<SNodeWorkerSimulationState> nodeWorkers;
};

struct SNodeAgentRealState : SNodeAgentState {
    std::vector<SNodeWorkerRealState> nodeWorkers;
};

struct SNodeAgentDumpState : SNodeAgentState {

};

// ---------------------------------------------------------------------------
// exchange ADT ( component <-> store, component <-> network, etc... )
// ---------------------------------------------------------------------------





// ---------------------------------------------------------------------------
// types deduction
// ---------------------------------------------------------------------------





// ---------------------------------------------------------------------------
// service interfaces
// ---------------------------------------------------------------------------
class IServiceInternalCommunication {
public:
    virtual ~IServiceInternalCommunication(){}

    virtual PNetworkClient getCoreCommunicator() { assert( false && "not implemented by derive" ); }
    virtual PNetworkClient getNodeAgentCommunicator( const std::string & _uniqueId ) { assert( false && "not implemented by derive" ); }
    virtual PNetworkClient getNodeWorkerCommunicator( const std::string & _uniqueId ) = 0;
};

class IServiceExternalCommunication {
public:
    virtual ~IServiceExternalCommunication(){}

    virtual PNetworkClient getUserCommunicator( const std::string & _uniqueId ) = 0;
};

class IUserDispatcherObserver {
public:
    virtual ~IUserDispatcherObserver(){}

    virtual void callbackUserOnline( const common_types::TUserId & _id, bool _online ) = 0;
};

class IServiceUserAuthorization {
public:
    virtual ~IServiceUserAuthorization(){}

    virtual const std::string & getLastError() = 0;

    virtual bool isRegistered( const TUserId & _id ) = 0;
    virtual void addObserver( IUserDispatcherObserver * _observer ) = 0;
    virtual void removeObserver( IUserDispatcherObserver * _observer ) = 0;
};

class INodeDispatcher {
public:
    virtual ~INodeDispatcher(){}

    virtual void runSystemClock() = 0;
};



// ---------------------------------------------------------------------------
// service locator
// ---------------------------------------------------------------------------
struct SIncomingCommandServices : SIncomingCommandGlobalServices {
    SIncomingCommandServices()
        : systemEnvironment(nullptr)
        , sourceManager(nullptr)
        , analyticManager(nullptr)
        , storageEngine(nullptr)
        , communicationGateway(nullptr)
        , communicationGatewayNode(nullptr)
    {}

    SystemEnvironmentFacade * systemEnvironment;
    SourceManagerFacade * sourceManager;
    AnalyticManagerFacade * analyticManager;
    StorageEngineFacade * storageEngine;
    CommunicationGatewayFacadeDSS * communicationGateway;
    CommunicationGatewayFacadeNode * communicationGatewayNode;
    std::function<void()> signalShutdownServer;
};


}

#endif // COMMON_TYPES_H
