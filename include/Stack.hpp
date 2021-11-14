#pragma once
#include <cstdint>
//TODO: should access happen using system addresses or just offset from the start?
namespace VWA
{
    //TODO: make this all c compatible
    class Stack
    {
        //TODO: geometric growth
        std::size_t capacity; //How many bytes can be stored on the stack
        uint8_t *data;
        uint64_t top = 0; //Points to the top of the stack The first free space
    public:
        Stack(std::size_t maxSize = 2000000) : capacity(maxSize), data(new uint8_t[maxSize]) {}
        ~Stack() { delete[] data; }
        //TODO: push functions for custom types (Create interface)
        uint64_t getTop() { return top; }
        uint8_t *getData() { return data; }
        //These functions are ugly, but they will have to do for now
        //TODO: over and underflow checks
        void push(std::size_t size) { top += size; }
        void pop(std::size_t size) { top -= size; }
        template <typename T>
        void pushVal(T val)
        {
            *(T*)(data+top) = val;
            top+=sizeof(T);
        }
        template <typename T>
        T popVal()
        {
            top -= sizeof(T);
            return *(T*)(data+top);
        }
        template <typename T>
        T readVal(uint64_t offset)
        {
            return *(T *)(data + offset);
        }
        template <typename T>
        void writeVal(uint64_t offset, T val)
        {
            *(T *)(data + offset) = val;
        }
    };
}