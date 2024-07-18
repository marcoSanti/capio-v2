#ifndef CONSENT_HPP
#define CONSENT_HPP
#include <cl-engine/cl_engine.hpp>
#include <storage-engine/storage_engine.hpp>

/*
This handler only checks if the client is allowed to continue
*/

inline void consent_to_proceed_handler(const char *const str) {
    int tid;
    char path[1024];
    sscanf(str, "%d %d %s", &tid, path);

    std::filesystem::path path_fs(path);

    if (storage_engine->is_committed(path_fs) ||
        capio_configuration->getFireRule(path_fs) == CAPIO_FILE_MODE_NO_UPDATE) {
        client_manager->reply_to_client(tid, 1);
    } else {
        storage_engine->add_thread_awaiting_for_commit(path, tid);
    }
}

#endif // CONSENT_HPP
