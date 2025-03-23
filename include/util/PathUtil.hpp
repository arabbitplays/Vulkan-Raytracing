//
// Created by oschdi on 3/23/25.
//

#ifndef PATHUTIL_HPP
#define PATHUTIL_HPP

#include <string>

class PathUtil {
  PathUtil() = delete;

  public:
    static std::string getFileName(const std::string& path)
    {
        size_t lastSlash = path.find_last_of("/\\");
        std::string filename = (lastSlash == std::string::npos) ? path : path.substr(lastSlash + 1);
        size_t lastDot = filename.find_last_of(".");
        filename = (lastDot == std::string::npos) ? filename : filename.substr(0, lastDot);
        return filename;
    }
};

#endif //PATHUTIL_HPP
