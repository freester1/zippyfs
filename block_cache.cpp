#include "block_cache.h"
#include "util.h"
using namespace std;

BlockCache::BlockCache(string path_to_shdw)
    : path_to_shdw_(path_to_shdw) {

}

int
BlockCache::write(string path, const uint8_t* buf, uint64_t size, uint64_t offset) {
    // create blocks for buf. Number of blocks = ceil(size / block_size)
    uint64_t num_blocks = Util::ulong_ceil(size, Block::get_logical_size());
    bool in_cache = file_cache_.find(path) == file_cache_.end();
    // check if path has blocks at those indexes
    // for each block, add to cache for file
    uint64_t cached_bytes = 0;
    uint64_t block_size = 0;
    for (unsigned int block_idx = offset / Block::get_logical_size(); block_idx < num_blocks; block_idx++) {

        // first jump to latest byte
        cached_bytes += block_size;

        // then determine how much space we have to add
        if (cached_bytes + Block::get_logical_size() > size)
            block_size = size - cached_bytes;
        else
            block_size = Block::get_logical_size();
        if (in_cache) {
            // invalidate old block
            auto block_mp = file_cache_.find(path)->second;
            if(block_mp.find(block_idx) != block_mp.end())
                file_cache_[path][block_idx]->set_dirty();
        }

        // finally create block with that much space at the current byte
        shared_ptr<Block> ptr(new Block(buf + cached_bytes, block_size));

        // add newly formed block to file cache
        file_cache_[path][block_idx] = ptr;
    }


    // write meta data to cache_data
    return 0;

}

int
BlockCache::read(string path, uint8_t* buf, uint64_t size, uint64_t offset) {
    // get blocks
    uint64_t read_bytes = 0;
    bool offsetted = false;
    auto data = file_cache_.find(path)->second;
    auto num_blocks = data.size();
    for (unsigned int block_idx = offset / Block::get_logical_size(); block_idx < num_blocks; block_idx++) {
        // we can read this block, find the data
        auto block = data.find(block_idx)->second;
        auto block_data = block->get_data();
        // offset into the data and add all to buf
        // should only offset once
        auto offset_amt = 0;
        if (offsetted == false) {
            offset_amt = offset < Block::get_logical_size() ? offset : offset % Block::get_logical_size();
            offsetted = true;
        }
        for (auto byte = block_data.begin() + offset_amt;
                byte != block_data.end() || read_bytes < size; byte++) {
            buf[read_bytes++] = *byte;
        }
    }
    assert(read_bytes == size);
    return 0;
}

bool
BlockCache::in_cache(string path) {
    (void)path;
    return true;
}


int
flush_to_shdw() {
    return 0;
}
