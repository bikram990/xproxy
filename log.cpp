#include "log.h"

namespace xproxy { namespace log {

void InitLogging(const std::string& conf_file) {
    log4cpp::PropertyConfigurator::configure(conf_file);
}

}}
