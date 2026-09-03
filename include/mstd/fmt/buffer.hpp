#pragma once 

namespace mstd {
    template<size_t capacity = 4096>
    class backward_buffer {
        char buf[capacity];
        char* iter;
    public:
        backward_buffer(): iter(buf + capacity - 1){}
        bool putc(char c) {
            if(iter < buf) {
                return false;
            }
            *(iter--) = c;
            return true;
        }
        size_t write(const char* str, size_t n) {
            size_t written = 0;
            while(written < n && putc(*(str++)))
                written++;
            return written;
        }

        char* begin() { return &buf[capacity - 1]; }
        char* end() { return iter; }
        char* rbegin() { return iter + 1; }
        char* rend() { return &buf[capacity]; }
        
        const char* begin() const { return &buf[capacity - 1]; }
        const char* end() const{ return iter; }
        const char* rbegin() const { return iter + 1; }
        const char* rend() const { return &buf[capacity]; }

        size_t len() const {
            return static_cast<size_t>(rend() - rbegin());
        }
    };
}