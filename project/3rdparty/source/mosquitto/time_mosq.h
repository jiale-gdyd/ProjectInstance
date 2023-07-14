#ifndef MOSQ_TIME_MOSQ_H
#define MOSQ_TIME_MOSQ_H

#include <time.h>

time_t mosquitto_time(void);
void mosquitto_time_ns(time_t *s, long *ns);

#endif
