#ifndef ZIP_WRITER_H
#define ZIP_WRITER_H

#include <string>
#include <vector>

bool createZip(const std::string& zipPath,
               const std::vector<std::pair<std::string, std::string>>& files,
               const std::string& baseDir);

#endif
