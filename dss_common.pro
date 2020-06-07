ROOT_DIR=../

TEMPLATE = lib
TARGET = dss_common

include($${ROOT_DIR}pri/common.pri)

CONFIG -= qt
CONFIG += plugin
#CONFIG += release

QMAKE_CXXFLAGS += -Wno-unused-parameter
QMAKE_CXXFLAGS += -Wno-unused-variable

# TODO: add defines to logger, system monitor, restbed webserver, database, etc...
DEFINES += \
    SWITCH_LOGGER_ASTRA \

LIBS += \
    -lpthread \
    -lrti1516e \
    -lfedtime1516e \
    -lFedTime \     # OpenRTI
    -lOpenRTI \     # OpenRTI
    -lRTI-NG \      # OpenRTI
    -lboost_program_options \ # TODO: wtf?
    -lboost_regex \
    -lunilog \  # TODO: wtf?
    -lobjrepr \  # TODO: wtf?
    -lmicroservice_common \

# NOTE: paths for dev environment ( all projects sources in one dir )
INCLUDEPATH +=  \
    /usr/include/libmongoc-1.0 \
    /usr/include/libbson-1.0 \
    /usr/include/rti1516e/ \
    $${ROOT_DIR}/microservice_common/ \

SOURCES += \
    communication/rti/federate.cpp \
    communication/rti/federate_ambassador.cpp \
    communication/rti/rti_object_aero.cpp \
    communication/rti/rti_object_federate_info.cpp \
    communication/rti/rti_objects_manager.cpp \
    system/args_parser.cpp \
    system/config_reader.cpp \
    system/path_locator.cpp \

HEADERS += \
    common/common_types.h \
    common/common_vars.h \
    communication/rti/federate.h \
    communication/rti/federate_ambassador.h \
    communication/rti/rti_base_types.h \
    communication/rti/rti_object.h \
    communication/rti/rti_object_aero.h \
    communication/rti/rti_object_federate_info.h \
    communication/rti/rti_objects_manager.h \
    system/args_parser.h \
    system/config_reader.h \
    system/path_locator.h \
