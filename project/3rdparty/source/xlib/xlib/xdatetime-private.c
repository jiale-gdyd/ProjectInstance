#include <xlib/xlib.h>
#include <xlib/xlib/xdatetime-private.h>

int _x_era_date_compare(const XEraDate *date1, const XEraDate *date2)
{
    if (date1->type == X_ERA_DATE_SET && date2->type == X_ERA_DATE_SET) {
        if (date1->year != date2->year) {
            return date1->year - date2->year;
        }

        if (date1->month != date2->month) {
            return date1->month - date2->month;
        }

        return date1->day - date2->day;
    }

    if (date1->type == date2->type) {
        return 0;
    }

    if (date1->type == X_ERA_DATE_MINUS_INFINITY || date2->type == X_ERA_DATE_PLUS_INFINITY) {
        return -1;
    }

    if (date1->type == X_ERA_DATE_PLUS_INFINITY || date2->type == X_ERA_DATE_MINUS_INFINITY) {
        return 1;
    }

    x_assert_not_reached ();
}

static xboolean parse_era_date(const char *str, const char *endptr, XEraDate *out_date)
{
    int year_multiplier;
    xuint64 year, month, day;
    const char *str_endptr = NULL;

    year_multiplier = (str[0] == '-') ? -1 : 1;
    if (str[0] == '-' || str[0] == '+') {
        str++;
    }

    year = x_ascii_strtoull(str, (xchar **)&str_endptr, 10);
    x_assert(str_endptr <= endptr);

    if (str_endptr == endptr || *str_endptr != '/' || year > X_MAXINT) {
        return FALSE;
    }
    str = str_endptr + 1;

    month = x_ascii_strtoull(str, (xchar **)&str_endptr, 10);
    x_assert(str_endptr <= endptr);

    if (str_endptr == endptr || *str_endptr != '/' || month < 1 || month > 12) {
        return FALSE;
    }
    str = str_endptr + 1;

    day = x_ascii_strtoull(str, (xchar **)&str_endptr, 10);
    x_assert(str_endptr <= endptr);

    if (str_endptr != endptr || day < 1 || day > 31) {
        return FALSE;
    }

    out_date->type = X_ERA_DATE_SET;
    out_date->year = year_multiplier * year;
    out_date->month = month;
    out_date->day = day;

    return TRUE;
}

XEraDescriptionSegment *_x_era_description_segment_ref(XEraDescriptionSegment *segment)
{
    x_atomic_ref_count_inc(&segment->ref_count);
    return segment;
}

void _x_era_description_segment_unref(XEraDescriptionSegment *segment)
{
    if (x_atomic_ref_count_dec(&segment->ref_count)) {
        x_free(segment->era_format);
        x_free(segment->era_name);
        x_free(segment);
    }
}

XPtrArray *_x_era_description_parse(const char *desc)
{
    XPtrArray *segments = x_ptr_array_new_with_free_func((XDestroyNotify)_x_era_description_segment_unref);

    for (const char *p = desc; *p != '\0';) {
        char direction;
        xuint64 offset;
        XEraDate start_date, end_date;
        const char *next_colon, *endptr = NULL;
        XEraDescriptionSegment *segment = NULL;
        char *era_name = NULL, *era_format = NULL;

        direction = *p++;
        if (direction != '+' && direction != '-') {
            goto error;
        }

        if (*p++ != ':') {
            goto error;
        }

        next_colon = strchr(p, ':');
        if (next_colon == NULL) {
            goto error;
        }

        offset = x_ascii_strtoull(p, (xchar **)&endptr, 10);
        if (endptr != next_colon) {
            goto error;
        }
        p = next_colon + 1;

        next_colon = strchr(p, ':');
        if (next_colon == NULL) {
            goto error;
        }

        if (!parse_era_date(p, next_colon, &start_date)) {
            goto error;
        }
        p = next_colon + 1;

        next_colon = strchr(p, ':');
        if (next_colon == NULL) {
            goto error;
        }

        if (strncmp(p, "-*", 2) == 0) {
            end_date.type = X_ERA_DATE_MINUS_INFINITY;
        } else if (strncmp(p, "+*", 2) == 0) {
            end_date.type = X_ERA_DATE_PLUS_INFINITY;
        } else if (!parse_era_date(p, next_colon, &end_date)) {
            goto error;
        }
        p = next_colon + 1;

        next_colon = strchr(p, ':');
        if (next_colon == NULL) {
            goto error;
        }

        if (next_colon - p == 0) {
            goto error;
        }
        era_name = x_strndup(p, next_colon - p);
        p = next_colon + 1;

        next_colon = strchr(p, ';');
        if (next_colon == NULL) {
            next_colon = p + strlen(p);
        }

        if (next_colon - p == 0) {
            x_free(era_name);
            goto error;
        }

        era_format = x_strndup(p, next_colon - p);
        if (*next_colon == ';') {
            p = next_colon + 1;
        } else {
            p = next_colon;
        }

        segment = x_new0(XEraDescriptionSegment, 1);
        x_atomic_ref_count_init(&segment->ref_count);
        segment->offset = offset;
        segment->start_date = start_date;
        segment->end_date = end_date;
        segment->direction_multiplier = ((_x_era_date_compare(&segment->start_date, &segment->end_date) <= 0) ? 1 : -1) * ((direction == '-') ? -1 : 1);
        segment->era_name = x_steal_pointer(&era_name);
        segment->era_format = x_steal_pointer(&era_format);

        x_ptr_array_add(segments, x_steal_pointer(&segment));
    }

    return x_steal_pointer(&segments);

error:
    x_ptr_array_unref(segments);
    return NULL;
}
