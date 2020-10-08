//
// Created by dcnick3 on 6/26/20.
//

#ifndef UWIN_EXECUTABLE_MEMORY_ALLOCATOR_H
#define UWIN_EXECUTABLE_MEMORY_ALLOCATOR_H

#include <cstdlib>

namespace uwin {
    namespace jit {
        namespace xmem {
            // both operate on rw pointers
            // to get rx pointer - add executable offset
            void *alloc(size_t size);

            void free(void *ptr);

            void free_unused_regions();

            std::ptrdiff_t executable_offset(void *ptr);
        }

        class xmem_piece {
            void *rw_pointer;
            void *rx_pointer;
            size_t m_size;
        public:
            xmem_piece(size_t size)
                : rw_pointer(xmem::alloc(size)), rx_pointer(rw_ptr() + xmem::executable_offset(rw_pointer)), m_size(size) {
            }

            xmem_piece(const xmem_piece& other) = delete;
            void operator=(const xmem_piece& other) = delete;

            xmem_piece(xmem_piece&& other) {
                rw_pointer = other.rw_pointer;
                rx_pointer = other.rx_pointer;
                m_size = other.m_size;

                other.rw_pointer = nullptr;
            }

            ~xmem_piece() {
                if (rw_pointer != nullptr) {
                    xmem::free(rw_pointer);
                }
            }

            inline uint8_t* rw_ptr() {
                return static_cast<uint8_t*>(rw_pointer);
            }

            inline void* rx_ptr() {
                return rx_pointer;
            }

            inline size_t size() {
                return this->m_size;
            }

            inline void prepare_execute() {
#ifdef __x86_64__
                // nothing needed, x86 is happy with it as is =)
#else
#ifdef __aarch64__
                // TODO: write the code for the horizon OS

#ifdef __linux__
                // flush the performed write
                auto start_ptr = reinterpret_cast<char*>(rw_ptr());
                __clear_cache(start_ptr, start_ptr + m_size);

                // flush the instruction cache
                start_ptr = reinterpret_cast<char*>(rx_ptr());
                __clear_cache(start_ptr, start_ptr + m_size);
#else
#error "Don't know what to do on this OS"
#endif
#else
#error "don't know how to prepare for execution on this platform"
#endif
#endif
            }
        };
    }
}

#endif //UWIN_EXECUTABLE_MEMORY_ALLOCATOR_H
