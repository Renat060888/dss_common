#ifndef FEDERATE_H
#define FEDERATE_H

#include <string>
#include <queue>
#include <memory>
#include <atomic>
#include <vector>
#include <condition_variable>

//using TRtiTime = int64_t;
//const TRtiTime ONE_RTI_STEP = 1;
using TRtiTime = double;
constexpr TRtiTime ONE_RTI_STEP = 1.0f;
const std::string FEDINFO_OBJECT_INSTANCE_NAME( "fedinfo" );

namespace rti1516e{
    class RTIambassador;
    class LogicalTimeInterval;
    class LogicalTime;
}

namespace rti_client_vega {

class RTIObjectsManager;
class RTIObject;
using RTIObjectPtr = std::shared_ptr<RTIObject>;

enum class EEventType {
    OBJECT_FROM_RTI,
    SEND_OBJECT,
    TERMINATE,
    SIMULATOR_FINISHED,
    TIME_ADVANCE_GRANTED,
    UNDEFINED
};

enum class ETaskTypeFromOtherThread {
    SWITCH_TO_FEDERATION,
    PUBLISH_OBJECT_CLASS,
    SUBSCRIBE_OBJECT_CLASS,
    CREATE_OBJECT,
    WAIT_EVENTS,
    UNDEFINED
};

struct SFederateState{
    SFederateState(){
        clear();
    }

    void clear(){
        isMasterFederate = false;
        federationSynchronized = false;
        timeAdvanceGranted = false;
        lookaheadTime = 1.0f; // TODO: - ПОЧЕМУ В RRTI ПРИ "1" ПРОДВИГАЕТСЯ 2 РАЗА, А В OPEN-RTI ТАКОГО НЕТ (ЧЕРЕЗ РАЗ)
        currentFederateTime = 0.0f; // TODO: std::numeric_limits<TRtiTime>::min()

        timeRegulationEnabled = false;
        timeConstrainEnabled = false;

        currentFederationName.clear();
        federateName.clear();
        RTIFOMFile.clear();
        RTIServerAddress.clear();
    }

    std::atomic_bool    isMasterFederate;
    std::atomic_bool    federationSynchronized;
    std::atomic_bool    timeAdvanceGranted;
    TRtiTime            lookaheadTime;
    TRtiTime            currentFederateTime;

    bool                timeRegulationEnabled;
    bool                timeConstrainEnabled;

    std::string         currentFederationName;
    std::string         federateName;
    std::string         RTIFOMFile;
    std::string         RTIServerAddress;
};

class IRTIFederateObserver {

public:
    virtual ~IRTIFederateObserver(){}
    virtual void callbackReflectedRTIObjects( const std::vector<RTIObjectPtr> & _rtiObjectsFromCallback ) = 0;
};

// если используется реализация HLA от Open-RTI, где возможны только синхронные вызовы. В RRTI возможны оба способа вызова callback'ов
#define HLA_NOT_ASYNC

const std::string SYNCH_POINT_NAME_FOR_MASTER_FEDERATE = "master_init_synch_point";
const std::string COMMON_FEDERATION_NAME = "VEGA_FEDERATION";
const std::string FEDERATE_TYPE = "VEGA_SIMULATING";

/*
 * HLA FEDERATE CLASS
 * RTI-клиент НЕ ПОТОКОБЕЗОПАСНЫЙ, чтобы не снижать производительность обкладыванием всего и вся мьютексами
 * сделана очередь, в которую если нужно кидать задачи из других потоков
 */
class RTIFederate final {

    friend class RTIObjectsManager;

public:
    struct SInitParameters {
        SInitParameters() :
            enableAsyncDelivery( true ),
            federataionName(COMMON_FEDERATION_NAME)
        {}

        bool enableAsyncDelivery;
        std::string federateName;
        std::string RTIFOMFile;
        std::string RTIServerAddress;
        std::string federataionName;
    };

    RTIFederate();
    virtual ~RTIFederate();

    // init
    bool                init( SInitParameters _settings );
    void                shutdown();
    bool                connectToRTI();
    bool                disconnectFromRTI();
    void                addObserver( IRTIFederateObserver * _observer );
    void                runRTICallbacks();
    void                waitEventsFromRTINonBlock(); // TODO temp

    // quick methods
    bool quickReconnect( SInitParameters _params );
    bool quickTime( bool _enable );

    // service message
    bool sendServiceMessage( const std::string & _msg );
    const std::string & receiveServiceMessage();

    // federation management
    bool                createFederation( const std::string & _federationName = "" );
    bool                destroyFederation();
    bool                joinToFederation( const std::string & _federationName = "" );
    bool                quitFromFederation( const bool _destroyFederation = true );
    bool                switchToFederation( const std::string & _federationName = "" );
    bool                listFederations();

    // time
    bool                enableTimeRegulation();
    bool                enableTimeConstrain();
    bool                disableTimeRegulation();
    bool                disableTimeConstrain();
    bool                isTimeEnabled();    
    bool                isTimeRegulated();
    bool                isTimeConstrained();
    // код, вызвавший этот метод вместе с runRTICallbacks() в одном потоке - войдет в deadlock !
    bool                advanceTimeRequestBlocked( const TRtiTime _oneStepValue = ONE_RTI_STEP );
    void                advanceTimeRequestNonBlocked( const TRtiTime _oneStepValue );
    bool                advanceTimeGranted();

    // synch
    bool                registerSynchronizePoint( const std::string _syncPointName );
    bool                achieveSynchronizePoint( const std::string _syncPointName );
    bool                waitForOtherFederates( const double _timeToWaitSec );

    // event
    bool isEventOccur(){ return (EEventType::UNDEFINED != m_eventType ); }
    EEventType getEvent();

    // objects management
    RTIObjectsManager * getRTIObjectsManager(){ return m_RTIObjectsManager; }

    // utils
    void                getLastError( std::string & _errorStr );
    const SFederateState & getState(){ return m_state; }

private:
    bool enableAsyncDelivery();

    // data
    SInitParameters m_settings;
    SFederateState m_state;
    RTIObjectPtr m_serviceRTIObject;
    std::vector<IRTIFederateObserver *> m_rtiFederateObservers;
    EEventType m_eventType;
    std::string m_lastError;

    // service
    std::mutex                      m_FederateServicesMutex;
    RTIObjectsManager *             m_RTIObjectsManager;
    std::condition_variable         m_RTIEvent;
    rti1516e::RTIambassador *       m_RTIAmbassador;
    class FederateAmbassadorImpl *  m_fedAmbassador;

    // class non-copyable
    RTIFederate( const RTIFederate & inst );
    RTIFederate & operator ==( const RTIFederate & inst );
};

}

#endif // FEDERATE_H
