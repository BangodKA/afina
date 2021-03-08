#include "SimpleLRU.h"
#include "StripedLRU.h"

#include <exception>
#include <utility>

#include <memory>

namespace Afina {

namespace Backend {

template<typename T, typename ...Args>
std::unique_ptr<T> MakeUnique( Args&& ...args )
{
    return std::unique_ptr<T>( new T( std::forward<Args>(args)... ) );
}

StripedLRU StripedLRU::CreateCache(size_t shards_amount, size_t max_size) {
  if (max_size / shards_amount < 1 * 1024 * 1024UL) {
    throw std::runtime_error("Too many shards for that size");
  }
  return std::move(StripedLRU {shards_amount, max_size / shards_amount});
}

StripedLRU::StripedLRU(size_t shards_amount, size_t max_shard_size)
                      : SimpleLRU(max_shard_size)
{
  shards = MakeUnique<std::vector<std::unique_ptr<ThreadSafeSimplLRU> > >();
  for (size_t i = 0; i < shards_amount; ++i) {
    shards->emplace_back(MakeUnique<ThreadSafeSimplLRU>(max_shard_size));
  }
}

  
bool StripedLRU::Put(const std::string &key, const std::string &value) {
  return shards->at(hasher(key) % shards->size()).get()->Put(key, value);
}

bool StripedLRU::PutIfAbsent(const std::string &key, const std::string &value) {
  return shards->at(hasher(key) % shards->size()).get()->PutIfAbsent(key, value);
}

bool StripedLRU::Set(const std::string &key, const std::string &value) {
  return shards->at(hasher(key) % shards->size()).get()->Set(key, value);
}

bool StripedLRU::Delete(const std::string &key) {
  return shards->at(hasher(key) % shards->size()).get()->Delete(key);
}

bool StripedLRU::Get(const std::string &key, std::string &value) {
  return shards->at(hasher(key) % shards->size()).get()->Get(key, value);
}

} // namespace Backend
} // namespace Afina