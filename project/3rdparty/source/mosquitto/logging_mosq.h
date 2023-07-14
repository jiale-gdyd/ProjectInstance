#ifndef MOSQ_LOGGING_MOSQ_H
#define MOSQ_LOGGING_MOSQ_H

#include <mosquitto/mosquitto.h>

#ifndef __GNUC__
#define __attribute__(attrib)
#endif

int log__printf(struct mosquitto *mosq, unsigned int level, const char *fmt, ...) __attribute__((format(printf, 3, 4)));

#endif
