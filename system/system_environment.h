#ifndef SYSTEM_ENVIRONMENT_H
#define SYSTEM_ENVIRONMENT_H

#include <unordered_map>

#include <dss_common/common/common_types.h>

class SystemEnvironment
{
public:
    struct SServiceLocator {
        SServiceLocator()
        {}        
    };

    struct SInitSettings {
        SInitSettings()
        {}
        SServiceLocator services;
        std::vector<common_types::TPid> zombieChildProcesses;
    };

    struct SState {
        SInitSettings settings;
        std::string lastError;
    };

    SystemEnvironment();
    ~SystemEnvironment();

    bool init( SInitSettings _settings );
    const SState & getState(){ return m_state; }

    bool openContext( common_types::TContextId _ctxId );
    bool closeContext();


private:
    bool isApplicationInstanceUnique();
    void writePidFile();

    // data
    SState m_state;

    // service





};

#endif // SYSTEM_ENVIRONMENT_H

