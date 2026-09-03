#pragma once 

namespace mstd {
    template<size_t capacity = 1024>
    class backward_buffer {
        char buf[capacity];
        char* iter;
    public:
        constexpr backward_buffer(): iter(buf + capacity - 1){}
        constexpr bool putc(char c) {
            if(iter < buf) {
                return false;
            }
            *(iter--) = c;
            return true;
        }
        constexpr size_t write(const char* str, size_t n) {
            size_t written = 0;
            while(written < n && putc(*(str++)))
                written++;
            return written;
        }

        constexpr char* begin() { return &buf[capacity - 1]; }
        constexpr char* end() { return iter; }
        constexpr char* rbegin() { return iter + 1; }
        constexpr char* rend() { return &buf[capacity]; }
        
        constexpr const char* begin() const { return &buf[capacity - 1]; }
        constexpr const char* end() const{ return iter; }
        constexpr const char* rbegin() const { return iter + 1; }
        constexpr const char* rend() const { return &buf[capacity]; }

        constexpr size_t len() const {
            return static_cast<size_t>(rend() - rbegin());
        }
    };
}