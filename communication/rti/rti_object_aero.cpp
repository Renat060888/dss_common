
// std
#include <cassert>
#include <string.h>
// project
#include "rti_objects_manager.h"
#include "rti_object_aero.h"

namespace rti_client_vega {

using namespace std;
using namespace rti1516e;

#define OBJECT_CLASS_NAME   "RTIObjectAero" // TODO не путать с именами других классов объектов, иначе хер поймешь
#define ARG_data            "m_data"
#define ARG_federateName    "m_federateName"

const string            RTIObjectAero::m_RTI_OBJECT_CLASS_NAME = OBJECT_CLASS_NAME;
const vector<string>    RTIObjectAero::m_RTI_ATTR_NAMES{ ARG_federateName, ARG_data };

RTIObjectAero::RTIObjectAero( const std::string _instanceName ) : RTIObject(_instanceName){

}

void RTIObjectAero::setData( const rti1516e::AttributeHandleValueMap & _dataFromRTI ){

    // TODO save RTI attributes
//    m_attributeHandleValueMap = _dataFromRTI;

    Attributes_t infoAttributes;

    map< string, AttributeHandle> & attrs = m_objectClassInfo->attributes;

    // set values from RTI
    for( auto & attrInfo : attrs ){

        const VariableLengthData & vld = _dataFromRTI.find( attrInfo.second )->second;

        if( ARG_federateName == attrInfo.first ){

            char str[ vld.size() + 1 ];
            memcpy( (void*) & str, vld.data(), vld.size() );
            str[ vld.size() ] = '\0';
            infoAttributes.federateName.assign( str );
        }
        else if( ARG_data == attrInfo.first ){
            infoAttributes.binaryPtr = new char[ vld.size() ];
            infoAttributes.binaryLen = vld.size();
            memcpy( (void*) infoAttributes.binaryPtr, vld.data(), vld.size() );
        }
    }

    m_dataIncoming.push( infoAttributes );

    cout << "Coming from callback m_federateName: " << infoAttributes.federateName << endl;
    cout << "Coming from callback m_data len: " << infoAttributes.binaryLen << endl;
}

rti1516e::AttributeHandleValueMap & RTIObjectAero::getData(){

    map< string, AttributeHandle> & attrs = m_objectClassInfo->attributes;
    m_attributeHandleValueMap.clear();

    // ----------------------------
    // set values to RTI
    // ----------------------------
    VariableLengthData attr1Val;
    attr1Val.setDataPointer( (void*) m_dataOutcoming.federateName.data(), m_dataOutcoming.federateName.size() );
    m_attributeHandleValueMap.insert( {attrs[ ARG_federateName ], attr1Val} );

    VariableLengthData attr2Val;
    attr2Val.setDataPointer( (void*) m_dataOutcoming.binaryPtr, m_dataOutcoming.binaryLen );
    m_attributeHandleValueMap.insert( {attrs[ ARG_data ], attr2Val} );

    return m_attributeHandleValueMap;
}

}

















