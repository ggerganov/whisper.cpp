#include "wer.h"

#include <cstdio>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <map>

std::vector<std::string> read_files_from_directory(const std::string& dir_path) {
    std::vector<std::string> file_paths;
    try {
        for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {
            if (entry.is_regular_file() && entry.path().extension() == ".txt") {
                file_paths.push_back(entry.path().string());
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        printf("Error reading directory %s: %s\n", dir_path.c_str(), e.what());
    }
    return file_paths;
}

std::string read_file_content(const std::string& file_path) {
    std::ifstream file(file_path);
    std::string content;

    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            content += line + "\n";
        }
        file.close();
    } else {
        printf("Unable to open file: %s\n", file_path.c_str());
    }

    return content;
}

std::string get_base_filename(const std::string& path) {
    return std::filesystem::path(path).filename().string();
}

void print_usage(const char* program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  -r, --reference PATH   Full path to reference transcriptions directory\n");
    printf("  -a, --actual PATH      Full path to actual transcriptions directory\n");
    printf("  --help                 Display this help message\n");
}

int main(int argc, char** argv) {
    if (argc == 1) {
        print_usage(argv[0]);
        return 0;
    }

    std::string reference_path;
    std::string actual_path;
    bool reference_set = false;
    bool actual_set = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--reference") == 0) {
            if (i + 1 < argc) {
                reference_path = argv[++i];
                reference_set = true;
            } else {
                printf("Error: Missing path after %s\n", argv[i]);
                print_usage(argv[0]);
                return 1;
            }
        } else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--actual") == 0) {
            if (i + 1 < argc) {
                actual_path = argv[++i];
                actual_set = true;
            } else {
                printf("Error: Missing path after %s\n", argv[i]);
                print_usage(argv[0]);
                return 1;
            }
        } else {
            printf("Error: Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    if (!reference_set || !actual_set) {
        printf("Error: Both reference and actual paths must be provided\n");
        print_usage(argv[0]);
        return 1;
    }

    if (!std::filesystem::exists(reference_path) || !std::filesystem::is_directory(reference_path)) {
        printf("Error: Reference path '%s' does not exist or is not a directory\n", reference_path.c_str());
        return 1;
    }

    if (!std::filesystem::exists(actual_path) || !std::filesystem::is_directory(actual_path)) {
        printf("Error: Actual path '%s' does not exist or is not a directory\n", actual_path.c_str());
        return 1;
    }

    std::vector<std::string> reference_files = read_files_from_directory(reference_path);
    std::vector<std::string> actual_files = read_files_from_directory(actual_path);

    //printf("Found %zu reference files in %s\n", reference_files.size(), reference_path.c_str());
    //printf("Found %zu actual files in %s\n", actual_files.size(), actual_path.c_str());

    std::map<std::string, std::string> reference_map;
    std::map<std::string, std::string> actual_map;

    for (const auto& file : reference_files) {
        reference_map[get_base_filename(file)] = file;
    }

    for (const auto& file : actual_files) {
        actual_map[get_base_filename(file)] = file;
    }

    for (const auto& [filename, ref_path] : reference_map) {
        auto actual_it = actual_map.find(filename);
        if (actual_it != actual_map.end()) {
            std::string reference_content = read_file_content(ref_path);
            std::string actual_content = read_file_content(actual_it->second);

            wer_result result = calculate_wer(reference_content, actual_content);
            printf("Word Error Rate for : %s\n", filename.c_str());
            printf("  Reference words: %ld\n", result.n_ref_words);
            printf("  Actual words: %ld\n", result.n_act_words);
            printf("  Substitutions: %d\n", result.n_sub);
            printf("  Deletions: %d\n", result.n_del);
            printf("  Insertions: %d\n", result.n_ins);
            printf("  Total edits: %d\n", result.n_edits);
            printf("  WER: %f\n", result.wer);
        } else {
            printf("Warning: No matching actual file found for reference file: %s\n", filename.c_str());
        }
    }

    return 0;
}
