#include <algorithm>
#include <initializer_list>
#include <stdexcept>
#include <vector>

/*
 * Implementation of hash map using seperate chaining with dynamic arrays (vectors) and linear probing.
 * Iteration over elements of hash map is linear as we store all elements in a separate array
 * over which we can simply iterate.
 * In hash_table we store only indexes of elements of this array.
 * More information can be found at https://en.wikipedia.org/wiki/Hash_table.
 * O(1) average time complexity is achived by maintaining the invariant that:
 * kMinLoadFactor < # of buckets in hash table / # of elements in hash map < 1/kMaxLoadFactor
 * More precisely, invariant above holds only if # of elements in hash map >= kMinLoad;
 * otherwise we store kMinLoad buckets.
 * To achieve this we resize our hash table each time this invariant breaks.
 */
template<class KeyType, class ValueType, class Hash = std::hash<KeyType>>
class HashMap {
public:
    constexpr static size_t kMinLoad = 3;
    constexpr static size_t kMinLoadFactor = 3;
    constexpr static size_t kMaxLoadFactor = 2;

    using KeyValuePair = typename std::pair<KeyType, ValueType>;
    using KeyValuePairConstKey = typename std::pair<const KeyType, ValueType>;
public:
    // In both const and regular iterator we store position in array with key-value pairs.
    // Complexity: O(1) guaranteed for each in-class operation.
    class iterator {
        friend HashMap;
    public:
        iterator() = default;

        iterator& operator++() {
            ++data_position_;
            return *this;
        }

        iterator operator++(int) {
            size_t old_data_position = data_position_;
            ++data_position_;
            return iterator(hash_map_link_, old_data_position);
        }

        bool operator==(const iterator& other) const {
            return other.data_position_ == data_position_;
        }

        bool operator!=(const iterator& other) const {
            return other.data_position_ != data_position_;
        }

        KeyValuePair& operator*() {
            return *reinterpret_cast<KeyValuePair*>(&hash_map_link_->data_[data_position_]);
        }

        KeyValuePairConstKey* operator->() {
            return reinterpret_cast<KeyValuePairConstKey*>(&hash_map_link_->data_[data_position_]);
        }

    private:
        iterator(HashMap<KeyType, ValueType, Hash>* hash_map_link_, size_t data_position__) :
                hash_map_link_(hash_map_link_), data_position_(data_position__) {}

    private:
        HashMap<KeyType, ValueType, Hash>* hash_map_link_;
        size_t data_position_;
    };

    // O(1) guaranteed.
    iterator begin() {
        return iterator(this, 0);
    }

    // Complexity: O(1) guaranteed.
    iterator end() {
        return iterator(this, data_.size());
    }

    // Complexity: O(1) average case.
    iterator find(const KeyType& key) {
        return FindByHashTableBucket(GetHashTableBucket(key), key);
    }

    // Code bellow is practically the same as code for regular iterator.
    // Complexity: O(1) guaranteed for  each in-class operation.
    class const_iterator {
        friend HashMap;
    public:
        const_iterator() = default;

        bool operator==(const const_iterator& other) const {
            return other.data_position_ == data_position_;
        }

        bool operator!=(const const_iterator& other) const {
            return other.data_position_ != data_position_;
        }

        const KeyValuePair& operator*() {
            return hash_map_link_->data_[data_position_];
        }

        const KeyValuePair* operator->() {
            return &hash_map_link_->data_[data_position_];
        }

        const_iterator& operator++() {
            ++data_position_;
            return *this;
        }

        const_iterator operator++(int) {
            size_t old_data_position = data_position_;
            ++data_position_;
            return const_iterator(hash_map_link_, old_data_position);
        }

    private:
        const_iterator(const HashMap<KeyType, ValueType, Hash>* hash_map_link_,
                       const size_t data_position__) : hash_map_link_(hash_map_link_),
                                                       data_position_(data_position__) {}

    private:
        const HashMap<KeyType, ValueType, Hash>* hash_map_link_;
        size_t data_position_;
    };

public:

    // Complexity: O(1) guaranteed.
    const_iterator begin() const {
        return const_iterator(this, 0);
    }

    // Complexity: O(1) guaranteed.
    const_iterator end() const {
        return const_iterator(this, data_.size());
    }

    // Complexity: O(1) average.
    const_iterator find(const KeyType& key) const {
        return FindByHashTableBucket(GetHashTableBucket(key), key);
    }

    // Complexity: O(1) guaranteed.
    HashMap(const Hash& hasher_ = Hash()) : hasher_(hasher_) {
        RehashIfNecessary();
    }

    // Complexity: O(end - begin) guaranteed, where end - begin = # of elements within range.
    template<class Iter>
    HashMap(Iter begin, Iter end, const Hash& hasher_ = Hash()): hasher_(hasher_) {
        RehashIfNecessary();
        for (Iter cur = begin; cur != end; ++cur) {
            insert(*cur);
        }
    }

    // Complexity: O(# of elements in initializer_list) guaranteed.
    HashMap(const std::initializer_list<KeyValuePair>& init_list,
            const Hash& hasher_ = Hash()) :
            hasher_(hasher_) {
        RehashIfNecessary();
        for (const KeyValuePair& element : init_list) {
            insert(element);
        }
    }

    // Complexity: O(1) guaranteed.
    Hash hash_function() const {
        return hasher_;
    }

    // Complexity: O(1) guaranteed.
    size_t size() const {
        return data_.size();
    }

    // Complexity: O(1) guaranteed.
    bool empty() const {
        return data_.empty();
    }

    // Complexity: O(# of elements in hash map) guaranteed.
    void clear() {
        data_.clear();
        RehashIfNecessary();
    }

    // Complexity: O(1) average case.
    // Inserts new element and resizes hash table if this is neccesary.
    void insert(const KeyValuePair& elem) {
        size_t hash_table_key_position = GetHashTableBucket(elem.first);
        iterator key_iterator = FindByHashTableBucket(hash_table_key_position, elem.first);
        if (key_iterator != end()) {
            return;
        }
        hash_table_[hash_table_key_position].push_back(data_.size());
        data_.push_back(elem);
        RehashIfNecessary();
    }

    // Complexity: O(1) average case.
    // Algorithm:
    // 1) Swap element with the last element in the storage array.
    // 2) Remove last element from the array.
    // 3) Update hash table according to changes made with the storage array.
    void erase(const KeyType& key) {
        size_t hash_table_key_bucket = GetHashTableBucket(key);
        iterator key_iterator = FindByHashTableBucket(hash_table_key_bucket, key);
        if (key_iterator == end()) {
            return;
        }
        size_t key_data_position = key_iterator.data_position_;
        auto bucket_key_position = std::find(hash_table_[hash_table_key_bucket].begin(),
                                             hash_table_[hash_table_key_bucket].end(), key_data_position);
        hash_table_[hash_table_key_bucket].erase(bucket_key_position);
        size_t last_element_bucket = GetHashTableBucket(data_.back().first);

        std::swap(data_[key_data_position], data_.back());
        data_.pop_back();

        auto last_element_bucket_position = std::find(hash_table_[last_element_bucket].begin(),
                                                      hash_table_[last_element_bucket].end(),
                                                      data_.size());
        *last_element_bucket_position = key_data_position;

        RehashIfNecessary();
    }

    // Return element of hash map with Key == key if it exists.
    // Otherwise create element with Key = key and Value set with default value of Valuetype
    // Complexity: O(1) average case
    ValueType& operator[](const KeyType& key) {
        iterator key_iterator = find(key);

        if (key_iterator == end()) {
            insert(std::make_pair(key, ValueType()));
            key_iterator = find(key);
        }
        return key_iterator->second;
    }

    // Complexity: O(1) guaranteed.
    const ValueType& at(const KeyType& key) const {
        const_iterator key_iterator = find(key);
        if (key_iterator != end()) {
            return key_iterator->second;
        }
        throw std::out_of_range("Element not in HashTable.");
    }

private:
    // Calculates position of bucket of hash table, where element with key = Key belongs.
    size_t GetHashTableBucket(const KeyType& key) const {
        return hasher_(key) % hash_table_.size();
    }

    // Checks where resize of hash table is necessary and resizes it accordingly.
    // Complexity: O(1) average is achived by maintaining the invariant that:
    // kMinLoadFactor < # of buckets in hash table / # of elements in hash map < 1/kMaxLoadFactor
    // More precisely, invariant above holds only if # of elements in hash map >= kMinLoad;
    // otherwise we store kMinLoad buckets.
    // Also used for initialization.
    void RehashIfNecessary() {
        if (hash_table_.empty()) {
            hash_table_.resize(kMinLoad);
            return;
        }
        if (hash_table_.size() * kMaxLoadFactor < data_.size() ||
            data_.size() * kMinLoadFactor < hash_table_.size()) {

            size_t new_size = std::max(data_.size(), static_cast<size_t>(kMinLoad));
            if (hash_table_.size() == new_size) {
                return;
            }

            hash_table_.clear();
            hash_table_.resize(new_size);

            for (size_t ind = 0; ind < data_.size(); ++ind) {
                size_t hash_table_position = GetHashTableBucket(data_[ind].first);
                hash_table_[hash_table_position].push_back(ind);
            }
        }
    }

    // Finds iterator that has key == Key when bucket where element with key = Key is given.
    // Complexity: O(1) average case.
    iterator FindByHashTableBucket(const size_t hash_table_key_bucket, const KeyType& key) {
        for (size_t data_index : hash_table_[hash_table_key_bucket]) {
            if (data_[data_index].first == key) {
                return iterator(this, data_index);
            }
        }
        return end();
    }

    // Finds iterator that has key == Key when bucket where element with key = Key is given.
    // Complexity: O(1) average case.
    const_iterator FindByHashTableBucket(const size_t hash_table_key_bucket, const KeyType& key) const {
        for (size_t data_index : hash_table_[hash_table_key_bucket]) {
            if (data_[data_index].first == key) {
                return const_iterator(this, data_index);
            }
        }
        return end();
    }

private:
    std::vector<std::vector<size_t>> hash_table_;
    std::vector<KeyValuePair> data_;
    Hash hasher_;
};
