#include <string.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <xlib/xlib/xjournal-private.h>

static int str_has_prefix(const char *str, const char *prefix)
{
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

int _x_fd_is_journal(int output_fd)
{
    union {
        struct sockaddr_storage storage;
        struct sockaddr         sa;
        struct sockaddr_un      un;
    } addr;

    int err;
    socklen_t addr_len;

    if (output_fd < 0) {
        return 0;
    }

    memset (&addr, 0, sizeof (addr));
    addr_len = sizeof(addr);
    err = getpeername(output_fd, &addr.sa, &addr_len);
    if (err == 0 && addr.storage.ss_family == AF_UNIX) {
        return (str_has_prefix(addr.un.sun_path, "/run/systemd/journal/") || str_has_prefix(addr.un.sun_path, "/run/systemd/journal."));
    }

    return 0;
}
