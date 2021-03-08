#pragma once

#include "SimpleLRU.h"
#include "ThreadSafeSimpleLRU.h"

#include <vector>
#include <memory>

namespace Afina {

namespace Backend {

class StripedLRU : public SimpleLRU {
public:
  static StripedLRU CreateCache(size_t shards_amount = 16, size_t max_size = 1024 * 1024 * 1024);
  StripedLRU(StripedLRU &&other) : shards(std::move(other.shards)) {}
  StripedLRU(const StripedLRU&) = delete;

  // Implements Afina::Storage interface
  bool Put(const std::string &key, const std::string &value) override;

  // Implements Afina::Storage interface
  bool PutIfAbsent(const std::string &key, const std::string &value) override;

  // Implements Afina::Storage interface
  bool Set(const std::string &key, const std::string &value) override;

  // Implements Afina::Storage interface
  bool Delete(const std::string &key) override;

  // Implements Afina::Storage interface
  bool Get(const std::string &key, std::string &value) override;

  ~StripedLRU() = default;
private:
  StripedLRU(size_t shards_amount, size_t max_shard_size = 1024);

private:
  std::unique_ptr<std::vector<std::unique_ptr<ThreadSafeSimplLRU> > > shards;
  std::hash<std::string> hasher;
};

} // namespace Backend
} // namespace Afina