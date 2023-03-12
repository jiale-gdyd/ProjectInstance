#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "logc_defs.h"
#include "record_table.h"

void liblog_record_table_profile(struct logc_hashtable *records, int flag)
{
    struct liblog_record *a_record;
    struct logc_hashtable_entry *a_entry;

    logc_assert(records, );
    logc_profile(flag, "-record_table[%p]-", records);
    logc_hashtable_foreach(records, a_entry) {
        a_record = (struct liblog_record *)a_entry->value;
        liblog_record_profile(a_record, flag);
    }
}

void liblog_record_table_del(struct logc_hashtable *records)
{
    logc_assert(records, );
    logc_hashtable_del(records);
    logc_debug("liblog_record_table_del[%p]", records);
}

struct logc_hashtable *liblog_record_table_new(void)
{
    struct logc_hashtable *records;

    records = logc_hashtable_new(20, (hashtable_hash_func)logc_hashtable_str_hash, (hashtable_equal_func)logc_hashtable_str_equal, NULL, (hashtable_del_func)liblog_record_del);
    if (!records) {
        logc_error("logc_hashtable_new fail");
        return NULL;
    } else {
        liblog_record_table_profile(records, LOGC_DEBUG);
        return records;
    }
}
