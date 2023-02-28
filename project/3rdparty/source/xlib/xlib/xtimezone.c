#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <xlib/xlib/config.h>
#include <xlib/xlib/xlibconfig.h>

#include <xlib/xlib/xhash.h>
#include <xlib/xlib/xdate.h>
#include <xlib/xlib/xbytes.h>
#include <xlib/xlib/xslice.h>
#include <xlib/xlib/xstdio.h>
#include <xlib/xlib/xthread.h>
#include <xlib/xlib/xenviron.h>
#include <xlib/xlib/xdatetime.h>
#include <xlib/xlib/xstrfuncs.h>
#include <xlib/xlib/xtimezone.h>
#include <xlib/xlib/xtestutils.h>
#include <xlib/xlib/xfileutils.h>
#include <xlib/xlib/xmappedfile.h>

typedef struct { xchar bytes[8]; } xint64_be;
typedef struct { xchar bytes[4]; } xint32_be;
typedef struct { xchar bytes[4]; } xuint32_be;

static inline xint64 xint64_from_be(const xint64_be be)
{
    xint64 tmp;
    memcpy(&tmp, &be, sizeof(tmp));
    return XINT64_FROM_BE(tmp);
}

static inline xint32 xint32_from_be(const xint32_be be)
{
    xint32 tmp;
    memcpy(&tmp, &be, sizeof(tmp));
    return XINT32_FROM_BE(tmp);
}

static inline xuint32 xuint32_from_be(const xuint32_be be)
{
    xuint32 tmp;
    memcpy(&tmp, &be, sizeof(tmp));
    return XUINT32_FROM_BE(tmp);
}

struct tzhead {
    xchar      tzh_magic[4];
    xchar      tzh_version;
    xuchar     tzh_reserved[15];

    xuint32_be tzh_ttisgmtcnt;
    xuint32_be tzh_ttisstdcnt;
    xuint32_be tzh_leapcnt;
    xuint32_be tzh_timecnt;
    xuint32_be tzh_typecnt;
    xuint32_be tzh_charcnt;
};

struct ttinfo {
    xint32_be tt_gmtoff;
    xuint8    tt_isdst;
    xuint8    tt_abbrind;
};

typedef struct {
    xint   year;
    xint   mon;
    xint   mday;
    xint   wday;
    xint   week;
    xint32 offset;
} TimeZoneDate;

#define NAME_SIZE           33
#define MIN_TZYEAR          1916
#define MAX_TZYEAR          2999

typedef struct {
  xuint        start_year;
  xint32       std_offset;
  xint32       dlt_offset;
  TimeZoneDate dlt_start;
  TimeZoneDate dlt_end;
  xchar        std_name[NAME_SIZE];
  xchar        dlt_name[NAME_SIZE];
} TimeZoneRule;

typedef struct {
    xint32   gmt_offset;
    xboolean is_dst;
    xchar    *abbrev;
} TransitionInfo;

typedef struct {
    xint64 time;
    xint   info_index;
} Transition;

struct _XTimeZone {
    xchar  *name;
    XArray *t_info;
    XArray *transitions;
    xint   ref_count;
};

X_LOCK_DEFINE_STATIC(time_zones);
static XHashTable *time_zones;

X_LOCK_DEFINE_STATIC(tz_default);
static XTimeZone *tz_default = NULL;

X_LOCK_DEFINE_STATIC(tz_local);
static XTimeZone *tz_local = NULL;

static XTimeZone *parse_footertz(const xchar *, size_t);

void x_time_zone_unref(XTimeZone *tz)
{
    int ref_count;

again:
    ref_count = x_atomic_int_get(&tz->ref_count);

    x_assert(ref_count > 0);

    if (ref_count == 1) {
        if (tz->name != NULL) {
            X_LOCK(time_zones);

            if X_UNLIKELY(x_atomic_int_get(&tz->ref_count) != 1) {
                X_UNLOCK(time_zones);
                goto again;
            }

            if (time_zones != NULL) {
                x_hash_table_remove(time_zones, tz->name);
            }
            X_UNLOCK(time_zones);
        }

        if (tz->t_info != NULL) {
            xuint idx;
            for (idx = 0; idx < tz->t_info->len; idx++) {
                TransitionInfo *info = &x_array_index(tz->t_info, TransitionInfo, idx);
                x_free(info->abbrev);
            }

            x_array_free(tz->t_info, TRUE);
        }

        if (tz->transitions != NULL) {
            x_array_free(tz->transitions, TRUE);
        }
        x_free(tz->name);

        x_slice_free(XTimeZone, tz);
    } else if X_UNLIKELY(!x_atomic_int_compare_and_exchange(&tz->ref_count, ref_count, ref_count - 1)) {
        goto again;
    }
}

XTimeZone *x_time_zone_ref(XTimeZone *tz)
{
    x_assert(tz->ref_count > 0);
    x_atomic_int_inc(&tz->ref_count);

    return tz;
}

static xboolean parse_time(const xchar *time_, xint32 *offset, xboolean rfc8536)
{
    if (*time_ < '0' || '9' < *time_) {
        return FALSE;
    }

    *offset = 60 * 60 * (*time_++ - '0');
    if (*time_ == '\0') {
        return TRUE;
    }

    if (*time_ != ':') {
        if (*time_ < '0' || '9' < *time_) {
            return FALSE;
        }

        *offset *= 10;
        *offset += 60 * 60 * (*time_++ - '0');

        if (rfc8536) {
            if ('0' <= *time_ && *time_ <= '9') {
                *offset *= 10;
                *offset += 60 * 60 * (*time_++ - '0');
            }

            if (*offset > 167 * 60 * 60) {
                return FALSE;
            }
        } else if (*offset > 24 * 60 * 60) {
            return FALSE;
        }

        if (*time_ == '\0') {
            return TRUE;
        }
    }

    if (*time_ == ':') {
        time_++;
    } else if (rfc8536) {
        return FALSE;
    }

    if (*time_ < '0' || '5' < *time_) {
        return FALSE;
    }

    *offset += 10 * 60 * (*time_++ - '0');
    if (*time_ < '0' || '9' < *time_) {
        return FALSE;
    }

    *offset += 60 * (*time_++ - '0');
    if (*time_ == '\0') {
        return TRUE;
    }

    if (*time_ == ':') {
        time_++;
    } else if (rfc8536) {
        return FALSE;
    }

    if (*time_ < '0' || '5' < *time_) {
        return FALSE;
    }

    *offset += 10 * (*time_++ - '0');
    if (*time_ < '0' || '9' < *time_) {
        return FALSE;
    }

    *offset += *time_++ - '0';
    return *time_ == '\0';
}

static xboolean parse_constant_offset(const xchar *name, xint32 *offset, xboolean rfc8536)
{
    if (!rfc8536 && x_strcmp0(name, "UTC") == 0) {
        *offset = 0;
        return TRUE;
    }

    if (*name >= '0' && '9' >= *name) {
        return parse_time(name, offset, rfc8536);
    }

    switch (*name++) {
        case 'Z':
            *offset = 0;
            return !rfc8536 && !*name;

        case '+':
            return parse_time(name, offset, rfc8536);

        case '-':
            if (parse_time(name, offset, rfc8536)) {
                *offset = -*offset;
                return TRUE;
            } else {
                return FALSE;
            }

        default:
            return FALSE;
    }
}

static void zone_for_constant_offset(XTimeZone *gtz, const xchar *name)
{
    xint32 offset;
    TransitionInfo info;

    if (name == NULL || !parse_constant_offset(name, &offset, FALSE)) {
        return;
    }

    info.gmt_offset = offset;
    info.is_dst = FALSE;
    info.abbrev = x_strdup(name);

    gtz->name = x_strdup(name);
    gtz->t_info = x_array_sized_new(FALSE, TRUE, sizeof(TransitionInfo), 1);
    x_array_append_val(gtz->t_info, info);

    gtz->transitions = NULL;
}

static const xchar *zone_info_base_dir(void)
{
    if (x_file_test("/usr/share/zoneinfo", X_FILE_TEST_IS_DIR)) {
        return "/usr/share/zoneinfo";
    } else if (x_file_test("/usr/share/lib/zoneinfo", X_FILE_TEST_IS_DIR)) {
        return "/usr/share/lib/zoneinfo";
    }

    return "/usr/share/zoneinfo";
}

static xchar *zone_identifier_unix(void)
{
    const xchar *tzdir;
    xsize prefix_len = 0;
    struct stat file_status;
    xchar *canonical_path = NULL;
    XError *read_link_err = NULL;
    xchar *resolved_identifier = NULL;
    xboolean not_a_symlink_to_zoneinfo = FALSE;

    resolved_identifier = x_file_read_link("/etc/localtime", &read_link_err);
    if (resolved_identifier == NULL) {
        if (!x_path_is_absolute(resolved_identifier)) {
            xchar *absolute_resolved_identifier = x_build_filename("/etc", resolved_identifier, NULL);
            x_free(resolved_identifier);
            resolved_identifier = x_steal_pointer(&absolute_resolved_identifier);
        }

        if (x_lstat(resolved_identifier, &file_status) == 0) {
            if ((file_status.st_mode & S_IFMT) != S_IFREG) {
                x_clear_pointer(&resolved_identifier, x_free);
                not_a_symlink_to_zoneinfo = TRUE;
            }
        } else {
            x_clear_pointer(&resolved_identifier, x_free);
        }
    } else {
        not_a_symlink_to_zoneinfo = x_error_matches(read_link_err, X_FILE_ERROR, X_FILE_ERROR_INVAL);
        x_clear_error(&read_link_err);
    }

    if (resolved_identifier == NULL) {
        if (not_a_symlink_to_zoneinfo && (x_file_get_contents("/var/db/zoneinfo", &resolved_identifier, NULL, NULL) || x_file_get_contents("/etc/timezone", &resolved_identifier, NULL, NULL))) {
            x_strchomp(resolved_identifier);
        } else {
            x_assert(resolved_identifier == NULL);
            goto out;
        }
    } else {
        canonical_path = x_canonicalize_filename(resolved_identifier, "/etc");
        x_free(resolved_identifier);
        resolved_identifier = x_steal_pointer(&canonical_path);
    }

    tzdir = x_getenv("TZDIR");
    if (tzdir == NULL) {
        tzdir = zone_info_base_dir();
    }

    if (x_str_has_prefix(resolved_identifier, tzdir)) {
        prefix_len = strlen(tzdir);
        while (*(resolved_identifier + prefix_len) == '/') {
            prefix_len++;
        }
    }

    if (prefix_len > 0) {
        memmove(resolved_identifier, resolved_identifier + prefix_len, strlen(resolved_identifier) - prefix_len + 1);
    }
    x_assert(resolved_identifier != NULL);

out:
    x_free(canonical_path);
    return resolved_identifier;
}

static XBytes *zone_info_unix(const xchar *identifier, const xchar *resolved_identifier)
{
    const xchar *tzdir;
    xchar *filename = NULL;
    XBytes *zoneinfo = NULL;
    XMappedFile *file = NULL;

    tzdir = x_getenv("TZDIR");
    if (tzdir == NULL) {
        tzdir = zone_info_base_dir();
    }

    if (identifier != NULL) {
        if (*identifier == ':') {
            identifier++;
        }

        if (x_path_is_absolute(identifier)) {
            filename = x_strdup(identifier);
        } else {
            filename = x_build_filename(tzdir, identifier, NULL);
        }
    } else {
        if (resolved_identifier == NULL) {
            goto out;
        }

        filename = x_strdup("/etc/localtime");
    }

    file = x_mapped_file_new(filename, FALSE, NULL);
    if (file != NULL) {
        zoneinfo = x_bytes_new_with_free_func(x_mapped_file_get_contents(file), x_mapped_file_get_length(file), (XDestroyNotify)x_mapped_file_unref, x_mapped_file_ref(file));
        x_mapped_file_unref(file);
    }

    x_assert(resolved_identifier != NULL);

out:
    x_free(filename);
    return zoneinfo;
}

static void init_zone_from_iana_info(XTimeZone *gtz, XBytes *zoneinfo, xchar *identifier)
{
    xsize size;
    xuint index;
    xuint8 *tz_abbrs;
    XTimeZone *footertz = NULL;
    xuint32 time_count, type_count;
    xsize timesize = sizeof(xint32);
    xint64 last_explicit_transition_time = 0;
    xuint extra_time_count = 0, extra_type_count = 0;
    xuint8 *tz_transitions, *tz_type_index, *tz_ttinfo;

    xconstpointer header_data = x_bytes_get_data(zoneinfo, &size);
    const xchar *data = (const xchar *)header_data;
    const struct tzhead *header = (const struct tzhead *)header_data;

    x_return_if_fail(size >= sizeof(struct tzhead) && memcmp(header, "TZif", 4) == 0);

    if (header->tzh_version >= '2') {
        header = (const struct tzhead *)
            (((const xchar *) (header + 1)) +
            xuint32_from_be(header->tzh_ttisgmtcnt) +
            xuint32_from_be(header->tzh_ttisstdcnt) +
            8 * xuint32_from_be(header->tzh_leapcnt) +
            5 * xuint32_from_be(header->tzh_timecnt) +
            6 * xuint32_from_be(header->tzh_typecnt) +
            xuint32_from_be(header->tzh_charcnt));
        timesize = sizeof(xint64);
    }

    time_count = xuint32_from_be(header->tzh_timecnt);
    type_count = xuint32_from_be(header->tzh_typecnt);

    if (header->tzh_version >= '2') {
        const xchar *footer = (((const xchar *)(header + 1))
                                + xuint32_from_be(header->tzh_ttisgmtcnt)
                                + xuint32_from_be(header->tzh_ttisstdcnt)
                                + 12 * xuint32_from_be(header->tzh_leapcnt)
                                + 9 * time_count
                                + 6 * type_count
                                + xuint32_from_be(header->tzh_charcnt));

        size_t footerlen;
        const xchar *footerlast;

        x_return_if_fail(footer <= data + size - 2 && footer[0] == '\n');
        footerlast = (const xchar *)memchr(footer + 1, '\n', data + size - (footer + 1));
        x_return_if_fail(footerlast);

        footerlen = footerlast + 1 - footer;
        if (footerlen != 2) {
            footertz = parse_footertz(footer, footerlen);
            x_return_if_fail(footertz);
            extra_type_count = footertz->t_info->len;
            extra_time_count = footertz->transitions->len;
        }
    }

    tz_transitions = ((xuint8 *)(header) + sizeof(*header));
    tz_type_index = tz_transitions + timesize * time_count;
    tz_ttinfo = tz_type_index + time_count;
    tz_abbrs = tz_ttinfo + sizeof(struct ttinfo) * type_count;

    gtz->name = x_steal_pointer(&identifier);
    gtz->t_info = x_array_sized_new(FALSE, TRUE, sizeof(TransitionInfo), type_count + extra_type_count);
    gtz->transitions = x_array_sized_new(FALSE, TRUE, sizeof(Transition), time_count + extra_time_count);

    for (index = 0; index < type_count; index++) {
        TransitionInfo t_info;
        struct ttinfo info = ((struct ttinfo *)tz_ttinfo)[index];
        t_info.gmt_offset = xint32_from_be (info.tt_gmtoff);
        t_info.is_dst = info.tt_isdst ? TRUE : FALSE;
        t_info.abbrev = x_strdup((xchar *)&tz_abbrs[info.tt_abbrind]);
        x_array_append_val(gtz->t_info, t_info);
    }

    for (index = 0; index < time_count; index++) {
        Transition trans;
        if (header->tzh_version >= '2') {
            trans.time = xint64_from_be (((xint64_be *)tz_transitions)[index]);
        } else {
            trans.time = xint32_from_be (((xint32_be *)tz_transitions)[index]);
        }
        last_explicit_transition_time = trans.time;
        trans.info_index = tz_type_index[index];

        x_assert(trans.info_index >= 0);
        x_assert((xuint) trans.info_index < gtz->t_info->len);
        x_array_append_val(gtz->transitions, trans);
    }

    if (footertz) {
        for (index = 0; index < extra_type_count; index++) {
            TransitionInfo t_info;
            TransitionInfo *footer_t_info = &x_array_index(footertz->t_info, TransitionInfo, index);
            t_info.gmt_offset = footer_t_info->gmt_offset;
            t_info.is_dst = footer_t_info->is_dst;
            t_info.abbrev = x_steal_pointer(&footer_t_info->abbrev);
            x_array_append_val(gtz->t_info, t_info);
        }

        for (index = 0; index < extra_time_count; index++) {
            Transition *footer_transition = &x_array_index(footertz->transitions, Transition, index);
            if (time_count <= 0 || last_explicit_transition_time < footer_transition->time) {
                Transition trans;
                trans.time = footer_transition->time;
                trans.info_index = type_count + footer_transition->info_index;
                x_array_append_val(gtz->transitions, trans);
            }
        }

        x_time_zone_unref(footertz);
    }
}

static void find_relative_date(TimeZoneDate *buffer)
{
    xuint wday;
    XDate date;

    x_date_clear(&date, 1);
    wday = buffer->wday;

    if (buffer->mon == 13 || buffer->mon == 14) {
        x_date_set_dmy(&date, 1, (XDateMonth)1, buffer->year);
        if (wday >= 59 && buffer->mon == 13 && x_date_is_leap_year(buffer->year)) {
            x_date_add_days(&date, wday);
        } else {
            x_date_add_days(&date, wday - 1);
        }

        buffer->mon = (int)x_date_get_month(&date);
        buffer->mday = (int)x_date_get_day(&date);
        buffer->wday = 0;
    } else {
        xuint days;
        XDateWeekday first_wday;
        xuint days_in_month = x_date_get_days_in_month((XDateMonth)buffer->mon, buffer->year);

        x_date_set_dmy(&date, 1, (XDateMonth)buffer->mon, buffer->year);
        first_wday = x_date_get_weekday(&date);

        if ((xuint) first_wday > wday) {
            ++(buffer->week);
        }

        days = 7 * (buffer->week - 1) + wday - first_wday;
        while (days >= days_in_month) {
            days -= 7;
        }

        x_date_add_days(&date, days);
        buffer->mday = x_date_get_day(&date);
    }
}

static xint64 boundary_for_year(TimeZoneDate *boundary, xint year, xint32 offset)
{
    XDate date;
    TimeZoneDate buffer;
    const xuint64 seconds_per_day = 86400L;
    const xuint64 unix_epoch_start = 719163L;

    if (!boundary->mon) {
        return 0;
    }
    buffer = *boundary;

    if (boundary->year == 0) {
        buffer.year = year;
        if (buffer.wday) {
            find_relative_date(&buffer);
        }
    }

    x_assert(buffer.year == year);
    x_date_clear(&date, 1);
    x_date_set_dmy(&date, buffer.mday, (XDateMonth)buffer.mon, buffer.year);

    return ((x_date_get_julian(&date) - unix_epoch_start) * seconds_per_day + buffer.offset - offset);
}

static void fill_transition_info_from_rule(TransitionInfo *info, TimeZoneRule *rule, xboolean is_dst)
{
    xchar *name = is_dst ? rule->dlt_name : rule->std_name;
    xint offset = is_dst ? rule->dlt_offset : rule->std_offset;

    info->gmt_offset = offset;
    info->is_dst = is_dst;

    if (name) {
        info->abbrev = x_strdup(name);
    } else {
        info->abbrev = x_strdup_printf("%+03d%02d", (int)offset / 3600, (int)abs(offset / 60) % 60);
    }
}

static void init_zone_from_rules(XTimeZone *gtz, TimeZoneRule *rules, xuint rules_num, xchar *identifier)
{
    xuint ri;
    xint32 last_offset;
    xboolean skip_first_std_trans = TRUE;
    xuint type_count = 0, trans_count = 0, info_index = 0;

    type_count = 0;
    trans_count = 0;

    for (ri = 0; ri < rules_num - 1; ri++) {
        if (rules[ri].dlt_start.mon || rules[ri].dlt_end.mon) {
            xuint rulespan = (rules[ri + 1].start_year - rules[ri].start_year);
            xuint transitions = rules[ri].dlt_start.mon > 0 ? 1 : 0;
            transitions += rules[ri].dlt_end.mon > 0 ? 1 : 0;
            type_count += rules[ri].dlt_start.mon > 0 ? 2 : 1;
            trans_count += transitions * rulespan;
        } else {
            type_count++;
        }
    }

    gtz->name = x_steal_pointer(&identifier);
    gtz->t_info = x_array_sized_new(FALSE, TRUE, sizeof(TransitionInfo), type_count);
    gtz->transitions = x_array_sized_new(FALSE, TRUE, sizeof(Transition), trans_count);

    last_offset = rules[0].std_offset;

    for (ri = 0; ri < rules_num - 1; ri++) {
        if ((rules[ri].std_offset || rules[ri].dlt_offset) && rules[ri].dlt_start.mon == 0 && rules[ri].dlt_end.mon == 0) {
            TransitionInfo std_info;

            fill_transition_info_from_rule(&std_info, &(rules[ri]), FALSE);
            x_array_append_val(gtz->t_info, std_info);

            if (ri > 0 && ((rules[ri - 1].dlt_start.mon > 12 && rules[ri - 1].dlt_start.wday > rules[ri - 1].dlt_end.wday) || rules[ri - 1].dlt_start.mon > rules[ri - 1].dlt_end.mon)) {
                xuint year = rules[ri].start_year;
                xint64 std_time =  boundary_for_year(&rules[ri].dlt_end, year, last_offset);
                Transition std_trans = {std_time, info_index};
                x_array_append_val(gtz->transitions, std_trans);

            }

            last_offset = rules[ri].std_offset;
            ++info_index;
            skip_first_std_trans = TRUE;
        } else {
            xuint year;
            xboolean dlt_first;
            TransitionInfo std_info, dlt_info;
            const xuint start_year = rules[ri].start_year;
            const xuint end_year = rules[ri + 1].start_year;

            if (rules[ri].dlt_start.mon > 12) {
                dlt_first = rules[ri].dlt_start.wday > rules[ri].dlt_end.wday;
            } else {
                dlt_first = rules[ri].dlt_start.mon > rules[ri].dlt_end.mon;
            }

            fill_transition_info_from_rule(&std_info, &(rules[ri]), FALSE);
            fill_transition_info_from_rule(&dlt_info, &(rules[ri]), TRUE);

            x_array_append_val(gtz->t_info, std_info);
            x_array_append_val(gtz->t_info, dlt_info);

            for (year = start_year; year < end_year; year++) {
                xint32 dlt_offset = (dlt_first ? last_offset : rules[ri].dlt_offset);
                xint32 std_offset = (dlt_first ? rules[ri].std_offset : last_offset);
                xint64 std_time =  boundary_for_year(&rules[ri].dlt_end, year, dlt_offset);
                xint64 dlt_time = boundary_for_year(&rules[ri].dlt_start, year, std_offset);
                Transition std_trans = {std_time, info_index};
                Transition dlt_trans = {dlt_time, info_index + 1};

                last_offset = (dlt_first ? rules[ri].dlt_offset : rules[ri].std_offset);
                if (dlt_first) {
                    if (skip_first_std_trans) {
                        skip_first_std_trans = FALSE;
                    } else if (std_time) {
                        x_array_append_val(gtz->transitions, std_trans);
                    }

                    if (dlt_time) {
                        x_array_append_val(gtz->transitions, dlt_trans);
                    }
                } else {
                    if (dlt_time) {
                        x_array_append_val(gtz->transitions, dlt_trans);
                    }

                    if (std_time) {
                        x_array_append_val(gtz->transitions, std_trans);
                    }
                }
            }

            info_index += 2;
        }
    }

    if (ri > 0 && ((rules[ri - 1].dlt_start.mon > 12 && rules[ri - 1].dlt_start.wday > rules[ri - 1].dlt_end.wday) || rules[ri - 1].dlt_start.mon > rules[ri - 1].dlt_end.mon)) {
        Transition trans;
        TransitionInfo info;
        xuint year = rules[ri].start_year;

        fill_transition_info_from_rule(&info, &(rules[ri - 1]), FALSE);
        x_array_append_val(gtz->t_info, info);
        trans.time = boundary_for_year(&rules[ri - 1].dlt_end, year, last_offset);
        trans.info_index = info_index;
        x_array_append_val(gtz->transitions, trans);
    }
}

static xboolean parse_mwd_boundary(xchar **pos, TimeZoneDate *boundary)
{
    xint month, week, day;

    if (**pos == '\0' || **pos < '0' || '9' < **pos) {
        return FALSE;
    }

    month = *(*pos)++ - '0';

    if ((month == 1 && **pos >= '0' && '2' >= **pos) || (month == 0 && **pos >= '0' && '9' >= **pos)) {
        month *= 10;
        month += *(*pos)++ - '0';
    }

    if (*(*pos)++ != '.' || month == 0) {
        return FALSE;
    }

    if (**pos == '\0' || **pos < '1' || '5' < **pos) {
        return FALSE;
    }

    week = *(*pos)++ - '0';

    if (*(*pos)++ != '.') {
        return FALSE;
    }

    if (**pos == '\0' || **pos < '0' || '6' < **pos) {
        return FALSE;
    }

    day = *(*pos)++ - '0';
    if (!day) {
        day += 7;
    }

    boundary->year = 0;
    boundary->mon = month;
    boundary->week = week;
    boundary->wday = day;

    return TRUE;
}

static xboolean parse_julian_boundary(xchar** pos, TimeZoneDate *boundary, xboolean ignore_leap)
{
    XDate date;
    xint day = 0;

    while (**pos >= '0' && '9' >= **pos) {
        day *= 10;
        day += *(*pos)++ - '0';
    }

    if (ignore_leap) {
        if (day < 1 || 365 < day) {
            return FALSE;
        }

        if (day >= 59) {
            day++;
        }
    } else {
        if (day < 0 || 365 < day) {
            return FALSE;
        }

        day++;
    }

    x_date_clear(&date, 1);
    x_date_set_julian(&date, day);
    boundary->year = 0;
    boundary->mon = (int)x_date_get_month(&date);
    boundary->mday = (int)x_date_get_day(&date);
    boundary->wday = 0;

    return TRUE;
}

static xboolean parse_tz_boundary(const xchar  *identifier, TimeZoneDate *boundary)
{
    xchar *pos;

    pos = (xchar*)identifier;
    if (*pos == 'M') {
        ++pos;
        if (!parse_mwd_boundary(&pos, boundary)) {
            return FALSE;
        }
    } else if (*pos == 'J') {
        ++pos;
        if (!parse_julian_boundary(&pos, boundary, TRUE)) {
            return FALSE;
        }
    } else if (*pos >= '0' && '9' >= *pos) {
        if (!parse_julian_boundary(&pos, boundary, FALSE)) {
            return FALSE;
        }
    } else {
        return FALSE;
    }

    if (*pos == '/') {
        return parse_constant_offset(pos + 1, &boundary->offset, TRUE);
    } else {
        boundary->offset = 2 * 60 * 60;
        return *pos == '\0';
    }
}

static xuint create_ruleset_from_rule(TimeZoneRule **rules, TimeZoneRule *rule)
{
    *rules = x_new0(TimeZoneRule, 2);

    (*rules)[0].start_year = MIN_TZYEAR;
    (*rules)[1].start_year = MAX_TZYEAR;

    (*rules)[0].std_offset = -rule->std_offset;
    (*rules)[0].dlt_offset = -rule->dlt_offset;
    (*rules)[0].dlt_start  = rule->dlt_start;
    (*rules)[0].dlt_end = rule->dlt_end;
    strcpy((*rules)[0].std_name, rule->std_name);
    strcpy((*rules)[0].dlt_name, rule->dlt_name);

    return 2;
}

static xboolean parse_offset(xchar **pos, xint32 *target)
{
    xboolean ret;
    xchar *buffer;
    xchar *target_pos = *pos;

    while (**pos == '+' || **pos == '-' || **pos == ':' || (**pos >= '0' && '9' >= **pos)) {
        ++(*pos);
    }

    buffer = x_strndup(target_pos, *pos - target_pos);
    ret = parse_constant_offset(buffer, target, FALSE);
    x_free(buffer);

    return ret;
}

static xboolean parse_identifier_boundary(xchar **pos, TimeZoneDate *target)
{
    xboolean ret;
    xchar *buffer;
    xchar *target_pos = *pos;

    while (**pos != ',' && **pos != '\0') {
        ++(*pos);
    }

    buffer = x_strndup(target_pos, *pos - target_pos);
    ret = parse_tz_boundary(buffer, target);
    x_free(buffer);

    return ret;
}

static xboolean set_tz_name(xchar **pos, xchar *buffer, xuint size)
{
    xuint len;
    xchar *name_pos = *pos;
    xboolean quoted = **pos == '<';

    x_assert(size != 0);

    if (quoted) {
        name_pos++;
        do {
            ++(*pos);
        } while (x_ascii_isalnum(**pos) || **pos == '-' || **pos == '+');

        if (**pos != '>') {
            return FALSE;
        }
    } else {
        while (x_ascii_isalpha(**pos)) {
            ++(*pos);
        }
    }

    if (*pos - name_pos < 3) {
        return FALSE;
    }

    memset(buffer, 0, size);
    len = (xuint) (*pos - name_pos) > size - 1 ? size - 1 : (xuint) (*pos - name_pos);
    strncpy(buffer, name_pos, len);
    *pos += quoted;

    return TRUE;
}

static xboolean parse_identifier_boundaries(xchar **pos, TimeZoneRule *tzr)
{
    if (*(*pos)++ != ',') {
        return FALSE;
    }

    if (!parse_identifier_boundary(pos, &(tzr->dlt_start)) || *(*pos)++ != ',') {
        return FALSE;
    }

    if (!parse_identifier_boundary(pos, &(tzr->dlt_end))) {
        return FALSE;
    }

    return TRUE;
}

static xuint rules_from_identifier(const xchar *identifier, TimeZoneRule **rules)
{
    xchar *pos;
    TimeZoneRule tzr;

    x_assert(rules != NULL);

    *rules = NULL;

    if (!identifier) {
        return 0;
    }

    pos = (xchar*)identifier;
    memset(&tzr, 0, sizeof(tzr));

    if (!(set_tz_name(&pos, tzr.std_name, NAME_SIZE)) || !parse_offset(&pos, &(tzr.std_offset))) {
        return 0;
    }

    if (*pos == 0) {
        return create_ruleset_from_rule(rules, &tzr);
    }

    if (!(set_tz_name(&pos, tzr.dlt_name, NAME_SIZE))) {
        return 0;
    }

    parse_offset(&pos, &(tzr.dlt_offset));
    if (tzr.dlt_offset == 0) {
        tzr.dlt_offset = tzr.std_offset - 3600;
    }

    if (*pos == '\0') {
        return 0;
    }

    if (!parse_identifier_boundaries(&pos, &tzr)) {
        return 0;
    }

    return create_ruleset_from_rule(rules, &tzr);
}

static XTimeZone *parse_footertz(const xchar *footer, size_t footerlen)
{
    TimeZoneRule *rules;
    XTimeZone *footertz = NULL;
    xchar *tzstring = x_strndup(footer + 1, footerlen - 2);
    xuint rules_num = rules_from_identifier(tzstring, &rules);

    x_free(tzstring);
    if (rules_num > 1) {
        footertz = x_slice_new0(XTimeZone);
        init_zone_from_rules(footertz, rules, rules_num, NULL);
        footertz->ref_count++;
    }

    x_free(rules);
    return footertz;
}

XTimeZone *x_time_zone_new(const xchar *identifier)
{
    XTimeZone *tz = x_time_zone_new_identifier(identifier);
    if (tz == NULL) {
        tz = x_time_zone_new_utc();
    }
    x_assert(tz != NULL);

    return x_steal_pointer(&tz);
}

XTimeZone *x_time_zone_new_identifier(const xchar *identifier)
{
    xint rules_num;
    TimeZoneRule *rules;
    XTimeZone *tz = NULL;
    xchar *resolved_identifier = NULL;

    if (identifier) {
        X_LOCK(time_zones);
        if (time_zones == NULL) {
            time_zones = x_hash_table_new(x_str_hash, x_str_equal);
        }

        tz = (XTimeZone *)x_hash_table_lookup(time_zones, identifier);
        if (tz) {
            x_atomic_int_inc(&tz->ref_count);
            X_UNLOCK(time_zones);
            return tz;
        } else {
            resolved_identifier = x_strdup(identifier);
        }
    } else {
        X_LOCK(tz_default);
        resolved_identifier = zone_identifier_unix();
        if (tz_default) {
            if (!(resolved_identifier == NULL && x_str_equal(tz_default->name, "UTC")) && x_strcmp0(tz_default->name, resolved_identifier) != 0) {
                x_clear_pointer(&tz_default, x_time_zone_unref);
            } else {
                tz = x_time_zone_ref(tz_default);
                X_UNLOCK(tz_default);

                x_free(resolved_identifier);
                return tz;
            }
        }
    }

    tz = x_slice_new0(XTimeZone);
    tz->ref_count = 0;

    zone_for_constant_offset(tz, identifier);

    if (tz->t_info == NULL && (rules_num = rules_from_identifier(identifier, &rules))) {
        init_zone_from_rules (tz, rules, rules_num, x_steal_pointer(&resolved_identifier));
        x_free(rules);
    }

    if (tz->t_info == NULL) {
        XBytes *zoneinfo = zone_info_unix(identifier, resolved_identifier);
        if (zoneinfo != NULL) {
            init_zone_from_iana_info(tz, zoneinfo, x_steal_pointer(&resolved_identifier));
            x_bytes_unref(zoneinfo);
        }
    }

    x_free(resolved_identifier);

    if (tz->t_info == NULL) {
        x_slice_free(XTimeZone, tz);

        if (identifier) {
            X_UNLOCK(time_zones);
        } else {
            X_UNLOCK(tz_default);
        }

        return NULL;
    }

    x_assert(tz->name != NULL);
    x_assert(tz->t_info != NULL);

    if (identifier) {
        x_hash_table_insert(time_zones, tz->name, tz);
    } else if (tz->name) {
        x_atomic_int_inc(&tz->ref_count);
        tz_default = tz;
    }

    x_atomic_int_inc(&tz->ref_count);

    if (identifier) {
        X_UNLOCK(time_zones);
    } else {
        X_UNLOCK(tz_default);
    }

    return tz;
}

XTimeZone *x_time_zone_new_utc(void)
{
    static xsize initialised;
    static XTimeZone *utc = NULL;

    if (x_once_init_enter(&initialised)) {
        utc = x_time_zone_new_identifier("UTC");
        x_assert(utc != NULL);
        x_once_init_leave(&initialised, TRUE);
    }

    return x_time_zone_ref(utc);
}

XTimeZone *x_time_zone_new_local(void)
{
    XTimeZone *tz;
    const xchar *tzenv = x_getenv("TZ");

    X_LOCK(tz_local);
    if (tz_local && x_strcmp0(x_time_zone_get_identifier(tz_local), tzenv)) {
        x_clear_pointer(&tz_local, x_time_zone_unref);
    }

    if (tz_local == NULL) {
        tz_local = x_time_zone_new_identifier(tzenv);
    }

    if (tz_local == NULL) {
        tz_local = x_time_zone_new_utc();
    }

    tz = x_time_zone_ref(tz_local);
    X_UNLOCK(tz_local);

    return tz;
}

XTimeZone *x_time_zone_new_offset(xint32 seconds)
{
    XTimeZone *tz = NULL;
    xchar *identifier = NULL;

    identifier = x_strdup_printf("%c%02u:%02u:%02u", (seconds >= 0) ? '+' : '-', (ABS(seconds) / 60) / 60, (ABS(seconds) / 60) % 60, ABS(seconds) % 60);
    tz = x_time_zone_new_identifier(identifier);

    if (tz == NULL) {
        tz = x_time_zone_new_utc();
    } else {
        x_assert(x_time_zone_get_offset(tz, 0) == seconds);
    }

    x_assert(tz != NULL);
    x_free(identifier);

    return tz;
}

#define TRANSITION(n)         x_array_index(tz->transitions, Transition, n)
#define TRANSITION_INFO(n)    x_array_index(tz->t_info, TransitionInfo, n)

inline static const TransitionInfo *interval_info(XTimeZone *tz, xuint interval)
{
    xuint index;

    x_return_val_if_fail(tz->t_info != NULL, NULL);

    if (interval && tz->transitions && interval <= tz->transitions->len) {
        index = (TRANSITION(interval - 1)).info_index;
    } else {
        for (index = 0; index < tz->t_info->len; index++) {
            TransitionInfo *tzinfo = &(TRANSITION_INFO(index));
            if (!tzinfo->is_dst) {
                return tzinfo;
            }
        }

        index = 0;
    }

    return &(TRANSITION_INFO(index));
}

inline static xint64 interval_start(XTimeZone *tz, xuint interval)
{
    if (!interval || tz->transitions == NULL || tz->transitions->len == 0) {
        return X_MININT64;
    }

    if (interval > tz->transitions->len) {
        interval = tz->transitions->len;
    }

    return (TRANSITION(interval - 1)).time;
}

inline static xint64 interval_end(XTimeZone *tz, xuint interval)
{
    if (tz->transitions && interval < tz->transitions->len) {
        xint64 lim = (TRANSITION(interval)).time;
        return lim - (lim != X_MININT64);
    }

    return X_MAXINT64;
}

inline static xint32 interval_offset(XTimeZone *tz, xuint interval)
{
    x_return_val_if_fail(tz->t_info != NULL, 0);
    return interval_info(tz, interval)->gmt_offset;
}

inline static xboolean interval_isdst(XTimeZone *tz, xuint interval)
{
    x_return_val_if_fail(tz->t_info != NULL, 0);
    return interval_info(tz, interval)->is_dst;
}

inline static xchar *interval_abbrev(XTimeZone *tz, xuint interval)
{
    x_return_val_if_fail(tz->t_info != NULL, 0);
    return interval_info(tz, interval)->abbrev;
}

inline static xint64 interval_local_start(XTimeZone *tz, xuint interval)
{
    if (interval) {
        return interval_start(tz, interval) + interval_offset(tz, interval);
    }

    return X_MININT64;
}

inline static xint64 interval_local_end(XTimeZone *tz, xuint interval)
{
    if (tz->transitions && interval < tz->transitions->len) {
        return interval_end(tz, interval) + interval_offset(tz, interval);
    }

    return X_MAXINT64;
}

static xboolean interval_valid(XTimeZone *tz, xuint interval)
{
    if ( tz->transitions == NULL) {
        return interval == 0;
    }

    return interval <= tz->transitions->len;
}

xint x_time_zone_adjust_time(XTimeZone *tz, XTimeType type, xint64 *time_)
{
    xuint i, intervals;
    xboolean interval_is_dst;

    if (tz->transitions == NULL) {
        return 0;
    }

    intervals = tz->transitions->len;

    for (i = 0; i <= intervals; i++) {
        if (*time_ <= interval_end(tz, i)) {
            break;
        }
    }

    x_assert(interval_start(tz, i) <= *time_ && *time_ <= interval_end(tz, i));

    if (type != X_TIME_TYPE_UNIVERSAL) {
        if (*time_ < interval_local_start(tz, i)) {
            i--;
            if (*time_ > interval_local_end(tz, i)) {
                i++;
                *time_ = interval_local_start(tz, i);
            }
        } else if (*time_ > interval_local_end(tz, i)) {
            i++;
            if (*time_ < interval_local_start(tz, i)) {
                *time_ = interval_local_start(tz, i);
            }
        } else {
            interval_is_dst = interval_isdst(tz, i);
            if ((interval_is_dst && type != X_TIME_TYPE_DAYLIGHT) || (!interval_is_dst && type == X_TIME_TYPE_DAYLIGHT)) {
                if (i && *time_ <= interval_local_end(tz, i - 1)) {
                    i--;
                } else if (i < intervals && *time_ >= interval_local_start(tz, i + 1)) {
                    i++;
                }
            }
        }
    }

    return i;
}

xint x_time_zone_find_interval(XTimeZone *tz, XTimeType type, xint64 time_)
{
    xuint i, intervals;
    xboolean interval_is_dst;

    if (tz->transitions == NULL) {
        return 0;
    }

    intervals = tz->transitions->len;
    for (i = 0; i <= intervals; i++) {
        if (time_ <= interval_end(tz, i)) {
            break;
        }
    }

    if (type == X_TIME_TYPE_UNIVERSAL) {
        return i;
    }

    if (time_ < interval_local_start(tz, i)) {
        if (time_ > interval_local_end(tz, --i)) {
            return -1;
        }
    } else if (time_ > interval_local_end(tz, i)) {
        if (time_ < interval_local_start(tz, ++i)) {
            return -1;
        }
    } else {
        interval_is_dst = interval_isdst(tz, i);
        if  ((interval_is_dst && type != X_TIME_TYPE_DAYLIGHT) || (!interval_is_dst && type == X_TIME_TYPE_DAYLIGHT)) {
            if (i && time_ <= interval_local_end(tz, i - 1)) {
                i--;
            } else if (i < intervals && time_ >= interval_local_start(tz, i + 1)) {
                i++;
            }
        }
    }

    return i;
}

const xchar *x_time_zone_get_abbreviation(XTimeZone *tz, xint interval)
{
    x_return_val_if_fail(interval_valid(tz, (xuint)interval), NULL);
    return interval_abbrev(tz, (xuint)interval);
}

xint32 x_time_zone_get_offset(XTimeZone *tz, xint interval)
{
    x_return_val_if_fail(interval_valid(tz, (xuint)interval), 0);
    return interval_offset(tz, (xuint)interval);
}

xboolean x_time_zone_is_dst(XTimeZone *tz, xint interval)
{
    x_return_val_if_fail(interval_valid(tz, interval), FALSE);

    if (tz->transitions == NULL) {
        return FALSE;
    }

    return interval_isdst(tz, (xuint)interval);
}

const xchar *x_time_zone_get_identifier(XTimeZone *tz)
{
    x_return_val_if_fail(tz != NULL, NULL);
    return tz->name;
}
