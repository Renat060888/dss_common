
#include <iostream>
#include <sys/stat.h>

#include <microservice_common/common/ms_common_utils.h>
#include <microservice_common/system/logger.h>
#include <microservice_common/system/process_launcher.h>

#include "system_environment.h"
#include "config_reader.h"
#include "args_parser.h"
#include "objrepr_bus.h"
#include "path_locator.h"

using namespace std;

static constexpr const char * PRINT_HEADER = "SystemEnvironment:";

SystemEnvironment::SystemEnvironment()
{

}

SystemEnvironment::~SystemEnvironment()
{
    VS_LOG_INFO << PRINT_HEADER << " begin shutdown" << endl;

    OBJREPR_BUS.shutdown();

    VS_LOG_INFO << PRINT_HEADER << " shutdown success" << endl;
}

bool SystemEnvironment::init( SInitSettings _settings ){

    m_state.settings = _settings;

    // unique check
    if( ! isApplicationInstanceUnique() ){
        return false;
    }

    // TODO: what for ?
    ::umask( 0 );

    // objrepr
    if( ! OBJREPR_BUS.init() ){
        VS_LOG_CRITICAL << "objrepr init failed: " << OBJREPR_BUS.getLastError() << endl;
        return false;
    }    

    // process-zombies
    for( const common_types::TPid pid : _settings.zombieChildProcesses ){
        PROCESS_LAUNCHER.kill( pid );
    }

    // pid
    writePidFile();

    VS_LOG_INFO << PRINT_HEADER << " init success" << endl;
    return true;
}

bool SystemEnvironment::isApplicationInstanceUnique(){

    const int pidFile = ::open( PATH_LOCATOR.getUniqueLockFile().c_str(), O_CREAT | O_RDWR | O_TRUNC, 0666 );

    const int rc = ::flock( pidFile, LOCK_EX | LOCK_NB );
    if( rc ){
        if( EWOULDBLOCK == errno ){
            VS_LOG_ERROR << "CRITICAL: another instance of Video Server already is running."
                      << " (file already locked: " << PATH_LOCATOR.getUniqueLockFile() << ")"
                      << " Abort"
                      << endl;
            return false;
        }
    }

    return true;
}

void SystemEnvironment::writePidFile(){

    const char * pidStr = std::to_string( ::getpid() ).c_str();
    VS_LOG_INFO << "write pid [" << pidStr << "]"
             << " to pid file [" << PATH_LOCATOR.getUniqueLockFile() << "]"
             << endl;

    ofstream pidFile( PATH_LOCATOR.getUniqueLockFile(), std::ios::out | std::ios::trunc );
    if( ! pidFile.is_open() ){
        VS_LOG_ERROR << "cannot open pid file for write" << endl;
    }

    pidFile << pidStr;
}

bool SystemEnvironment::openContext( common_types::TContextId _ctxId ){

    if( ! ObjreprBus::singleton().openContextAsync(_ctxId) ){
        m_state.lastError = ObjreprBus::singleton().getLastError();
        return false;
    }
    return true;


}

bool SystemEnvironment::closeContext(){

    if( ! ObjreprBus::singleton().closeContext() ){
        m_state.lastError = ObjreprBus::singleton().getLastError();
        return false;
    }
    return true;

}

