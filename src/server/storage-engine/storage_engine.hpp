#ifndef STORAGE_ENGINE_HPP
#define STORAGE_ENGINE_HPP
#include <mutex>

#include "src/capio_file.hpp"

class StorageEngine {
  private:
    std::unordered_map<std::filesystem::path, CapioFile *> *open_metadata_descriptors;
    std::mutex metadata_mutex;

    /**
     * check if metadata associated with a given file exists.
     * If it exists, then there is an assumption that the associated file is present
     * @param path
     * @return
     */
    static bool exists(std::filesystem::path &path) {
        std::filesystem::path check = path.string() + ".capio";
        return std::filesystem::exists(check);
    }

  public:
    StorageEngine() {
        open_metadata_descriptors = new std::unordered_map<std::filesystem::path, CapioFile *>();
    }

    ~StorageEngine() { delete open_metadata_descriptors; }

    void create_capio_file(std::filesystem::path filename) {
        std::lock_guard<std::mutex> lg(metadata_mutex);
        if (open_metadata_descriptors->find(filename) == open_metadata_descriptors->end()) {
            open_metadata_descriptors->emplace(filename, new CapioFile(filename));
        }
    }

    void deleteFile(std::filesystem::path path) {
        std::lock_guard<std::mutex> lg(metadata_mutex);

        std::filesystem::path metadata_name = path.string() + ".capio";
        std::filesystem::remove(metadata_name);
        std::filesystem::remove(path);
        open_metadata_descriptors->erase(open_metadata_descriptors->find(path));
    }

    auto get_metadata(std::filesystem::path &filename) {
        std::lock_guard<std::mutex> lg(metadata_mutex);

        return open_metadata_descriptors->at(filename)->get_metadata();
    }

    void update_metadata(std::filesystem::path &filename, long filesize, long n_close,
                         bool committed) {
        std::lock_guard<std::mutex> lg(metadata_mutex);

        open_metadata_descriptors->at(filename)->update_metadata(filesize, n_close, committed);
    }

    void update_size(std::filesystem::path &filename, long size) {
        std::lock_guard<std::mutex> lg(metadata_mutex);

        open_metadata_descriptors->at(filename)->update_size(size);
    }

    void update_n_close(std::filesystem::path &filename, long n_close) {
        std::lock_guard<std::mutex> lg(metadata_mutex);

        open_metadata_descriptors->at(filename)->update_n_close(n_close);
    }

    void set_committed(std::filesystem::path &filename) const {
        open_metadata_descriptors->at(filename)->set_committed();
    }

    bool is_committed(std::filesystem::path &filename) const {
        return open_metadata_descriptors->at(filename)->is_committed();
    }
};

#endif // STORAGE_ENGINE_HPP
