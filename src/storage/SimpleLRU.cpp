#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

std::unique_ptr<SimpleLRU::lru_node> SimpleLRU::ExtractNode(hash_map::const_iterator it) {
  lru_node &del_node = it->second.get();
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


void SimpleLRU::UpdateValue(hash_map::const_iterator it, const std::string &value) {
  _cur_size += value.size() - _lru_tail->value.size();
  _lru_tail->value = value;
}

bool SimpleLRU::IsTooBigForCash(size_t key_size, size_t value_size) {
 return key_size + value_size > _max_size;
}

void SimpleLRU::FreeSpace(int delta_space) {
  while (delta_space + _cur_size > _max_size) {
    auto del_key = _lru_head->key;
    _cur_size -= del_key.size() + _lru_head->value.size();
    Delete(del_key);
  }
}

void SimpleLRU::AddFirstElem(const std::string &key, const std::string &value) {
  _lru_tail = new lru_node({key, value, nullptr, nullptr});
  _lru_head.reset(_lru_tail);
  _lru_index.emplace(_lru_tail->key, *_lru_tail);
}

void SimpleLRU::PutNewToHead(const std::string &key, const std::string &value) {
  _lru_tail->next.reset(new lru_node({key, value, _lru_tail, nullptr}));
  _lru_tail = _lru_tail->next.get();    void AddFirstElem(const std::string &key, const std::string &value);

  _lru_index.emplace(_lru_tail->key, *_lru_tail);
}

void SimpleLRU::InsertNewNode(const std::string &key, const std::string &value) {
  _cur_size += key.size() + value.size();
  if (_lru_head == nullptr) {
    AddFirstElem(key, value);
    return;
  }  
  PutNewToHead(key, value);
}

void SimpleLRU::MoveToHead(hash_map::const_iterator it) {
  if (it->second.get().next == nullptr) {
    return;
  }
  _lru_tail->next = ExtractNode(it);
  _lru_tail->next->prev = _lru_tail;
  _lru_tail = _lru_tail->next.get();
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
  if (IsTooBigForCash(key.size(), value.size())) {
    return false;
  }

  auto it = _lru_index.find(key);

  if (it == _lru_index.end()) {
    FreeSpace(key.size() + value.size());
    InsertNewNode(key, value);
    return true;
  }

  MoveToHead(it);

  FreeSpace(value.size() - it->second.get().value.size());

  UpdateValue(it, value);

  return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
  auto it = _lru_index.find(key);
  if (it != _lru_index.end() || IsTooBigForCash(key.size(), value.size())) {
    return false;
  }

  FreeSpace(key.size() + value.size());
  InsertNewNode(key, value);
  return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
  auto it = _lru_index.find(key);
  if (it == _lru_index.end() ||
      IsTooBigForCash(0, value.size() - it->second.get().value.size())) {
    return false;
  }
  
  FreeSpace(value.size() - it->second.get().value.size());
  UpdateValue(it, value);
  return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
  auto it = _lru_index.find(key);
  if (it == _lru_index.end()) {
    return false;
  }

  auto temp = ExtractNode(it);
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
  MoveToHead(it);
  return true;
}

} // namespace Backend
} // namespace Afina
