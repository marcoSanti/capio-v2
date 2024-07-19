#ifndef READ_HPP
#define READ_HPP

inline void read_handler(const char *const str) {

    long tid, read_size, fd;
    char path[PATH_MAX];

    sscanf(str, "%s %ld %ld %ld", path, &tid, &fd, &read_size);
    START_LOG(gettid(), "call(path=%s, tid=%ld, count=%ld)", path, tid, read_size);

    std::filesystem::path path_fs(path);
    // Skip operations on CAPIO_DIR
    // TODO: check if it is coherent with CAPIO_CL
    if (path_fs == get_capio_dir()) {
        LOG("Ignore calls on exactly CAPIO_DIR");
        client_manager->reply_to_client(tid, 1);
        return;
    }

    if (!capio_configuration->file_to_be_handled(path_fs)) {
        LOG("Ignore calls as fiel should not be treated by CAPIO");
        client_manager->reply_to_client(tid, 1);
        return;
    }

    if (storage_engine->size(path_fs, tid, fd) + storage_engine->offset_of(tid, fd) >= read_size) {
        storage_engine->update_offset(tid, fd, read_size, path);
        client_manager->reply_to_client(tid, 1);
    } else {
        storage_engine->add_thread_awaiting_for_data(path_fs, tid,fd, read_size);
    }
}

#endif // READ_HPP
