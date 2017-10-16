#include "../hashset.h"
#include "../hashmap.h"

hashset* hashset_new(size_t (*hash)(const void*)) {
    return (void*)hashmap_new(hash);
}

void hashset_destroy(hashset* hashset) {
    hashmap_destroy((void*)hashset);
}

size_t hashset_size(const hashset* hashset) {
    return hashmap_size((void*)hashset);
}

int hashset_contains(const hashset* hashset, const void* value, size_t size) {
    return hashmap_contains((void*)hashset, value, size, 0);
}

int hashset_reserve(hashset* hashset, size_t capacity, size_t size) {
    return hashmap_reserve((void*)hashset, capacity, size, 0);
}

int hashset_insert(hashset* hashset, const void* value, size_t size) {
    return hashmap_insert((void*)hashset, value, size, value, 0);
}

int hashset_erase(hashset* hashset, const void* value, size_t size) {
    return hashmap_erase((void*)hashset, value, size, 0);
}

typedef struct pair pair;
struct pair {
    void (*fun)(void* elem, void* userdata);
    void* userdata;
};

static void hashset_wrap_fun(void* key, void* value, void* _userdata) {
    pair* pair = _userdata;
    pair->fun(key, pair->userdata);
    (void)value;
}

void
hashset_iterate(hashset* hashset, size_t size,
                void (*fun)(void* elem, void* userdata),
                void* userdata) {
    pair pair = {fun, userdata};
    hashmap_iterate((void*)hashset, size, 0, hashset_wrap_fun, &pair);
}

hashset_iterator
hashset_iterator_new(hashset* hashset) {
    hashset_iterator si;
    hashmap_iterator mi = hashmap_iterator_new((void*)hashset);
    si._hashset = (void*)mi._hashmap;
    si._outer = mi._outer;
    si._inner = mi._inner;
    return si;
}

void*
hashset_iterator_next(hashset_iterator* iterator, size_t size) {
    return hashmap_iterator_next((void*)iterator, size, 0).key;
}

void*
hashset_iterator_peek(const hashset_iterator* iterator, size_t size) {
    return hashmap_iterator_peek((void*)iterator, size, 0).key;
}
