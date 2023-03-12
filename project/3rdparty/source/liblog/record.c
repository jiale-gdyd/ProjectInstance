#include <errno.h>

#include "record.h"
#include "logc_defs.h"

void liblog_record_profile(struct liblog_record *a_record, int flag)
{
    logc_assert(a_record, );
    logc_profile(flag, "--record:[%p][%s:%p]--", a_record, a_record->name, a_record->output);
}

void liblog_record_del(struct liblog_record *a_record)
{
    logc_assert(a_record, );
    logc_debug("liblog_record_del[%p]", a_record);
    free(a_record);
}

struct liblog_record *liblog_record_new(const char *name, liblog_record_func output)
{
    struct liblog_record *a_record;

    logc_assert(name, NULL);
    logc_assert(output, NULL);

    a_record = (struct liblog_record *)calloc(1, sizeof(struct liblog_record));
    if (!a_record) {
        logc_error("calloc fail, errno[%d]", errno);
        return NULL;
    }

    if (strlen(name) > (sizeof(a_record->name) - 1)) {
        logc_error("name[%s] is too long", name);
        goto err;
    }

    strcpy(a_record->name, name);
    a_record->output = output;

    liblog_record_profile(a_record, LOGC_DEBUG);
    return a_record;

err:
    liblog_record_del(a_record);
    return NULL;
}
