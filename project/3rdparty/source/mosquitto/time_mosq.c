#include <unistd.h>
#include <mosquitto/mosquitto.h>

#include "config.h"
#include "time_mosq.h"

time_t mosquitto_time(void)
{
    return time(NULL);
}
