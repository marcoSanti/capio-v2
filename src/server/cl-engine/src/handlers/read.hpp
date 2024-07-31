#ifndef READ_HPP
#define READ_HPP

inline void read_handler(const char *const str) {

    long tid, end_of_read, fd;
    char path[PATH_MAX];

    sscanf(str, "%s %ld %ld %ld", path, &tid, &fd, &end_of_read);
    START_LOG(gettid(), "call(path=%s, tid=%ld, count=%ld)", path, tid, end_of_read);

    std::filesystem::path path_fs(path);
    // Skip operations on CAPIO_DIR
    // TODO: check if it is coherent with CAPIO_CL
    if (path_fs == get_capio_dir()) {
        LOG("Ignore calls on exactly CAPIO_DIR");
        client_manager->reply_to_client(tid, 1);
        return;
    }

    if (!capio_configuration->file_to_be_handled(path_fs)) {
        LOG("Ignore calls as file should not be treated by CAPIO");
        client_manager->reply_to_client(tid, 1);
        return;
    }

    auto is_committed = CapioFileManager::is_committed(path);
    auto file_size    = std::filesystem::file_size(path);

    // return -1 to signal client cache that file is committed and no more requests are required
    if (file_size >= end_of_read || is_committed) {
        client_manager->reply_to_client(tid, is_committed ? -1 : file_size);
    } else {
        client_manager->add_thread_awaiting_data(path, tid, end_of_read);
    }
}

#endif // READ_HPP
