
#include <locale>
#include <assert.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <memory>

#include <RTI/RTIambassador.h>
#include <RTI/RTIambassadorFactory.h>
#include <RTI/time/HLAinteger64Time.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64TimeFactory.h>
#include <RTI/time/HLAfloat64Time.h>
#include <RTI/time/HLAfloat64Interval.h>
#include <RTI/time/HLAfloat64TimeFactory.h>
#include <microservice_common/system/logger.h>
#include <microservice_common/common/ms_common_utils.h>

#include "federate_ambassador.h"
#include "rti_object_federate_info.h"
#include "rti_objects_manager.h"
#include "federate.h"
#include "rti_object.h"

namespace rti_client_vega {

using namespace rti1516e;
using namespace std;

static mutex g_waitEventMutex;

static constexpr const char * PRINT_HEADER = "RTIClient:";

// стандарт RTI может работать как с целочисленным типом времени, так и с вещественным
//#define HLA_DOUBLE
#ifdef HLA_DOUBLE
#define HLA_TIME_DOUBLE
static const std::wstring HLA_TIME_DATATYPE = L"HLAfloat64Time";
#else
static const std::wstring HLA_TIME_DATATYPE = L"HLAinteger64Time";
#endif

RTIFederate::RTIFederate() :
    m_RTIAmbassador     ( nullptr ),
    m_fedAmbassador     ( nullptr ),
    m_RTIObjectsManager ( nullptr ),
    m_lastError         ("no errors"),
    m_eventType         ( EEventType::UNDEFINED ){

//    std::locale::global( std::locale("ru_RU.UTF-8")); // TODO связана как то с wchar_t
    std::locale::global( std::locale("C.UTF-8")); // TODO связана как то с wchar_t
}

RTIFederate::~RTIFederate(){

    shutdown();
}

bool RTIFederate::quickReconnect( SInitParameters _params ){

    // destroy previous
    shutdown();

    // create new
    if( ! init(_params) ){
        return false;
    }
    if( ! connectToRTI() ){
        return false;
    }
    if( ! createFederation(_params.federataionName) ){
        return false;
    }
    if( ! joinToFederation(_params.federataionName) ){
        return false;
    }
    if( ! enableAsyncDelivery() ){
        return false;
    }
    if( ! getRTIObjectsManager()->publishObjectClass<RTIObjectFederateInfo>() ||
        ! getRTIObjectsManager()->subcribeObjectClass<RTIObjectFederateInfo>() ){
        return false;
    }

    // service object
    RTIObjectPtr temp = getRTIObjectsManager()->createObject< RTIObjectFederateInfo >( FEDINFO_OBJECT_INSTANCE_NAME );
    m_serviceRTIObject = dynamic_pointer_cast<RTIObjectFederateInfo>( temp );

    // alive
    runRTICallbacks();

    return true;
}

bool RTIFederate::quickTime( bool _enable ){

    if( _enable ){

        if( ! enableTimeRegulation() || ! enableTimeConstrain() ){
            return false;
        }

        while( ! isTimeEnabled() ){
            runRTICallbacks();
        }
    }
    else{
        if( ! disableTimeRegulation() || ! disableTimeConstrain() ){
            return false;
        }
    }

    return true;
}

bool RTIFederate::sendServiceMessage( const string &_msg ){

//    m_serviceRTIObject->getFederateNameRef() = getState().federateName;
//    m_serviceRTIObject->getJsonMessageRef() = _msg;

    return true;
}

const std::string & RTIFederate::receiveServiceMessage(){

    // TODO: do
}

bool RTIFederate::init( SInitParameters _settings ){

    if( _settings.federateName.empty() ||
        _settings.RTIFOMFile.empty() ){

        VS_LOG_ERROR << PRINT_HEADER << " federate init fail. Federate name or FOM file path not provided" << endl;
        return false;
    }

    m_state.RTIFOMFile = _settings.RTIFOMFile;
    m_state.federateName = _settings.federateName;
    m_state.RTIServerAddress = _settings.RTIServerAddress;

    m_settings = _settings;

    return true;
}

void RTIFederate::shutdown(){

    if( m_RTIAmbassador ){
        quitFromFederation();
        disconnectFromRTI();
    }

    delete m_RTIAmbassador;
    m_RTIAmbassador = nullptr;

    delete m_fedAmbassador;
    m_fedAmbassador = nullptr;

    delete m_RTIObjectsManager;
    m_RTIObjectsManager = nullptr;

    m_lastError = "no errors";
    m_eventType = EEventType::UNDEFINED;

    m_state.clear();
}

bool RTIFederate::connectToRTI(){

    lock_guard<mutex> lock( m_FederateServicesMutex );

    try{
        RTIambassadorFactory factory;
        auto_ptr< RTIambassador > tempRtiAmb = factory.createRTIambassador();
        m_RTIAmbassador = tempRtiAmb.release();

        m_RTIObjectsManager = new RTIObjectsManager( this );

        m_fedAmbassador = new FederateAmbassadorImpl( m_RTIObjectsManager, & m_state, SYNCH_POINT_NAME_FOR_MASTER_FEDERATE );

#ifdef HLA_NOT_ASYNC
        // RTI от OpenRTI (режим HLA_IMMEDIATE у них не поддерживается и конфиг ведется не через файл, а строкой)
        m_RTIAmbassador->connect( * m_fedAmbassador, HLA_EVOKED, rti_utils::strToWstr(m_state.RTIServerAddress) );
        VS_LOG_INFO << "rti-client try connect to RTI server by address [" << m_state.RTIServerAddress << "]" << endl;
//        m_RTIAmbassador->connect( * m_fedAmbassador, HLA_EVOKED  );
#else
        // RTI от РусБиТЕХ, а может и PORTICO (ядро на Java)
        m_RTIAmbassador->connect( *m_fedAmbassador, HLA_IMMEDIATE );
#endif
    }
    catch( Exception & ex ){
        VS_LOG_ERROR << PRINT_HEADER << "EXCP: [" << m_state.federateName << "] connectToRTI [" << rti_utils::wstrToStr( ex.what() ) << "]" << endl;
        return false;
    }

    return true;
}

void RTIFederate::addObserver( IRTIFederateObserver * _observer ){

    m_rtiFederateObservers.push_back( _observer );
}

bool RTIFederate::createFederation( const string & _federationName ){

    lock_guard<mutex> lock( m_FederateServicesMutex );

    m_state.currentFederationName = (! _federationName.empty()) ? _federationName : COMMON_FEDERATION_NAME;

    try{
        const std::wstring cfn = rti_utils::strToWstr( m_state.currentFederationName );
        const std::wstring rff = rti_utils::strToWstr( m_state.RTIFOMFile );
        m_RTIAmbassador->createFederationExecution( cfn, rff, HLA_TIME_DATATYPE );
        VS_LOG_INFO << "[" << m_state.federateName << "] - [" << m_state.currentFederationName << "] federation created" << endl;
    }
    catch( FederationExecutionAlreadyExists & ex ){
        return true;
    }
    // TODO: наладить корректное завершение, если FOM не найден
    catch( CouldNotOpenFDD & ex ){
        VS_LOG_WARN << "EXCP: [" << m_state.federateName << "] createFederation, FOM not found [" << rti_utils::wstrToStr( ex.what() ) << "]" << endl;
        return false;
    }
    catch( Exception & ex ){
        VS_LOG_WARN << "EXCP: [" << m_state.federateName << "] createFederation [" << rti_utils::wstrToStr( ex.what() ) << "]" << endl;
        return false;
    }

    return true;
}

bool RTIFederate::destroyFederation(){

    lock_guard<mutex> lock( m_FederateServicesMutex );

    try{
        m_RTIAmbassador->destroyFederationExecution( rti_utils::strToWstr( m_state.currentFederationName ) );
        VS_LOG_INFO << "[" << m_state.federateName << "] federation [" << m_state.currentFederationName << "] has been destroyed" << endl;
    }
    catch( FederationExecutionDoesNotExist & ex ){
        return true;
    }
    catch( Exception & ex ){
        VS_LOG_WARN << "EXCP: [" << m_state.federateName << "] destroyFederation [" << rti_utils::wstrToStr( ex.what() ) << "]" << endl;
        return false;
    }
    return true;
}

bool RTIFederate::joinToFederation( const string & _federationName ){

    lock_guard<mutex> lock( m_FederateServicesMutex );

    const string federationName = (! _federationName.empty()) ? _federationName : COMMON_FEDERATION_NAME;

    try{        
        m_RTIAmbassador->joinFederationExecution( rti_utils::strToWstr(m_state.federateName), rti_utils::strToWstr(FEDERATE_TYPE), rti_utils::strToWstr(federationName) );

        VS_LOG_INFO << "[" << m_state.federateName << "] joined to federation [" << federationName << "]" << endl;

        if( m_settings.enableAsyncDelivery ){
            enableAsyncDelivery();
        }
    }    
    catch( Exception & ex ){
        VS_LOG_ERROR << "EXCP: [" << m_state.federateName << "] joinToFederation [" << rti_utils::wstrToStr( ex.what() ) << "]" << endl;
        return false;
    }

    return true;
}

bool RTIFederate::enableTimeRegulation(){

    // Чтобы получить одобрение - все остальные федераты должны непрерывно получать callback'и !

    lock_guard<mutex> lock( m_FederateServicesMutex );

    try{
#ifdef HLA_TIME_DOUBLE
        HLAfloat64Interval lt( m_federateState->lookaheadTime );
#else
        HLAinteger64Interval lt( m_state.lookaheadTime );
#endif

        m_RTIAmbassador->enableTimeRegulation( lt );

        VS_LOG_INFO << "[" << m_state.federateName << "] federate become REGULATED, lookahead [" << m_state.lookaheadTime << "]" << endl;
    }
    catch( Exception & ex ){
        VS_LOG_ERROR << "[" << m_state.federateName << "] enableTimeRegulated [" << rti_utils::wstrToStr( ex.what() ) << "]" << endl;
        return false;
    }

    return true;
}

bool RTIFederate::enableTimeConstrain(){

    lock_guard<mutex> lock( m_FederateServicesMutex );

    try{
        m_RTIAmbassador->enableTimeConstrained();

#ifdef HLA_TIME_DOUBLE
        HLAfloat64Time lt;
#else
        HLAinteger64Time lt;
#endif
        m_RTIAmbassador->queryLogicalTime( lt );

        VS_LOG_INFO << "[" << m_state.federateName << "] federate become CONSTRAINED, query log time [" << rti_utils::convertTime( lt.toString() ) << "]" << endl;
    }
    catch( Exception & ex ){
        VS_LOG_ERROR << "EXCP: [" << m_state.federateName << "] enableTimeConstrained [" << rti_utils::wstrToStr( ex.what() ) << "]" << endl;
        return false;
    }

    return true;
}

bool RTIFederate::disableTimeRegulation(){

    lock_guard<mutex> lock( m_FederateServicesMutex );

    try{
        m_state.timeRegulationEnabled = false; // TODO лучше продумать этот момент
        m_RTIAmbassador->disableTimeRegulation();

        VS_LOG_INFO << "[" << m_state.federateName << "] federate become NON-REGULATED, lookahead [" << m_state.lookaheadTime << "]" << endl;
    }
    catch( Exception & ex ){
        VS_LOG_ERROR << "[" << m_state.federateName << "] disableTimeRegulated [" << rti_utils::wstrToStr( ex.what() ) << "]" << endl;
        return false;
    }

    return true;
}

bool RTIFederate::disableTimeConstrain(){

    lock_guard<mutex> lock( m_FederateServicesMutex );

    try{
        m_RTIAmbassador->disableTimeConstrained();
        m_state.timeConstrainEnabled = false;

#ifdef HLA_TIME_DOUBLE
        HLAfloat64Time lt;
#else
        HLAinteger64Time lt;
#endif
        m_RTIAmbassador->queryLogicalTime( lt );

        VS_LOG_INFO << "[" << m_state.federateName << "] federate become NON-CONSTRAINED, query log time [" << rti_utils::convertTime( lt.toString() ) << "]" << endl;
    }
    catch( Exception & ex ){
        VS_LOG_ERROR << "EXCP: [" << m_state.federateName << "] disableTimeConstrained [" << rti_utils::wstrToStr( ex.what() ) << "]" << endl;
        return false;
    }

    return true;
}

bool RTIFederate::isTimeEnabled(){
    return (m_state.timeRegulationEnabled && m_state.timeConstrainEnabled);
}

bool RTIFederate::isTimeRegulated(){
    return m_state.timeRegulationEnabled;
}

bool RTIFederate::isTimeConstrained(){
    return m_state.timeConstrainEnabled;
}

bool RTIFederate::enableAsyncDelivery(){

    try{
        m_RTIAmbassador->enableAsynchronousDelivery();

        VS_LOG_INFO << "[" << m_state.federateName << "] async delivery - Enabled" << endl;
    }
    catch( Exception & ex ){
        VS_LOG_ERROR << "EXCP: [" << m_state.federateName << "] enableAsyncDelivery [" << rti_utils::wstrToStr( ex.what() ) << "]" << endl;
        return false;
    }

    return true;
}

bool RTIFederate::advanceTimeRequestBlocked( const TRtiTime _oneStepValue ){

    try{
#ifdef HLA_TIME_DOUBLE
        HLAfloat64Time newTime( m_federateState->currentFederateTime + _oneStepValue );
#else
        HLAinteger64Time newTime( m_state.currentFederateTime + _oneStepValue );
#endif

        // все оборачивать в мютекс нельзя, т.к. где то в клиенском коде вертится цикл с runRTICallbacks(), который тоже вызывает RTIAmbassador
        m_FederateServicesMutex.lock();
        m_RTIAmbassador->timeAdvanceRequest( newTime );
        m_FederateServicesMutex.unlock();

        while( ! m_state.timeAdvanceGranted.load() ){

            // ! в случае Objrepr-RTI цикл опроса идет у него
            runRTICallbacks();
#ifdef HLA_NOT_ASYNC
//            waitEventsFromRTINonBlock();
#endif
        }

        m_state.timeAdvanceGranted.store( false ); // reset grant-time flag
    }
    catch( Exception & ex ){
        VS_LOG_ERROR << "EXCP: [" << m_state.federateName << "] advanceTimeBlocked [" << rti_utils::wstrToStr( ex.what() ) << "]" << endl;
        VS_LOG_ERROR << "EXCP: [" << m_state.federateName << "] requested time [" <<  (m_state.currentFederateTime + _oneStepValue) << "]" << endl;
        VS_LOG_ERROR << "EXCP: [" << m_state.federateName << "] federate time [" << m_state.currentFederateTime << "]" << endl;
        VS_LOG_ERROR << "EXCP: [" << m_state.federateName << "] one step value [" << _oneStepValue << "]" << endl;
        return false;
    }

    return true;
}

void RTIFederate::advanceTimeRequestNonBlocked( const TRtiTime _oneStepValue ){

    try{
#ifdef HLA_TIME_DOUBLE
        HLAfloat64Time newTime( m_federateState->currentFederateTime + _oneStepValue );
#else
        HLAinteger64Time newTime( m_state.currentFederateTime + _oneStepValue );
#endif

        // все оборачивать в мютекс нельзя, т.к. где то в клиенском коде вертится цикл с runRTICallbacks(), который тоже вызывает RTIAmbassador
        m_FederateServicesMutex.lock();        
        m_RTIAmbassador->timeAdvanceRequest( newTime );
        m_FederateServicesMutex.unlock();
    }
    catch( Exception & ex ){
        VS_LOG_ERROR << "EXCP: [" << m_state.federateName << "] advanceTimeNonBlocked [" << rti_utils::wstrToStr( ex.what() ) << "]" << endl;
        VS_LOG_INFO << "EXCP: [" << m_state.federateName << "] requested time [" << (m_state.currentFederateTime + _oneStepValue) << "]" << endl;
    }
}

bool RTIFederate::advanceTimeGranted(){

    if( ! m_state.timeAdvanceGranted.load() ){
        return false;
    }
    else{
        m_state.timeAdvanceGranted.store( false ); // reset grant-time flag
        return true;
    }
}

bool RTIFederate::registerSynchronizePoint( const std::string _syncPointName ){

    lock_guard<mutex> lock( m_FederateServicesMutex );

    try{
        VariableLengthData dummyUserSuppliedTag;
        m_RTIAmbassador->registerFederationSynchronizationPoint( rti_utils::strToWstr(_syncPointName), dummyUserSuppliedTag );
    }
    catch( Exception & ex ){
        VS_LOG_ERROR << "EXCP: [" << m_state.federateName << "] registerSynchronizePoint [" << rti_utils::wstrToStr( ex.what() ) << "]" << endl;
        return false;
    }

    return true;
}

bool RTIFederate::achieveSynchronizePoint( const std::string _syncPointName ){

    lock_guard<mutex> lock( m_FederateServicesMutex );

    try{
        m_RTIAmbassador->synchronizationPointAchieved( rti_utils::strToWstr(_syncPointName) );

        VS_LOG_INFO << "[" << m_state.federateName << "] synchro point achieved [" << _syncPointName << "]" << endl;
    }
    catch( Exception & ex ){

        VS_LOG_ERROR << "EXCP: [" << m_state.federateName << "] achieveSynchronizePoint [" << rti_utils::wstrToStr( ex.what() ) << "]" << endl;
        return false;
    }

    return true;
}

bool RTIFederate::disconnectFromRTI(){

    lock_guard<mutex> lock( m_FederateServicesMutex );

    try{
#if 0
        m_RTIAmbassador->resignFederationExecution( DELETE_OBJECTS );
        LOGGER_INFO("{} out from federation {}", m_federateState->federateName, m_federateState->currentFederationName);

        if( m_federateState->isMasterFederate ){
            m_RTIAmbassador->destroyFederationExecution( strToWstr( m_federateState->currentFederationName ) );
            LOGGER_INFO("federation has been destroyed");
        }
#endif

        m_RTIAmbassador->disconnect();
        VS_LOG_INFO << "[" << m_state.federateName << "] disconnected from RTI" << endl;
    }
    catch( Exception & ex ){
        VS_LOG_ERROR << "EXCP: [" << m_state.federateName << "] disconnectFromRTI [" << rti_utils::wstrToStr( ex.what() ) << "]" << endl;
        return false;
    }
    return true;
}

bool RTIFederate::quitFromFederation( const bool _destroyFederation ){

    lock_guard<mutex> lock( m_FederateServicesMutex );

    try{        
        m_RTIAmbassador->resignFederationExecution( DELETE_OBJECTS );
        VS_LOG_INFO << "[" << m_state.federateName << "] out from federation [" << m_state.currentFederationName << "]. Federate's objects deleted" << endl;

        if( _destroyFederation ){
            m_RTIAmbassador->destroyFederationExecution( rti_utils::strToWstr( m_state.currentFederationName ) );
            VS_LOG_INFO << "[" << m_state.federateName << "] federation [" << m_state.currentFederationName << "] has been destroyed" << endl;
        }
    }
    catch( FederatesCurrentlyJoined & ex ){
        return true;
    }
    catch( FederationExecutionDoesNotExist & ex ){
        return true;
    }
    catch( Exception & ex ){
        VS_LOG_WARN << "EXCP: [" << m_state.federateName << "] quitFromFederation [" << rti_utils::wstrToStr( ex.what() ) << "]" << endl;
        return false;
    }
    return true;
}

void RTIFederate::runRTICallbacks(){

    lock_guard<mutex> lock( m_FederateServicesMutex );

#ifdef HLA_NOT_ASYNC
//    try{
        // reset events & objects state
//        m_eventType = EventType_en::UNDEFINED; // TODO for objrepr
//        m_RTIObjectsManager->clearChangedObjectsByCallbackList(); // TODO жопа чувствую будет

        // run
        const double minSec = 0.01f, maxSec = 0.015f; // TODO довольно сильно влияет на общую задержку
        m_RTIAmbassador->evokeMultipleCallbacks( minSec, maxSec );       

        // notify additional observers
        for( IRTIFederateObserver * observer : m_rtiFederateObservers ){
            observer->callbackReflectedRTIObjects( m_RTIObjectsManager->objectsChangedByCallback() );
        }
//    }
//    catch( Exception & ex ){
//        LOGGER_ERROR("EXCP: [{}] runRTICallbacks, {}", m_state.federateName, rti_utils::wstrToStr( ex.what() ));
//    }
#else
    // dummy
    return;
//    unique_lock<mutex> lock( g_waitEventMutex );
//    m_RTIEvent.wait( lock, [this](){ return m_eventType != EventType_t::UNKNOWN; } );
#endif
}

void RTIFederate::waitEventsFromRTINonBlock(){

#ifdef HLA_NOT_ASYNC
    try{
        const double minSec = 0.01f, maxSec = 0.015f;
        m_RTIAmbassador->evokeMultipleCallbacks( minSec, maxSec );
    }
    catch( Exception & ex ){
        VS_LOG_ERROR << PRINT_HEADER << "EXCP: [" << m_state.federateName << "] evoke wait event [" << rti_utils::wstrToStr( ex.what() ) << "]" << endl;
    }
#else
    // dummy
    return;
//    unique_lock<mutex> lock( g_waitEventMutex );
//    m_RTIEvent.wait( lock, [this](){ return m_eventType != EventType_t::UNKNOWN; } );
#endif
}

bool RTIFederate::listFederations(){

    // TODO валит Open-RTI Server к хренам "Ошибка сегментирования (сделан дамп памяти)"

#if 1
    try{
        m_RTIAmbassador->listFederationExecutions();
    }
    catch( Exception & ex ){
        VS_LOG_ERROR << PRINT_HEADER << "EXCP: [" << m_state.federateName << "] list federations [" << rti_utils::wstrToStr( ex.what() ) << "]" << endl;
        return false;
    }
#endif
    return true;
}

bool RTIFederate::waitForOtherFederates( const double _secToWait ){

    lock_guard<mutex> lock( m_FederateServicesMutex );

    map<string, bool> federatesToWait; // TODO

    /*
     * мастер-федерат учитывает самого себя сразу
     */
    auto iter = federatesToWait.find( m_state.federateName );
    if( iter != federatesToWait.end() ){
        federatesToWait[ m_state.federateName ] = true;
    }

    const int64_t millisecToWait       = _secToWait * 1000;
    const int64_t startMillisecWait    = common_utils::getCurrentTimeMillisec();
    const int64_t timeToEscape         = startMillisecWait + millisecToWait;
    const int listenFrequency       = 100;

//    logger() << "wait other federates for " << _secToWait << " seconds";
//    logger() << "listening federates every " << listenFrequency << " millisec...";

    while( common_utils::getCurrentTimeMillisec() <= timeToEscape ){

        runRTICallbacks();

        EEventType ev = getEvent();
        if( EEventType::OBJECT_FROM_RTI == ev ){

            vector<RTIObjectPtr> & vec = m_RTIObjectsManager->objectsChangedByCallback();

            for( RTIObjectPtr & obj : vec ){

                if( RTIObjectFederateInfo::m_RTI_OBJECT_CLASS_NAME == obj->getClassName() ){
                    RTIObjectFederateInfoPtr msg = dynamic_pointer_cast<RTIObjectFederateInfo>( obj );

                    // TODO index
//                    logger() << "incoming federate name:" << msg->m_attrsSnapshots[ 0 ].federateName;

                    auto iter = federatesToWait.find( msg->m_attrsSnapshotIn.federateName );
                    if( iter != federatesToWait.end() ){

                        federatesToWait[ msg->m_attrsSnapshotIn.federateName ] = true;
                    }
                }
            }
        }

        this_thread::sleep_for( chrono::milliseconds(listenFrequency) );
    }

//    logger() << "Wait complete: total ms " << getCurrentTimeMilliSec() - startMillisecWait;

    bool someoneNotConnected = false;

//    logger() << "expected federates ( 1 - ok, 0 - not connected ): ";
    for( const auto & pair : federatesToWait ){

//        logger() << pair.first << " = " << pair.second;

        if( false == pair.second ){
            someoneNotConnected = true;
        }
    }

    if( someoneNotConnected ){
        return false;
    }

    return true;
}

bool RTIFederate::switchToFederation( const std::string & _federationName ){

    // при выходе из федерации RTI удаляет объекты этого федерата, сам же класс тоже должен обнулить контекст
    m_state.currentFederateTime = 0.0f; // TODO: std::numeric_limits<TRtiTime>::min()
    m_RTIObjectsManager->resetManager();

    if( ! quitFromFederation() ){
        return false;
    }
    if( ! createFederation(_federationName) ){
        return false;
    }
    if( ! joinToFederation(_federationName) ){
        return false;
    }    

    m_state.currentFederationName = _federationName;

    return true;
}

EEventType RTIFederate::getEvent(){

    lock_guard<mutex> lk( m_FederateServicesMutex );

    EEventType temp = m_eventType;
    m_eventType = EEventType::UNDEFINED;

    return temp;
}

void RTIFederate::getLastError( std::string & _errorStr ){

    _errorStr = "FEDERATE ERROR: " + m_lastError;
}

}














