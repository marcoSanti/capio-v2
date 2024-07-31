#ifndef CAPIO_POSIX_UTILS_REQUESTS_HPP
#define CAPIO_POSIX_UTILS_REQUESTS_HPP

#include <utility>

#include "capio/requests.hpp"

#include "env.hpp"
#include "filesystem.hpp"
#include "types.hpp"

inline CircularBuffer<char> *buf_requests;
inline CPBufResponse_t *bufs_response;

class WriteRequestCache {

    int current_fd         = -1;
    long long current_size = 0;

    const long long _max_size;

    std::filesystem::path current_path;

    // non-blocking as write is not in the pre port of CAPIO semantics
    inline void _write_request(const off64_t count, const long tid, const long fd) {
        START_LOG(capio_syscall(SYS_gettid), "call(path=%s, count=%ld, tid=%ld)",
                  current_path.c_str(), count, tid);
        char req[CAPIO_REQ_MAX_SIZE];
        sprintf(req, "%04d %ld %ld %s %ld", CAPIO_REQUEST_WRITE, tid, fd, current_path.c_str(),
                count);
        buf_requests->write(req, CAPIO_REQ_MAX_SIZE);
    }

  public:
    explicit WriteRequestCache(long long max_size) : _max_size(max_size) {}

    ~WriteRequestCache() { this->flush(capio_syscall(SYS_gettid)); }

    void write_request(std::filesystem::path path, int tid, int fd, long count) {

        if (fd != current_fd) {
            this->flush(tid);
            current_path = std::move(path);
            current_fd   = fd;
        }
        current_size += count;

        if (current_size > _max_size) {
            this->flush(tid);
        }
    };

    void flush(int tid) {
        if (current_fd != -1 && current_size > 0) {
            _write_request(tid, current_fd, current_size);
        }
        current_size = 0;
    }
};

WriteRequestCache *write_request_cache;

class ReadRequestCache {
    int current_fd     = -1;
    long long max_read = 0;
    bool is_committed  = false;

    std::filesystem::path current_path;

    // return amount of readable bytes
    inline off64_t _read_request(const std::filesystem::path &path, const off64_t end_of_Read,
                                 const long tid, const long fd) {
        START_LOG(capio_syscall(SYS_gettid), "call(path=%s, end_of_Read=%ld, tid=%ld, fd=%ld)",
                  path.c_str(), end_of_Read, tid, fd);
        char req[CAPIO_REQ_MAX_SIZE];
        sprintf(req, "%04d %s %ld %ld %ld", CAPIO_REQUEST_READ, path.c_str(), tid, fd, end_of_Read);
        LOG("Sending read request %s", req);
        buf_requests->write(req, CAPIO_REQ_MAX_SIZE);
        off64_t res;
        bufs_response->at(tid)->read(&res);
        LOG("Response to request is %ld", res);
        return res;
    }

  public:
    explicit ReadRequestCache() = default;

    ~ReadRequestCache() = default;

    void read_request(std::filesystem::path path, long end_of_read, int tid, int fd) {

        if (fd != current_fd) {
            current_path = std::move(path);
            current_fd   = fd;
            max_read     = 0;
            is_committed = false;
        }

        if (is_committed) {
            return;
        }

        if (end_of_read > max_read) {
            max_read = _read_request(path, end_of_read, tid, fd);
            if (max_read == -1) {
                is_committed = true;
            }
        }
    };
};

ReadRequestCache *read_request_cache;

/**
 * Initialize request and response buffers
 * @return
 */
inline void init_client() {

    buf_requests =
        new CircularBuffer<char>(SHM_COMM_CHAN_NAME, CAPIO_REQ_BUFF_CNT, CAPIO_REQ_MAX_SIZE);
    bufs_response = new CPBufResponse_t();

    // TODO: use var to set cache size
    // TODO: also enable multithreading
    write_request_cache = new WriteRequestCache(8192);
    read_request_cache  = new ReadRequestCache();
}

/**
 * Add a new response buffer for thread @param tid
 * @param tid
 * @return
 */
inline void register_listener(long tid) {
    auto *p_buf_response = new CircularBuffer<off_t>(SHM_COMM_CHAN_NAME_RESP + std::to_string(tid),
                                                     CAPIO_REQ_BUFF_CNT, sizeof(off_t));
    bufs_response->insert(std::make_pair(tid, p_buf_response));
}

// Block until server allows for proceeding to a generic request
inline void consent_to_proceed_request(const std::filesystem::path &path, const long tid) {
    START_LOG(capio_syscall(SYS_gettid), "call(path=%s, tid=%ld)", path.c_str(), tid);
    char req[CAPIO_REQ_MAX_SIZE];
    sprintf(req, "%04d %ld %s", CAPIO_REQUEST_CONSENT, tid, path.c_str());
    buf_requests->write(req, CAPIO_REQ_MAX_SIZE);
    off64_t res;
    bufs_response->at(tid)->read(&res);
}

// non blocking
inline void close_request(const std::filesystem::path &path, const long tid) {
    START_LOG(capio_syscall(SYS_gettid), "call(path=%s, tid=%ld)", path.c_str(), tid);
    write_request_cache->flush(tid);
    char req[CAPIO_REQ_MAX_SIZE];
    sprintf(req, "%04d %ld %s", CAPIO_REQUEST_CLOSE, tid, path.c_str());
    buf_requests->write(req, CAPIO_REQ_MAX_SIZE);
}

// non blocking
inline void create_request(const int fd, const std::filesystem::path &path, const long tid) {
    START_LOG(capio_syscall(SYS_gettid), "call(fd=%ld, path=%s, tid=%ld)", fd, path.c_str(), tid);
    char req[CAPIO_REQ_MAX_SIZE];
    sprintf(req, "%04d %ld %d %s", CAPIO_REQUEST_CREATE, tid, fd, path.c_str());
    buf_requests->write(req, CAPIO_REQ_MAX_SIZE);
}

// non blocking
inline void exit_group_request(const long tid) {
    START_LOG(capio_syscall(SYS_gettid), "call(tid=%ld)", tid);
    write_request_cache->flush(tid);
    char req[CAPIO_REQ_MAX_SIZE];
    sprintf(req, "%04d %ld", CAPIO_REQUEST_EXIT_GROUP, tid);
    buf_requests->write(req, CAPIO_REQ_MAX_SIZE);
}

// non blocking
inline void handshake_anonymous_request(const long tid, const long pid) {
    START_LOG(capio_syscall(SYS_gettid), "call(tid=%ld, pid=%ld)", tid, pid);
    char req[CAPIO_REQ_MAX_SIZE];
    sprintf(req, "%04d %ld %ld", CAPIO_REQUEST_HANDSHAKE_ANONYMOUS, tid, pid);
    buf_requests->write(req, CAPIO_REQ_MAX_SIZE);
}

// non blocking
inline void handshake_named_request(const long tid, const long pid, const std::string &app_name) {
    START_LOG(capio_syscall(SYS_gettid), "call(tid=%ld, pid=%ld, app_name=%s)", tid, pid,
              app_name.c_str());
    char req[CAPIO_REQ_MAX_SIZE];
    sprintf(req, "%04d %ld %ld %s", CAPIO_REQUEST_HANDSHAKE_NAMED, tid, pid, app_name.c_str());
    buf_requests->write(req, CAPIO_REQ_MAX_SIZE);
}

// block until open is possible
inline void open_request(const int fd, const std::filesystem::path &path, const long tid) {
    START_LOG(capio_syscall(SYS_gettid), "call(fd=%ld, path=%s, tid=%ld)", fd, path.c_str(), tid);
    write_request_cache->flush(tid);
    char req[CAPIO_REQ_MAX_SIZE];
    sprintf(req, "%04d %ld %d %s", CAPIO_REQUEST_OPEN, tid, fd, path.c_str());
    buf_requests->write(req, CAPIO_REQ_MAX_SIZE);
    off64_t res;
    bufs_response->at(tid)->read(&res);
}

// non blocking
inline void rename_request(const std::filesystem::path &old_path,
                           const std::filesystem::path &new_path, const long tid) {
    START_LOG(capio_syscall(SYS_gettid), "call(old=%s, new=%s, tid=%ld)", old_path.c_str(),
              new_path.c_str(), tid);
    char req[CAPIO_REQ_MAX_SIZE];
    sprintf(req, "%04d %ld %s %s", CAPIO_REQUEST_RENAME, tid, old_path.c_str(), new_path.c_str());
    buf_requests->write(req, CAPIO_REQ_MAX_SIZE);
}

#endif // CAPIO_POSIX_UTILS_REQUESTS_HPP
