#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <sys/inotify.h>

#include <stdio.h>
#include <unistd.h>
#include <limits.h>

#define BUF_SIZE (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))

int main(int argc, char **argv) {
    std::filesystem::path filePath = "/home/eyusupov/.cache/dwltags";
    std::ifstream file(filePath);
    std::string line, component;
    std::vector<std::string> tag_names = {"1", "2", "3", "4", "5", "F1", "F2", "F3", "F4", "F5"};

    if (argc != 3) {
      std::cout << "Pass component as an argument\n";
      return 1;
    }
    // TODO: handle display (first arg)
    component = argv[2];

    int fd = inotify_init();
    if (fd == -1) {
        std::cerr << "Error initializing inotify.\n";
        return 1;
    }

    int wd = inotify_add_watch(fd, filePath.c_str(), IN_MODIFY);
    if (wd == -1) {
        std::cerr << "Error adding watch for file.\n";
        return 1;
    }

    char buffer[BUF_SIZE];
    ssize_t bytesRead;
    while (true) {
        bytesRead = read(fd, buffer, sizeof(buffer));
        if (bytesRead == -1) {
            std::cerr << "Error reading inotify events.\n";
            return 1;
        }

        for (char* p = buffer; p < buffer + bytesRead;) {
            struct inotify_event* event = reinterpret_cast<struct inotify_event*>(p);
            if (event->mask & IN_MODIFY) {
                file.clear();

                while (std::getline(file, line)) {
                    std::vector<std::string> words;
                    std::istringstream iss(line);
                    std::string display, event;
                    iss >> display;
                    iss >> event;

                    if (component == "layout") {
                      if (event == "layout") {
                        std::string layout;
                        iss >> layout;
                        std::cout << "{\"text\": \"  " << layout << "  \"}" << '\n';
                      }
                    } else if (component == "title") {
                      if (event == "title") {
                        std::string title;
                        iss >> title;
                        std::cout << "{\"text\": \"" << title << "\"}" << '\n';
                      }
                    } else {
                      if (event == "tags") {
                        int our_tag = std::stoi(component);
                        int mask = 1 << our_tag;

                        int active, selected, urgent;
                        std::string word;
                        iss >> word;
                        iss >> word;
                        active = std::stoi(word);
                        iss >> word;
                        selected = std::stoi(word);
                        iss >> word;
                        urgent = std::stoi(word);

                        std::string status = "";
                        if (active & mask) {
                          status += "\"active\",";
                        }
                        if (selected & mask) {
                          status += "\"selected\",";
                        }
                        if (urgent & mask) {
                          status += "\"urgent\",";
                        }
                        std::cout << "{\"text\": \" " << tag_names[our_tag] << " \"";
                        if (status.length()) {
                            std::cout << ", \"class\": [" << status << "]";
                        }
                        std::cout << "}" << '\n';
                      }
                    }
                }
                std::cout.flush();
            }
            p += sizeof(struct inotify_event) + event->len;
        }
    }

    file.close();
    inotify_rm_watch(fd, wd);
    close(fd);

    return 0;
}
