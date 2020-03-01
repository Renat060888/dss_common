#ifndef RTI_OBJECT_FEDERATE_INFO_H
#define RTI_OBJECT_FEDERATE_INFO_H

#include <string>
#include <queue>

#include "rti_object.h"

namespace rti_client_vega {

typedef std::shared_ptr<class RTIObjectFederateInfo> RTIObjectFederateInfoPtr;

class RTIObjectFederateInfo : public RTIObject
{
public:
    RTIObjectFederateInfo( const std::string _instanceName );

    static const std::string m_RTI_OBJECT_CLASS_NAME;
    static const std::vector<std::string> m_RTI_ATTR_NAMES;

    // разумнее передавать просто json-строку и знать что она пришла полностью, затем распарсить ее (парсер заодно проверит ее валидность)
    // при этом структура доп. классов проще и проще FOM модель. вероятность что где то пропадет байт и тд - меньше
    struct Attributes_t {
        Attributes_t()
            : federateName("empty_str")
            , jsonMessage("empty_str")
        {}
        std::string federateName;
        std::string jsonMessage;
    };

    Attributes_t m_attrsSnapshotIn;
    std::queue< Attributes_t > m_attrsSnapshotsOut;

    std::string & getFederateNameRef() { return m_attrsSnapshotIn.federateName; }
    std::string & getJsonMessageRef() { return m_attrsSnapshotIn.jsonMessage; }


private:
    virtual void setData( const rti1516e::AttributeHandleValueMap & _dataFromRTI ) override;

    virtual rti1516e::AttributeHandleValueMap & getData() override;

};

}

#endif // RTI_OBJECT_FEDERATE_INFO_H
