/**
 * implement a container like std::linked_hashmap
 */
#ifndef SJTU_LINKEDHASHMAP_HPP
#define SJTU_LINKEDHASHMAP_HPP

// only for std::equal_to<T> and std::hash<T>
#include <functional>
#include <cstddef>
#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {
    /**
     * In linked_hashmap, iteration ordering is differ from map,
     * which is the order in which keys were inserted into the map.
     * You should maintain a doubly-linked list running through all
     * of its entries to keep the correct iteration order.
     *
     * Note that insertion order is not affected if a key is re-inserted
     * into the map.
     */

template<
    class Key,
    class T,
    class Hash = std::hash<Key>,
    class Equal = std::equal_to<Key>
> class linked_hashmap {
public:
    typedef pair<const Key, T> value_type;

private:
    struct Node {
        value_type kv;
        Node *prev;
        Node *next;
        Node *hnext;
        Node(const value_type &v) : kv(v), prev(nullptr), next(nullptr), hnext(nullptr) {}
    };

    size_t _size;
    size_t _cap;
    Node **_buckets;
    Node *_head;
    Node *_tail;
    Hash _hash;
    Equal _eq;

    static size_t _initial_cap() { return 16; }
    size_t _bucket_index(const Key &key) const { return _hash(key) % _cap; }

    void _init_buckets(size_t cap) {
        _cap = cap;
        _buckets = new Node*[cap];
        for (size_t i = 0; i < cap; ++i) _buckets[i] = nullptr;
    }

    void _link_back(Node *n) {
        if (_tail) {
            _tail->next = n;
            n->prev = _tail;
            _tail = n;
        } else {
            _head = _tail = n;
        }
    }

    void _unlink(Node *n) {
        if (n->prev) n->prev->next = n->next; else _head = n->next;
        if (n->next) n->next->prev = n->prev; else _tail = n->prev;
    }

    void _insert_bucket(Node *n) {
        size_t idx = _bucket_index(n->kv.first);
        n->hnext = _buckets[idx];
        _buckets[idx] = n;
    }

    Node* _find_node(const Key &key) const {
        size_t idx = _bucket_index(key);
        Node *p = _buckets[idx];
        while (p) {
            if (_eq(p->kv.first, key)) return p;
            p = p->hnext;
        }
        return nullptr;
    }

    void _erase_bucket(Node *n) {
        size_t idx = _bucket_index(n->kv.first);
        Node *p = _buckets[idx];
        Node *prev = nullptr;
        while (p) {
            if (p == n) {
                if (prev) prev->hnext = p->hnext; else _buckets[idx] = p->hnext;
                return;
            }
            prev = p; p = p->hnext;
        }
    }

    void _rehash(size_t new_cap) {
        Node **new_buckets = new Node*[new_cap];
        for (size_t i = 0; i < new_cap; ++i) new_buckets[i] = nullptr;
        Node *p = _head;
        while (p) {
            size_t idx = _hash(p->kv.first) % new_cap;
            p->hnext = new_buckets[idx];
            new_buckets[idx] = p;
            p = p->next;
        }
        delete[] _buckets;
        _buckets = new_buckets;
        _cap = new_cap;
    }

    void _maybe_rehash_on_insert() {
        if ((_size + 1) * 4 >= _cap * 3) {
            _rehash(_cap ? _cap * 2 : _initial_cap());
        }
    }

public:
    class const_iterator;
    class iterator {
    private:
        linked_hashmap *owner;
        Node *node;
        friend class linked_hashmap;
        friend class const_iterator;
        iterator(linked_hashmap *o, Node *n) : owner(o), node(n) {}
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = typename linked_hashmap::value_type;
        using pointer = value_type*;
        using reference = value_type&;
        using iterator_category = std::output_iterator_tag;

        iterator() : owner(nullptr), node(nullptr) {}
        iterator(const iterator &other) : owner(other.owner), node(other.node) {}

        iterator operator++(int) {
            iterator tmp(*this);
            if (!owner || node == nullptr) throw invalid_iterator();
            node = node->next;
            return tmp;
        }
        iterator & operator++() {
            if (!owner || node == nullptr) throw invalid_iterator();
            node = node->next;
            return *this;
        }
        iterator operator--(int) {
            iterator tmp(*this);
            if (!owner) throw invalid_iterator();
            if (node == nullptr) {
                if (owner->_tail == nullptr) throw invalid_iterator();
                node = owner->_tail;
                return tmp;
            }
            if (node->prev == nullptr) throw invalid_iterator();
            node = node->prev;
            return tmp;
        }
        iterator & operator--() {
            if (!owner) throw invalid_iterator();
            if (node == nullptr) {
                if (owner->_tail == nullptr) throw invalid_iterator();
                node = owner->_tail;
                return *this;
            }
            if (node->prev == nullptr) throw invalid_iterator();
            node = node->prev;
            return *this;
        }
        value_type & operator*() const {
            if (!owner || node == nullptr) throw invalid_iterator();
            return node->kv;
        }
        bool operator==(const iterator &rhs) const { return owner == rhs.owner && node == rhs.node; }
        bool operator==(const const_iterator &rhs) const { return owner == rhs.owner && node == rhs.node; }
        bool operator!=(const iterator &rhs) const { return !(*this == rhs); }
        bool operator!=(const const_iterator &rhs) const { return !(*this == rhs); }
        value_type* operator->() const noexcept { return &node->kv; }
    };

    class const_iterator {
    private:
        const linked_hashmap *owner;
        const Node *node;
        friend class linked_hashmap;
        friend class iterator;
        const_iterator(const linked_hashmap *o, const Node *n) : owner(o), node(n) {}
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = typename linked_hashmap::value_type;
        using pointer = const value_type*;
        using reference = const value_type&;
        using iterator_category = std::output_iterator_tag;

        const_iterator() : owner(nullptr), node(nullptr) {}
        const_iterator(const const_iterator &other) : owner(other.owner), node(other.node) {}
        const_iterator(const iterator &other) : owner(other.owner), node(other.node) {}

        const_iterator operator++(int) {
            const_iterator tmp(*this);
            if (!owner || node == nullptr) throw invalid_iterator();
            node = node->next;
            return tmp;
        }
        const_iterator & operator++() {
            if (!owner || node == nullptr) throw invalid_iterator();
            node = node->next;
            return *this;
        }
        const_iterator operator--(int) {
            const_iterator tmp(*this);
            if (!owner) throw invalid_iterator();
            if (node == nullptr) {
                if (owner->_tail == nullptr) throw invalid_iterator();
                node = owner->_tail;
                return tmp;
            }
            if (node->prev == nullptr) throw invalid_iterator();
            node = node->prev;
            return tmp;
        }
        const_iterator & operator--() {
            if (!owner) throw invalid_iterator();
            if (node == nullptr) {
                if (owner->_tail == nullptr) throw invalid_iterator();
                node = owner->_tail;
                return *this;
            }
            if (node->prev == nullptr) throw invalid_iterator();
            node = node->prev;
            return *this;
        }
        const value_type & operator*() const {
            if (!owner || node == nullptr) throw invalid_iterator();
            return node->kv;
        }
        const value_type* operator->() const noexcept { return &node->kv; }
        bool operator==(const const_iterator &rhs) const { return owner == rhs.owner && node == rhs.node; }
        bool operator==(const iterator &rhs) const { return owner == rhs.owner && node == rhs.node; }
        bool operator!=(const const_iterator &rhs) const { return !(*this == rhs); }
        bool operator!=(const iterator &rhs) const { return !(*this == rhs); }
    };

    linked_hashmap() : _size(0), _cap(_initial_cap()), _buckets(nullptr), _head(nullptr), _tail(nullptr), _hash(Hash()), _eq(Equal()) {
        _init_buckets(_cap);
    }
    linked_hashmap(const linked_hashmap &other) : _size(0), _cap(_initial_cap()), _buckets(nullptr), _head(nullptr), _tail(nullptr), _hash(other._hash), _eq(other._eq) {
        _init_buckets(_cap);
        if (other._size > 0) {
            size_t desired = _initial_cap();
            while (desired * 3 / 4 < other._size) desired <<= 1;
            _rehash(desired);
        }
        const Node *p = other._head;
        while (p) { insert(p->kv); p = p->next; }
    }

    linked_hashmap & operator=(const linked_hashmap &other) {
        if (this == &other) return *this;
        clear();
        _hash = other._hash; _eq = other._eq;
        if (other._size > 0) {
            size_t desired = _initial_cap();
            while (desired * 3 / 4 < other._size) desired <<= 1;
            _rehash(desired);
        }
        const Node *p = other._head;
        while (p) { insert(p->kv); p = p->next; }
        return *this;
    }

    ~linked_hashmap() { clear(); delete[] _buckets; }

    T & at(const Key &key) {
        Node *n = _find_node(key);
        if (!n) throw index_out_of_bound();
        return n->kv.second;
    }
    const T & at(const Key &key) const {
        Node *n = _find_node(key);
        if (!n) throw index_out_of_bound();
        return n->kv.second;
    }

    T & operator[](const Key &key) {
        Node *n = _find_node(key);
        if (n) return n->kv.second;
        _maybe_rehash_on_insert();
        value_type v(key, T());
        Node *nn = new Node(v);
        _link_back(nn);
        _insert_bucket(nn);
        ++_size;
        return nn->kv.second;
    }

    const T & operator[](const Key &key) const {
        Node *n = _find_node(key);
        if (!n) throw index_out_of_bound();
        return n->kv.second;
    }

    iterator begin() { return iterator(this, _head); }
    const_iterator cbegin() const { return const_iterator(this, _head); }

    iterator end() { return iterator(this, nullptr); }
    const_iterator cend() const { return const_iterator(this, nullptr); }

    bool empty() const { return _size == 0; }

    size_t size() const { return _size; }

    void clear() {
        Node *p = _head;
        while (p) { Node *nxt = p->next; delete p; p = nxt; }
        _head = _tail = nullptr;
        _size = 0;
        if (_buckets) {
            for (size_t i = 0; i < _cap; ++i) _buckets[i] = nullptr;
        }
    }

    pair<iterator, bool> insert(const value_type &value) {
        Node *exist = _find_node(value.first);
        if (exist) return pair<iterator, bool>(iterator(this, exist), false);
        _maybe_rehash_on_insert();
        Node *n = new Node(value);
        _link_back(n);
        _insert_bucket(n);
        ++_size;
        return pair<iterator, bool>(iterator(this, n), true);
    }

    void erase(iterator pos) {
        if (pos.owner != this || pos.node == nullptr) throw invalid_iterator();
        Node *n = pos.node;
        _erase_bucket(n);
        _unlink(n);
        delete n;
        --_size;
    }

    size_t count(const Key &key) const { return _find_node(key) ? 1 : 0; }

    iterator find(const Key &key) { return iterator(this, _find_node(key)); }
    const_iterator find(const Key &key) const { return const_iterator(this, _find_node(key)); }
};

}

#endif
