#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

std::unique_ptr<SimpleLRU::lru_node> SimpleLRU::ExtractNode(const std::string &key) {
  lru_node &del_node = _lru_index.at(key);
  if (del_node.prev == nullptr) {
    std::unique_ptr<lru_node> res = std::move(_lru_head);
    _lru_head = std::move(res->next);
    if (_lru_head) {
      _lru_head->prev = nullptr;
    }
    return res;
  }

  if (del_node.next == nullptr) {
    std::unique_ptr<lru_node> res(_lru_tail);
    _lru_tail = _lru_tail->prev;
    _lru_tail->next.release();
    _lru_tail->next = nullptr;
    return res;
  }

  del_node.next->prev = del_node.prev;
  std::unique_ptr<lru_node> res = std::move(del_node.prev->next);
  del_node.prev->next = std::move(del_node.next);
  return res;
}

void SimpleLRU::BoostRank(const std::string &key) {
  lru_node &del_node = _lru_index.at(key);
  if (del_node.next == nullptr) {
    return;
  }
  _lru_tail->next = ExtractNode(key);
  _lru_tail->next->prev = _lru_tail;
  _lru_tail = _lru_tail->next.get();
}

void SimpleLRU::ChangeValue(const std::string &key, const std::string &value) {
  BoostRank(key);

  _cur_size += value.size() - _lru_tail->value.size();
  _lru_tail->value = value;
}

void SimpleLRU::AddKeyValue(const std::string &key, const std::string &value){
  size_t additional_size = key.size() + value.size();
  while (additional_size + _cur_size > _max_size) {
    auto del_key = _lru_head->key;
    _cur_size -= del_key.size() + _lru_head->value.size();
    Delete(del_key);
  }
  
  if (_lru_head == nullptr) {
    _cur_size += additional_size;
    _lru_tail = new lru_node({key, value, nullptr, nullptr});
    _lru_head.reset(_lru_tail);
    _lru_index.emplace(_lru_tail->key, *_lru_tail);
    return;
  }

  if (_lru_index.find(key) == _lru_index.end()) {
    _cur_size += additional_size;
    _lru_tail->next.reset(new lru_node({key, value, _lru_tail, nullptr}));
    _lru_tail = _lru_tail->next.get();
    _lru_index.emplace(_lru_tail->key, *_lru_tail);
    return;
  }

  ChangeValue(key, value);
}

bool SimpleLRU::CheckSize(std::size_t value_size, std::size_t new_key_size,
                          std::size_t prev_value_size) {
  if (value_size + new_key_size - prev_value_size > _max_size) {
    return false;
  }
  return true;
}


// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
  auto it = _lru_index.find(key);
  if (it != _lru_index.end()) {
    if (!CheckSize(value.size(), 0, it->first.get().size())) {
      return false;
    }
  } else if (!CheckSize(value.size(), key.size())) {
    return false;
  }

  AddKeyValue(key, value);
  return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
  if (_lru_index.find(key) != _lru_index.end() || !CheckSize(value.size(), key.size())) {
    return false;
  }

  AddKeyValue(key, value);
  return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
  auto it = _lru_index.find(key);
  if (it == _lru_index.end() || !CheckSize(value.size(), 0, it->first.get().size())) {
    return false;
  }
  
  ChangeValue(key, value);
  return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
  auto it = _lru_index.find(key);
  if (it == _lru_index.end()) {
    return false;
  }

  auto temp = ExtractNode(key);
  _lru_index.erase(key);
  return true;
}

// See MapBasedGlobalLockImpl.h

bool SimpleLRU::Get(const std::string &key, std::string &value) {
  auto it = _lru_index.find(key);
  if (it == _lru_index.end()) {
    return false;
  }

  value = it->second.get().value;
  BoostRank(key);
  return true;
}

} // namespace Backend
} // namespace Afina
