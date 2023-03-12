#ifndef LIBLOG_RECORD_H
#define LIBLOG_RECORD_H

#include "logc_defs.h"
#include <liblog/liblog.h>

struct liblog_record {
    char               name[LOG_MAXLEN_PATH + 1];
    liblog_record_func output;
};

void liblog_record_del(struct liblog_record *a_record);
void liblog_record_profile(struct liblog_record *a_record, int flag);
struct liblog_record *liblog_record_new(const char *name, liblog_record_func output);

#endif
