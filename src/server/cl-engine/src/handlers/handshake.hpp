#ifndef HANDSHAKE_HPP
#define HANDSHAKE_HPP

#include "capio/constants.hpp"

void handshake_anonymous_handler(const char *const str) {
    int tid, pid;
    sscanf(str, "%d %d", &tid, &pid);
    client_manager->register_new_client(tid, CAPIO_DEFAULT_APP_NAME);
}

void handshake_named_handler(const char *const str) {
    int tid, pid;
    char app_name[1024];
    sscanf(str, "%d %d %s", &tid, &pid, app_name);
    client_manager->register_new_client(tid, app_name);
}

#endif // HANDSHAKE_HPP
