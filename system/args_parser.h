#ifndef ARGS_PARSER_H
#define ARGS_PARSER_H

#include <microservice_common/system/a_args_parser.h>

enum class EPlayerArguments {
    SHELL_COMMAND_START_SERVER,
    SHELL_COMMAND_TO_SERVER,
    AS_DAEMON,
    MAIN_CONFIG_PATH_FROM_CONSOLE,
    NODE_AGENT_SIMULATION,
    NODE_WORKER_SIMULATION,
    UNDEFINED
};

class ArgsParser : public AArgsParser<EPlayerArguments>
{
public:
    static ArgsParser & singleton(){
        static ArgsParser instance;
        return instance;
    }


private:
    ArgsParser(){}
    ~ArgsParser(){}

    ArgsParser( const ArgsParser & _inst ) = delete;
    ArgsParser & operator=( const ArgsParser & _inst ) = delete;

    virtual bpo::options_description getArgumentsDescrTemplateMethodPart() override;
    virtual void checkArgumentsTemplateMethodPart( const bpo::variables_map & _varMap ) override;
    virtual void version() override;
    virtual void about() override;

};
#define ARGS_PARSER ArgsParser::singleton()

#endif // ARGS_PARSER_H
