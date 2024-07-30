#ifndef FILE_MANAGER_HPP
#define FILE_MANAGER_HPP
#include "client_manager.hpp"

inline void CapioFileManager::set_committed(const std::filesystem::path &path) {
    auto metadata_path = path / ".capio";
    close(creat(metadata_path.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
    client_manager->unlock_thread_awaiting_data(path);
    client_manager->unlock_thread_awaiting_creation(path);
}

inline void CapioFileManager::set_committed(long tid) {
    auto files = client_manager->get_produced_files(tid);
    for(const auto& file : *files) {
        CapioFileManager::set_committed(file);
    }
}

inline bool CapioFileManager::is_committed(const std::filesystem::path &path) {
    return std::filesystem::exists(path / ".capio");
}

#endif // FILE_MANAGER_HPP
