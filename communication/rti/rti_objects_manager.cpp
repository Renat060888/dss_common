
#include <algorithm>
#include <thread>
#include <chrono>

#include <RTI/time/HLAinteger64Time.h>        // TODO: 'int' throws exception in Open-RTI implementation
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64TimeFactory.h>
#include <RTI/time/HLAfloat64Time.h>
#include <RTI/time/HLAfloat64Interval.h>
#include <RTI/time/HLAfloat64TimeFactory.h>
#include <microservice_common/common/ms_common_utils.h>
#include <microservice_common/system/logger.h>
#include <microservice_common/system/class_factory.h>

#include "rti_object_federate_info.h"
#include "rti_objects_manager.h"
#include "rti_object_aero.h"
#include "rti_object.h"

namespace rti_client_vega {

using namespace std;
using namespace rti1516e;

RTIObjectsManager::RTIObjectsManager( RTIFederate * _federatePtr ) : m_federatePtr(_federatePtr){

    // init class factory
    CLASS_FACTORY.addClassFactory<RTIObjectFederateInfo>( RTIObjectFederateInfo::m_RTI_OBJECT_CLASS_NAME );
    CLASS_FACTORY.addClassFactory<RTIObjectAero>( RTIObjectAero::m_RTI_OBJECT_CLASS_NAME );
}

RTIObjectsManager::~RTIObjectsManager(){}

bool RTIObjectsManager::removeObject( RTIObjectPtr _obj ){

    // TODO
}

bool RTIObjectsManager::publishOrSubcribeObjectClass( const bool _publish, const string & _objClassName , const vector<string> &_objAttrNames ){

    lock_guard<mutex> lock(m_RTIObjectManagerMutex);    

    auto it = m_RTIObjectClassInfoMap.find( _objClassName );

    // exists obj class (previous published or subscribed)
    if( it != m_RTIObjectClassInfoMap.end() ){

        // TODO
//        logger(WARN) << "attempt to publish/subscribe once more: " << _objClassName;

        RTIObjectClassInfoPtr & objClassInfo = it->second;

        AttributeHandleSet ahs;
        for( auto & it : objClassInfo->attributes ){
            ahs.insert( it.second );
        }

        if( _publish )  m_federatePtr->m_RTIAmbassador->publishObjectClassAttributes( objClassInfo->classHandle, ahs );
        else            m_federatePtr->m_RTIAmbassador->subscribeObjectClassAttributes( objClassInfo->classHandle, ahs );
    }
    // new obj class
    else{
        RTIObjectClassInfoPtr objClassInfo = make_shared<RTIObjectClassInfo_t>();

        try{
            objClassInfo->classHandle = m_federatePtr->m_RTIAmbassador->getObjectClassHandle( rti_utils::strToWstr(_objClassName) );
        }
        catch( Exception & ex ){
            VS_LOG_ERROR << "EXCP: publishOrSubcribeObjectClass [" << rti_utils::wstrToStr( ex.what() ) << "]" << endl;
            return false;
        }

        AttributeHandleSet ahs;

        for( const string & attr : _objAttrNames ){

            try{

                AttributeHandle attrHandle = m_federatePtr->m_RTIAmbassador->getAttributeHandle( objClassInfo->classHandle, rti_utils::strToWstr(attr) );
                objClassInfo->attributes.insert( {attr, attrHandle} );
                ahs.insert( attrHandle );
            }
            catch( Exception & ex ){
                VS_LOG_ERROR << "EXCP: publishOrSubcribeObjectClass [" << rti_utils::wstrToStr( ex.what() ) << "]" << endl;
                return false;
            }
        }

        if( _publish )  m_federatePtr->m_RTIAmbassador->publishObjectClassAttributes( objClassInfo->classHandle, ahs );
        else            m_federatePtr->m_RTIAmbassador->subscribeObjectClassAttributes( objClassInfo->classHandle, ahs );

        m_RTIObjectClassInfoMap.insert( {_objClassName, objClassInfo} );
    }

    if( _publish ){
        VS_LOG_INFO << "published [" << _objClassName << "]" << endl;
    }
    else{
        VS_LOG_INFO << "subscribed to [" << _objClassName << "]" << endl;
    }

    return true;
}

void RTIObjectsManager::requestAttributeValuesUpdate( const std::string & _objInstanceName ){

    lock_guard<mutex> lock(m_RTIObjectManagerMutex); // TODO

    // find object
    string nameToFind = _objInstanceName;
    bool objectFound = false;

    for( auto it : m_RTIObjects ){

        RTIObjectPtr & obj = it.second;

        if( nameToFind == obj->getInstanceName() ){

            // find class name handle
            objectFound = true;

            auto iter = m_RTIObjectClassInfoMap.find( obj->getClassName() );
            if( iter != m_RTIObjectClassInfoMap.end() ){

                RTIObjectClassInfoPtr & classinfo = iter->second;

                AttributeHandleSet ahs;
                for( auto & it : classinfo->attributes ){
                    ahs.insert( it.second );
                }

                m_federatePtr->m_RTIAmbassador->requestAttributeValueUpdate( classinfo->classHandle, ahs, VariableLengthData() );
            }
        }
    }

    if( ! objectFound ){
        VS_LOG_ERROR << "object [" << _objInstanceName << "] not found in RTIObjectManager" << endl;
    }
}

void RTIObjectsManager::resetManager(){

    m_objInstancesReservStatus.clear();

    m_objectsChangedByCallback.clear();

    m_RTIObjects.clear();

    m_RTIObjectClassInfoMap.clear();
}

bool RTIObjectsManager::publishInteraction(){

    lock_guard<mutex> lock(m_RTIObjectManagerMutex);

    // TODO

    return true;
}

bool RTIObjectsManager::subcribeInteraction(){

    lock_guard<mutex> lock(m_RTIObjectManagerMutex);

    // TODO

    return true;
}

bool RTIObjectsManager::updateTSOObjectToRTI( RTIObjectPtr & _obj, const double _targetTime ){

    lock_guard<mutex> lock(m_RTIObjectManagerMutex);

    try{
#ifdef HLA_TIME_DOUBLE
        auto_ptr<HLAfloat64Time> time( new HLAfloat64Time(_targetTime) );
//        HLAfloat64Time time( _targetTime ); // TODO
#else
        auto_ptr<HLAinteger64Time> time( new HLAinteger64Time(_targetTime) );
//        HLAinteger64Time time( _targetTime ); // TODO
#endif

        m_federatePtr->m_RTIAmbassador->updateAttributeValues( _obj->m_objInstHandle, _obj->getData(), VariableLengthData(), * time );
    }
    catch( rti1516e::Exception & ex ){
        VS_LOG_ERROR << "EXCP: updateObjectToRTI [" << rti_utils::wstrToStr( ex.what() ) << "]" << endl;
        return false;
    }

    return true;
}

bool RTIObjectsManager::updateObjectToRTI( RTIObjectPtr _obj ){

    lock_guard<mutex> lock(m_RTIObjectManagerMutex);

    try{
        m_federatePtr->m_RTIAmbassador->updateAttributeValues( _obj->m_objInstHandle, _obj->getData(), VariableLengthData() );
    }
    catch( rti1516e::Exception & ex ){
        VS_LOG_ERROR << "EXCP: updateObjectToRTI [" << _obj->getInstanceName() << "] Reason [" << rti_utils::wstrToStr( ex.what() ) << "]" << endl;
        return false;
    }

    return true;
}

void RTIObjectsManager::requestObjectOwnership( RTIObjectPtr _obj, const bool _blocked ){

    lock_guard<mutex> lock(m_RTIObjectManagerMutex);

    try{
        // collect attrs
        auto iter = m_RTIObjectClassInfoMap.find( _obj->m_objClassName );
        RTIObjectClassInfoPtr objectClassinfo = iter->second;

        AttributeHandleSet rtiAttributeHandleSet;
        for( auto & attrIter : objectClassinfo->attributes ){

            const AttributeHandle & ah = attrIter.second;

            rtiAttributeHandleSet.insert( ah );
        }

        // call RTI
        m_federatePtr->m_RTIAmbassador->attributeOwnershipAcquisition( _obj->m_objInstHandle, rtiAttributeHandleSet, VariableLengthData() );

        if( _blocked ){
            while( ! _obj->isOwner() ){
                // wait
            }
        }
    }
    catch( rti1516e::Exception & ex ){
        VS_LOG_ERROR << "EXCP: requestObjectOwnership [" << rti_utils::wstrToStr( ex.what() ) << "]" << endl;
    }
}

void RTIObjectsManager::callbackObjectOwnershipRequest( rti1516e::ObjectInstanceHandle & _theObject,
                                                        rti1516e::AttributeHandleSet const & _candidateAttributes,
                                                        rti1516e::VariableLengthData const & _theUserSuppliedTag ){

    auto iter = m_RTIObjects.find( _theObject );
    if( iter != m_RTIObjects.end() ){

        RTIObjectPtr & object = iter->second;
        VS_LOG_INFO << "local object [" << object->getInstanceName() << "] ownership requested from other federate. Accept ownership transfer" << endl;

        // collect attrs
        auto iter = m_RTIObjectClassInfoMap.find( object->m_objClassName );
        RTIObjectClassInfoPtr objectClassinfo = iter->second;

        AttributeHandleSet rtiAttributeHandleSet;
        for( auto & attrIter : objectClassinfo->attributes ){

            const AttributeHandle & ah = attrIter.second;

            rtiAttributeHandleSet.insert( ah );
        }

        // call RTI
        object->m_owner = false;
        m_federatePtr->m_RTIAmbassador->unconditionalAttributeOwnershipDivestiture( object->m_objInstHandle, rtiAttributeHandleSet );
    }
    else{
        VS_LOG_ERROR << "callback object ownership request fail, such object instance handle not found [" << rti_utils::wstrToStr( _theObject.toString() ) << "]" << endl;
    }
}

void RTIObjectsManager::callbackOwnershipRequestSuccess(    rti1516e::ObjectInstanceHandle & _theObject,
                                                            rti1516e::AttributeHandleSet const & _candidateAttributes,
                                                            rti1516e::VariableLengthData const & _theUserSuppliedTag ){

    auto iter = m_RTIObjects.find( _theObject );
    if( iter != m_RTIObjects.end() ){

        RTIObjectPtr & object = iter->second;
        VS_LOG_INFO << "object [" << object->getInstanceName() << "] ownership capture success !" << endl;

        object->m_owner = true;
    }
    else{
        VS_LOG_ERROR << "callback object ownership success fail, such object instance handle not found [" << rti_utils::wstrToStr( _theObject.toString() ) << "]" << endl;
    }

}

bool RTIObjectsManager::callbackProvideObjectAttrsUpdate( rti1516e::ObjectInstanceHandle _theObject, rti1516e::AttributeHandleSet const & _theAttributes ){

    lock_guard<mutex> lock(m_RTIObjectManagerMutex);

    auto iter = m_RTIObjects.find( _theObject );
    if( iter != m_RTIObjects.end() ){

        RTIObjectPtr obj = iter->second;

        try{
            m_federatePtr->m_RTIAmbassador->updateAttributeValues( iter->first, obj->getData(), VariableLengthData() );
        }
        catch( rti1516e::Exception & ex ){
            VS_LOG_ERROR << "EXCP: update attrs values [" << rti_utils::wstrToStr( ex.what() ) << "]" << endl;
            return false;
        }
    }
    else{
        VS_LOG_ERROR << "RTIObjectManager - provide object attrs update fail, don't found object instance handle" << endl;
        return false;
    }

    return true;
}

RTIObjectPtr RTIObjectsManager::getObject( const std::string _objInstanceName ){

    string nameToFind = m_federatePtr->getState().federateName + "_" + _objInstanceName;

    for( auto it : m_RTIObjects ){

        RTIObjectPtr & obj = it.second;

        if( nameToFind == obj->getInstanceName() ){
            return obj;
        }
    }

    VS_LOG_ERROR << "object by name: [" << nameToFind << "] not found" << endl;

    return RTIObjectPtr();
}

bool RTIObjectsManager::checkInstanceName(const std::string & _name){

    // no mutex: non-public method

    m_objInstancesReservStatus.insert( {_name, false} );

    m_federatePtr->m_RTIAmbassador->reserveObjectInstanceName( rti_utils::strToWstr(_name) );

    this_thread::sleep_for( chrono::milliseconds(10) ); // TODO почему то без паузы RTI не успевает выработать callback

    // чтобы RTI успела обновить "m_objInstancesReservStatus"
#ifdef HLA_NOT_ASYNC
    const double minSec = 0.1f, maxSec = 0.2f;
    m_federatePtr->m_RTIAmbassador->evokeMultipleCallbacks( minSec, maxSec );
#endif

    this_thread::sleep_for( chrono::milliseconds(10) ); // TODO почему то без паузы RTI не успевает выработать callback

    return m_objInstancesReservStatus.find( _name )->second;
}

void RTIObjectsManager::setReservStatusCallback( string _instName, bool _status ){

    // no mutex: conflict Federate = createObject <-> RTI = setStatus

    auto it = m_objInstancesReservStatus.find( _instName );
    if( it != m_objInstancesReservStatus.end() ){

        it->second = _status;
    }
    else{
        VS_LOG_ERROR << "not found such instance name for reserv status set [" << _instName << "]" << endl;
    }
}

void RTIObjectsManager::callbackObjectReflectFromRTI(   rti1516e::ObjectInstanceHandle          & _objInstHandle,
                                                        rti1516e::AttributeHandleValueMap const & _theAttrValues,
                                                        rti1516e::VariableLengthData const      & _theUserSuppliedTag){

    lock_guard<mutex> lock(m_RTIObjectManagerMutex);

    auto it = m_RTIObjects.find( _objInstHandle );
    if( it != m_RTIObjects.end() ){

        RTIObjectPtr & obj = it->second;

        obj->setData( _theAttrValues );

        m_objectsChangedByCallback.push_back( obj );
    }
    else{
        VS_LOG_ERROR << "object instance handle not found in MAIN CONTAINER" << endl;
    }

    m_federatePtr->m_eventType = EEventType::OBJECT_FROM_RTI;

    // TODO
//    m_RTIEventPtr->notify_one();
}

vector<RTIObjectPtr> & RTIObjectsManager::objectsChangedByCallback(){

    lock_guard<mutex> lock(m_RTIObjectManagerMutex);

    return m_objectsChangedByCallback;
}

//=====================
// get all objects
//=====================
vector<RTIObjectPtr> RTIObjectsManager::getAllObjects(){

    lock_guard<mutex> lock(m_RTIObjectManagerMutex);

    vector<RTIObjectPtr> objects;
    objects.resize( m_RTIObjects.size() );

    int i = 0;
    for( auto iter = m_RTIObjects.begin(); iter != m_RTIObjects.end(); ++iter, i++ ){
        objects[ i ] = iter->second;
    }

    return objects;
}

void RTIObjectsManager::objectDiscoverFromRTICallback(  rti1516e::ObjectInstanceHandle  & _theObject,
                                                rti1516e::ObjectClassHandle     & _objClassHandle,
                                                std::wstring const              & _theObjectInstanceName ){

    lock_guard<mutex> lock(m_RTIObjectManagerMutex);

    RTIObjectPtr obj;

    for( auto classInfoPair : m_RTIObjectClassInfoMap ){

        RTIObjectClassInfoPtr & classInfo = classInfoPair.second;

        if( classInfo->classHandle == _objClassHandle ){

            obj = CLASS_FACTORY.createInstanceOf( classInfoPair.first, rti_utils::wstrToStr(_theObjectInstanceName) );
            obj->m_objectClassInfo = classInfo;
            obj->m_objClassName = classInfoPair.first;
            break;
        }
    }

    if( ! obj ){
        VS_LOG_ERROR << "discovered object not created" << endl;
        return;
    }

    obj->m_instanceName = rti_utils::wstrToStr( _theObjectInstanceName );
    obj->m_objInstHandle = _theObject;    
    obj->m_owner = false;

    m_RTIObjects.insert( {_theObject, obj} );
}

}




