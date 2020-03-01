#pragma once

// std
#include <string>
// project
#include "rti_object.h"

namespace rti_client_vega {

typedef std::shared_ptr<class RTIObjectAero> RTIObjectAeroPtr;

class RTIObjectAero : public RTIObject {

public:

    RTIObjectAero( const std::string _instanceName );

    static const std::string                m_RTI_OBJECT_CLASS_NAME;
    static const std::vector<std::string>   m_RTI_ATTR_NAMES;

    /*
     * разумнее передавать просто json-строку и знать что она пришла полностью, затем распарсить ее (парсер заодно проверит ее валидность)
     * при этом структура доп. классов проще и проще FOM модель. вероятность что где то пропадет байт и тд - меньше
     */

    struct Attributes_t {
        Attributes_t() : federateName("empty_str"), binaryPtr(nullptr),binaryLen(0){}

        char *          binaryPtr;
        int             binaryLen;
        std::string     federateName;
    };

    Attributes_t m_dataOutcoming;
    std::queue< Attributes_t > m_dataIncoming;

    std::string &   getFederateNameRef() { return m_dataOutcoming.federateName; }
    void            getDataRef( char * _dataPtr, int & _dataLen ) { m_dataOutcoming.binaryPtr = _dataPtr;
                                                                    m_dataOutcoming.binaryLen = _dataLen; }


private:

    virtual void setData( const rti1516e::AttributeHandleValueMap & _dataFromRTI ) override;
    virtual rti1516e::AttributeHandleValueMap & getData() override;

};

}

