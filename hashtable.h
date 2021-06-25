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
 * O(1) average time complexity is achived by maintaining invariant that:
 * kMinLoadFactor < # of buckets in hash table / # of elements in hash map < 1/kMaxLoadFactor
 * More precisely, invariant above holds only if # of elements in hash map >= kMinLoad;
 * otherwise we store kMinLoad buckets (kMinLoad > 0).
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

    using DataIterator = typename std::vector<KeyValuePair>::iterator;
    using DataConstIterator = typename std::vector<KeyValuePair>::const_iterator;


  public:
    // In both const and regular iterator we store iterator in array with key-value pairs.
    // Complexity: O(1) guaranteed for each in-class operation.
    class iterator {
        friend HashMap;
      public:
        iterator() = default;

        iterator& operator++() {
            ++position_;
            return *this;
        }

        iterator operator++(int) {
            DataIterator old_position = position_;
            ++position_;
            return iterator(old_position);
        }

        bool operator==(const iterator& other) const {
            return position_ == other.position_;
        }

        bool operator!=(const iterator& other) const {
            return position_ != other.position_;
        }

        KeyValuePair& operator*() {
            return *position_;
        }

        KeyValuePairConstKey *operator->() {
            return reinterpret_cast<KeyValuePairConstKey*>(&(*position_));
        }

      private:
        iterator(DataIterator position_) : position_(position_) {}

      private:
        DataIterator position_;
    };

    // O(1) guaranteed.
    iterator begin() {
        return iterator(data_.begin());
    }

    // Complexity: O(1) guaranteed.
    iterator end() {
        return iterator(data_.end());
    }

    // Complexity: O(1) average case.
    iterator find(const KeyType& key) {
        return FindByTableBucket(GetTableBucket(key), key);
    }

    // Code bellow is practically the same as code for regular iterator.
    // Complexity: O(1) guaranteed for  each in-class operation.
    class const_iterator {
        friend HashMap;
      public:
        const_iterator() = default;

        const_iterator& operator++() {
            ++position_;
            return *this;
        }

        const_iterator operator++(int) {
            DataConstIterator old_position = position_;
            ++position_;
            return const_iterator(old_position);
        }

        bool operator==(const const_iterator& other) const {
            return position_ == other.position_;
        }

        bool operator!=(const const_iterator& other) const {
            return position_ != other.position_;
        }

        const KeyValuePair& operator*() {
            return *position_;
        }

        const KeyValuePair *operator->() {
            return (&(*position_));
        }

      private:
        const_iterator(const DataConstIterator position_) : position_(position_) {}

      private:
        DataConstIterator position_;
    };

    // Complexity: O(1) guaranteed.
    const_iterator begin() const {
        return const_iterator(data_.cbegin());
    }

    // Complexity: O(1) guaranteed.
    const_iterator end() const {
        return const_iterator(data_.cend());
    }

    // Complexity: O(1) average case.
    const_iterator find(const KeyType& key) const {
        return FindByTableBucket(GetTableBucket(key), key);
    }

    // Complexity: O(1) guaranteed.
    HashMap(const Hash& hasher_ = Hash()) : hasher_(hasher_) {
        RehashIfNecessary();
    }

    // Complexity: O(end - begin) guaranteed, where end - begin = # of elements within range.
    template<class Iter>
    HashMap(Iter begin, Iter end, const Hash& hasher_ = Hash()): hasher_(hasher_) {
        for (Iter cur = begin; cur != end; ++cur) {
            data_.push_back(*cur);
        }
        RehashIfNecessary();
    }

    // Complexity: O(# of elements in initializer_list) guaranteed.
    HashMap(const std::initializer_list<KeyValuePair>& init_list,
            const Hash& hasher_ = Hash()) :
            hasher_(hasher_) {
        for (const KeyValuePair& element : init_list) {
            data_.push_back(element);
        }
        RehashIfNecessary();
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
    void insert(const KeyValuePair& element) {
        size_t table_element_bucket = GetTableBucket(element.first);
        iterator element_iterator = FindByTableBucket(table_element_bucket, element.first);
        if (element_iterator != end()) {
            return;
        }
        hash_table_[table_element_bucket].push_back(data_.size());
        data_.push_back(element);
        RehashIfNecessary();
    }

    // Complexity: O(1) average case.
    // Algorithm:
    // 1) Swap element with the last element in the storage array.
    // 2) Remove last element from the array.
    // 3) Update hash table according to changes made with the storage array.
    void erase(const KeyType& key) {
        size_t key_bucket = GetTableBucket(key);
        iterator key_iterator = FindByTableBucket(key_bucket, key);
        if (key_iterator == end()) {
            return;
        }
        size_t key_data_position = GetDataPosition(key_iterator);
        auto bucket_key_position = std::find(hash_table_[key_bucket].begin(),
                                             hash_table_[key_bucket].end(), key_data_position);
        hash_table_[key_bucket].erase(bucket_key_position);
        size_t last_element_bucket = GetTableBucket(data_.back().first);

        std::swap(data_[key_data_position], data_.back());
        data_.pop_back();

        auto last_element_bucket_position = std::find(hash_table_[last_element_bucket].begin(),
                                                      hash_table_[last_element_bucket].end(),
                                                      data_.size());
        *last_element_bucket_position = key_data_position;

        RehashIfNecessary();
    }

    // Return element of hash map with Key == key if it exists.
    // Otherwise create element with Key = key and Value set with default value of Valuetype.
    // Complexity: O(1) average case.
    ValueType& operator[](const KeyType& key) {
        size_t table_key_bucket = GetTableBucket(key);
        iterator key_iterator = FindByTableBucket(table_key_bucket, key);
        if (key_iterator != end()) {
            return key_iterator->second;
        }
        hash_table_[table_key_bucket].push_back(data_.size());
        size_t last_element_index = data_.size();
        data_.push_back({key, ValueType()});
        key_iterator = iterator(data_.begin() + last_element_index);

        if (RehashIfNecessary()) {
            return find(key)->second;
        }
        return key_iterator->second;
    }

    // Complexity: O(1) average.
    const ValueType& at(const KeyType& key) const {
        const_iterator key_iterator = find(key);
        if (key_iterator != end()) {
            return key_iterator->second;
        }
        throw std::out_of_range("Element not in HashTable.");
    }

  private:
    // Returns index in data_ array, that corresponds to the given iterator.
    // Complexity: O(1) guaranteed.
    size_t GetDataPosition(const iterator& it){
        return it.position_ - begin().position_;
    }

    // Calculates position of bucket of hash table, where element with key = Key belongs.
    // Complexity: O(1) guaranteed.
    size_t GetTableBucket(const KeyType& key) const {
        return hasher_(key) % hash_table_.size();
    }

    // Checks where resize of hash table is necessary and resizes it accordingly.
    // Returns true bool value whether hash table has been rehashed.
    // Complexity: O(|hash_table|)  guaranteed if we perform rehash;
    // otherwise O(1) guaranteed.
    // Also used for initialization.
    // Resize policy: we maintan invariant that:
    // kMinLoadFactor < # of buckets in hash table / # of elements in hash map < 1/kMaxLoadFactor.
    // More precisely, invariant above holds only if # of elements in hash map >= kMinLoad;
    // otherwise we store kMinLoad buckets (KMinLoad > 0).
    bool RehashIfNecessary() {
        if (hash_table_.empty() && data_.empty()) {
            hash_table_.resize(kMinLoad);
            return true;
        }
        if (hash_table_.size() * kMaxLoadFactor < data_.size() ||
            data_.size() * kMinLoadFactor < hash_table_.size()) {

            size_t new_size = std::max(data_.size(), static_cast<size_t>(kMinLoad));
            if (hash_table_.size() == new_size) {
                return false;
            }

            hash_table_.clear();
            hash_table_.resize(new_size);

            for (size_t ind = 0; ind < data_.size(); ++ind) {
                size_t hash_table_position = GetTableBucket(data_[ind].first);
                hash_table_[hash_table_position].push_back(ind);
            }
            return true;
        }
        return false;
    }

    // Finds iterator that has key == Key when bucket where element with key = Key is given.
    // Complexity: O(1) average case.
    iterator FindByTableBucket(const size_t key_bucket, const KeyType& key) {
        for (size_t data_index : hash_table_[key_bucket]) {
            if (data_[data_index].first == key) {
                return iterator(data_.begin() + data_index);
            }
        }
        return end();
    }

    // Finds iterator that has key == Key when bucket where element with key = Key is given.
    // Complexity: O(1) average case.
    const_iterator FindByTableBucket(const size_t key_bucket, const KeyType& key) const {
        for (size_t data_index : hash_table_[key_bucket]) {
            if (data_[data_index].first == key) {
                return const_iterator(data_.cbegin() + data_index);
            }
        }
        return end();
    }

  private:
    std::vector<std::vector<size_t>> hash_table_;
    std::vector<KeyValuePair> data_;
    Hash hasher_;
};
