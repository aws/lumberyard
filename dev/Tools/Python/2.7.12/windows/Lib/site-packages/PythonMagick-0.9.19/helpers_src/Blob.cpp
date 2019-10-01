#include "Blob.h"

void update_wrapper(Magick::Blob& blob, const std::string& data) {
    blob.update(data.c_str(),data.size());
}

void updateNoCopy_wrapper(Magick::Blob& blob, std::string& data) {
    std::string str;
    char* w = new char[data.size() + 1];
    std::copy(str.begin(), str.end(), w);
    w[str.size()] = '\0';
    blob.updateNoCopy(w,data.size(),Magick::Blob::NewAllocator);
}

std::string get_blob_data(const Magick::Blob& blob) {
    const char* data = static_cast<const char*>(blob.data());
    size_t length = blob.length();
    return std::string(data,data+length);
}
