#include <algorithm>
#include <array>
#include <emscripten.h>

#include "zstd-dict.h"
#include "zstd-read.h"

//
// ZstdDecompressRead
//
///////////////////////////////////////////////////////////////////////////////

ZstdDecompressRead::ZstdDecompressRead()
    : stream_(nullptr, ZSTD_freeDStream)
    , chunk_offset_()
    , next_read_size_()
    , src_offset_()
    , src_bytes_()
    , dest_bytes_()
{
}


ZstdDecompressRead::~ZstdDecompressRead()
{
}


bool ZstdDecompressRead::Begin()
{
    return Begin([](ZSTD_DStream* dstream) {
        return ZSTD_initDStream(dstream);
    });
}


bool ZstdDecompressRead::Begin(const ZstdDecompressionDict& ddict)
{
    return Begin([&ddict](ZSTD_DStream* dstream) {
        return ZSTD_initDStream_usingDDict(dstream, ddict.get());
    });
}

bool ZstdDecompressRead::Load(const Vec<u8>& chunk) {
    
    EM_ASM({
        console.log("Loading chunks");
        console.log("checking chunk_bytes_ size... : ", $0);
    }, chunk_bytes_.size());

    // cannot load chunk while there is still one
    if (chunk_bytes_.size()) return false;

    // load chunk into chunk_bytes_
    const auto copy_begin = std::begin(chunk);
    const auto copy_end = copy_begin + chunk.size();

    std::copy(copy_begin, copy_end, std::back_inserter(chunk_bytes_));

    //checking the contents of loaded chunk_bytes_
    EM_ASM({
        console.log("loaded chunk_bytes_ size: ", $0);
    }, chunk_bytes_.size());

    return true;
}

bool ZstdDecompressRead::Read(StreamCallback callback) {
    EM_ASM(console.log("Read"));

    // returns false if there is nothing left to read
    if (chunk_offset_ == chunk_bytes_.size()) {
        chunk_bytes_.clear();
        chunk_offset_ = 0;
        return false;
    }

    EM_ASM({
        console.log("checking src_bytes_ size... : ", $0);
    }, src_bytes_.size());

    if (src_bytes_.size() == 0) {
        const auto src_available = src_bytes_.capacity() - src_bytes_.size();
        const auto chunk_remains = chunk_bytes_.size() - chunk_offset_;
        const auto copy_size = std::min(src_available, chunk_remains);

        const auto copy_begin = std::begin(chunk_bytes_) + chunk_offset_;
        const auto copy_end = copy_begin + copy_size;

        // chunk_offset += copy_size;

        // append src bytes
        std::copy(copy_begin, copy_end, std::back_inserter(src_bytes_));

        // compress if enough bytes ready
        if (src_bytes_.size() >= next_read_size_ || src_available == 0u) {
            const auto success = Decompress(callback);
            if (!success) return false;
        }
    }
    else {
        const auto success = Decompress(callback);
        return success;
    }
   
    return true;
}

bool ZstdDecompressRead::Decompress(const StreamCallback& callback)
{
    if (src_bytes_.empty()) return true;

    ZSTD_inBuffer input { &src_bytes_[0], src_bytes_.size(), src_offset_};
    int prev_src_offset = src_offset_;
    if (input.pos < input.size) {
        dest_bytes_.resize(dest_bytes_.capacity());
        ZSTD_outBuffer output { &dest_bytes_[0], dest_bytes_.size(), 0};
        next_read_size_ = ZSTD_decompressStream(stream_.get(), &output, &input);
        if (ZSTD_isError(next_read_size_)) return false;

        dest_bytes_.resize(output.pos);
        callback(dest_bytes_);
    }

    src_offset_ = input.pos;
    chunk_offset_ += src_offset_ - prev_src_offset;

    EM_ASM({
        console.log("src_offset_", $0);
        console.log("out of src_bytes_.size()", $1);
    }, src_offset_, src_bytes_.size());

    if (src_offset_ == src_bytes_.size()) {
        src_offset_ = 0; //maybe don't need
        src_bytes_.clear();
    }

    return true;
}


bool ZstdDecompressRead::Flush(StreamCallback callback)
{
    return Decompress(callback);
}


bool ZstdDecompressRead::End(StreamCallback callback)
{
    if (!HasStream()) return true;

    auto success = true;
    if (!src_bytes_.empty()) {
        success = Decompress(callback);
    }

    stream_.reset();
    return success;
}


bool ZstdDecompressRead::HasStream() const
{
    return stream_ != nullptr;
}


bool ZstdDecompressRead::Begin(DStreamInitializer initializer)
{
    if (HasStream()) return true;

    DStreamPtr stream(ZSTD_createDStream(), ZSTD_freeDStream);
    if (stream == nullptr) return false;

    const auto init_rc = initializer(stream.get());
    if (ZSTD_isError(init_rc)) return false;

    stream_ = std::move(stream);
    src_bytes_.reserve(ZSTD_DStreamInSize());
    dest_bytes_.resize(ZSTD_DStreamOutSize());  // resize
    next_read_size_ = init_rc;

    return true;
}


