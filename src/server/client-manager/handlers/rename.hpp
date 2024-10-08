#ifndef CAPIO_RENAME_HPP
#define CAPIO_RENAME_HPP

inline void rename_handler(const char *const str) {
    pid_t tid;
    char old_path[PATH_MAX], new_path[PATH_MAX];
    sscanf(str, "%d %s %s", &tid, old_path, new_path);
    START_LOG(gettid(), "call(tid=%d, old=%s, new=%s)", tid, old_path, new_path);
    file_manager->unlockThreadAwaitingCreation(new_path);
    // TODO: gestire le rename?
}

#endif // CAPIO_RENAME_HPP
