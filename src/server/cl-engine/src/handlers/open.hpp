#ifndef OPEN_HPP
#define OPEN_HPP
inline void open_handler(const char *const str) {
    int tid, fd;
    char path[PATH_MAX];
    sscanf(str, "%ld %d %s", &tid, &fd, path);
    START_LOG(gettid(), "call(tid=%d, fd=%d, path=%s", tid, fd, path);


}
#endif // OPEN_HPP
