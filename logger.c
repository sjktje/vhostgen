#include <syslog.h>
#include <stdarg.h>

void log_info(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    openlog("vhostgen", LOG_CONS | LOG_PID, LOG_INFO);
    vsyslog(LOG_NOTICE, fmt, va);
    closelog();
    va_end(va);
}
