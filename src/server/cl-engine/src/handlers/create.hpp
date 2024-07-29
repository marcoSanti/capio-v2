#ifndef CAPIO_CREATE_HPP
#define CAPIO_CREATE_HPP

#include <cl-engine/cl_engine.hpp>


inline void create_handler(const char *const str) {
    int tid;
    char path[PATH_MAX];
    sscanf(str, "%d %s", &tid, path);
    std::filesystem::path filename(path);

}

#endif // CAPIO_CREATE_HPP
