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

    virtual PNetworkClient getNodeAgentCommunicator( const std::string & _uniqueId ) = 0;
    virtual PNetworkClient getNodeWorkerCommunicator( const std::string & _uniqueId ) = 0;
    virtual PNetworkClient getPlayerAgentCommunicator() = 0;
    virtual PNetworkClient getPlayerWorkerCommunicator( const std::string & _uniqueId ) = 0;
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
    {}

    SystemEnvironmentFacade * systemEnvironment;
    SourceManagerFacade * sourceManager;
    AnalyticManagerFacade * analyticManager;
    StorageEngineFacade * storageEngine;
    CommunicationGatewayFacadeDSS * communicationGateway;
    std::function<void()> signalShutdownServer;
};


}

#endif // COMMON_TYPES_H
