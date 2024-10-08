#ifndef CAPIO_EXIT_HPP
#define CAPIO_EXIT_HPP

inline void exit_handler(const char *const str) {
    // TODO: register files open for each tid ti register a close
    pid_t tid;
    sscanf(str, "%d", &tid);
    START_LOG(gettid(), "call(tid=%d)", tid);

    // At exit, all files are considered to be committed. hence, call the set_committed
    // method. The increase_close_count method is not called, as it would add a close count
    // to a file that might have already been closing (hence increasing the close count by an extra
    // close
    file_manager->setCommitted(tid);
    client_manager->remove_client(tid);
}

#endif // CAPIO_EXIT_HPP
