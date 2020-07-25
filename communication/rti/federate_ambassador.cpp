
#include <RTI/time/HLAinteger64Time.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64TimeFactory.h>
#include <microservice_common/common/ms_common_utils.h>
#include <microservice_common/system/logger.h>

#include "federate_ambassador.h"
#include "rti_objects_manager.h"

//#define CALLBACK_PRINT_ENABLE

using namespace std;

//=====================
// CTOR
//=====================
FederateAmbassadorImpl::FederateAmbassadorImpl( rti_client_vega::RTIObjectsManager *_mgr, rti_client_vega::SFederateState * _RTICallbackFlags, const string _synchPointNameForMasterFederate ) :
    m_RTIObjectsManagerPtr( _mgr ),
    m_federateStatePtr(_RTICallbackFlags),
    m_synchPointNameForMasterFederate(_synchPointNameForMasterFederate){

}

//=====================
// DTOR
//=====================
FederateAmbassadorImpl::~FederateAmbassadorImpl() throw() {

}

/*
 *                                      OBJECT MANAGEMENT CALLBACKS
 */

//=====================
// discoverObjectInstance
//=====================
void FederateAmbassadorImpl::discoverObjectInstance(
    ObjectInstanceHandle theObject,
    ObjectClassHandle theObjectClass,
    std::wstring const & theObjectInstanceName)
    throw( FederateInternalError ){

#ifdef CALLBACK_PRINT_ENABLE
    Logger::singleton().m_log->debug( "RTI callback - detected instance of subcribed object: {}", dss_utils::wstrToStr(theObjectInstanceName) );
#endif

    m_RTIObjectsManagerPtr->objectDiscoverFromRTICallback( theObject, theObjectClass, theObjectInstanceName );

    // TODO требовать обновление, если объект уже долгое время существует но данные не пришли, а для работы нужны
    // m_RTIObjectsManagerPtr->requestAttributeValuesUpdate( dss_utils::wstrToStr(theObjectInstanceName) );
}

void FederateAmbassadorImpl::provideAttributeValueUpdate (
   ObjectInstanceHandle theObject,
   AttributeHandleSet const & theAttributes,
   VariableLengthData const & theUserSuppliedTag)
   throw (
      FederateInternalError) {

#ifdef CALLBACK_PRINT_ENABLE
    Logger::singleton().m_log->debug( "RTI callback - provide attrubute value update" );
#endif

    m_RTIObjectsManagerPtr->callbackProvideObjectAttrsUpdate( theObject, theAttributes );
}

//=====================
// reflectAttributeValues
//=====================
void FederateAmbassadorImpl::reflectAttributeValues(
    ObjectInstanceHandle theObject,
    AttributeHandleValueMap const & theAttributeValues,
    VariableLengthData const & theUserSuppliedTag,
    OrderType sentOrder,
    TransportationType theType,
    SupplementalReflectInfo theReflectInfo)
    throw ( FederateInternalError ){

#ifdef CALLBACK_PRINT_ENABLE
    Logger::singleton().m_log->debug( "RTI callback - detected attr reflect of subcribed object" );
#endif

    m_RTIObjectsManagerPtr->callbackObjectReflectFromRTI( theObject, theAttributeValues, theUserSuppliedTag );
}

//=====================
// reflectAttributeValues
//=====================
void FederateAmbassadorImpl::reflectAttributeValues (
   ObjectInstanceHandle theObject,
   AttributeHandleValueMap const & theAttributeValues,
   VariableLengthData const & theUserSuppliedTag,
   OrderType sentOrder,
   TransportationType theType,
   LogicalTime const & theTime,
   OrderType receivedOrder,
   SupplementalReflectInfo theReflectInfo)
   throw (
      FederateInternalError){

#ifdef CALLBACK_PRINT_ENABLE
    Logger::singleton().m_log->debug( "RTI callback - detected attr reflect 2 of subcribed object" );
#endif

    m_RTIObjectsManagerPtr->callbackObjectReflectFromRTI( theObject, theAttributeValues, theUserSuppliedTag );
}

//=====================
// reflectAttributeValues
//=====================
void FederateAmbassadorImpl::reflectAttributeValues (
   ObjectInstanceHandle theObject,
   AttributeHandleValueMap const & theAttributeValues,
   VariableLengthData const & theUserSuppliedTag,
   OrderType sentOrder,
   TransportationType theType,
   LogicalTime const & theTime,
   OrderType receivedOrder,
   MessageRetractionHandle theHandle,
   SupplementalReflectInfo theReflectInfo)
   throw (
      FederateInternalError){

#ifdef CALLBACK_PRINT_ENABLE
    Logger::singleton().m_log->debug( "RTI callback - detected attr reflect 3 of subcribed object" );
#endif

    m_RTIObjectsManagerPtr->callbackObjectReflectFromRTI( theObject, theAttributeValues, theUserSuppliedTag );
}

//=====================
// objectInstanceNameReservationSucceeded
//=====================
void FederateAmbassadorImpl::objectInstanceNameReservationSucceeded (
   std::wstring const & theObjectInstanceName)
   throw (
      FederateInternalError) {

#ifdef CALLBACK_PRINT_ENABLE
    Logger::singleton().m_log->debug( "RTI callback - name reservation success: {}", dss_utils::wstrToStr(theObjectInstanceName) );
#endif

    m_RTIObjectsManagerPtr->setReservStatusCallback( rti_utils::wstrToStr(theObjectInstanceName), true );
}

//=====================
// objectInstanceNameReservationFailed
//=====================
void FederateAmbassadorImpl::objectInstanceNameReservationFailed (
   std::wstring const & theObjectInstanceName)
   throw (
      FederateInternalError) {

    VS_LOG_DBG << "RTI callback - name reservation failed [" << rti_utils::wstrToStr(theObjectInstanceName) << "]" << endl;

    m_RTIObjectsManagerPtr->setReservStatusCallback( rti_utils::wstrToStr(theObjectInstanceName), false );
}

/*
 *                                      TIME MANAGEMENT CALLBACKS
 */

//=====================
// timeAdvanceGrant
//=====================
void FederateAmbassadorImpl::timeAdvanceGrant (
   LogicalTime const & theTime)
   throw (
      FederateInternalError){

    // TODO: may be wstring too slooow ?
    m_federateStatePtr->currentFederateTime = rti_utils::convertTime( theTime.toString() );

    m_federateStatePtr->timeAdvanceGranted.store( true );

//#ifdef CALLBACK_PRINT_ENABLE
//    Logger::singleton().m_log->debug("RTI callback - time advance granted, current federate time: {}", m_federateStatePtr->currentFederateTime);
//#endif
}

void FederateAmbassadorImpl::timeRegulationEnabled (
   LogicalTime const & theFederateTime)
   throw (
      FederateInternalError) {

    m_federateStatePtr->currentFederateTime = rti_utils::convertTime( theFederateTime.toString() );
    m_federateStatePtr->timeRegulationEnabled = true;

    if( m_federateStatePtr->currentFederateTime != 0 ){

        m_federateStatePtr->isMasterFederate.store( false );
        m_federateStatePtr->federationSynchronized.store( true );
    }

    VS_LOG_DBG << "RTI callback - time regulation enabled, current federate time [" << m_federateStatePtr->currentFederateTime << "]" << endl;
}

void FederateAmbassadorImpl::timeConstrainedEnabled (
   LogicalTime const & theFederateTime)
   throw (
      FederateInternalError) {

    m_federateStatePtr->currentFederateTime = rti_utils::convertTime( theFederateTime.toString() );
    m_federateStatePtr->timeConstrainEnabled = true;

    if( m_federateStatePtr->currentFederateTime != 0 ){

        m_federateStatePtr->isMasterFederate.store( false );
        m_federateStatePtr->federationSynchronized.store( true );
    }

    VS_LOG_DBG << "RTI callback - time constrained enabled, current federate time [" << m_federateStatePtr->currentFederateTime << "]" << endl;
}

/*
 *                                      - FEDERATION MANAGEMENT CALLBACKS -
 */

//=====================
// connectionLost
//=====================
void FederateAmbassadorImpl::connectionLost (
   std::wstring const & faultDescription)
   throw (
      FederateInternalError){

    VS_LOG_DBG << "RTI callback - connection lost [" << rti_utils::wstrToStr(faultDescription) << "]" << endl;
}

void FederateAmbassadorImpl::reportFederationExecutions( FederationExecutionInformationVector const & theFederationExecutionInformationList)
    throw (FederateInternalError){

    VS_LOG_INFO << "federationExecutionName %% logicalTimeImplementationName:";
    for( const FederationExecutionInformation & fei : theFederationExecutionInformationList ){

        VS_LOG_INFO << "[" << rti_utils::wstrToStr(fei.federationExecutionName) << "] %% [" << rti_utils::wstrToStr(fei.logicalTimeImplementationName) << "]" << endl;
    }

    VS_LOG_DBG << "RTI callback - report federation executions" << endl;
}

//=====================
// synchronizationPointRegistrationSucceeded
//=====================
void FederateAmbassadorImpl::synchronizationPointRegistrationSucceeded (
   std::wstring const & label)
   throw (
      FederateInternalError) {

    VS_LOG_DBG << "RTI callback - synchronization point registration succeeded [" << rti_utils::wstrToStr(label) << "]" << endl;

    /*
     * Предназначено для единовременного старта моделирования.
     * Если федерат первым успел зарегить точку синхронизации - то он считается Мастер-Федератом
     * и ждет других (сверяет по списку имена пришедших по RTI федератов) для начала моделирования
     */
    if( m_synchPointNameForMasterFederate == rti_utils::wstrToStr(label) && m_federateStatePtr->currentFederateTime == 0 ){

        m_federateStatePtr->isMasterFederate.store( true );
        VS_LOG_INFO << "synch point for Master-Federate [" << m_synchPointNameForMasterFederate << "] registered SUCCESS, become Master Federate !" << endl;
    }
}

//=====================
// synchronizationPointRegistrationFailed
//=====================
void FederateAmbassadorImpl::synchronizationPointRegistrationFailed (
   std::wstring const & label,
   SynchronizationPointFailureReason reason)
   throw (
      FederateInternalError) {

    VS_LOG_WARN << "RTI callback - sync point registration failed: SP ALREADY EXIST - [" << rti_utils::wstrToStr(label) << "]" << endl;
    VS_LOG_WARN << "this federate not Master" << endl;

    if( m_synchPointNameForMasterFederate == rti_utils::wstrToStr(label)){

        m_federateStatePtr->isMasterFederate.store( false );
    }
}

//=====================
// announceSynchronizationPoint
//=====================
void FederateAmbassadorImpl::announceSynchronizationPoint (
   std::wstring  const & label,
   VariableLengthData const & theUserSuppliedTag)
   throw (
      FederateInternalError) {

    VS_LOG_DBG << "RTI callback - announce synchronization point [" << rti_utils::wstrToStr(label) << "]" << endl;

}

//=====================
// federationSynchronized
//=====================
void FederateAmbassadorImpl::federationSynchronized (
   std::wstring const & label,
   FederateHandleSet const& failedToSyncSet)
   throw (
      FederateInternalError) {

    VS_LOG_DBG << "RTI callback - federation synchronized [" << rti_utils::wstrToStr(label) << "]" << endl;

    m_federateStatePtr->federationSynchronized.store( true );
}

/*
 *                                      - OWNERSHIP MANAGEMENT SERVICES -
 */
void FederateAmbassadorImpl::requestAttributeOwnershipAssumption(  ObjectInstanceHandle theObject,
                                                                   AttributeHandleSet const & offeredAttributes,
                                                                   VariableLengthData const & theUserSuppliedTag ) throw (FederateInternalError) {

    VS_LOG_DBG << "RTI callback - requestAttributeOwnershipAssumption" << endl;
}

void FederateAmbassadorImpl::requestDivestitureConfirmation (   ObjectInstanceHandle theObject,
                                                                AttributeHandleSet const & releasedAttributes) throw (FederateInternalError) {

    VS_LOG_DBG << "RTI callback - requestDivestitureConfirmation" << endl;
}

void FederateAmbassadorImpl::attributeOwnershipAcquisitionNotification (    ObjectInstanceHandle theObject,
                                                                            AttributeHandleSet const & securedAttributes,
                                                                            VariableLengthData const & theUserSuppliedTag) throw (FederateInternalError) {

    VS_LOG_DBG << "RTI callback - attributeOwnershipAcquisitionNotification" << endl;
    m_RTIObjectsManagerPtr->callbackOwnershipRequestSuccess( theObject, securedAttributes, theUserSuppliedTag );
}

void FederateAmbassadorImpl::attributeOwnershipUnavailable (    ObjectInstanceHandle theObject,
                                                                AttributeHandleSet const & theAttributes) throw (FederateInternalError) {

    VS_LOG_DBG << "RTI callback - attributeOwnershipUnavailable" << endl;
}

void FederateAmbassadorImpl::requestAttributeOwnershipRelease ( ObjectInstanceHandle theObject,
                                                                AttributeHandleSet const & candidateAttributes,
                                                                VariableLengthData const & theUserSuppliedTag) throw (FederateInternalError) {

    VS_LOG_DBG << "RTI callback - requestAttributeOwnershipRelease" << endl;
    m_RTIObjectsManagerPtr->callbackObjectOwnershipRequest( theObject, candidateAttributes, theUserSuppliedTag );
}

void FederateAmbassadorImpl::confirmAttributeOwnershipAcquisitionCancellation ( ObjectInstanceHandle theObject,
                                                                                AttributeHandleSet const & theAttributes) throw (FederateInternalError) {

    VS_LOG_DBG << "RTI callback - confirmAttributeOwnershipAcquisitionCancellation" << endl;

}

void FederateAmbassadorImpl::informAttributeOwnership ( ObjectInstanceHandle theObject,
                                                        AttributeHandle theAttribute,
                                                        FederateHandle theOwner) throw (FederateInternalError) {

    VS_LOG_DBG << "RTI callback - informAttributeOwnership" << endl;

}

void FederateAmbassadorImpl::attributeIsNotOwned (  ObjectInstanceHandle theObject,
                                                    AttributeHandle theAttribute) throw (FederateInternalError) {

    VS_LOG_DBG << "RTI callback - attributeIsNotOwned" << endl;

}

void FederateAmbassadorImpl::attributeIsOwnedByRTI (    ObjectInstanceHandle theObject,
                                                        AttributeHandle theAttribute) throw (FederateInternalError) {

    VS_LOG_DBG << "RTI callback - attributeIsOwnedByRTI" << endl;

}
















