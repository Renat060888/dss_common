ROOT_DIR=../

TEMPLATE = lib
TARGET = dss_common

include($${ROOT_DIR}pri/common.pri)

CONFIG -= qt
CONFIG += plugin
#CONFIG += release

QMAKE_CXXFLAGS += -Wno-unused-parameter
QMAKE_CXXFLAGS += -Wno-unused-variable

DEFINES += \
    SWITCH_LOGGER_SIMPLE \

LIBS += \
    -lpthread \
    -lrti1516e \
    -lfedtime1516e \
    -lFedTime \     # OpenRTI
    -lOpenRTI \     # OpenRTI
    -lRTI-NG \      # OpenRTI
    -lboost_program_options \ # TODO: wtf?
    -lboost_regex \
    -lmicroservice_common \

# NOTE: paths for dev environment ( all projects sources in one dir )
INCLUDEPATH +=  \
    $${INSTALL_PATH_INCLUDE}/rti1516e/ \
    $${ROOT_DIR}/microservice_common/ \

SOURCES += \
    communication/rti/federate.cpp \
    communication/rti/federate_ambassador.cpp \
    communication/rti/rti_object_aero.cpp \
    communication/rti/rti_object_federate_info.cpp \
    communication/rti/rti_objects_manager.cpp \
    system/args_parser.cpp \
    system/config_reader.cpp \
    system/objrepr_bus.cpp \
    system/path_locator.cpp \
    system/system_environment.cpp

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
    system/objrepr_bus.h \
    system/path_locator.h \
    system/system_environment.h