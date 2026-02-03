#pragma once
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <new>
#include <iostream>

template<typename T, std::size_t ChunkSize = 64>
class MemoryPool {
public:
    using FreeBlock = struct FreeBlock { FreeBlock* next; };
    // 块大小等于目标大小
    static constexpr std::size_t BLOCK_SIZE = sizeof(T);
    // 对齐要求按照目标类型进行对齐
    static constexpr std::size_t BLOCK_ALOGN = alignof(T);
    static_assert(BLOCK_SIZE >= sizeof(FreeBlock), 
        "Type size must be at least the size of a pointer (FreeBlock)");
    static_assert(BLOCK_ALOGN >= alignof(FreeBlock), 
        "Type alignment must be at least the alignment of a pointer");

    // 构造函数
    MemoryPool() : _free_list(nullptr) {
        // 初始化时申请第一次内存页
        allocate_chunk();
    }

    // 析构函数
    ~MemoryPool() {
        for (auto* chunk = _free_list; chunk != nullptr; ) {
            auto* next_chunk = reinterpret_cast<FreeBlock*>(chunk)->next;
            // 按照对齐要求进行内存释放
            operator delete(chunk, std::align_val_t{BLOCK_ALOGN});
            chunk = next_chunk;
        }
    }

    // 禁用拷贝 (内存池为资源管理类, 拷贝会导致双重释放)
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    // 启用移动
    MemoryPool(MemoryPool&& other) noexcept
        : _free_list(other._free_list), _chunk_list(other._chunk_list) {
    
        other._free_list = nullptr;
        other._chunk_list = nullptr;
    }
    MemoryPool& operator=(MemoryPool&& other) noexcept {
        if (this != &other) {
            // 释放当前内存
            this->~MemoryPool();
            _free_list = other._free_list;
            _chunk_list = other._chunk_list;
            other._free_list = nullptr;
            other._chunk_list = nullptr;
        }
        return *this;
    }

    // 分配内存块
    T* allocate() {
        if (_free_list == nullptr) {
            allocate_chunk();
        }

        // 从空闲链表头取块(O(1)操作)
        FreeBlock* block = _free_list;
        _free_list = block->next;

        // 强制转为T*, 保证类型安全和对齐
        return reinterpret_cast<T*>(block);
    }

    // 释放内存块
    void deallocate(T* ptr) {
        if (ptr == nullptr) {
            return;
        }

        // 将释放的块转为FreeBlock, 插回链表头
        FreeBlock* block = reinterpret_cast<FreeBlock*>(ptr);
        block->next = _free_list;
        _free_list = block;
    }

    // 原地构造对象: 结合内存池分配+构造
    template<typename... Args>
    T* construct(Args&&... args) {
        T* ptr = allocate();
        // 定位new: 在已分配的内存上构造对象
        new (ptr) T(std::forward<Args>(args)...);
        return ptr;
    }

    // 原地析构对象: 析构+内存池释放
    void destory(T* ptr) {
        if (ptr == nullptr) {
            return;
        }

        // 显示析构
        ptr->~T();
        deallocate(ptr);
    }

private:
    // 分配新的内存页: 一次性申请ChunkSize个块的连续内存, 初始化空闲链表
    void allocate_chunk() {
        const std::size_t total_size = ChunkSize * BLOCK_SIZE;

        std::cout << "total_size: " << total_size << ", block_size: " << BLOCK_SIZE << std::endl;
        void* raw_memory = operator new(total_size, std::align_val_t(BLOCK_ALOGN));
        if (raw_memory == nullptr) {
            // 内存分配失败, 抛出标准异常
            throw std::bad_alloc();
        }

        // 将新内存页加入页链表
        reinterpret_cast<FreeBlock*>(raw_memory)->next = reinterpret_cast<FreeBlock*>(_chunk_list);
        _chunk_list = raw_memory;

        // 初始化当前页的空闲链表: 将连续内存切分为ChunkSize个块,串联成链表
        FreeBlock* current = reinterpret_cast<FreeBlock*>(raw_memory);
        for (std::size_t i = 0; i < ChunkSize - 1; ++i) {
            current->next = reinterpret_cast<FreeBlock*>(
                reinterpret_cast<uint8_t*>(current) + BLOCK_SIZE
            );
            current = current->next;
        }
        current->next = nullptr;

        _free_list = reinterpret_cast<FreeBlock*>(raw_memory);
    }

private:
    // 空闲链表头指针
    FreeBlock* _free_list;
    // 内存页链表
    void* _chunk_list = nullptr;
};
