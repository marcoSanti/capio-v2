#ifndef STORAGE_ENGINE_HPP
#define STORAGE_ENGINE_HPP
#include <mutex>

#include "src/capio_file.hpp"

// TODO: SPLITTARE ASAP LA GESTIONE DELLE RICHIESTE FUTURE IN UNA CLASSE DEDICATA

class StorageEngine {
  private:
    // path -> CapioFile (only for metadata)
    std::unordered_map<std::string, CapioFile *> *open_metadata_descriptors;

    // path -> [tids waiting for response on path]
    std::unordered_map<std::string, std::vector<long> *> *pending_requests_on_file;

    // path -> [tids waiting for creation of path]
    std::unordered_map<std::string, std::vector<long> *> *pending_requests_on_creation;

    // [path, tid, fd] -> read size request. used to store the pending requests to read
    std::unordered_map<std::string, std::unordered_map<long, std::unordered_map<long, long> *> *>
        *pending_requests_on_available_data;

    // tid, path  -> number of open on file (used to know opened and how many times a file has been
    // open by a thread)
    std::unordered_map<long, std::unordered_map<std::string, long> *> *threads_opened_files;

    // [tid, file_descriptor] -> offset
    std::unordered_map<long, std::unordered_map<long, long> *> *file_descriptors_offsets;

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

    void unlock_awaiting_threads_for_commit(const std::filesystem::path &path) const {
        if (open_metadata_descriptors->at(path)->is_committed()) {
            auto queue = pending_requests_on_file->at(path);
            for (long tid : *queue) {
                client_manager->reply_to_client(tid, 1);
            }
        }
    }

    void unlock_awaiting_threads_for_creation(const std::filesystem::path &path) const {
        if (pending_requests_on_creation->find(path) != pending_requests_on_creation->end()) {
            auto queue = pending_requests_on_creation->at(path);
            for (long tid : *queue) {
                client_manager->reply_to_client(tid, 1);
            }
        }
    }

  public:
    StorageEngine() {
        open_metadata_descriptors    = new std::unordered_map<std::string, CapioFile *>;
        pending_requests_on_file     = new std::unordered_map<std::string, std::vector<long> *>;
        pending_requests_on_creation = new std::unordered_map<std::string, std::vector<long> *>;
        threads_opened_files =
            new std::unordered_map<long, std::unordered_map<std::string, long> *>;

        file_descriptors_offsets = new std::unordered_map<long, std::unordered_map<long, long> *>;
        pending_requests_on_available_data =
            new std::unordered_map<std::string,
                                   std::unordered_map<long, std::unordered_map<long, long> *> *>;
    }

    ~StorageEngine() {
        delete open_metadata_descriptors;
        delete pending_requests_on_creation;
        delete pending_requests_on_file;
        delete threads_opened_files;
        delete file_descriptors_offsets;
        delete pending_requests_on_available_data;
    }

    void create_capio_file(std::filesystem::path filename, long tid) {
        std::lock_guard<std::mutex> lg(metadata_mutex);
        if (open_metadata_descriptors->find(filename) == open_metadata_descriptors->end()) {
            open_metadata_descriptors->emplace(filename, new CapioFile(filename));
        }

        // register a new open on a file
        if (threads_opened_files->find(tid) == threads_opened_files->end()) {
            threads_opened_files->emplace(tid, new std::unordered_map<std::string, long>);
        }

        if (threads_opened_files->at(tid)->find(filename) == threads_opened_files->at(tid)->end()) {
            threads_opened_files->at(tid)->emplace(filename, 1);
        } else {
            threads_opened_files->at(tid)->at(filename) =
                threads_opened_files->at(tid)->at(filename) + 1;
        }

        unlock_awaiting_threads_for_creation(filename);
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
        auto metadata = open_metadata_descriptors->at(filename)->get_metadata();
        unlock_awaiting_threads_for_commit(
            filename); // if committed unlock threads waiting to continue
        return metadata;
    }

    void update_metadata(std::filesystem::path &filename, long filesize, long n_close,
                         bool committed) {
        std::lock_guard<std::mutex> lg(metadata_mutex);
        open_metadata_descriptors->at(filename)->update_metadata(filesize, n_close, committed);
        if (committed) {
            unlock_awaiting_threads_for_commit(filename);
        }
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
        unlock_awaiting_threads_for_commit(filename);
    }

    bool is_committed(std::filesystem::path &filename, long tid) {
        this->create_capio_file(filename, tid);
        return open_metadata_descriptors->at(filename)->is_committed();
    }

    void add_thread_awaiting_for_commit(const std::filesystem::path &filename, long tid) const {
        if (pending_requests_on_file->find(filename) == pending_requests_on_file->end()) {
            pending_requests_on_file->emplace(filename, new std::vector<long>);
        }
        pending_requests_on_file->at(filename)->emplace_back(tid);
    }

    void add_thread_awaiting_for_creation(const std::filesystem::path &filename, long tid) const {
        if (pending_requests_on_creation->find(filename) == pending_requests_on_creation->end()) {
            pending_requests_on_creation->emplace(filename, new std::vector<long>);
        }
        pending_requests_on_creation->at(filename)->emplace_back(tid);
    }

    void close_all_files(int tid) {

        if (threads_opened_files->find(tid) == threads_opened_files->end()) {
            return;
        }

        auto map = threads_opened_files->at(tid);

        for (auto entry : *map) {
            std::filesystem::path filename(entry.first);
            if (filename == get_capio_dir() || !capio_configuration->file_to_be_handled(filename)) {
                continue;
            }
            long opens = entry.second;
            update_n_close(filename, opens);
            // TODO: check if is committed and then update commit
        }

        threads_opened_files->erase(threads_opened_files->find(tid));
    }

    static bool exists_file(const std::filesystem::path &path) {
        return std::filesystem::exists(path);
    }

    auto size(std::filesystem::path path, long tid, long fd) {
        // TODO: fare meglio con i metadati, ma per ora mi accontento di questo
        // TODO: serve anche tenere in considerazione gli offset dei files
        return std::get<0>(get_metadata(path));
    }

    void register_tid_offset(int tid, int fd) {
        if (file_descriptors_offsets->find(tid) == file_descriptors_offsets->end()) {
            file_descriptors_offsets->emplace(tid, new std::unordered_map<long, long>);
        }

        file_descriptors_offsets->at(tid)->emplace(fd, 0);
    }

    // TODO: handle seeks
    void update_offset(long tid, long fd, long read_size, std::string path, int whence = -1) {
        START_LOG(gettid(), "call()");
        if (whence == -1) {
            file_descriptors_offsets->at(tid)->at(fd) =
                file_descriptors_offsets->at(tid)->at(fd) + read_size;
            return;
        }
        LOG("handling seek");
        //if i get here I am handling a seek

        if (whence == SEEK_SET) {
            file_descriptors_offsets->at(tid)->at(fd) = 0;
        } else if (whence == SEEK_CUR) {
            file_descriptors_offsets->at(tid)->at(fd) = read_size;
        } else if (whence == SEEK_END) {
            file_descriptors_offsets->at(tid)->at(fd) = std::filesystem::file_size(path);
        }
    }

    [[nodiscard]] long offset_of(long tid, long fd) const {

        auto process_offset = file_descriptors_offsets->find(tid);
        if (process_offset == file_descriptors_offsets->end()) {
            return 0;
        }

        auto fd_offset = process_offset->second->find(fd);

        return fd_offset != process_offset->second->end() ? fd_offset->second : 0;
    }

    void add_thread_awaiting_for_data(std::string path, long tid, long fd, long read_size) {
        // [path, tid, fd] -> read size request. used to store the pending requests to read

        if (pending_requests_on_available_data->find(path) ==
            pending_requests_on_available_data->end()) {
            pending_requests_on_available_data->emplace(
                path, new std::unordered_map<long, std::unordered_map<long, long> *>);
        }

        auto filename_map = pending_requests_on_available_data->at(path);

        if (filename_map->find(tid) == filename_map->end()) {
            filename_map->emplace(tid, new std::unordered_map<long, long>);
        }

        filename_map->at(tid)->emplace(fd, read_size);
    }
};

StorageEngine *storage_engine;

#endif // STORAGE_ENGINE_HPP
