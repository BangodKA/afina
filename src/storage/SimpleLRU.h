#ifndef AFINA_STORAGE_SIMPLE_LRU_H
#define AFINA_STORAGE_SIMPLE_LRU_H

#include <map>
#include <memory>
#include <mutex>
#include <string>

#include <afina/Storage.h>

namespace Afina {
namespace Backend {

/**
 * # Map based implementation
 * That is NOT thread safe implementaiton!!
 */
class SimpleLRU : public Afina::Storage {
public:
    SimpleLRU(size_t max_size = 1024) : _max_size(max_size) {}

    ~SimpleLRU() {
        _lru_index.clear();
        // TODO: Here is stack overflow
        while (_lru_head) {
            auto temp = move(_lru_head->next);
            _lru_head = move(temp);
        }
    }

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


private:
    // LRU cache node
    using lru_node = struct lru_node {
        const std::string key;
        std::string value;
        lru_node *prev;
        std::unique_ptr<lru_node> next;
    };

    using hash_map = std::map<const std::reference_wrapper<const std::string>, std::reference_wrapper<lru_node>, std::less<std::string>>;    

    // Maximum number of bytes could be stored in this cache.
    // i.e all (keys+values) must be not greater than the _max_size
    std::size_t _max_size;
    std::size_t _cur_size = 0;

    // Main storage of lru_nodes, elements in this list ordered descending by "freshness": in the head
    // element that wasn't used for longest time.
    //
    // List owns all nodes
    std::unique_ptr<lru_node> _lru_head;
    lru_node *_lru_tail = nullptr;

    // Index of nodes from list above, allows fast random access to elements by lru_node#key
    hash_map _lru_index;

private:
    std::unique_ptr<lru_node> ExtractHead(hash_map::const_iterator it);
    std::unique_ptr<lru_node> ExtractTail(hash_map::const_iterator it);
    std::unique_ptr<lru_node> ExtractNode(hash_map::const_iterator it);
    void MoveToHead(hash_map::const_iterator it);
    
    lru_node* AddToHash(const std::string &key, const std::string &value);
    bool AddToEmptyCash(const std::string &key, const std::string &value);
    bool AddAnotherElem(const std::string &key, const std::string &value);
    bool InsertNewNode(const std::string &key, const std::string &value);
    bool IsTooBigForCash(size_t key_size, size_t value_size);
    void FreeSpace(int delta_space);
    void UpdateValue(hash_map::const_iterator it, const std::string &value);
};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_SIMPLE_LRU_H
