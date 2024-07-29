#ifndef CAPIO_SEEK_HPP
#define CAPIO_SEEK_HPP

inline void seek_handler(const char *const str) {
    long tid, offset;
    int whence, fd;
    char path[PATH_MAX];
    sscanf(str, "%ld %s %ld %d %d", &tid, path, &offset, &fd, &whence);
    START_LOG(gettid(), "call()");
    std::filesystem::path filename(path);

    if (path == get_capio_dir() || !capio_configuration->file_to_be_handled(filename)) {
        return;
    }


    //TODO: HANDLE CORECTLY SEEKS AS NOW CAPIO-CL LANGUAGE IS NOT ENFORCES
    client_manager->reply_to_client(tid, 1);
}

#endif // CAPIO_SEEK_HPP
