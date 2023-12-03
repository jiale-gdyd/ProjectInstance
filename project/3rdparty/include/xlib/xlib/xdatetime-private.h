#ifndef __X_DATETIME_PRIVATE_H__
#define __X_DATETIME_PRIVATE_H__

#include "../xlib.h"

X_BEGIN_DECLS

typedef struct {
    enum {
        X_ERA_DATE_SET,
        X_ERA_DATE_PLUS_INFINITY,
        X_ERA_DATE_MINUS_INFINITY,
    } type;
    int year;
    int month;
    int day;
} XEraDate;

int _x_era_date_compare(const XEraDate *date1, const XEraDate *date2);

typedef struct {
    xatomicrefcount ref_count;
    int             direction_multiplier;
    xuint64         offset;
    XEraDate        start_date;
    XEraDate        end_date;
    char            *era_name;
    char            *era_format;
} XEraDescriptionSegment;

XPtrArray *_x_era_description_parse(const char *desc);

XEraDescriptionSegment *_x_era_description_segment_ref(XEraDescriptionSegment *segment);
void _x_era_description_segment_unref(XEraDescriptionSegment *segment);

X_END_DECLS

#endif
