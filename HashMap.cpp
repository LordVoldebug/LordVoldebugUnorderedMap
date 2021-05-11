#include <algorithm>
#include <stdexcept>
#include <vector>

template<class KeyType, class ValueType, class Hash = std::hash<KeyType>>
class HashMap {
private:
    using hash_pair = typename std::pair<KeyType, ValueType>;

    std::vector<std::vector<size_t>> hash_table;
    std::vector<hash_pair> els;

    Hash hasher;

    void rehash() {
        if (hash_table.size() <= els.size() || els.size() * 4 + 1 < hash_table.size()) {
            hash_table.clear();
            hash_table.resize(els.size() * 2 + 1);
            for (size_t ind = 0; ind < els.size(); ++ind) {
                hash_table[hasher(els[ind].first) % hash_table.size()].push_back(ind);
            }
        }
    }

public:

    class iterator {
        friend HashMap;
    private:
        HashMap<KeyType, ValueType, Hash> *link;
        size_t pos;

        iterator(HashMap<KeyType, ValueType, Hash> *link_, size_t pos_) : link(link_), pos(pos_) {}

    public:
        iterator() {}

        iterator& operator++() {
            ++pos;
            return *this;
        }

        iterator operator++(int) {
            ++pos;
            return iterator(this->link, pos - 1);
        }

        bool operator==(const iterator& other) const {
            return other.pos == pos;
        }

        bool operator!=(const iterator& other) const {
            return other.pos != pos;
        }

        std::pair<const KeyType, ValueType>& operator*() {
            return *(std::pair<const KeyType, ValueType> *) (&link->els[pos]);
        }

        std::pair<const KeyType, ValueType> *operator->() {
            return (std::pair<const KeyType, ValueType> *) (&link->els[pos]);
        }
    };

    class const_iterator {
        friend HashMap;
    private:
        const HashMap<KeyType, ValueType, Hash> *link;
        size_t pos;

        const_iterator(const HashMap<KeyType, ValueType, Hash> *link_, const size_t pos_) : link(link_), pos(pos_) {}

    public:
        const_iterator() {}


        bool operator==(const const_iterator& other) const {
            return other.pos == pos;
        }

        bool operator!=(const const_iterator& other) const {
            return other.pos != pos;
        }

        const hash_pair& operator*() {
            return link->els[pos];
        }

        const hash_pair *operator->() {
            return &link->els[pos];
        }

        const_iterator& operator++() {
            ++pos;
            return *this;
        }

        const_iterator operator++(int) {
            ++pos;
            return const_iterator(this->link, pos - 1);
        }
    };

    HashMap(const Hash& hasher_ = Hash()) : hasher(hasher_) {
        rehash();
    }

    size_t size() const {
        return els.size();
    }

    bool empty() const {
        return els.size() == 0;
    }

    Hash hash_function() const {
        return hasher;
    }

    void clear() {
        els.clear();
        rehash();
    }

    iterator begin() {
        return iterator(this, 0);
    }

    iterator end() {
        return iterator(this, els.size());
    }

    const_iterator begin() const {
        return const_iterator(this, 0);
    }

    const_iterator end() const {
        return const_iterator(this, els.size());
    }

    iterator find(const KeyType& key) {
        size_t index = hasher(key) % hash_table.size();
        for (auto ind : hash_table[index]) {
            if (els[ind].first == key) {
                return iterator(this, ind);
            }
        }
        return end();
    }

    const_iterator find(const KeyType& key) const {
        size_t index = hasher(key) % hash_table.size();
        for (auto ind : hash_table[index]) {
            if (els[ind].first == key) {
                return const_iterator(this, ind);
            }
        }
        return end();
    }

    void insert(const hash_pair& elem) {
        auto it = find(elem.first);
        if (it != end()) {
            return;
        }
        size_t index = hasher(elem.first) % hash_table.size();
        hash_table[index].push_back(els.size());
        els.push_back(elem);
        rehash();
    }


    template<class Iter>
    HashMap(Iter begin, Iter end, const Hash& hasher_ = Hash()): hasher(hasher_) {
        rehash();
        for (Iter cur = begin; cur != end; ++cur) {
            insert(*cur);
        }
        rehash();
    }

    HashMap(const std::initializer_list<hash_pair>& list,
            const Hash& hasher_ = Hash()) : hasher(hasher_) {
        rehash();
        for (auto el : list) {
            insert(el);
        }
        rehash();
    }

    void erase(const KeyType& key) {
        auto it = find(key);
        if (it == end()) {
            return;
        }
        size_t id = it.pos;
        size_t index = hasher(key) % hash_table.size();
        hash_table[index].erase(std::find(hash_table[index].begin(), hash_table[index].end(), id));
        size_t hsh = hasher(els.back().first) % hash_table.size();
        for (auto& el : hash_table[hsh]) {
            if (el + 1 == els.size()) {
                el = id;
            }
        }
        std::swap(els[id], els.back());
        els.pop_back();
        rehash();
    }

    ValueType& operator[](const KeyType& key) {
        if (find(key) == end()) {
            insert(std::make_pair(key, ValueType()));
            rehash();
        }
        auto it = find(key);
        return els[it.pos].second;
    }

    const ValueType& at(const KeyType& key) const {
        auto it = find(key);
        if (it != end()) {
            return els[it.pos].second;
        }
        throw std::out_of_range("At Error");
    }
};
