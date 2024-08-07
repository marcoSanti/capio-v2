#ifndef CAPIO_POSIX_HANDLERS_EXIT_GROUP_HPP
#define CAPIO_POSIX_HANDLERS_EXIT_GROUP_HPP

#if defined(SYS_exit) || defined(SYS_exit_group)

#include "capio/logger.hpp"

#include "utils/requests.hpp"

/*
 * TODO: adding cleaning of shared memory
 * The process can never interact with the server
 * maybe because is a child process don't need to interact
 * with CAPIO
 */

int exit_handler(long arg0, long arg1, long arg2, long arg3, long arg4, long arg5, long *result) {
    auto tid = static_cast<pid_t>(syscall_no_intercept(SYS_gettid));
    START_LOG(tid, "call()");

    if (is_capio_tid(tid)) {
        LOG("Thread %d is a CAPIO thread: clean up", tid);

        exit_group_request(tid);
        remove_capio_tid(tid);
    }

    delete write_request_cache;
    delete read_request_cache;

    return CAPIO_POSIX_SYSCALL_SKIP;
}

#endif // SYS_exit || SYS_exit_group
#endif // CAPIO_POSIX_HANDLERS_EXIT_GROUP_HPP
