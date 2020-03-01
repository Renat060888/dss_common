#pragma once
// teamcity
// std
#include <string>
#include <vector>
#include <map>
#include <memory> // for RTI auto_ptr
#include <iostream>
// 3rd party
#include "RTI/RTIambassador.h"

namespace rti_client_vega {

// TODO дочерние классы желательно создавать скриптом в виде cpp-файлов из оригинального FOM файла с описанием классов объектов
class RTIObject;
typedef std::shared_ptr<RTIObject> RTIObjectPtr;
class RTIObject{

public:

    RTIObject( const std::string _instanceName ) :
        m_owner(false),
        m_gdmId(0),
        m_instanceName(_instanceName){}
    virtual ~RTIObject(){}

    const std::string & getClassName(){ return m_objClassName; }
    const std::string & getInstanceName(){ return m_instanceName; }

    bool isOwner(){ return m_owner; }

    bool operator<( const RTIObject & _rhs ){
        return ( this->m_gdmId < _rhs.m_gdmId );
    }

protected:

    rti1516e::AttributeHandleValueMap m_attributeHandleValueMap;

    std::shared_ptr<class RTIObjectClassInfo_t> m_objectClassInfo;

private:

    friend class RTIObjectsManager;

    virtual rti1516e::AttributeHandleValueMap & getData() = 0;
    virtual void setData( const rti1516e::AttributeHandleValueMap & _dataFromRTI ) = 0;

    rti1516e::ObjectInstanceHandle  m_objInstHandle;

    bool                            m_owner;
    int64_t                         m_gdmId;
    std::string                     m_objClassName;
    std::string                     m_instanceName;

    // non-copyable
    RTIObject( const RTIObject & inst );
    RTIObject & operator ==( const RTIObject & inst );


    // при достаточно малой паузе пересылки изменений - сообщения RTI, которые не синхронизированы временной меткой, могут приходить с такой скоростью,
    // что невозможно успеть уследить за всеми изменениями объекта за 1 callback ( например объект кидается дважды с интервалом 10 мс )
    // ( потому в RTI-объектах без временной метки есть очередь, в которую складываются все его изменения, пришедшие из RTI за 1 callback )
};

class RTIObjectPtrMapComparator{
public:
    bool operator()( const RTIObjectPtr & _lhs, const RTIObjectPtr & _rhs ) const {

        return ( (* _lhs) < (* _rhs) );
    }
};

}


















