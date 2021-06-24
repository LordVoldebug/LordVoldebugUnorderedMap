#include<iostream>
#include<vector>
#include<stdexcept>
#include<list>
#include<utility>
#include<memory>

using namespace std;


template<class KeyType, class ValueType, class Hash = std::hash<KeyType> > class HashMap {

private:

    size_t sz = 0, els = 0;

    vector<vector<size_t>> indexes;
    vector<pair<KeyType, ValueType>> hashmap;
    Hash hasher;

public:

    typedef pair<KeyType, ValueType>* pp;
    typedef const pair<KeyType, ValueType>* ppc;


    class iterator {
    public:
        pair<const KeyType, ValueType>* p;
        iterator() {};
        iterator(pp a) {
            p = reinterpret_cast<pair<const KeyType, ValueType>*>(a);
        };
        pair<const KeyType, ValueType> operator*() {return *p;};
        pair<const KeyType, ValueType>* operator->() {return p;};
        bool operator==(iterator other) const {return p == other.p;};
        bool operator!=(iterator other) const {return p != other.p;};
        iterator operator++() {++p; return *this;};
        iterator operator++(int) {iterator cp = *this; ++*this; return cp;}
    };

    class const_iterator {
    public:
        const pair<const KeyType, ValueType>* p;
        const_iterator() {};
        const_iterator(ppc a) {
            p = reinterpret_cast<const pair<const KeyType, ValueType>*>(a);
        }
        const pair<const KeyType, ValueType> operator*() const {return *p;};
        const pair<const KeyType, ValueType>* operator->() const {return p;};
        bool operator==(const_iterator other) const {return p == other.p;};
        bool operator!=(const_iterator other) const {return p != other.p;};
        const_iterator operator++() {++p; return *this;};
        const_iterator operator++(int) {const_iterator cp = *this; ++*this; return cp;}
    };

    HashMap(const Hash& hasher_ = Hash()): hasher(hasher_) {}

    template<class Iterator>
    HashMap(Iterator begin_, Iterator end_, const Hash& h = Hash()) {
        hasher = h;
        while (begin_ != end_) {
            insert(*begin_);
            ++begin_;
        };
    };

    HashMap(initializer_list<pair<KeyType, ValueType>> L, const Hash& h = Hash()) {
        hasher = h;
        for (auto i : L) insert(i);
    };

    iterator begin() {
        return iterator(&*hashmap.begin());
    }

    iterator end() {
        return iterator(&*hashmap.end());
    }

    const_iterator begin() const {
        return const_iterator(&*hashmap.begin());
    }

    const_iterator end() const {
        return const_iterator(&*hashmap.end());
    }

    Hash hash_function() const {
        return hasher;
    }

    iterator find(KeyType key) {

        if (sz == 0) return end();
        int pos = hasher(key) % sz;
        for (auto i : indexes[pos]) {
            if (hashmap[i].first == key) {
                return iterator(&*(hashmap.begin() + i));
            }
        }
        return end();
    }

    const_iterator find(KeyType key) const {
        if (sz == 0) return end();
        int pos = hasher(key) % sz;
        for (auto i : indexes[pos]) {
            if (hashmap[i].first == key) {
                return const_iterator(&*(hashmap.begin() + i));
            }
        }
        return end();
    }

    void clear() {
        sz = 1;
        els = 0;
        hashmap.clear();
        indexes.clear();
        indexes.resize(1);
    }

    void rebuild() {
        vector<pair<KeyType, ValueType>> tmp;
        tmp = std::move(hashmap);
        els = 0;
        hashmap.clear();
        indexes.clear();
        indexes.resize(sz);
        for (auto i : tmp) insert(i);
    }

    void insert(pair<KeyType, ValueType> x) {
        if (find(x.first) != end()) {
            return;
        }
        if (els + 1 > sz) sz = sz * 2 + 1, rebuild();
        ++els;
        int pos = hasher(x.first) % sz;
        indexes[pos].push_back(els - 1);
        hashmap.push_back(x);
    }

    void erase(KeyType key) {
        if (find(key) == end()) return;
        pair<KeyType, ValueType> last = hashmap.back();
        if (last.first == key) {
            hashmap.pop_back();
            int pos = hasher(last.first) % sz;
            --els;
            auto it = indexes[pos].begin();
            while (*it != els) ++it;
            indexes[pos].erase(it);
            return;
        } else {
            int pos_ = hasher(last.first) % sz;
            int pos = hasher(key) % sz;
            size_t good = 0;
            for (auto i : indexes[pos]) if (hashmap[i].first == key) good = i;
            --els;
            auto it = indexes[pos_].begin();
            while (*it != els) ++it;
            *it = good;
            it = indexes[pos].begin();
            while (*it != good) ++it;
            indexes[pos].erase(it);
            hashmap[good] = last;
            hashmap.pop_back();
        }
    }

    size_t size() const {
        return els;
    }

    bool empty() const {
        return size() == 0;
    }

    ValueType& operator[](KeyType key) {
        if (find(key) == end()) insert({key, ValueType()});
        return find(key)->second;
    }

    const ValueType& at(KeyType key) const {
        if (find(key) == end()) throw out_of_range("");
        return find(key)->second;
    }

};