#pragma once

#include <string>
#include <cctype>

class Uri {
public:
    Uri() = default;
    ~Uri() = default;

    Uri(const std::string& path)
        :path_(path) {SetPathToLowercase();}
    
    std::string path() const {
        return path_;
    }

    inline bool operator<(const Uri& other) const {
        return path_ < other.path_;
    }
    inline bool operator==(const Uri& other) const {
        return path_ == other.path_;
    }
private:
    std::string path_;
    void SetPathToLowercase() {
        for(auto& v: path_) v = static_cast<char>(tolower(v));
    }
};
