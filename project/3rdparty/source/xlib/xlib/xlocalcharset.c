#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <langinfo.h>
#include <xlib/xlib/config.h>
#include <xlib/xlib/xlocalcharset.h>

#define relocate(pathname)          (pathname)

#ifndef XLIB_CHARSETALIAS_DIR
#define XLIB_CHARSETALIAS_DIR       /*LIBDIR*/"/usr/lib/"
#endif

#ifndef DIRECTORY_SEPARATOR
#define DIRECTORY_SEPARATOR         '/'
#endif

#ifndef ISSLASH
#define ISSLASH(C)                  ((C) == DIRECTORY_SEPARATOR)
#endif

#if HAVE_DECL_GETC_UNLOCKED
#undef getc
#define getc                        getc_unlocked
#endif

#if __STDC__ != 1
#define volatile
#endif

static const char *volatile charset_aliases;

const char *_x_locale_get_charset_aliases(void)
{
    const char *cp;

    cp = charset_aliases;
    if (cp == NULL) {
        FILE *fp;
        const char *dir;
        char *file_name;
        const char *base = "charset.alias";


        dir = relocate(XLIB_CHARSETALIAS_DIR);

        {
            size_t dir_len = strlen(dir);
            size_t base_len = strlen(base);
            int add_slash = (dir_len > 0 && !ISSLASH(dir[dir_len - 1]));
            file_name = (char *)malloc(dir_len + add_slash + base_len + 1);
            if (file_name != NULL) {
                memcpy(file_name, dir, dir_len);
                if (add_slash) {
                    file_name[dir_len] = DIRECTORY_SEPARATOR;
                }
                memcpy(file_name + dir_len + add_slash, base, base_len + 1);
            }
        }

        if (file_name == NULL || (fp = fopen(file_name, "r")) == NULL) {
            cp = "";
        } else {
            size_t res_size = 0;
            char *res_ptr = NULL;

            for (; ;) {
                int c;
                size_t l1, l2;
                char buf1[50+1];
                char buf2[50+1];
                char *old_res_ptr;

                c = getc(fp);
                if (c == EOF) {
                    break;
                }

                if (c == '\n' || c == ' ' || c == '\t') {
                    continue;
                }

                if (c == '#') {
                    do {
                        c = getc(fp);
                    } while (!(c == EOF || c == '\n'));

                    if (c == EOF) {
                        break;
                    }

                    continue;
                }

                ungetc(c, fp);
                if (fscanf(fp, "%50s %50s", buf1, buf2) < 2) {
                    break;
                }

                l1 = strlen(buf1);
                l2 = strlen(buf2);
                old_res_ptr = res_ptr;
                if (res_size == 0) {
                    res_size = l1 + 1 + l2 + 1;
                    res_ptr = (char *)malloc(res_size + 1);
                }
                else {
                    res_size += l1 + 1 + l2 + 1;
                    res_ptr = (char *)realloc(res_ptr, res_size + 1);
                }

                if (res_ptr == NULL) {
                    res_size = 0;
                    if (old_res_ptr != NULL) {
                        free(old_res_ptr);
                    }
                    break;
                }

                strcpy(res_ptr + res_size - (l2 + 1) - (l1 + 1), buf1);
                strcpy(res_ptr + res_size - (l2 + 1), buf2);
            }

            fclose(fp);
            if (res_size == 0) {
                cp = "";
            } else {
                *(res_ptr + res_size) = '\0';
                cp = res_ptr;
            }
        }

        if (file_name != NULL) {
            free(file_name);
        }

        charset_aliases = cp;
    }

    return cp;
}

const char *_x_locale_charset_raw(void)
{
    const char *codeset;

    codeset = nl_langinfo(CODESET);
    return codeset;
}

const char *_x_locale_charset_unalias(const char *codeset)
{
    const char *aliases;

    if (codeset == NULL) {
        codeset = "";
    }

    for (aliases = _x_locale_get_charset_aliases(); *aliases != '\0'; aliases += strlen(aliases) + 1, aliases += strlen(aliases) + 1) {
        if (strcmp (codeset, aliases) == 0 || (aliases[0] == '*' && aliases[1] == '\0')) {
            codeset = aliases + strlen(aliases) + 1;
            break;
        }
    }

    if (codeset[0] == '\0') {
        codeset = "ASCII";
    }

    return codeset;
}
