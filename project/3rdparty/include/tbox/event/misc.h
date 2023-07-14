#ifndef TBOX_EVENT_MISC_H
#define TBOX_EVENT_MISC_H

namespace tbox {
namespace event {

bool CreateFdPair(int &read_fd, int &write_fd);
int CreateEventFd();

}
}

#endif //TBOX_EVENT_MISC_H_20220303
