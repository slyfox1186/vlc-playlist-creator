#include <iostream>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string>
#include <vector>
#include <thread>
#include <future>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sstream>
#include <iomanip>
#include <array>
#include <cctype>
#include <cmath>
#include <getopt.h>

namespace fs = std::filesystem;

bool is_wsl() {
    std::ifstream file("/proc/version");
    if (!file.is_open()) {
        return false;
    }
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return content.find("microsoft") != std::string::npos;
}

std::string convert_to_wsl_path(const std::string& path) {
    std::string command = "wslpath -w \"" + path + "\"";
    std::array<char, 1024> buffer;
    std::string result;
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return path;
    }
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    pclose(pipe);
    if (result.empty() || result.find("Invalid argument") != std::string::npos) {
        return path;  // Return original path if conversion failed
    }
    // Remove trailing newline and replace backslashes with forward slashes
    result = result.substr(0, result.find_last_not_of("\n\r") + 1);
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}

struct VideoInfo {
    std::string file_path;
    double quality;
    double duration;
};

VideoInfo get_video_quality(const std::string& file_path, bool verbose = false) {
    if (verbose) {
        std::cout << "Processing video: " << file_path << std::endl;
    }
    std::string command = "ffprobe -v error -select_streams v:0 -show_entries stream=width,height,bit_rate,avg_frame_rate,codec_name -show_entries format=duration -of default=noprint_wrappers=1 \"" + file_path + "\"";
    std::array<char, 1024> buffer;
    std::string result;
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return {file_path, 0, 0};
    }
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    int status = pclose(pipe);
    if (status != 0) {
        if (verbose) {
            std::cerr << "Error processing " << file_path << std::endl;
        }
        return {file_path, 0, 0};
    }

    std::regex width_re("width=(\\d+)");
    std::regex height_re("height=(\\d+)");
    std::regex bit_rate_re("bit_rate=(\\d+)");
    std::regex frame_rate_re("avg_frame_rate=(\\d+/\\d+)");
    std::regex codec_name_re("codec_name=(\\w+)");
    std::regex duration_re("duration=(\\d+\\.?\\d*)");

    std::smatch match;
    int width = 0, height = 0, bit_rate = 0;
    double frame_rate = 0, duration = 0;
    std::string codec_name;

    if (std::regex_search(result, match, width_re)) {
        width = std::stoi(match[1]);
    }
    if (std::regex_search(result, match, height_re)) {
        height = std::stoi(match[1]);
    }
    if (std::regex_search(result, match, bit_rate_re)) {
        bit_rate = std::stoi(match[1]);
    }
    if (std::regex_search(result, match, frame_rate_re)) {
        std::string fr = match[1];
        int num = std::stoi(fr.substr(0, fr.find('/')));
        int denom = std::stoi(fr.substr(fr.find('/') + 1));
        frame_rate = denom != 0 ? static_cast<double>(num) / denom : 0;
    }
    if (std::regex_search(result, match, codec_name_re)) {
        codec_name = match[1];
    }
    if (std::regex_search(result, match, duration_re)) {
        duration = std::stod(match[1]);
    }

    double quality = (width * height * frame_rate * bit_rate) / 1'000'000.0;

    if (codec_name == "h265" || codec_name == "hevc") {
        quality *= 1.5;
    } else if (codec_name == "h264" || codec_name == "avc") {
        quality *= 1.2;
    } else if (codec_name == "vp9") {
        quality *= 1.3;
    }

    return {file_path, quality, duration};
}

std::vector<std::string> find_mp4_files(const std::string& root_dir, bool verbose = false) {
    if (verbose) {
        std::cout << "Searching for MP4 files in directory: " << root_dir << std::endl;
    }
    std::vector<std::string> mp4_files;
    for (const auto& entry : fs::recursive_directory_iterator(root_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".mp4") {
            mp4_files.push_back(entry.path().string());
        }
    }
    if (verbose) {
        std::cout << "Found " << mp4_files.size() << " MP4 files." << std::endl;
    }
    return mp4_files;
}

std::string url_encode(const std::string &value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (char c : value) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || c == '/' || c == ':') {
            escaped << c;
        } else {
            escaped << '%' << std::setw(2) << int((unsigned char) c);
        }
    }

    return escaped.str();
}

void create_vlc_playlist(const std::vector<VideoInfo>& video_qualities, const std::string& output_playlist_file) {
    std::ofstream playlist(output_playlist_file);
    if (!playlist.is_open()) {
        std::cerr << "Failed to create playlist file: " << output_playlist_file << std::endl;
        return;
    }

    playlist << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    playlist << "<playlist xmlns=\"http://xspf.org/ns/0/\" xmlns:vlc=\"http://www.videolan.org/vlc/playlist/ns/0/\" version=\"1\">\n";
    playlist << "\t<title>Playlist</title>\n";
    playlist << "\t<trackList>\n";

    int count = 0;
    for (const auto& video : video_qualities) {
        playlist << "\t\t<track>\n";
        playlist << "\t\t\t<location>file:///" << url_encode(video.file_path) << "</location>\n";
        playlist << "\t\t\t<duration>" << std::fixed << std::setprecision(0) << std::llround(video.duration * 1000) << "</duration>\n";
        playlist << "\t\t\t<extension application=\"http://www.videolan.org/vlc/playlist/0\">\n";
        playlist << "\t\t\t\t<vlc:id>" << count << "</vlc:id>\n";
        playlist << "\t\t\t</extension>\n";
        playlist << "\t\t</track>\n";
        count++;
    }

    playlist << "\t</trackList>\n";
    playlist << "\t<extension application=\"http://www.videolan.org/vlc/playlist/0\">\n";

    for (int i = 0; i < count; i++) {
        playlist << "\t\t<vlc:item tid=\"" << i << "\"/>\n";
    }

    playlist << "\t</extension>\n";
    playlist << "</playlist>\n";

    playlist.close();
    std::cout << "VLC playlist created at " << output_playlist_file << std::endl;
}

void display_help() {
    std::cout << "Video Quality Ranker\n";
    std::cout << "====================\n\n";
    std::cout << "This program scans a directory for MP4 files, analyzes their quality, and optionally creates a VLC playlist.\n";
    std::cout << "By default, it lists files sorted by path. Use the -q option to sort by quality instead.\n\n";
    std::cout << "Usage: video-quality-ranker [OPTIONS] <directory>\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help        Display this help menu and exit\n";
    std::cout << "  -l <file>         Specify a log file to save the output\n";
    std::cout << "  -f                Force Linux path style (don't convert to Windows paths in WSL)\n";
    std::cout << "  -v                Verbose mode: display detailed processing information\n";
    std::cout << "  -q                Order videos by quality instead of file path\n\n";
    std::cout << "Examples:\n";
    std::cout << "  video-quality-ranker /path/to/videos\n";
    std::cout << "    List all MP4 files in the directory, sorted by file path\n\n";
    std::cout << "  video-quality-ranker -q /path/to/videos\n";
    std::cout << "    List all MP4 files, sorted by video quality (highest first)\n\n";
    std::cout << "  video-quality-ranker -v /path/to/videos\n";
    std::cout << "    List files and show verbose output\n\n";
    std::cout << "  video-quality-ranker -l output.txt -q /path/to/videos\n";
    std::cout << "    Sort by quality, and save the output to a log file\n";
}

int main(int argc, char* argv[]) {
    std::string root_dir;
    std::string log_file;
    bool force_linux_path = false;
    bool verbose = false;
    bool order_by_quality = false;

    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;
    while ((opt = getopt_long(argc, argv, "hl:fvq", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'h':
                display_help();
                return 0;
            case 'l':
                log_file = optarg;
                break;
            case 'f':
                force_linux_path = true;
                break;
            case 'v':
                verbose = true;
                break;
            case 'q':
                order_by_quality = true;
                break;
            default:
                std::cerr << "Usage: " << argv[0] << " [-h] [-l log_file] [-f] [-v] [-q] root_dir" << std::endl;
                std::cerr << "Use -h or --help for more information." << std::endl;
                exit(EXIT_FAILURE);
        }
    }

    if (optind >= argc) {
        std::cerr << "Expected argument after options" << std::endl;
        std::cerr << "Use -h or --help for more information." << std::endl;
        exit(EXIT_FAILURE);
    }

    root_dir = argv[optind];
    
    // Convert to absolute path
    try {
        root_dir = fs::absolute(root_dir).string();
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error resolving path: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    if (verbose) {
        std::cout << "Processing directory: " << root_dir << std::endl;
    }

    auto mp4_files = find_mp4_files(root_dir, verbose);

    std::vector<std::future<VideoInfo>> futures;
    for (const auto& file : mp4_files) {
        futures.push_back(std::async(std::launch::async, get_video_quality, file, verbose));
    }

    std::vector<VideoInfo> video_qualities;
    for (auto& future : futures) {
        video_qualities.push_back(future.get());
    }

    if (order_by_quality) {
        std::sort(video_qualities.begin(), video_qualities.end(), [](const VideoInfo& a, const VideoInfo& b) {
            return b.quality < a.quality;
        });
    } else {
        std::sort(video_qualities.begin(), video_qualities.end(), [](const VideoInfo& a, const VideoInfo& b) {
            return a.file_path < b.file_path;
        });
    }

    bool wsl = is_wsl() && !force_linux_path;

    std::ostringstream output;
    for (size_t i = 0; i < video_qualities.size(); ++i) {
        auto& [file, quality, duration] = video_qualities[i];
        if (wsl) {
            file = convert_to_wsl_path(file);
        }
        output << file << "\n";
    }

    std::cout << output.str();

    if (!log_file.empty()) {
        std::ofstream log_output(log_file);
        log_output << output.str();
    }

    create_vlc_playlist(video_qualities, "vlc_playlist.xspf");

    return 0;
}
