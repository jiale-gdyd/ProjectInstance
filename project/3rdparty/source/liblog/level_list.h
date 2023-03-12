#ifndef LIBLOG_LEVEL_LIST_H
#define LIBLOG_LEVEL_LIST_H

#include "level.h"
#include "logc_defs.h"

struct logc_arraylist *liblog_level_list_new(void);
void liblog_level_list_del(struct logc_arraylist *levels);
void liblog_level_list_profile(struct logc_arraylist *levels, int flag);

int liblog_level_list_atoi(struct logc_arraylist *levels, char *str);
int liblog_level_list_set(struct logc_arraylist *levels, char *line);
struct liblog_level *liblog_level_list_get(struct logc_arraylist *levels, int l);

#endif
