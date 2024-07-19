#ifndef WRITE_HPP
#define WRITE_HPP

inline void write_handler(const char *const str) {
    int tid, count, fd;
    char path[PATH_MAX];
    sscanf(str, "%ld %ld %s %ld", &tid, &fd, path, &count);

    std::filesystem::path filename(path);

    if (path == get_capio_dir() || !capio_configuration->file_to_be_handled(filename)) {
        return;
    }
    auto size = storage_engine->size(path, tid, fd);
    storage_engine->update_size(filename, size + count);
}

#endif //WRITE_HPP
