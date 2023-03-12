#ifndef RECORD_TABLE_H
#define RECORD_TABLE_H

#include "record.h"
#include "logc_defs.h"

struct logc_hashtable *liblog_record_table_new(void);
void liblog_record_table_del(struct logc_hashtable *records);
void liblog_record_table_profile(struct logc_hashtable *records, int flag);

#endif
