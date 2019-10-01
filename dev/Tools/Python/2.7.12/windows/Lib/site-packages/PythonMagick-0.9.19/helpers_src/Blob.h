#ifndef HELPER_BLOB_H
#define HELPER_BLOB_H

#include <Magick++/Blob.h>
#include <string>

void update_wrapper(Magick::Blob& blob, const std::string& data);
void updateNoCopy_wrapper(Magick::Blob& blob, std::string& data);
std::string get_blob_data(const Magick::Blob& blob);

#endif
