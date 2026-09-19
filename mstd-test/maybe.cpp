#include <cstdlib>
#include <cstring>
#include <mstd/monadic/maybe.hpp>
#include <iostream>
#include <cstdint>
#include <cstddef>

using namespace std;
using mstd::maybe;
using mstd::some;
using mstd::nothing;

template<class T, size_t size>
struct pool {
    alignas(T) union storage_t{
        T obj[size];
        char bytes[size * sizeof(T)];
        storage_t(){}
        ~storage_t(){}
    } storage;
    static constexpr auto bitmap_size = (size+7) / 8;
    uint8_t bitmap[bitmap_size];
    T* acquire_raw(){
        size_t index = 0;
        while(true){
            if(index >= bitmap_size) return nullptr;
            if(bitmap[index] != 0xff) {
                for(auto i = 0; i < 8; i++){
                    if(index * 8 + i >= size) return nullptr;
                    auto bit = (bitmap[index] >> i) & 0x01;
                    if(bit == 0) {
                        bitmap[index] |= 1 << i;
                        return &storage.obj[index * 8 + i];
                    }
                }
            }
            index++;
        }
    }
    void release_raw(T* ptr){
        auto index = reinterpret_cast<long int>(ptr - storage.obj) / sizeof(T); 
        auto byte_id = index / 8;
        auto bit_id = index % 8;
        bitmap[byte_id] &= ~(1 << bit_id);
    }
public:
    pool(): storage(){
        memset(bitmap, 0, bitmap_size);
    }
    template<class... Args>
    maybe<T&> acquire(Args&&... args){
        auto ptr = acquire_raw();
        if(ptr){
            new(ptr) T(forward<Args>(args)...);
            return some<T&>(*ptr);
        } else {
            return nothing;
        }
    }
    void release(T& t){
        t.~T();
        release_raw(&t);
    }
    ~pool(){
        for(size_t i = 0; i < size; i++){
            if(bitmap[i / 8]){
                auto byte_id = i / 8;
                auto bit_id = i % 8;
                if((bitmap[byte_id] >> bit_id) & 0x01)
                    storage.obj[i].~T();
            }
        }
    }
};

struct person {
    char name[32];
    int age;
public:
    person(const char* str, int age): age(age){
        strcpy(name, str);
    }
    void who() const {
        cout << "I am " << name << endl;
    }
    ~person(){
        cout << name << " is dead" << endl;
    }
};

int main(){
    pool<person, 64> people_pool;
    const char* names[] = {"Giovanni", "Greg", "Max", "Michael", "Muhammad", "Ray", "Zach", "Henry", "James"};
    int i = 0;
    auto maybe_person = people_pool.acquire(names[0], 10);
    auto& new_person = maybe_person.take();
    cout << "maybe<person&> size: " << sizeof(maybe_person) << endl;
    cout << "person size: " << sizeof(person) << endl;
    cout << "pool size: " << sizeof(people_pool) << endl;
    people_pool.release(new_person);
    for(int i = 1; i < 32; i++){
        auto new_person 
            = people_pool
                .acquire(names[i % 9], i)
                .apply(
                    [i](person& p){
                        cout << i << " acquired " << &p << endl;
                        p.who();
                    }
                );
        if(!new_person) break;
    }

    return 0;
}

namespace mstd {
    extern "C" [[noreturn]] void panic(const char* msg) {
        cout << "panic! " << msg << endl;
        exit(-1);
    }
};