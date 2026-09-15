#include "mstd/fmt.hpp"
namespace mstd {
    dyn_fmt_buffer::dyn_fmt_buffer(): vptr(nullptr), writer(nullptr){}
    dyn_fmt_buffer::dyn_fmt_buffer(const dyn_fmt_buffer& writer)
        : writer(writer.writer), vptr(writer.vptr){}

    __attribute__((always_inline)) 
    dyn_fmt_buffer& dyn_fmt_buffer::operator=(const dyn_fmt_buffer& writer){
        this->vptr = writer.vptr;
        this->writer = &writer.writer;
    }

    bool dyn_fmt_buffer::putc(char c){
        if(vptr == nullptr || writer == nullptr)
            return false;
        return vptr->putc(writer, c);
    }

    fmt_result dyn_fmt_buffer::write(const char* str, size_t len) {
        if(vptr == nullptr || writer == nullptr)
            return fmt_result{0, len};
        return vptr->write(writer, str, len);
    }

    bool dyn_fmt_buffer::unchecked_putc(char c){
        return vptr->putc(writer, c);
    }

    fmt_result dyn_fmt_buffer::unchecked_write(const char* str, size_t len) {
        return vptr->write(writer, str, len);
    }
}