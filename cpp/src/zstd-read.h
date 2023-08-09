#pragma once

#include <functional>
#include <array>

#include "common-types.h"
#include "zstd.h"


using StreamCallback = std::function<void(const Vec<u8>&)>;

class ZstdDecompressionDict;

class ZstdDecompressRead
{
public:
    ZstdDecompressRead(); //??
    ~ZstdDecompressRead(); //??

    bool Begin();
    bool Begin(const ZstdDecompressionDict& ddict);

    bool Load(const Vec<u8>& chunk);
    bool Read(StreamCallback callback);

    bool Flush(StreamCallback callback);
    bool End(StreamCallback callback);

    bool Print();

private:
    using DStreamPtr = std::unique_ptr<ZSTD_DStream, decltype(&ZSTD_freeDStream)>;
    using DStreamInitializer = std::function<size_t(ZSTD_DStream*)>;

    bool HasStream() const;
    bool Begin(DStreamInitializer initializer);
    bool Decompress(const StreamCallback& callback);

    DStreamPtr  stream_;
    size_t      chunk_offset_;
    Vec<u8>     chunk_bytes_;
    size_t      next_read_size_;
    size_t      src_offset_;
    Vec<u8>     src_bytes_;
    Vec<u8>     dest_bytes_;
};

