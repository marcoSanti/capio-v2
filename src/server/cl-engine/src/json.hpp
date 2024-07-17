#ifndef CAPIO_SERVER_UTILS_JSON_HPP
#define CAPIO_SERVER_UTILS_JSON_HPP

#include <singleheader/simdjson.h>


inline void load_configuration(const std::string &conf_file, const std::filesystem::path &capio_dir,
                               simdjson::padded_string &json) {
    CapioFileLocations file_locations;
    simdjson::ondemand::parser parser;

    std::unordered_map<std::string, std::vector<std::string_view>> alias_map;

    START_LOG(gettid(), "call(config_file='%s', capio_dir='%s')", conf_file.c_str(),
              capio_dir.c_str());

    auto doc                           = parser.iterate(json);
    simdjson::ondemand::object objects = doc.get_object();

    try {
        workflow_name = std::string(objects["name"].get_string().value());
    } catch (simdjson::simdjson_error &e) {
        std::cout << CAPIO_LOG_SERVER_CLI_LEVEL_ERROR
                  << "Current configuration file does not provide required section name"
                  << std::endl;
        ERR_EXIT("Current configuration file does not provide required section name");
    }
    std::cout << CAPIO_LOG_SERVER_CLI_LEVEL_JSON << "Workflow name: " << workflow_name << std::endl;

    try {
        auto aliases = objects["alias"].get_object();
        for (auto alias : aliases) {
            std::string_view alias_name = alias.unescaped_key();
            std::vector<std::string_view> resolved_alias;
            for (auto t : alias.value().get_array().value()) {
                std::string_view tmp_str;
                t.get(tmp_str);
                resolved_alias.emplace_back(tmp_str);
                file_locations.newFile(std::string(tmp_str));
            }
            alias_map.emplace(alias_name, resolved_alias);
            std::cout << CAPIO_LOG_SERVER_CLI_LEVEL_JSON << "Alias: " << alias_name << " = [";
            for (auto str : resolved_alias) {
                std::cout << " " << str;
            }
            std::cout << " ]" << std::endl;
        }
    } catch (simdjson::simdjson_error &e) {
    }

    simdjson::simdjson_result<simdjson::ondemand::object> io_graph;

    try {
        io_graph = objects["io_graph"].get_object();
    } catch (simdjson::simdjson_error &e) {
        std::cout << CAPIO_LOG_SERVER_CLI_LEVEL_ERROR
                  << "Current configuration file does not provide required section io_graph"
                  << std::endl;
        ERR_EXIT("Current configuration file does not provide required section io_graph");
    }

    for (auto application : io_graph) {
        auto application_name = std::string(application.unescaped_key().value());

        std::cout << CAPIO_LOG_SERVER_CLI_LEVEL_JSON
                  << "Parsing configuration for: " << application_name << std::endl;

        try {
            auto output = application.value()["output"].get_object();
            for (auto output_file : output) {
                std::string file_name     = std::string(output_file.unescaped_key().value());
                std::string commit_number = "-1";
                bool isAlias              = (alias_map.find(file_name) != alias_map.end());

                file_locations.newFile(file_name);

                if (isAlias) {
                    for (auto name : alias_map.at(file_name)) {
                        std::string producer_name = std::string(name);
                        file_locations.addProducer(application_name, producer_name);
                    }
                } else {
                    file_locations.addProducer(file_name, application_name);
                }

                try {
                    auto commit_rule =
                        output_file.value()["committed"].value().get_string().value();

                    if (commit_rule.find(":") != std::string::npos) {
                        commit_number = std::string(
                            commit_rule.substr(commit_rule.find(":") + 1, commit_rule.length()));
                        commit_rule = commit_rule.substr(0, commit_rule.find(":"));
                        std::cout << CAPIO_LOG_SERVER_CLI_LEVEL_JSON << "File is committed "
                                  << commit_rule << " after " << commit_number << " times."
                                  << std::endl;
                    }

                    if (isAlias) {
                        for (auto name : alias_map.at(file_name)) {
                            std::string name_str = std::string(name);
                            file_locations.setCommitRule(name_str, std::string(commit_rule));
                            if (commit_number != "-1") {
                                file_locations.setCommitedNumber(name_str,
                                                                 std::stoi(commit_number));
                            }
                        }
                    } else {
                        file_locations.setCommitRule(file_name, std::string(commit_rule));
                        if (commit_number != "-1") {
                            file_locations.setCommitedNumber(file_name, std::stoi(commit_number));
                        }
                    }

                } catch (simdjson::simdjson_error &e) {
                    std::cout << CAPIO_LOG_SERVER_CLI_LEVEL_JSON << "No committed rule for file "
                              << file_name << std::endl;
                }
                try {
                    auto fire_rule = output_file.value()["mode"].value().get_string().value();

                    if (isAlias) {
                        for (auto name : alias_map.at(file_name)) {
                            std::string name_str = std::string(name);
                            file_locations.setFireRule(name_str, std::string(fire_rule));
                        }
                    } else {
                        file_locations.setFireRule(file_name, std::string(fire_rule));
                    }

                } catch (simdjson::simdjson_error &e) {
                    std::cout << CAPIO_LOG_SERVER_CLI_LEVEL_JSON << "No fire rule for file "
                              << file_name << std::endl;
                }

                try {
                    auto permanent = output_file.value()["permanent"].value().get_bool().value();

                    if (isAlias) {
                        for (auto name : alias_map.at(file_name)) {
                            std::string name_str = std::string(name);
                            file_locations.setPermanent(name_str, permanent);
                        }
                    } else {
                        file_locations.setPermanent(file_name, permanent);
                    }

                } catch (simdjson::simdjson_error &e) {
                    std::cout << CAPIO_LOG_SERVER_CLI_LEVEL_JSON << "No permanent rule for file "
                              << file_name << std::endl;
                }

                try {
                    auto exclude = output_file.value()["exclude"].value().get_bool().value();

                    if (isAlias) {
                        for (auto name : alias_map.at(file_name)) {
                            std::string name_str = std::string(name);
                            file_locations.setExclude(name_str, exclude);
                        }
                    } else {
                        file_locations.setExclude(file_name, exclude);
                    }
                } catch (simdjson::simdjson_error &e) {
                    std::cout << CAPIO_LOG_SERVER_CLI_LEVEL_JSON << "No exclude rule for file "
                              << file_name << std::endl;
                }

                try {
                    auto exclude = output_file.value()["kind"].value().get_string().value();
                    if (exclude == "d") {
                        std::cout << CAPIO_LOG_SERVER_CLI_LEVEL_JSON << "File " << file_name
                                  << " is a directory" << std::endl;
                        file_locations.setDirectory(file_name);
                    } else {
                        std::cout << CAPIO_LOG_SERVER_CLI_LEVEL_JSON << "File " << file_name
                                  << " is a file" << std::endl;
                        file_locations.setFile(file_name);
                    }
                } catch (simdjson::simdjson_error &e) {
                    std::cout << CAPIO_LOG_SERVER_CLI_LEVEL_JSON << "Setting file " << file_name
                              << " to be a file" << std::endl;
                    file_locations.setFile(file_name);
                }

                try {
                    auto policy_name = output_file.value()["policy"].value().get_string();
                    std::cout << CAPIO_LOG_SERVER_CLI_LEVEL_JSON
                              << "Policy was specified but CAPIO does not support other policies "
                                 "other than CREATE";
                } catch (simdjson::simdjson_error &e) {
                    std::cout << CAPIO_LOG_SERVER_CLI_LEVEL_JSON << "No policy rule for file "
                              << file_name << std::endl;
                }

                try {
                    auto directory_file_count =
                        output_file.value()["n_files"].value().get_int64().value();

                    if (directory_file_count > 0) {
                        if (isAlias) {
                            for (auto name : alias_map.at(file_name)) {
                                file_locations.setDirectoryFileCount(std::string(name),
                                                                     directory_file_count);
                            }
                        } else {
                            file_locations.setDirectoryFileCount(file_name, directory_file_count);
                        }
                    }

                } catch (simdjson::simdjson_error &e) {
                }
            }
        } catch (simdjson::simdjson_error &e) {
            std::cout << CAPIO_LOG_SERVER_CLI_LEVEL_JSON << "No output files for app "
                      << application_name << std::endl;
        }

        /**
         * TODO: AS OF NOW COMMIT, FIRE AND OTHER FIELDS ARE IGNORED IN INPUT STREAM SECTION!
         */
        try {
            auto input = application.value()["input"].get_object();
            for (auto input_file : input) {
                std::string file_name = std::string(input_file.unescaped_key().value());

                bool isAlias = (alias_map.find(file_name) != alias_map.end());

                if (isAlias) {
                    for (auto name : alias_map.at(file_name)) {
                        std::string name_str = std::string(name);
                        file_locations.addConsumer(name_str, application_name);
                    }
                } else {
                    file_locations.newFile(file_name);
                    file_locations.addConsumer(file_name, application_name);
                }
            }
        } catch (simdjson::simdjson_error &e) {
            std::cout << CAPIO_LOG_SERVER_CLI_LEVEL_JSON << "No input files for app "
                      << application_name << std::endl;
        }
    }

    file_locations.print();
    exit(EXIT_SUCCESS);
}

#endif // CAPIO_SERVER_UTILS_JSON_HPP
