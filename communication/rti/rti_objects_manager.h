#ifndef RTI_OBJECTS_MANAGER_H
#define RTI_OBJECTS_MANAGER_H

#include <map>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <chrono>
#include <string>
#include <memory>
#include <condition_variable>

#include <RTI/RTIambassador.h>

#include "federate.h"
#include "federate_ambassador.h"
#include "rti_object.h"
#include "rti_object_federate_info.h"
#include "rti_object_aero.h"

// частичная специализация шаблона std::hash, чтобы rti1516e::ObjectInstanceHandle использовать как хеш-ключ в unordered_map
namespace std {

template<>
struct hash<rti1516e::ObjectInstanceHandle>{
    std::size_t operator()(const rti1516e::ObjectInstanceHandle & k) const {
        return k.hash();
    }
};
}

namespace rti_client_vega {

struct SFederateState;

struct RTIObjectClassInfo_t {

    rti1516e::ObjectClassHandle                         classHandle;
    std::map< std::string, rti1516e::AttributeHandle>   attributes;
    bool                                                published;
    bool                                                subscribed;
};

typedef std::shared_ptr<RTIObjectClassInfo_t>   RTIObjectClassInfoPtr;
typedef std::string objectClassName_tdf;


class RTIObjectsManager{


public: // for federate

    RTIObjectsManager( RTIFederate * _federatePtr );


    ~RTIObjectsManager();

    template< typename T_RTIObject >
    bool publishObjectClass(){

        if( ! publishOrSubcribeObjectClass( true, T_RTIObject::m_RTI_OBJECT_CLASS_NAME, T_RTIObject::m_RTI_ATTR_NAMES ) ){
            return false;
        }
        return true;
    }

    template< typename T_RTIObject >
    bool subcribeObjectClass(){

        if( ! publishOrSubcribeObjectClass( false, T_RTIObject::m_RTI_OBJECT_CLASS_NAME, T_RTIObject::m_RTI_ATTR_NAMES ) ){
            return false;
        }
        return true;
    }

    bool publishInteraction();
    bool subcribeInteraction();

    template< typename ObjectType >
    RTIObjectPtr createObject( const std::string & _objInstanceName ){

        // TODO watch for deadlock (опять чуть не нарвался)
//        std::lock_guard<std::mutex> lock(m_RTIObjectManagerMutex); // TODO watch for deadlock

        std::lock_guard<std::mutex> lock( m_federatePtr->m_FederateServicesMutex );

        std::string objInstanceName = m_federatePtr->getState().federateName + "_" + _objInstanceName;

        if( ! checkInstanceName( objInstanceName ) ){
            return nullptr;
        }

        std::shared_ptr<ObjectType> newObj;

        auto it = m_RTIObjectClassInfoMap.find( ObjectType::m_RTI_OBJECT_CLASS_NAME );
        if( it != m_RTIObjectClassInfoMap.end() ){

            RTIObjectClassInfoPtr & objClassInfo = it->second;

            newObj = std::make_shared< ObjectType >(objInstanceName);

            newObj->m_objInstHandle = m_federatePtr->m_RTIAmbassador->registerObjectInstance( objClassInfo->classHandle, rti_utils::strToWstr(objInstanceName) );
            newObj->m_objClassName = ObjectType::m_RTI_OBJECT_CLASS_NAME;
            newObj->m_objectClassInfo = objClassInfo;
            newObj->m_owner = true;
        }
        else{
            // TODO
//            Logger::singleton().m_log->error("create object instance fail, not found objClassName: {}", ObjectType::m_RTI_OBJECT_CLASS_NAME);
            std::cerr << "ERROR: create object instance fail, not found objClassName: " << ObjectType::m_RTI_OBJECT_CLASS_NAME << std::endl;
            return nullptr;
        }

        m_RTIObjects.insert( {newObj->m_objInstHandle, newObj} );

        /*
         * TODO после создания объекта должно пройти некоторое время перед отсылкой его обновлений
         */
        std::this_thread::sleep_for( std::chrono::milliseconds(1) );

        return newObj;
    }

    bool removeObject( RTIObjectPtr _obj );

    // get object
    RTIObjectPtr getObject( const std::string _objInstanceName );

    // write object
    bool updateTSOObjectToRTI( RTIObjectPtr & _obj, const double _targetTime );
    bool updateObjectToRTI( RTIObjectPtr _obj );

    // read objects
    std::vector<RTIObjectPtr> & objectsChangedByCallback();
    void clearChangedObjectsByCallbackList(){ m_objectsChangedByCallback.clear(); }
    std::vector<RTIObjectPtr> getAllObjects();

    // ownership
    void requestObjectOwnership( RTIObjectPtr _obj, const bool _blocked = false );


    void requestAttributeValuesUpdate( const std::string & _objInstanceName );

    void resetManager();

    // calls from RTI
    void objectDiscoverFromRTICallback( rti1516e::ObjectInstanceHandle  & _theObject,
                                        rti1516e::ObjectClassHandle     & _theObjectClass,
                                        std::wstring const              & _theObjectInstanceName );

    void callbackObjectReflectFromRTI(  rti1516e::ObjectInstanceHandle          & _objInstHandle,
                                        rti1516e::AttributeHandleValueMap const & _theAttrValues,
                                        rti1516e::VariableLengthData const      & _theUserSuppliedTag);

    void callbackObjectOwnershipRequest(rti1516e::ObjectInstanceHandle & _theObject,
                                        rti1516e::AttributeHandleSet const & _candidateAttributes,
                                        rti1516e::VariableLengthData const & _theUserSuppliedTag );

    void callbackOwnershipRequestSuccess(rti1516e::ObjectInstanceHandle & _theObject,
                                        rti1516e::AttributeHandleSet const & _candidateAttributes,
                                        rti1516e::VariableLengthData const & _theUserSuppliedTag );


    void setReservStatusCallback( std::string _instName, bool _status );

    bool callbackProvideObjectAttrsUpdate( rti1516e::ObjectInstanceHandle _theObject, rti1516e::AttributeHandleSet const & _theAttributes );

private:

    bool checkInstanceName( const std::string & _name );

    bool publishOrSubcribeObjectClass( const bool _publish, const std::string & _objClassName , const std::vector<std::string> &_objAttrNames );

    // for federate functional use
    RTIFederate * m_federatePtr;
    // synchronyze read/write to RTIObjectManager
    std::mutex m_RTIObjectManagerMutex;
    //
    std::vector<RTIObjectPtr> m_objectsChangedByCallback;
    //
    std::map< std::string, bool > m_objInstancesReservStatus;
    // MAIN CONTAINER for all federate's objects
    std::unordered_map< rti1516e::ObjectInstanceHandle, RTIObjectPtr > m_RTIObjects;
    // generic info about RTI object classes
    std::map< objectClassName_tdf, RTIObjectClassInfoPtr > m_RTIObjectClassInfoMap;
};

}

#endif // RTI_OBJECTS_MANAGER_H
