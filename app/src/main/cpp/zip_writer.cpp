#include "zip_writer.h"
#include <cstring>
#include <fstream>
#include <zlib.h>

#pragma pack(push, 1)

struct ZipLocalFileHeader {
  uint32_t signature = 0x04034b50;
  uint16_t versionNeeded = 20;
  uint16_t flags = 0;
  uint16_t compression = 8;
  uint16_t modTime = 0;
  uint16_t modDate = 0;
  uint32_t crc32 = 0;
  uint32_t compressedSize = 0;
  uint32_t uncompressedSize = 0;
  uint16_t filenameLength = 0;
  uint16_t extraFieldLength = 0;
};

struct ZipCentralDirEntry {
  uint32_t signature = 0x02014b50;
  uint16_t versionMadeBy = 20;
  uint16_t versionNeeded = 20;
  uint16_t flags = 0;
  uint16_t compression = 8;
  uint16_t modTime = 0;
  uint16_t modDate = 0;
  uint32_t crc32 = 0;
  uint32_t compressedSize = 0;
  uint32_t uncompressedSize = 0;
  uint16_t filenameLength = 0;
  uint16_t extraFieldLength = 0;
  uint16_t fileCommentLength = 0;
  uint16_t diskNumberStart = 0;
  uint16_t internalFileAttrs = 0;
  uint32_t externalFileAttrs = 0;
  uint32_t localHeaderOffset = 0;
};

struct ZipEOCD {
  uint32_t signature = 0x06054b50;
  uint16_t diskNumber = 0;
  uint16_t diskWithCentralDir = 0;
  uint16_t numEntriesThisDisk = 0;
  uint16_t numEntriesTotal = 0;
  uint32_t centralDirSize = 0;
  uint32_t centralDirOffset = 0;
  uint16_t commentLength = 0;
};

#pragma pack(pop)

static uint32_t crc32Buf(const char* data, size_t len) {
  return crc32(0, reinterpret_cast<const Bytef*>(data), len);
}

static bool deflateBuf(const char* in, size_t inLen,
                        std::vector<char>& out, int level = Z_BEST_COMPRESSION) {
  z_stream strm = {};
  deflateInit(&strm, level);
  out.resize(deflateBound(&strm, inLen));
  strm.next_in = reinterpret_cast<const Bytef*>(in);
  strm.avail_in = inLen;
  strm.next_out = reinterpret_cast<Bytef*>(out.data());
  strm.avail_out = out.size();
  deflate(&strm, Z_FINISH);
  out.resize(strm.total_out);
  deflateEnd(&strm);
  return true;
}

bool createZip(const std::string& zipPath,
               const std::vector<std::pair<std::string, std::string>>& files,
               const std::string& baseDir) {
  std::ofstream ofs(zipPath, std::ios::binary);
  if (!ofs) return false;

  std::vector<ZipCentralDirEntry> dirEntries;
  std::vector<uint32_t> localOffsets;

  for (const auto& f : files) {
    std::string entryName = f.first;
    if (!baseDir.empty()) {
      entryName = baseDir + "/" + f.first;
    }

    ZipLocalFileHeader lh;
    lh.filenameLength = entryName.size();

    const std::string& content = f.second;
    uint32_t crc = crc32Buf(content.data(), content.size());

    std::vector<char> compressed;
    bool useDeflate = (content.size() > 0);
    if (useDeflate) {
      deflateBuf(content.data(), content.size(), compressed);
      if (compressed.size() >= content.size()) {
        useDeflate = false;
      }
    }

    if (!useDeflate) {
      lh.compression = 0;
      lh.compressedSize = content.size();
      lh.uncompressedSize = content.size();
      lh.crc32 = crc;
    } else {
      lh.compression = 8;
      lh.compressedSize = compressed.size();
      lh.uncompressedSize = content.size();
      lh.crc32 = crc;
    }

    localOffsets.push_back(ofs.tellp());
    ofs.write(reinterpret_cast<const char*>(&lh), sizeof(lh));
    ofs.write(entryName.data(), entryName.size());

    if (!useDeflate) {
      ofs.write(content.data(), content.size());
    } else {
      ofs.write(compressed.data(), compressed.size());
    }

    ZipCentralDirEntry ce;
    ce.compression = lh.compression;
    ce.compressedSize = lh.compressedSize;
    ce.uncompressedSize = lh.uncompressedSize;
    ce.crc32 = lh.crc32;
    ce.filenameLength = entryName.size();
    ce.localHeaderOffset = localOffsets.back();
    dirEntries.push_back(ce);
  }

  uint32_t centralDirOffset = ofs.tellp();
  for (size_t i = 0; i < dirEntries.size(); ++i) {
    auto& ce = dirEntries[i];
    std::string entryName = files[i].first;
    if (!baseDir.empty()) {
      entryName = baseDir + "/" + files[i].first;
    }
    ce.filenameLength = entryName.size();
    ofs.write(reinterpret_cast<const char*>(&ce), sizeof(ce));
    ofs.write(entryName.data(), entryName.size());
  }

  uint32_t centralDirSize = ofs.tellp() - centralDirOffset;

  ZipEOCD eocd;
  eocd.numEntriesThisDisk = dirEntries.size();
  eocd.numEntriesTotal = dirEntries.size();
  eocd.centralDirSize = centralDirSize;
  eocd.centralDirOffset = centralDirOffset;
  ofs.write(reinterpret_cast<const char*>(&eocd), sizeof(eocd));

  ofs.close();
  return true;
}
