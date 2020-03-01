
#include "args_parser.h"

using namespace std;

bpo::options_description ArgsParser::getArgumentsDescrTemplateMethodPart() {

    bpo::options_description desc("Available options");

    desc.add_options()
            ("help,h","show this help")
            ("version,V", "version of program")
            ("about,A", "description")
            ("main-config,C", bpo::value<std::string>(), "main config file")
            ("main-config-sample,P", "print main config sample")
            ("daemon,D", "start as daemon")
            ("start", "start server")
            ("stop", "stop server")
            ;

    return desc;
}

void ArgsParser::checkArgumentsTemplateMethodPart( const bpo::variables_map & _varMap ) {

    if( _varMap.find("main-config") != _varMap.end() ){
        m_commmandLineArgs[EDssArguments::MAIN_CONFIG_PATH_FROM_CONSOLE] = _varMap["main-config"].as<std::string>();
    }

    if( _varMap.find("daemon") != _varMap.end() ){
        m_commmandLineArgs[EDssArguments::AS_DAEMON] = "bla-bla";
    }

    if( _varMap.find("start") != _varMap.end() ){
        m_commmandLineArgs[EDssArguments::SHELL_COMMAND_START_SERVER] = "bla-bla";
    }

    if( _varMap.find("stop") != _varMap.end() ){
        m_commmandLineArgs[EDssArguments::SHELL_COMMAND_TO_SERVER]
            = AArgsParser::getSettings().commandConvertor->getCommandsFromProgramArgs( { {"", ""} } );
    }
}

void ArgsParser::version() {

    // TODO: do
}

void ArgsParser::about() {

    // TODO: do
}
