
#include <cassert>
#include <string.h>

#include <microservice_common/common/ms_common_utils.h>

#include "rti_objects_manager.h"
#include "rti_object_federate_info.h"

namespace rti_client_vega {

using namespace std;
using namespace rti1516e;

#define OBJECT_CLASS_NAME   "RTIObjectFederateInfo" // TODO не путать с именами других классов объектов, иначе хер поймешь
#define ARG_federateName    "m_federateName"
#define ARG_jsonMessage     "m_jsonMessage"

const string            RTIObjectFederateInfo::m_RTI_OBJECT_CLASS_NAME = OBJECT_CLASS_NAME;
const vector<string>    RTIObjectFederateInfo::m_RTI_ATTR_NAMES{ ARG_federateName, ARG_jsonMessage };

RTIObjectFederateInfo::RTIObjectFederateInfo( const std::string _instanceName ) : RTIObject(_instanceName){

}

void RTIObjectFederateInfo::setData( const rti1516e::AttributeHandleValueMap & _dataFromRTI ){

    // TODO save RTI attributes
//    m_attributeHandleValueMap = _dataFromRTI;

    Attributes_t infoAttributes;

    // set values from RTI
    map< string, AttributeHandle> & attrs = m_objectClassInfo->attributes;
    for( auto & attrInfo : attrs ){

        const VariableLengthData & vld = _dataFromRTI.find( attrInfo.second )->second;

        if( ARG_federateName == attrInfo.first ){
            infoAttributes.federateName.assign( (const char *)vld.data(), vld.size() );
        }
        else if( ARG_jsonMessage == attrInfo.first ){
            infoAttributes.jsonMessage.assign( (const char *)vld.data(), vld.size() );
        }
    }

    m_attrsSnapshotsOut.push( infoAttributes );

    cout << "Coming from callback m_federateName: " << infoAttributes.federateName << endl;
    cout << "Coming from callback m_jsonMessage: " << infoAttributes.jsonMessage << endl;
}

rti1516e::AttributeHandleValueMap & RTIObjectFederateInfo::getData(){

    map< string, AttributeHandle> & attrs = m_objectClassInfo->attributes;
    m_attributeHandleValueMap.clear();

    // set values to RTI
    VariableLengthData attr1Val;
    attr1Val.setDataPointer( (void*)m_attrsSnapshotIn.federateName.data(), m_attrsSnapshotIn.federateName.size() );
    m_attributeHandleValueMap.insert( {attrs[ ARG_federateName ], attr1Val} );

    VariableLengthData attr2Val;
    attr2Val.setDataPointer( (void*)m_attrsSnapshotIn.jsonMessage.data(), m_attrsSnapshotIn.jsonMessage.size() );
    m_attributeHandleValueMap.insert( {attrs[ ARG_jsonMessage ], attr2Val} );

    return m_attributeHandleValueMap;
}

}

















