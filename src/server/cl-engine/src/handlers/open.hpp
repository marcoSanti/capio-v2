#ifndef OPEN_HPP
#define OPEN_HPP
inline void open_handler(const char *const str) {
    int tid, fd;
    char path[PATH_MAX];
    sscanf(str, "%ld %d %s", &tid, &fd, path);
    START_LOG(gettid(), "call(tid=%d, fd=%d, path=%s", tid, fd, path);

    if (StorageEngine::exists_file(path)) {
        storage_engine->register_tid_offset(tid, fd);
        LOG("File exists on filesystem. returning");
        client_manager->reply_to_client(tid, 1);
    } else {
        LOG("File does not exists. awaiting for creation of file");
        storage_engine->add_thread_awaiting_for_creation(path, tid);
    }
}
#endif // OPEN_HPP
