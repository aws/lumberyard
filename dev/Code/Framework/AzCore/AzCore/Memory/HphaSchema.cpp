/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#ifndef AZ_UNITY_BUILD

#include <AzCore/PlatformIncl.h>
#include <AzCore/Memory/HphaSchema.h>

#include <limits.h>
//Added for memalign. Perhaps leverage OSAllocator instead of calling allocations directly?
#if defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID)
#include <malloc.h>
#elif defined(AZ_PLATFORM_APPLE)
#include <malloc/malloc.h>
#endif
#include <AzCore/Math/Sfmt.h>
#include <AzCore/Memory/OSAllocator.h> // required by certain platforms
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/containers/intrusive_set.h>

#ifdef _DEBUG
//#define DEBUG_ALLOCATOR
//#define DEBUG_PTR_IN_BUCKET_CHECK // enabled this when NOT sure if PTR in bucket marker check is successfully
#endif

#ifdef DEBUG_ALLOCATOR
#include <assert.h>
#include <AzCore/Debug/StackTracer.h>
#include <AzCore/Memory/MallocSchema.h>
#include <AzCore/std/containers/set.h>
#define HPPA_ASSERT(_exp)       assert(_exp)
#else
#define HPPA_ASSERT(_exp)       AZ_Assert(_exp, "HPHA Assert, expression: \"" #_exp "\"")
#endif

namespace AZ {

    /// default windows virtual page size \todo Read this from the OS when we create the allocator)
#define OS_VIRTUAL_PAGE_SIZE AZ_PAGE_SIZE
    //////////////////////////////////////////////////////////////////////////

#define MULTITHREADED

#ifdef MULTITHREADED
#   define  SPIN_COUNT 4000
#endif

    //////////////////////////////////////////////////////////////////////////
    // TODO: Replace with AZStd::intrusive_list
    class intrusive_list_base
    {
    public:
        class node_base
        {
            node_base* mPrev;
            node_base* mNext;
        public:
            node_base* next() const {return mNext; }
            node_base* prev() const {return mPrev; }
            void reset()
            {
                mPrev = this;
                mNext = this;
            }
            void unlink()
            {
                mNext->mPrev = mPrev;
                mPrev->mNext = mNext;
            }
            void link(node_base* node)
            {
                mPrev = node->mPrev;
                mNext = node;
                node->mPrev = this;
                mPrev->mNext = this;
            }
        };
        intrusive_list_base()
        {
            mHead.reset();
        }
        intrusive_list_base(const intrusive_list_base&)
        {
            mHead.reset();
        }
        bool empty() const {return mHead.next() == &mHead; }
        void swap(intrusive_list_base& other)
        {
            node_base* node = &other.mHead;
            if (!empty())
            {
                node = mHead.next();
                mHead.unlink();
                mHead.reset();
            }
            node_base* other_node = &mHead;
            if (!other.empty())
            {
                other_node = other.mHead.next();
                other.mHead.unlink();
                other.mHead.reset();
            }
            mHead.link(other_node);
            other.mHead.link(node);
        }
    protected:
        node_base mHead;
    };

    //////////////////////////////////////////////////////////////////////////
    // TODO: Replace with AZStd::intrusive_list
    template<class T>
    class intrusive_list
        : public intrusive_list_base
    {
        intrusive_list(const intrusive_list& rhs);
        intrusive_list& operator=(const intrusive_list& rhs);
    public:
        class node
            : public node_base
        {
        public:
            T* next() const {return static_cast<T*>(node_base::next()); }
            T* prev() const {return static_cast<T*>(node_base::prev()); }
            const T& data() const {return *static_cast<const T*>(this); }
            T& data() {return *static_cast<T*>(this); }
        };

        class const_iterator;
        class iterator
        {
            typedef T& reference;
            typedef T* pointer;
            friend class const_iterator;
            T* mPtr;
        public:
            iterator()
                : mPtr(0) {}
            explicit iterator(T* ptr)
                : mPtr(ptr) {}
            reference operator*() const {return mPtr->data(); }
            pointer operator->() const {return &mPtr->data(); }
            operator pointer() const {
                return &mPtr->data();
            }
            iterator& operator++()
            {
                mPtr = mPtr->next();
                return *this;
            }
            iterator& operator--()
            {
                mPtr = mPtr->prev();
                return *this;
            }
            bool operator==(const iterator& rhs) const {return mPtr == rhs.mPtr; }
            bool operator!=(const iterator& rhs) const {return mPtr != rhs.mPtr; }
            T* ptr() const {return mPtr; }
        };

        class const_iterator
        {
            typedef const T& reference;
            typedef const T* pointer;
            const T* mPtr;
        public:
            const_iterator()
                : mPtr(0) {}
            explicit const_iterator(const T* ptr)
                : mPtr(ptr) {}
            const_iterator(const iterator& it)
                : mPtr(it.mPtr) {}
            reference operator*() const {return mPtr->data(); }
            pointer operator->() const {return &mPtr->data(); }
            operator pointer() const {
                return &mPtr->data();
            }
            const_iterator& operator++()
            {
                mPtr = mPtr->next();
                return *this;
            }
            const_iterator& operator--()
            {
                mPtr = mPtr->prev();
                return *this;
            }
            bool operator==(const const_iterator& rhs) const {return mPtr == rhs.mPtr; }
            bool operator!=(const const_iterator& rhs) const {return mPtr != rhs.mPtr; }
            const T* ptr() const {return mPtr; }
        };

        intrusive_list()
            : intrusive_list_base() {}
        ~intrusive_list() {clear(); }

        const_iterator begin() const {return const_iterator((const T*)mHead.next()); }
        iterator begin() {return iterator((T*)mHead.next()); }
        const_iterator end() const {return const_iterator((const T*)&mHead); }
        iterator end() {return iterator((T*)&mHead); }

        const T& front() const
        {
            HPPA_ASSERT(!empty());
            return *begin();
        }
        T& front()
        {
            HPPA_ASSERT(!empty());
            return *begin();
        }
        const T& back() const
        {
            HPPA_ASSERT(!empty());
            return *(--end());
        }
        T& back()
        {
            HPPA_ASSERT(!empty());
            return *(--end());
        }

        void push_front(T* v) {insert(this->begin(), v); }
        void pop_front() {erase(this->begin()); }
        void push_back(T* v) {insert(this->end(), v); }
        void pop_back() {erase(--(this->end())); }

        iterator insert(iterator where, T* node)
        {
            T* newLink = node;
            newLink->link(where.ptr());
            return iterator(newLink);
        }
        iterator erase(iterator where)
        {
            T* node = where.ptr();
            ++where;
            node->unlink();
            return where;
        }
        void erase(T* node)
        {
            node->unlink();
        }
        void clear()
        {
            while (!this->empty())
            {
                this->pop_back();
            }
        }
    };

    //////////////////////////////////////////////////////////////////////////
    class HpAllocator
    {
    public:
        // the guard size controls how many extra bytes are stored after
        // every allocation payload to detect memory stomps
#ifdef DEBUG_ALLOCATOR
        static const size_t MEMORY_GUARD_SIZE  = 16UL;
#else
        static const size_t MEMORY_GUARD_SIZE  = 0UL;
#endif

        // minimum allocation size, must be a power of two
        // and it needs to be able to fit a pointer
        static const size_t MIN_ALLOCATION_LOG2 = 3UL;
        static const size_t MIN_ALLOCATION  = 1UL << MIN_ALLOCATION_LOG2;

        // the maximum small allocation, anything larger goes to the tree HpAllocator
        // must be a power of two
        static const size_t MAX_SMALL_ALLOCATION_LOG2 = 8UL;
        static const size_t MAX_SMALL_ALLOCATION  = 1UL << MAX_SMALL_ALLOCATION_LOG2;

        // default alignment, must be a power of two
        static const size_t DEFAULT_ALIGNMENT  = sizeof(double);

        static const size_t NUM_BUCKETS  = (MAX_SMALL_ALLOCATION / MIN_ALLOCATION);

        static inline bool is_small_allocation(size_t s)
        {
            return s + MEMORY_GUARD_SIZE <= MAX_SMALL_ALLOCATION;
        }
        static inline size_t clamp_small_allocation(size_t s)
        {
            return (s + MEMORY_GUARD_SIZE < MIN_ALLOCATION) ? MIN_ALLOCATION - MEMORY_GUARD_SIZE : s;
        }

        // bucket spacing functions control how the size-space is divided between buckets
        // currently we use linear spacing, could be changed to logarithmic etc
        static inline unsigned bucket_spacing_function(size_t size)
        {
            return (unsigned)((size + (MIN_ALLOCATION - 1)) >> MIN_ALLOCATION_LOG2) - 1;
        }
        static inline unsigned bucket_spacing_function_aligned(size_t size)
        {
            return (unsigned)(size >> MIN_ALLOCATION_LOG2) - 1;
        }
        static inline size_t bucket_spacing_function_inverse(unsigned index)
        {
            return (size_t)(index + 1) << MIN_ALLOCATION_LOG2;
        }

        // block header is where the large HpAllocator stores its book-keeping information
        // it is always located in front of the payload block
        class block_header
        {
            enum block_flags : uint64_t
            {
                BL_USED = 1,
                BL_TAG_MASK = 0xffff000000000000,
                BL_FLAG_MASK = 0x3
            };
            block_header* mPrev;
            // 16 bits of tag, 46 bits of size, 2 bits of flags (used or not)
            uint64_t mSizeAndFlags;
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning (disable : 4200) // zero sized array
#endif
            unsigned char _padding[DEFAULT_ALIGNMENT <= sizeof(block_header*) + sizeof(uint64_t) ? 0 : DEFAULT_ALIGNMENT - sizeof(block_header*) - sizeof(uint64_t)];
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
        public:
            typedef block_header* block_ptr;
            size_t size() const { return mSizeAndFlags & ~(BL_FLAG_MASK|BL_TAG_MASK); }
            uint16_t tag() const { return (mSizeAndFlags & BL_TAG_MASK) >> 48; }
            block_ptr next() const {return (block_ptr)((char*)mem() + size()); }
            block_ptr prev() const {return mPrev; }
            void* mem() const {return (void*)((char*)this + sizeof(block_header)); }
            bool used() const {return (mSizeAndFlags & BL_USED) != 0; }
            void set_used() {mSizeAndFlags |= BL_USED; }
            void set_unused() {mSizeAndFlags &= ~BL_USED; }
            void unlink()
            {
                next()->prev(prev());
                prev()->next(next());
            }
            void link_after(block_ptr link)
            {
                prev(link);
                next(link->next());
                next()->prev(this);
                prev()->next(this);
                tag(link->tag());
            }
            void size(size_t size)
            {
                (void)_padding;
                HPPA_ASSERT((size & BL_FLAG_MASK) == 0);
                HPPA_ASSERT((size & BL_TAG_MASK) == 0);
                mSizeAndFlags = (mSizeAndFlags & BL_TAG_MASK) | (mSizeAndFlags & BL_FLAG_MASK) | size;
            }
            void next(block_ptr next)
            {
                HPPA_ASSERT(next >= mem());
                size((char*)next - (char*)mem());
            }
            void prev(block_ptr prev)
            {
                mPrev = prev;
            }
            void tag(uint16_t tag)
            {
                mSizeAndFlags = (mSizeAndFlags & ~BL_TAG_MASK) | (uint64_t(tag) << 48);
            }
        };

        struct block_header_proxy
        {
            unsigned char _proxy_size[sizeof(block_header)];
        };

        // the page structure is where the small HpAllocator stores all its book-keeping information
        // it is always located at the front of a OS page
        struct free_link
        {
            free_link* mNext;
        };
        struct page
            : public block_header_proxy     /* must be first */
            , public intrusive_list<page>::node
        {
            page(size_t elemSize, size_t pageSize, size_t marker)
                : mBucketIndex((unsigned short)bucket_spacing_function_aligned(elemSize))
                , mUseCount(0)
            {
                mMarker = marker ^ ((size_t)this);

                // build the free list inside the new page
                // the page info sits at the front of the page
                size_t numElements = (pageSize - sizeof(page)) / elemSize;
                char* endMem = (char*)this + pageSize;
                char* currentMem = endMem - numElements * elemSize;
                char* nextMem = currentMem + elemSize;
                mFreeList = (free_link*)currentMem;
                while (nextMem != endMem)
                {
                    ((free_link*)currentMem)->mNext = (free_link*)(nextMem);
                    currentMem = nextMem;
                    nextMem += elemSize;
                }
                ((free_link*)currentMem)->mNext = nullptr;
            }

            inline void setInvalid()                { mMarker = 0; }

            free_link*      mFreeList = nullptr;
            size_t          mMarker = 0;
            unsigned short  mBucketIndex = 0;
            unsigned short  mUseCount = 0;

            size_t elem_size() const                { return bucket_spacing_function_inverse(mBucketIndex); }
            unsigned bucket_index() const           { return mBucketIndex; }
            size_t count() const                    { return mUseCount; }
            bool empty() const                      { return mUseCount == 0; }
            void inc_ref()                          { mUseCount++; }
            void dec_ref()                          { HPPA_ASSERT(mUseCount > 0); mUseCount--; }
            bool check_marker(size_t marker) const  { return mMarker == (marker ^ ((size_t)this)); }
        };
        typedef intrusive_list<page> page_list;
        class bucket
        {
            page_list mPageList;
#ifdef MULTITHREADED
            mutable AZStd::mutex mLock;
#endif
            size_t          mMarker;
#ifdef MULTITHREADED
            unsigned char _padding[sizeof(void*) * 16 - sizeof(page_list) - sizeof(AZStd::mutex) - sizeof(size_t)];
#else
            unsigned char _padding[sizeof(void*) * 4 - sizeof(page_list) - sizeof(size_t)];
#endif
        public:
            bucket();
#ifdef MULTITHREADED
            AZStd::mutex& get_lock() const {return mLock; }
#endif
            size_t marker() const {return mMarker; }
            const page* page_list_begin() const {return mPageList.begin(); }
            page* page_list_begin() {return mPageList.begin(); }
            const page* page_list_end() const {return mPageList.end(); }
            page* page_list_end() {return mPageList.end(); }
            bool page_list_empty() const {return mPageList.empty(); }
            void add_free_page(page* p) {mPageList.push_front(p); }
            page* get_free_page();
            const page* get_free_page() const;
            void* alloc(page* p);
            void free(page* p, void* ptr);
        };
        void* bucket_system_alloc();
        void bucket_system_free(void* ptr);
        page* bucket_grow(size_t elemSize, size_t marker);
        void* bucket_alloc(size_t size);
        void* bucket_alloc_direct(unsigned bi);
        void* bucket_realloc(void* ptr, size_t size);
        void* bucket_realloc_aligned(void* ptr, size_t size, size_t alignment);
        void bucket_free(void* ptr);
        void bucket_free_direct(void* ptr, unsigned bi);
        /// return the block size for the pointer.
        size_t bucket_ptr_size(void* ptr) const;
        size_t bucket_get_max_allocation() const;
        size_t bucket_get_unused_memory(bool isPrint) const;
        void bucket_purge();

        // locate the page information from a pointer
        inline page* ptr_get_page(void* ptr) const
        {
            return (page*)AZ::PointerAlignDown((char*)ptr, m_poolPageSize);
        }

        bool ptr_in_bucket(void* ptr) const
        {
            bool result = false;
            if (m_isPoolAllocations)
            {
                page* p = ptr_get_page(ptr);
                unsigned bi = p->bucket_index();
                if (bi < NUM_BUCKETS)
                {
                    result = p->check_marker(mBuckets[bi].marker());
                    // there's a minimal chance the marker check is not sufficient
                    // due to random data that happens to match the marker
                    // the exhaustive search below will catch this case
                    // and that will indicate that more secure measures are needed
#ifdef DEBUG_PTR_IN_BUCKET_CHECK
#ifdef MULTITHREADED
                    AZStd::lock_guard<AZStd::mutex> lock(mBuckets[bi].get_lock());
#endif
                    const page* pe = mBuckets[bi].page_list_end();
                    const page* pb = mBuckets[bi].page_list_begin();
                    for (; pb != pe && pb != p; pb = pb->next())
                    {
                    }
                    HPPA_ASSERT(result == (pb == p));
#endif
                }
            }
            return result;
        }

        // free node is what the large HpAllocator uses to find free space
        // it's stored next to the block header when a block is not in use
        struct free_node
            : public AZStd::intrusive_multiset_node<free_node>
        {
            block_header* get_block() const { return (block_header*)((char*)this - sizeof(block_header)); }
            operator size_t() const {
                return get_block()->size();
            }
        };
        typedef AZStd::intrusive_multiset<free_node, AZStd::intrusive_multiset_base_hook<free_node>, AZStd::less<size_t> > free_node_tree;

        static inline block_header* ptr_get_block_header(void* ptr)
        {
            return (block_header*)((char*)ptr - sizeof(block_header));
        }

        // helper functions to manipulate block headers
        void split_block(block_header* bl, size_t size);
        block_header* shift_block(block_header* bl, size_t offs);
        block_header* coalesce_block(block_header* bl);

        void* tree_system_alloc(size_t size);
        void tree_system_free(void* ptr, size_t size);
        block_header* tree_extract(size_t size);
        block_header* tree_extract_aligned(size_t size, size_t alignment);
        block_header* tree_extract_bucket_page();
        block_header* tree_add_block(void* mem, size_t size);
        block_header* tree_grow(size_t size);
        void tree_attach(block_header* bl);
        void tree_detach(block_header* bl);
        void tree_purge_block(block_header* bl);

        void* tree_alloc(size_t size);
        void* tree_alloc_aligned(size_t size, size_t alignment);
        void* tree_alloc_bucket_page();
        void* tree_realloc(void* ptr, size_t size);
        void* tree_realloc_aligned(void* ptr, size_t size, size_t alignment);
        size_t tree_resize(void* ptr, size_t size);
        void tree_free(void* ptr);
        void tree_free_bucket_page(void* ptr); // only allocations with tree_alloc_bucket_page should be passed here, we don't have any guards.
        size_t tree_ptr_size(void* ptr) const;
        size_t tree_get_max_allocation() const;
        size_t tree_get_unused_memory(bool isPrint) const;
        void tree_purge();
        // the lower 16 bits of the allocator's address should be enough to be unique for a handful of allocators
        uint16_t tree_tag() const { return reinterpret_cast<uintptr_t>(this) & 0xffff; }

        bucket mBuckets[NUM_BUCKETS];
        free_node_tree mFreeTree;
#ifdef MULTITHREADED
        // TODO rbbaklov: switched to recursive_mutex from mutex for Linux support.
        mutable AZStd::recursive_mutex mTreeMutex;
#endif

        enum debug_source
        {
            DEBUG_SOURCE_BUCKETS = 0, 
            DEBUG_SOURCE_TREE = 1,
            DEBUG_INVALID = 2
        };

#ifdef DEBUG_ALLOCATOR
        // debug record stores all debugging information for every allocation
        class debug_record
        {
        public:
            static const unsigned MAX_CALLSTACK_DEPTH = 16;
            debug_record() 
                : mPtr(nullptr)
                , mSize(0)
                , mSource(DEBUG_INVALID)
                , mGuardByte(0)
                , mCallStack()
            {}
            
            debug_record(void* ptr) // used for searching based on the pointer
                : mPtr(ptr)
                , mSize(0)
                , mSource(DEBUG_INVALID)
                , mGuardByte(0)
                , mCallStack()
            {}

            debug_record(void* ptr, size_t size, debug_source source)
                : mPtr(ptr)
                , mSize(size)
                , mSource(source)
                , mCallStack()
            {
                write_guard();
                record_stack();
            }
            void* ptr() const {return mPtr; }
            void set_ptr(void* ptr) {mPtr = ptr; }
            size_t size() const {return mSize; }
            debug_source source() const {return (debug_source)mSource; }
            void set_size(size_t size) {mSize = size; }
            const void print_stack() const;
            void record_stack();
            void write_guard();
            bool check_guard() const;

            // required for the multiset comparator
            bool operator<(const debug_record& other) const { return mPtr < other.mPtr; }

        private:
            void* mPtr;
            size_t mSize;
            debug_source mSource;
            unsigned char mGuardByte;
            AZ::Debug::StackFrame mCallStack[MAX_CALLSTACK_DEPTH];
        };

        // helper structure for returning debug record information
        struct debug_info
        {
            size_t size;
            debug_source source;
            debug_info(size_t _size, debug_source _source)
                : size(_size)
                , source(_source) {}
        };

        class DebugMapAllocator
            : public AllocatorBase<MallocSchema>
        {
        public:
            AZ_TYPE_INFO(DebugMapAllocator, "{CFFEAB45-6E7F-405E-851A-72A6B7B35814}");

            using Base = AllocatorBase<MallocSchema>;
            using Descriptor = Base::Descriptor;

            DebugMapAllocator()
                : Base("DebugMapAllocator", "Allocator for HPHA debug map")
            {
            }

            bool Create(const Descriptor& desc = Descriptor())
            {
                m_schema = new (&m_schemaStorage) MallocSchema(desc);
                return m_schema != nullptr;
            }
        };

        // record map that keeps all debug records in a set, sorted by memory address of the allocation
        class debug_record_map
            : public AZStd::set<debug_record, AZStd::less<debug_record>, AZStdAlloc<DebugMapAllocator> >
        {
            typedef AZStd::set<debug_record, AZStd::less<debug_record>, AZStdAlloc<DebugMapAllocator> > base;

            static void initial_fill(void* ptr, size_t size);
        public:
            debug_record_map()
            {
                if (!AZ::AllocatorInstance<DebugMapAllocator>::IsReady())
                {
                    AZ::AllocatorInstance<DebugMapAllocator>::Create();
                }
            }
            ~debug_record_map()
            {
            }

            typedef base::const_iterator const_iterator;
            typedef base::iterator iterator;
            void add(void* ptr, size_t size, debug_source source);
            debug_info remove(void* ptr);
            debug_info remove(void* ptr, size_t size);
            debug_info replace(void* ptr, void* newPtr, size_t size, debug_source source);
            debug_info update(void* ptr, size_t size);
            void check(void* ptr) const;
            void purge();
        };
        debug_record_map mDebugMap;
#ifdef MULTITHREADED
        mutable AZStd::mutex mDebugMutex;
#endif

        size_t mTotalRequestedSizeBuckets;
        size_t mTotalRequestedSizeTree;

        void* debug_add(void* ptr, size_t size, debug_source source);
        void debug_remove(void* ptr);
        void debug_remove(void* ptr, size_t size);
        void debug_replace(void* ptr, void* newPtr, size_t size, debug_source source);
        void debug_update(void* ptr, size_t size);
        void debug_check(void* ptr) const;
        void debug_purge();

#else // !DEBUG_ALLOCATOR

        void* debug_add(void* ptr, size_t, debug_source) {return ptr; }
        void debug_remove(void*) {}
        void debug_remove(void*, size_t) {}
        void debug_replace(void*, void*, size_t, debug_source) {}
        void debug_update(void*, size_t) {}
        void debug_check(void*) const {}
        void debug_purge() {}

#endif // DEBUG_ALLOCATOR

        size_t mTotalAllocatedSizeBuckets;
        size_t mTotalAllocatedSizeTree;
    public:
        HpAllocator(AZ::HphaSchema::Descriptor desc);
        ~HpAllocator();
        // allocate memory using DEFAULT_ALIGNMENT
        // size == 0 returns NULL
        inline void* alloc(size_t size)
        {
            if (size == 0)
            {
                return nullptr;
            }
            if (m_isPoolAllocations && is_small_allocation(size))
            {
                size = clamp_small_allocation(size);
                void* ptr = bucket_alloc_direct(bucket_spacing_function(size + MEMORY_GUARD_SIZE));
                return debug_add(ptr, size, DEBUG_SOURCE_BUCKETS);
            }
            else
            {
                void* ptr = tree_alloc(size + MEMORY_GUARD_SIZE);
                return debug_add(ptr, size, DEBUG_SOURCE_TREE);
            }
        }
        // allocate memory with a specific alignment
        // size == 0 returns NULL
        // alignment <= DEFAULT_ALIGNMENT acts as alloc
        inline void* alloc(size_t size, size_t alignment)
        {
            HPPA_ASSERT((alignment & (alignment - 1)) == 0);
            if (alignment <= DEFAULT_ALIGNMENT)
            {
                return alloc(size);
            }
            if (size == 0)
            {
                return nullptr;
            }
            if (m_isPoolAllocations && is_small_allocation(size) && alignment <= MAX_SMALL_ALLOCATION)
            {
                size = clamp_small_allocation(size);
                void* ptr = bucket_alloc_direct(bucket_spacing_function(AZ::SizeAlignUp(size + MEMORY_GUARD_SIZE, alignment)));
                return debug_add(ptr, size, DEBUG_SOURCE_BUCKETS);
            }
            else
            {
                void* ptr = tree_alloc_aligned(size + MEMORY_GUARD_SIZE, alignment);
                return debug_add(ptr, size, DEBUG_SOURCE_TREE);
            }
        }
        // allocate count*size and clear the memory block to zero, always uses DEFAULT_ALIGNMENT
        inline  void* calloc(size_t count, size_t size)
        {
            void* p = alloc(count * size);
            if (p)
            {
                memset(p, 0, count * size);
            }
            return p;
        }
        // realloc the memory block using DEFAULT_ALIGNMENT
        // ptr == NULL acts as alloc
        // size == 0 acts as free
        void* realloc(void* ptr, size_t size)
        {
            if (ptr == NULL)
            {
                return alloc(size);
            }
            if (size == 0)
            {
                free(ptr);
                return NULL;
            }
            debug_check(ptr);
            void* newPtr = NULL;
            if (ptr_in_bucket(ptr))
            {
                if (is_small_allocation(size)) // no point to check m_isPoolAllocations as if it's false pointer can't be in a bucket.
                {
                    size = clamp_small_allocation(size);
                    newPtr = bucket_realloc(ptr, size + MEMORY_GUARD_SIZE);
                    debug_replace(ptr, newPtr, size, DEBUG_SOURCE_BUCKETS);
                }
                else
                {
                    newPtr = tree_alloc(size + MEMORY_GUARD_SIZE);
                    if (!newPtr)
                    {
                        return NULL;
                    }
                    HPPA_ASSERT(size >= (ptr_get_page(ptr)->elem_size() - MEMORY_GUARD_SIZE));
                    memcpy(newPtr, ptr, ptr_get_page(ptr)->elem_size() - MEMORY_GUARD_SIZE);
                    bucket_free(ptr);
                    debug_replace(ptr, newPtr, size, DEBUG_SOURCE_TREE);
                }
            }
            else
            {
                if (m_isPoolAllocations && is_small_allocation(size))
                {
                    size = clamp_small_allocation(size);
                    newPtr = bucket_alloc(size + MEMORY_GUARD_SIZE);
                    if (!newPtr)
                    {
                        return NULL;
                    }
                    HPPA_ASSERT(size <= ptr_get_block_header(ptr)->size() - MEMORY_GUARD_SIZE);
                    memcpy(newPtr, ptr, size);
                    tree_free(ptr);
                    debug_replace(ptr, newPtr, size, DEBUG_SOURCE_BUCKETS);
                }
                else
                {
                    newPtr = tree_realloc(ptr, size + MEMORY_GUARD_SIZE);
                    debug_replace(ptr, newPtr, size, DEBUG_SOURCE_TREE);
                }
            }
            return newPtr;
        }
        // realloc the memory block using a specific alignment
        // ptr == NULL acts as alloc
        // size == 0 acts as free
        // alignment <= DEFAULT_ALIGNMENT acts as realloc
        void* realloc(void* ptr, size_t size, size_t alignment)
        {
            HPPA_ASSERT((alignment & (alignment - 1)) == 0);
            if (alignment <= DEFAULT_ALIGNMENT)
            {
                return realloc(ptr, size);
            }
            if (ptr == NULL)
            {
                return alloc(size, alignment);
            }
            if (size == 0)
            {
                free(ptr);
                return NULL;
            }
            if ((size_t)ptr & (alignment - 1))
            {
                void* newPtr = alloc(size, alignment);
                if (!newPtr)
                {
                    return NULL;
                }
                size_t count = this->size(ptr);
                if (count > size)
                {
                    count = size;
                }
                memcpy(newPtr, ptr, count);
                free(ptr);
                return newPtr;
            }
            debug_check(ptr);
            void* newPtr = NULL;
            if (ptr_in_bucket(ptr))
            {
                if (is_small_allocation(size) && alignment <= MAX_SMALL_ALLOCATION) // no point to check m_isPoolAllocations as if it was false, pointer can't be in a bucket
                {
                    size = clamp_small_allocation(size);
                    newPtr = bucket_realloc_aligned(ptr, size + MEMORY_GUARD_SIZE, alignment);
                    debug_replace(ptr, newPtr, size, DEBUG_SOURCE_BUCKETS);
                }
                else
                {
                    newPtr = tree_alloc_aligned(size + MEMORY_GUARD_SIZE, alignment);
                    if (!newPtr)
                    {
                        return NULL;
                    }
                    HPPA_ASSERT(size >= (ptr_get_page(ptr)->elem_size() - MEMORY_GUARD_SIZE));
                    memcpy(newPtr, ptr, ptr_get_page(ptr)->elem_size() - MEMORY_GUARD_SIZE);
                    bucket_free(ptr);
                    debug_replace(ptr, newPtr, size, DEBUG_SOURCE_TREE);
                }
            }
            else
            {
                if (m_isPoolAllocations && is_small_allocation(size) && alignment <= MAX_SMALL_ALLOCATION)
                {
                    size = clamp_small_allocation(size);
                    newPtr = bucket_alloc_direct(bucket_spacing_function(AZ::SizeAlignUp(size + MEMORY_GUARD_SIZE, alignment)));
                    if (!newPtr)
                    {
                        return NULL;
                    }
                    HPPA_ASSERT(size <= ptr_get_block_header(ptr)->size() - MEMORY_GUARD_SIZE);
                    memcpy(newPtr, ptr, size);
                    tree_free(ptr);
                    debug_replace(ptr, newPtr, size, DEBUG_SOURCE_BUCKETS);
                }
                else
                {
                    newPtr = tree_realloc_aligned(ptr, size + MEMORY_GUARD_SIZE, alignment);
                    debug_replace(ptr, newPtr, size, DEBUG_SOURCE_TREE);
                }
            }
            return newPtr;
        }
        // resize the memory block to the extent possible
        // returns the size of the resulting memory block
        inline  size_t resize(void* ptr, size_t size)
        {
            if (ptr == NULL)
            {
                return 0;
            }
            HPPA_ASSERT(size > 0);
            debug_check(ptr);
            if (ptr_in_bucket(ptr))
            {
                size = ptr_get_page(ptr)->elem_size() - MEMORY_GUARD_SIZE;
                debug_update(ptr, size);
                return size;
            }

            // from the tree we should allocate only >= MAX_SMALL_ALLOCATION
            size = AZStd::GetMax(size, MAX_SMALL_ALLOCATION + MIN_ALLOCATION);

            size = tree_resize(ptr, size + MEMORY_GUARD_SIZE) - MEMORY_GUARD_SIZE;
            debug_update(ptr, size);
            return size;
        }
        // query the size of the memory block
        inline  size_t size(void* ptr) const
        {
            if (ptr == NULL)
            {
                return 0;
            }
            if (ptr_in_bucket(ptr))
            {
                debug_check(ptr);
                return ptr_get_page(ptr)->elem_size() - MEMORY_GUARD_SIZE;
            }
            block_header* bl = ptr_get_block_header(ptr);
            // If this block doesn't belong to this allocator, return 0
            if (bl->tag() != tree_tag())
            {
                return 0;
            }
            debug_check(ptr);
            size_t blockSize = bl->size() - MEMORY_GUARD_SIZE;
            return blockSize;
        }
        // free the memory block
        inline void free(void* ptr)
        {
            if (ptr == NULL)
            {
                return;
            }
            debug_remove(ptr);
            if (ptr_in_bucket(ptr))
            {
                return bucket_free(ptr);
            }
            tree_free(ptr);
        }
        // free the memory block supplying the original size with DEFAULT_ALIGNMENT
        inline void free(void* ptr, size_t origSize)
        {
            if (ptr == NULL)
            {
                return;
            }
            debug_remove(ptr, origSize);
            if (m_isPoolAllocations && is_small_allocation(origSize))
            {
                // if this asserts probably the original alloc used alignment
                HPPA_ASSERT(ptr_in_bucket(ptr));
                return bucket_free_direct(ptr, bucket_spacing_function(origSize + MEMORY_GUARD_SIZE));
            }
            tree_free(ptr);
        }
        // free the memory block supplying the original size and alignment
        inline void free(void* ptr, size_t origSize, size_t oldAlignment)
        {
            if (ptr == NULL)
            {
                return;
            }
            debug_remove(ptr, origSize);
            if (m_isPoolAllocations && is_small_allocation(origSize) && oldAlignment <= MAX_SMALL_ALLOCATION)
            {
                HPPA_ASSERT(ptr_in_bucket(ptr));
                return bucket_free_direct(ptr, bucket_spacing_function(AZ::SizeAlignUp(origSize + MEMORY_GUARD_SIZE, oldAlignment)));
            }
            tree_free(ptr);
        }
        // return all unused memory pages to the OS
        // this function doesn't need to be called unless needed
        // it can be called periodically when large amounts of memory can be reclaimed
        // in all cases memory is never automatically returned to the OS
        void purge()
        {
            // Purge buckets first since they use tree pages
            bucket_purge();
            tree_purge();
            debug_purge();
        }
#ifdef DEBUG_ALLOCATOR
        // print HpAllocator statistics
        void report();
        // check memory integrity
        void check();
        // return the total number of requested memory
        size_t requested() const
        {
            return mTotalRequestedSizeBuckets + mTotalRequestedSizeTree;
        }
#endif
        // return the total number of allocated memory
        inline  size_t allocated() const
        {
            return mTotalAllocatedSizeBuckets + mTotalAllocatedSizeTree;
        }
        /// returns allocation size for the pointer if it belongs to the allocator. result is undefined if the pointer doesn't belong to the allocator.
        size_t  AllocationSize(void* ptr);
        size_t  GetMaxAllocationSize() const;
        size_t  GetUnAllocatedMemory(bool isPrint) const;

        void*   SystemAlloc(size_t size, size_t align);
        void    SystemFree(void* ptr);

        void*        m_fixedBlock;
        size_t       m_fixedBlockSize;
        const size_t m_treePageSize;
        const size_t m_treePageAlignment;
        const size_t m_poolPageSize;
        bool         m_isPoolAllocations;
        IAllocatorAllocate* m_subAllocator;
    };

    //////////////////////////////////////////////////////////////////////////
    // Prevent SystemAllocator from growing in small chunks
    HpAllocator::HpAllocator(AZ::HphaSchema::Descriptor desc)
#if AZ_TRAIT_OS_ALLOW_DIRECT_ALLOCATIONS
        // We will use the os for direct allocations if memoryBlock == NULL
        // If m_systemChunkSize is specified, use that size for allocating tree blocks from the OS
        // m_treePageAlignment should be OS_VIRTUAL_PAGE_SIZE in all cases with this trait as we work 
        // with virtual memory addresses when the tree grows and we cannot specify an alignment in all cases
        : m_treePageSize(desc.m_fixedMemoryBlock != NULL ? desc.m_pageSize :
                         desc.m_systemChunkSize != 0 ? desc.m_systemChunkSize : OS_VIRTUAL_PAGE_SIZE)
        , m_treePageAlignment(OS_VIRTUAL_PAGE_SIZE)
        , m_poolPageSize(desc.m_fixedMemoryBlock != NULL ? desc.m_poolPageSize : OS_VIRTUAL_PAGE_SIZE)
#else
        : m_treePageSize(desc.m_pageSize)
        , m_treePageAlignment(desc.m_pageSize)
        , m_poolPageSize(desc.m_poolPageSize)
#endif
        , m_subAllocator(desc.m_subAllocator)
    {
#ifdef DEBUG_ALLOCATOR
        mTotalRequestedSizeBuckets = 0;
        mTotalRequestedSizeTree = 0;
#endif
        mTotalAllocatedSizeBuckets = 0;
        mTotalAllocatedSizeTree = 0;

        m_fixedBlock = desc.m_fixedMemoryBlock;
        m_fixedBlockSize = desc.m_fixedMemoryBlockByteSize;
        m_isPoolAllocations = desc.m_isPoolAllocations;
        if (desc.m_fixedMemoryBlock)
        {
            block_header* bl = tree_add_block(m_fixedBlock, m_fixedBlockSize);
            tree_attach(bl);
        }

#if AZ_TRAIT_OS_HAS_CRITICAL_SECTION_SPIN_COUNT
#   if  defined(MULTITHREADED)
        // For some platforms we can use an actual spin lock, test and profile. We don't expect much contention there
        SetCriticalSectionSpinCount(mTreeMutex.native_handle(), SPIN_COUNT);
#   endif // MULTITHREADED
#endif // AZ_TRAIT_OS_HAS_CRITICAL_SECTION_SPIN_COUNT
    }

    HpAllocator::~HpAllocator()
    {
#ifdef DEBUG_ALLOCATOR
        // Check if there are not-freed allocations
        report();
        check();
#endif
        
        purge();

#ifdef DEBUG_ALLOCATOR 
        // Check if all the memory was returned to the OS
        for (unsigned i = 0; i < NUM_BUCKETS; i++)
        {
            HPPA_ASSERT(mBuckets[i].page_list_empty());
        }

        if (!m_fixedBlockSize)
        {
            if (!mFreeTree.empty())
            {
                free_node_tree::iterator node = mFreeTree.begin();
                free_node_tree::iterator end = mFreeTree.end();
                while (node != end)
                {
                    block_header* cur = node->get_block();
                    AZ_TracePrintf("HPHA", "Block in free tree: %p, size=%zi bytes", cur, cur->size());
                    ++node;
                }
            }
            HPPA_ASSERT(mFreeTree.empty());
        }
        else
        {
            // Verify that the last block in the free tree is the m_fixedBlock
            HPPA_ASSERT(mFreeTree.size() == 1);
            HPPA_ASSERT(mFreeTree.begin()->get_block()->prev() == m_fixedBlock);
            HPPA_ASSERT((mFreeTree.begin()->get_block()->size() + sizeof(block_header) * 3) == m_fixedBlockSize);
        }
#endif
    }

    HpAllocator::bucket::bucket()
    {
#if AZ_TRAIT_OS_HAS_CRITICAL_SECTION_SPIN_COUNT
#   if  defined(MULTITHREADED)
        // For some platforms we can use an actual spin lock, test and profile. We don't expect much contention there
        SetCriticalSectionSpinCount(mLock.native_handle(), SPIN_COUNT);
#   endif // MULTITHREADED
#endif // AZ_TRAIT_OS_HAS_CRITICAL_SECTION_SPIN_COUNT

        // Use the global AZ random generator.
#ifdef AZ_OS32
        mMarker = AZ::Sfmt::GetInstance().Rand32();
#else
        mMarker = AZ::Sfmt::GetInstance().Rand64();
#endif

        (void)_padding;
    }

    HpAllocator::page* HpAllocator::bucket::get_free_page()
    {
        if (!mPageList.empty())
        {
            page* p = &mPageList.front();
            if (p->mFreeList)
            {
                return p;
            }
        }
        return NULL;
    }

    const HpAllocator::page* HpAllocator::bucket::get_free_page() const
    {
        if (!mPageList.empty())
        {
            const page* p = &mPageList.front();
            if (p->mFreeList)
            {
                return p;
            }
        }
        return NULL;
    }

    void* HpAllocator::bucket::alloc(page* p)
    {
        // get an element from the free list
        HPPA_ASSERT(p && p->mFreeList);
        p->inc_ref();
        free_link* free = p->mFreeList;
        free_link* next = free->mNext;
        p->mFreeList = next;
        if (!next)
        {
            // if full, auto sort to back
            p->unlink();
            mPageList.push_back(p);
        }
        return (void*)free;
    }

    void HpAllocator::bucket::free(page* p, void* ptr)
    {
        // add the element back to the free list
        free_link* free = p->mFreeList;
        free_link* lnk = (free_link*)ptr;
        lnk->mNext = free;
        p->mFreeList = lnk;
        p->dec_ref();
        if (!free)
        {
            // if the page was previously full, auto sort to front
            p->unlink();
            mPageList.push_front(p);
        }
    }

    void* HpAllocator::bucket_system_alloc()
    {
        void* ptr;
        if (m_fixedBlock)
        {
            ptr = tree_alloc_bucket_page();
            // mTotalAllocatedSizeBuckets memory is part of the tree allocations
        }
        else
        {
            ptr = SystemAlloc(m_poolPageSize, m_poolPageSize);
            mTotalAllocatedSizeBuckets += m_poolPageSize;
        }
        HPPA_ASSERT(((size_t)ptr & (m_poolPageSize - 1)) == 0);
        return ptr;
    }

    void HpAllocator::bucket_system_free(void* ptr)
    {
        HPPA_ASSERT(ptr);
        if (m_fixedBlock)
        {
            tree_free_bucket_page(ptr);
            // mTotalAllocatedSizeBuckets memory is part of the tree allocations
        }
        else
        {
            SystemFree(ptr);
            mTotalAllocatedSizeBuckets -= m_poolPageSize;
        }
    }

    HpAllocator::page* HpAllocator::bucket_grow(size_t elemSize, size_t marker)
    {
        // make sure mUseCount won't overflow
        HPPA_ASSERT((m_poolPageSize - sizeof(page)) / elemSize <= USHRT_MAX);
        if (void* mem = bucket_system_alloc())
        {
            return new (mem) page((unsigned short)elemSize, m_poolPageSize, marker);
        }
        return nullptr;
    }

    void* HpAllocator::bucket_alloc(size_t size)
    {
        HPPA_ASSERT(size <= MAX_SMALL_ALLOCATION);
        unsigned bi = bucket_spacing_function(size);
        HPPA_ASSERT(bi < NUM_BUCKETS);
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::mutex> lock(mBuckets[bi].get_lock());
#endif
        // get the page info and check if there's any available elements
        page* p = mBuckets[bi].get_free_page();
        if (!p)
        {
            // get a page from the OS, initialize it and add it to the list
            size_t bsize = bucket_spacing_function_inverse(bi);
            p = bucket_grow(bsize, mBuckets[bi].marker());
            if (!p)
            {
                return NULL;
            }
            mBuckets[bi].add_free_page(p);
        }
        HPPA_ASSERT(p->elem_size() >= size);
        return mBuckets[bi].alloc(p);
    }

    void* HpAllocator::bucket_alloc_direct(unsigned bi)
    {
        HPPA_ASSERT(bi < NUM_BUCKETS);
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::mutex> lock(mBuckets[bi].get_lock());
#endif
        page* p = mBuckets[bi].get_free_page();
        if (!p)
        {
            size_t bsize = bucket_spacing_function_inverse(bi);
            p = bucket_grow(bsize, mBuckets[bi].marker());
            if (!p)
            {
                return NULL;
            }
            mBuckets[bi].add_free_page(p);
        }
        return mBuckets[bi].alloc(p);
    }

    void* HpAllocator::bucket_realloc(void* ptr, size_t size)
    {
        page* p = ptr_get_page(ptr);
        size_t elemSize = p->elem_size();
        //if we do this and we use the bucket_free with size hint we will crash, as the hit will not be the real index/bucket size
        //if (size <= elemSize)
        if (size == elemSize)
        {
            return ptr;
        }
        void* newPtr = bucket_alloc(size);
        if (!newPtr)
        {
            return NULL;
        }
        memcpy(newPtr, ptr, AZStd::GetMin(elemSize - MEMORY_GUARD_SIZE, size - MEMORY_GUARD_SIZE));
        bucket_free(ptr);
        return newPtr;
    }

    void* HpAllocator::bucket_realloc_aligned(void* ptr, size_t size, size_t alignment)
    {
        page* p = ptr_get_page(ptr);
        size_t elemSize = p->elem_size();
        //if we do this and we use the bucket_free with size hint we will crash, as the hit will not be the real index/bucket size
        //if (size <= elemSize && (elemSize&(alignment-1))==0)
        if (size == elemSize && (elemSize & (alignment - 1)) == 0)
        {
            return ptr;
        }
        void* newPtr = bucket_alloc_direct(bucket_spacing_function(AZ::SizeAlignUp(size, alignment)));
        if (!newPtr)
        {
            return NULL;
        }
        memcpy(newPtr, ptr, AZStd::GetMin(elemSize - MEMORY_GUARD_SIZE, size - MEMORY_GUARD_SIZE));
        bucket_free(ptr);
        return newPtr;
    }

    void HpAllocator::bucket_free(void* ptr)
    {
        page* p = ptr_get_page(ptr);
        unsigned bi = p->bucket_index();
        HPPA_ASSERT(bi < NUM_BUCKETS);
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::mutex> lock(mBuckets[bi].get_lock());
#endif
        mBuckets[bi].free(p, ptr);
    }

    void HpAllocator::bucket_free_direct(void* ptr, unsigned bi)
    {
        HPPA_ASSERT(bi < NUM_BUCKETS);
        page* p = ptr_get_page(ptr);
        // if this asserts, the free size doesn't match the allocated size
        // most likely a class needs a base virtual destructor
        HPPA_ASSERT(bi == p->bucket_index());
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::mutex> lock(mBuckets[bi].get_lock());
#endif
        mBuckets[bi].free(p, ptr);
    }

    size_t HpAllocator::bucket_ptr_size(void* ptr) const
    {
        page* p = ptr_get_page(ptr);
        unsigned bi = p->bucket_index();
        HPPA_ASSERT(bi < NUM_BUCKETS);
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::mutex> lock(mBuckets[bi].get_lock());
#endif
        return p->elem_size() - MEMORY_GUARD_SIZE;
    }

    size_t HpAllocator::bucket_get_max_allocation() const
    {
        for (int i = (int)NUM_BUCKETS - 1; i > 0; i--)
        {
#ifdef MULTITHREADED
            AZStd::lock_guard<AZStd::mutex> lock(mBuckets[i].get_lock());
#endif
            const page* p = mBuckets[i].get_free_page();
            if (p)
            {
                return p->elem_size();
            }
        }

        return 0;
    }

    size_t HpAllocator::bucket_get_unused_memory(bool isPrint) const
    {
        size_t unusedMemory = 0;
        size_t availablePageMemory = m_poolPageSize - sizeof(page);
        for (int i = (int)NUM_BUCKETS - 1; i > 0; i--)
        {
#ifdef MULTITHREADED
            AZStd::lock_guard<AZStd::mutex> lock(mBuckets[i].get_lock());
#endif
            const page* pageEnd = mBuckets[i].page_list_end();
            for (const page* p = mBuckets[i].page_list_begin(); p != pageEnd; )
            {
                // early out if we reach fully occupied page (the remaining should all be full)
                if (p->mFreeList == nullptr)
                {
                    break;
                }
                size_t elementSize = p->elem_size();
                size_t availableMemory = availablePageMemory - (elementSize * p->count());
                unusedMemory += availableMemory;
                if (isPrint)
                {
                    AZ_TracePrintf("System", "Unused Bucket %d page %p elementSize: %d available: %d elements\n", i, p, elementSize, availableMemory / elementSize);
                }
                p = p->next();
            }
        }
        return unusedMemory;
    }

    void HpAllocator::bucket_purge()
    {
        for (unsigned i = 0; i < NUM_BUCKETS; i++)
        {
#ifdef MULTITHREADED
            AZStd::lock_guard<AZStd::mutex> lock(mBuckets[i].get_lock());
#endif
            page* pageEnd = mBuckets[i].page_list_end();
            for (page* p = mBuckets[i].page_list_begin(); p != pageEnd; )
            {
                // early out if we reach fully occupied page (the remaining should all be full)
                if (p->mFreeList == nullptr)
                {
                    break;
                }
                page* next = p->next();
                if (p->empty())
                {
                    HPPA_ASSERT(p->mFreeList);
                    p->unlink();
                    p->setInvalid();
                    bucket_system_free(p);
                }
                p = next;
            }
        }
    }

    void HpAllocator::split_block(block_header* bl, size_t size)
    {
        HPPA_ASSERT(size + sizeof(block_header) + sizeof(free_node) <= bl->size());
        block_header* newBl = (block_header*)((char*)bl + size + sizeof(block_header));
        newBl->link_after(bl);
        newBl->set_unused();
    }

    HpAllocator::block_header* HpAllocator::shift_block(block_header* bl, size_t offs)
    {
        HPPA_ASSERT(offs > 0);
        block_header* prev = bl->prev();
        bl->unlink();
        bl = (block_header*)((char*)bl + offs);
        bl->link_after(prev);
        bl->set_unused();
        return bl;
    }

    HpAllocator::block_header* HpAllocator::coalesce_block(block_header* bl)
    {
        HPPA_ASSERT(!bl->used());
        block_header* next = bl->next();
        if (!next->used())
        {
            tree_detach(next);
            next->unlink();
        }
        block_header* prev = bl->prev();
        if (!prev->used())
        {
            tree_detach(prev);
            bl->unlink();
            bl = prev;
        }
        return bl;
    }

    void* HpAllocator::tree_system_alloc(size_t size)
    {
        if (m_fixedBlock)
        {
            return nullptr; // we ran out of memory in our fixed block
        }
        return SystemAlloc(size, m_treePageAlignment);
    }

    void HpAllocator::tree_system_free(void* ptr, size_t size)
    {
        HPPA_ASSERT(ptr);
        (void)size;

        if (m_fixedBlock)
        {
            return; // no need to free the fixed block
        }
        SystemFree(ptr);
    }

    HpAllocator::block_header* HpAllocator::tree_add_block(void* mem, size_t size)
    {
        // create a dummy block to avoid prev() NULL checks and allow easy block shifts
        // potentially this dummy block might grow (due to shift_block) but not more than sizeof(free_node)
        block_header* front = (block_header*)mem;
        front->prev(0);
        front->size(0);
        front->set_used();
        front->tag(tree_tag());
        block_header* back = (block_header*)front->mem();
        back->prev(front);
        back->size(0);
        // now the real free block
        front = back;
        back = (block_header*)((char*)mem + size - sizeof(block_header));
        back->size(0);
        back->set_used();
        front->set_unused();
        front->next(back);
        front->tag(tree_tag());
        back->prev(front);
        back->tag(tree_tag());
        return front;
    }

    HpAllocator::block_header* HpAllocator::tree_grow(size_t size)
    {
        const size_t paddedSize = size + 3 * sizeof(block_header); // two fences plus one fake
        size_t alignedSize = paddedSize;
        if (alignedSize < m_treePageSize)
        {
            alignedSize = AZ::SizeAlignUp(alignedSize, m_treePageSize);
        }
        HPPA_ASSERT(alignedSize >= paddedSize);
        if (void* mem = tree_system_alloc(alignedSize))
        {
            mTotalAllocatedSizeTree += alignedSize;
            return tree_add_block(mem, alignedSize);
        }
        return nullptr;
    }

    HpAllocator::block_header* HpAllocator::tree_extract(size_t size)
    {
        // search the tree and get the smallest fitting block
        free_node_tree::iterator it = mFreeTree.lower_bound(size);
        if (it == mFreeTree.end())
        {
            return nullptr;
        }
        free_node* bestNode = it->next(); // improves removal time
        block_header* bestBlock = bestNode->get_block();
        tree_detach(bestBlock);
        return bestBlock;
    }

    HpAllocator::block_header* HpAllocator::tree_extract_aligned(size_t size, size_t alignment)
    {
        // get the sequence of nodes from size to (size + alignment - 1) including
        size_t sizeUpper = size + alignment;
        free_node_tree::iterator bestNode = mFreeTree.lower_bound(size);
        free_node_tree::iterator lastNode = mFreeTree.upper_bound(sizeUpper);
        while (bestNode != lastNode)
        {
            free_node* node = &*bestNode;
            size_t alignmentOffs = AZ::PointerAlignUp((char*)node, alignment) - (char*)node;
            if (node->get_block()->size() >= size + alignmentOffs)
            {
                break;
            }
            // keep walking the sequence till we find a match or reach the end
            // the larger the alignment the more linear searching will be done
            ++bestNode;
        }
        if (bestNode == mFreeTree.end())
        {
            return nullptr;
        }
        if (bestNode == lastNode)
        {
            bestNode = bestNode->next(); // improves removal time
        }
        block_header* bestBlock = bestNode->get_block();
        tree_detach(bestBlock);
        return bestBlock;
    }

    HpAllocator::block_header* HpAllocator::tree_extract_bucket_page()
    {
        return tree_extract_aligned(m_poolPageSize, m_poolPageSize);
    }

    void HpAllocator::tree_attach(block_header* bl)
    {
        mFreeTree.insert((free_node*)bl->mem());
    }

    void HpAllocator::tree_detach(block_header* bl)
    {
        bl->tag(tree_tag());
        mFreeTree.erase((free_node*)bl->mem());
    }

    void* HpAllocator::tree_alloc(size_t size)
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        // modify the size to make sure we can fit the block header and free node
        if (size < sizeof(free_node))
        {
            size = sizeof(free_node);
        }
        size = AZ::SizeAlignUp(size, sizeof(block_header));
        // extract a block from the tree if found
        block_header* newBl = tree_extract(size);
        if (!newBl)
        {
            // ask the OS for more memory
            newBl = tree_grow(size);
            if (!newBl)
            {
                return NULL;
            }
        }
        HPPA_ASSERT(!newBl->used());
        HPPA_ASSERT(newBl && newBl->size() >= size);
        // chop off any remainder
        if (newBl->size() >= size + sizeof(block_header) + sizeof(free_node))
        {
            split_block(newBl, size);
            tree_attach(newBl->next());
        }
        newBl->set_used();
        return newBl->mem();
    }

    void* HpAllocator::tree_alloc_aligned(size_t size, size_t alignment)
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        if (size < sizeof(free_node))
        {
            size = sizeof(free_node);
        }
        size = AZ::SizeAlignUp(size, sizeof(block_header));
        block_header* newBl = tree_extract_aligned(size, alignment);
        if (!newBl)
        {
            newBl = tree_grow(size + alignment);
            if (!newBl)
            {
                return nullptr;
            }
        }
        HPPA_ASSERT(newBl && newBl->size() >= size);
        size_t alignmentOffs = AZ::PointerAlignUp((char*)newBl->mem(), alignment) - (char*)newBl->mem();
        HPPA_ASSERT(newBl->size() >= size + alignmentOffs);
        if (alignmentOffs >= sizeof(block_header) + sizeof(free_node))
        {
            split_block(newBl, alignmentOffs - sizeof(block_header));
            tree_attach(newBl);
            newBl = newBl->next();
        }
        else if (alignmentOffs > 0)
        {
            newBl = shift_block(newBl, alignmentOffs);
        }
        if (newBl->size() >= size + sizeof(block_header) + sizeof(free_node))
        {
            split_block(newBl, size);
            tree_attach(newBl->next());
        }
        newBl->set_used();
        HPPA_ASSERT(((size_t)newBl->mem() & (alignment - 1)) == 0);
        return newBl->mem();
    }

    void* HpAllocator::tree_alloc_bucket_page()
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        // We are allocating pool pages m_poolPageSize aligned at m_poolPageSize
        // what is special is that we are keeping the block_header at the beginning of the
        // memory and we return the offset pointer.
        size_t size = m_poolPageSize;
        size_t alignment = m_poolPageSize;
        block_header* newBl = tree_extract_bucket_page();
        if (!newBl)
        {
            newBl = tree_grow(size + alignment);
            if (!newBl)
            {
                return nullptr;
            }
        }
        HPPA_ASSERT(newBl && (newBl->size() + sizeof(block_header)) >= size);
        size_t alignmentOffs = AZ::PointerAlignUp((char*)newBl, alignment) - (char*)newBl;
        HPPA_ASSERT(newBl->size() + sizeof(block_header) >= size + alignmentOffs);
        if (alignmentOffs >= sizeof(block_header) + sizeof(free_node))
        {
            split_block(newBl, alignmentOffs - sizeof(block_header));
            tree_attach(newBl);
            newBl = newBl->next();
        }
        else if (alignmentOffs > 0)
        {
            newBl = shift_block(newBl, alignmentOffs);
        }
        if (newBl->size() >= (size - sizeof(block_header)) + sizeof(block_header) + sizeof(free_node))
        {
            split_block(newBl, size - sizeof(block_header));
            tree_attach(newBl->next());
        }
        newBl->set_used();
        HPPA_ASSERT(((size_t)newBl & (alignment - 1)) == 0);
        return newBl;
    }

    void* HpAllocator::tree_realloc(void* ptr, size_t size)
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        if (size < sizeof(free_node))
        {
            size = sizeof(free_node);
        }
        size = AZ::SizeAlignUp(size, sizeof(block_header));
        block_header* bl = ptr_get_block_header(ptr);
        size_t blSize = bl->size();
        if (blSize >= size)
        {
            // the block is being shrunk, truncate and mark the remainder as free
            if (blSize >= size + sizeof(block_header) + sizeof(free_node))
            {
                split_block(bl, size);
                block_header* next = bl->next();
                next = coalesce_block(next);
                tree_attach(next);
            }
            HPPA_ASSERT(bl->size() >= size);
            return ptr;
        }
        // check if the following block is free and if it can accommodate the requested size
        block_header* next = bl->next();
        size_t nextSize = next->used() ? 0 : next->size() + sizeof(block_header);
        if (blSize + nextSize >= size)
        {
            HPPA_ASSERT(!next->used());
            tree_detach(next);
            next->unlink();
            HPPA_ASSERT(bl->size() >= size);
            if (bl->size() >= size + sizeof(block_header) + sizeof(free_node))
            {
                split_block(bl, size);
                tree_attach(bl->next());
            }
            return ptr;
        }
        // check if the previous block can be used to avoid searching
        block_header* prev = bl->prev();
        size_t prevSize = prev->used() ? 0 : prev->size() + sizeof(block_header);
        if (blSize + prevSize + nextSize >= size)
        {
            HPPA_ASSERT(!prev->used());
            tree_detach(prev);
            bl->unlink();
            if (!next->used())
            {
                tree_detach(next);
                next->unlink();
            }
            bl = prev;
            bl->set_used();
            HPPA_ASSERT(bl->size() >= size);
            void* newPtr = bl->mem();
            // move the memory before we split the block
            memmove(newPtr, ptr, blSize - MEMORY_GUARD_SIZE);
            if (bl->size() >= size + sizeof(block_header) + sizeof(free_node))
            {
                split_block(bl, size);
                tree_attach(bl->next());
            }
            return newPtr;
        }
        // fall back to alloc/copy/free
        void* newPtr = tree_alloc(size);
        if (newPtr)
        {
            memcpy(newPtr, ptr, blSize - MEMORY_GUARD_SIZE);
            tree_free(ptr);
            return newPtr;
        }
        return NULL;
    }

    void* HpAllocator::tree_realloc_aligned(void* ptr, size_t size, size_t alignment)
    {
        HPPA_ASSERT(((size_t)ptr & (alignment - 1)) == 0);
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        if (size < sizeof(free_node))
        {
            size = sizeof(free_node);
        }
        size = AZ::SizeAlignUp(size, sizeof(block_header));
        block_header* bl = ptr_get_block_header(ptr);
        size_t blSize = bl->size();
        if (blSize >= size)
        {
            if (blSize >= size + sizeof(block_header) + sizeof(free_node))
            {
                split_block(bl, size);
                block_header* next = bl->next();
                next = coalesce_block(next);
                tree_attach(next);
            }
            HPPA_ASSERT(bl->size() >= size);
            return ptr;
        }
        block_header* next = bl->next();
        size_t nextSize = next->used() ? 0 : next->size() + sizeof(block_header);
        if (blSize + nextSize >= size)
        {
            HPPA_ASSERT(!next->used());
            tree_detach(next);
            next->unlink();
            HPPA_ASSERT(bl->size() >= size);
            if (bl->size() >= size + sizeof(block_header) + sizeof(free_node))
            {
                split_block(bl, size);
                tree_attach(bl->next());
            }
            return ptr;
        }
        block_header* prev = bl->prev();
        size_t prevSize = prev->used() ? 0 : prev->size() + sizeof(block_header);
        size_t alignmentOffs = prev->used() ? 0 : AZ::PointerAlignUp((char*)prev->mem(), alignment) - (char*)prev->mem();
        if (blSize + prevSize + nextSize >= size + alignmentOffs)
        {
            HPPA_ASSERT(!prev->used());
            tree_detach(prev);
            bl->unlink();
            if (!next->used())
            {
                tree_detach(next);
                next->unlink();
            }
            if (alignmentOffs >= sizeof(block_header) + sizeof(free_node))
            {
                split_block(prev, alignmentOffs - sizeof(block_header));
                tree_attach(prev);
                prev = prev->next();
            }
            else if (alignmentOffs > 0)
            {
                prev = shift_block(prev, alignmentOffs);
            }
            bl = prev;
            bl->set_used();
            HPPA_ASSERT(bl->size() >= size && ((size_t)bl->mem() & (alignment - 1)) == 0);
            void* newPtr = bl->mem();
            memmove(newPtr, ptr, blSize - MEMORY_GUARD_SIZE);
            if (bl->size() >= size + sizeof(block_header) + sizeof(free_node))
            {
                split_block(bl, size);
                tree_attach(bl->next());
            }
            return newPtr;
        }
        void* newPtr = tree_alloc_aligned(size, alignment);
        if (newPtr)
        {
            memcpy(newPtr, ptr, blSize - MEMORY_GUARD_SIZE);
            tree_free(ptr);
            return newPtr;
        }
        return NULL;
    }

    size_t HpAllocator::tree_resize(void* ptr, size_t size)
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        if (size < sizeof(free_node))
        {
            size = sizeof(free_node);
        }
        size = AZ::SizeAlignUp(size, sizeof(block_header));
        block_header* bl = ptr_get_block_header(ptr);
        size_t blSize = bl->size();
        if (blSize >= size)
        {
            if (blSize >= size + sizeof(block_header) + sizeof(free_node))
            {
                split_block(bl, size);
                block_header* next = bl->next();
                next = coalesce_block(next);
                tree_attach(next);
            }
            HPPA_ASSERT(bl->size() >= size);
        }
        else
        {
            block_header* next = bl->next();
            if (!next->used() && blSize + next->size() + sizeof(block_header) >= size)
            {
                tree_detach(next);
                next->unlink();
                if (bl->size() >= size + sizeof(block_header) + sizeof(free_node))
                {
                    split_block(bl, size);
                    tree_attach(bl->next());
                }
                HPPA_ASSERT(bl->size() >= size);
            }
        }
        return bl->size();
    }

    void HpAllocator::tree_free(void* ptr)
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        block_header* bl = ptr_get_block_header(ptr);
        AZ_Assert(bl->used(), "Double free detected of address 0x%p", ptr);
        AZ_Assert(bl->tag() == tree_tag(), "Attempted to free address 0x%p which does not belong to this allocator", ptr);
        bl->set_unused();
        bl = coalesce_block(bl);
        tree_attach(bl);
    }

    void HpAllocator::tree_free_bucket_page(void* ptr)
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        HPPA_ASSERT(AZ::PointerAlignDown(ptr, m_poolPageSize) == ptr);
        block_header* bl = (block_header*)ptr;
        HPPA_ASSERT(bl->size() >= m_poolPageSize - sizeof(block_header));
        bl->set_unused();
        bl = coalesce_block(bl);
        tree_attach(bl);
    }

    void HpAllocator::tree_purge_block(block_header* bl)
    {
        HPPA_ASSERT(!bl->used());
        HPPA_ASSERT(bl->prev() && bl->prev()->used());
        HPPA_ASSERT(bl->next() && bl->next()->used());
        if (bl->prev()->prev() == NULL && bl->next()->size() == 0)
        {
            tree_detach(bl);
            char* memStart = (char*)bl->prev();
            char* memEnd = (char*)bl->mem() + bl->size() + sizeof(block_header);
            void* mem = memStart;
            size_t size = memEnd - memStart;
            HPPA_ASSERT(((size_t)mem & (m_treePageAlignment - 1)) == 0);
            memset(mem, 0xff, sizeof(block_header));
            mTotalAllocatedSizeTree -= size;
            tree_system_free(mem, size);
        }
        //else if( m_fixedBlock )
        //{
        //  size_t requiredFrontSize = 2*sizeof(block_header) + sizeof(free_node); // 1 fake start (prev()->prev()) and one free block
        //  size_t requiredEndSize = 2*sizeof(block_header) + sizeof(free_node); // 1 fake start (prev()->prev()) and one free block
        //  if( bl->prev()->prev() == NULL )
        //  {
        //      char* alignedEnd = AZ::PointerAlignDown((char*)bl->next()-requiredFrontSize,m_treePageSize);
        //      char* memStart = (char*)bl->prev();
        //      if( alignedEnd > memStart )
        //      {
        //          tree_detach(bl);
        //          block_header* frontFence = (block_header*)alignedEnd;
        //          frontFence->prev(0);
        //          frontFence->size(0);
        //          frontFence->set_used();
        //          block_header* frontFreeBlock = (block_header*)frontFence->mem();
        //          frontFreeBlock->prev(frontFence);
        //          frontFreeBlock->set_unused();
        //          frontFreeBlock->next(bl->next()); // set's the size
        //          bl->next()->prev(frontFreeBlock);
        //          tree_attach(frontFreeBlock);

        //          memset(memStart,0xff,sizeof(block_header));
        //          tree_system_free(memStart, alignedEnd - memStart);
        //      }
        //  }
        //  else if( bl->next()->size() == 0)
        //  {
        //      char* alignedStart = AZ::PointerAlignUp((char*)bl+requiredEndSize,m_treePageSize);
        //      char* memEnd = (char*)bl->next()+sizeof(block_header);
        //      if( alignedStart < memEnd )
        //      {
        //          tree_detach(bl);
        //          block_header* backFence = (block_header*)(alignedStart-sizeof(block_header));
        //          block_header* backFreeBlock = bl;
        //          backFreeBlock->next(backFence); // set's the size
        //
        //          backFence->prev(backFreeBlock);
        //          backFence->size(0);
        //          backFence->set_used();

        //          tree_attach(backFreeBlock);
        //
        //          memset(alignedStart,0xff,sizeof(block_header));
        //          tree_system_free(alignedStart, memEnd - alignedStart);
        //      }
        //  }
        //  else
        //  {
        //      char* alignedStart = AZ::PointerAlignUp((char*)bl+requiredEndSize,m_treePageSize);
        //      char* alignedEnd = AZ::PointerAlignDown((char*)bl->next()-requiredFrontSize,m_treePageSize);
        //      if( alignedStart < alignedEnd )
        //      {
        //          tree_detach(bl);

        //          //
        //          block_header* frontFence = (block_header*)alignedEnd;
        //          frontFence->prev(0);
        //          frontFence->size(0);
        //          frontFence->set_used();
        //          block_header* frontFreeBlock = (block_header*)frontFence->mem();
        //          frontFreeBlock->prev(frontFence);
        //          frontFreeBlock->set_unused();
        //          frontFreeBlock->next(bl->next()); // set's the size
        //          bl->next()->prev(frontFreeBlock);
        //          tree_attach(frontFreeBlock);

        //          //
        //          block_header* backFence = (block_header*)(alignedStart-sizeof(block_header));
        //          block_header* backFreeBlock = bl;
        //          backFreeBlock->next(backFence); // set's the size

        //          backFence->prev(backFreeBlock);
        //          backFence->size(0);
        //          backFence->set_used();
        //          tree_attach(backFreeBlock);

        //          memset(alignedStart,0xff,sizeof(block_header));
        //          tree_system_free(alignedStart, alignedEnd - alignedStart);
        //      }
        //  }
        //}
    }

    size_t HpAllocator::tree_ptr_size(void* ptr) const
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        block_header* bl = ptr_get_block_header(ptr);
        if (bl->used()) // add a magic number to avoid better bad pointer data.
        {
            return bl->size();
        }
        else
        {
            return 0;
        }
    }

    size_t HpAllocator::tree_get_max_allocation() const
    {
        return mFreeTree.maximum()->get_block()->size();
    }

    size_t HpAllocator::tree_get_unused_memory(bool isPrint) const
    {
        size_t unusedMemory = 0;
        for (free_node_tree::const_iterator it = mFreeTree.begin(); it != mFreeTree.end(); ++it)
        {
            unusedMemory += it->get_block()->size();
            if (isPrint)
            {
                AZ_TracePrintf("System", "Unused Treenode %p size: %d\n", it->get_block()->mem(), it->get_block()->size());
            }
        }
        return unusedMemory;
    }

    void HpAllocator::tree_purge()
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        free_node_tree::iterator node = mFreeTree.begin();
        free_node_tree::iterator end = mFreeTree.end();
        while (node != end)
        {
            block_header* cur = node->get_block();
            ++node;
            if (cur->prev() != m_fixedBlock) // check we are not purging the fixed block
            {
                tree_purge_block(cur);
            }
        }
    }

    //=========================================================================
    // allocationSize
    // [2/22/2011]
    //=========================================================================
    size_t
    HpAllocator::AllocationSize(void* ptr)
    {
        if (m_fixedBlock)
        {
            if (ptr < m_fixedBlock && ptr >= (char*)m_fixedBlock + m_fixedBlockSize)
            {
                return 0;
            }
        }
        return size(ptr);
    }

    //=========================================================================
    // GetMaxAllocationSize
    // [2/22/2011]
    //=========================================================================
    size_t
    HpAllocator::GetMaxAllocationSize() const
    {
        const_cast<HpAllocator*>(this)->purge(); // slow

        size_t maxSize = 0;
        maxSize = AZStd::GetMax(bucket_get_max_allocation(), maxSize);
        maxSize = AZStd::GetMax(tree_get_max_allocation(), maxSize);
        return maxSize;
    }

    //=========================================================================
    // GetUnAllocatedMemory
    // [9/30/2013]
    //=========================================================================
    size_t
    HpAllocator::GetUnAllocatedMemory(bool isPrint) const
    {
        return bucket_get_unused_memory(isPrint) + tree_get_unused_memory(isPrint);
    }

    //=========================================================================
    // SystemAlloc
    // [2/22/2011]
    //=========================================================================
    void*
    HpAllocator::SystemAlloc(size_t size, size_t align)
    {
        if (m_subAllocator)
        {
            return m_subAllocator->Allocate(size, align, 0, "HphaSchema sub allocation", __FILE__, __LINE__);
        }
#if AZ_TRAIT_OS_ALLOW_DIRECT_ALLOCATIONS
#   if AZ_TRAIT_OS_ENFORCE_STRICT_VIRTUAL_ALLOC_ALIGNMENT
        AZ_Assert(align % OS_VIRTUAL_PAGE_SIZE == 0, "Invalid allocation/page alignment %d should be a multiple of %d!", size, OS_VIRTUAL_PAGE_SIZE);
        return AZ_OS_MALLOC(size, align);
#   else
        (void)align;
        size = AZ::SizeAlignUp(size, OS_VIRTUAL_PAGE_SIZE);
        return VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
#   endif
#else
        return memalign(align, size);
#endif
    }

    //=========================================================================
    // SystemFree
    // [2/22/2011]
    //=========================================================================
    void
    HpAllocator::SystemFree(void* ptr)
    {
        if (m_subAllocator)
        {
            m_subAllocator->DeAllocate(ptr);
            return;
        }
#if defined(AZ_PLATFORM_WINDOWS)
        BOOL ret = VirtualFree(ptr, 0, MEM_RELEASE);
        (void)ret;
        AZ_Assert(ret, "Failed to free memory!");
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/HphaSchema_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/HphaSchema_cpp_provo.inl"
    #endif
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
        ::free(ptr);
#endif
    }

#ifdef DEBUG_ALLOCATOR

    const void HpAllocator::debug_record::print_stack() const
    {
        AZ::Debug::SymbolStorage::StackLine stackLines[MAX_CALLSTACK_DEPTH];
        AZ::Debug::SymbolStorage::DecodeFrames(mCallStack, MAX_CALLSTACK_DEPTH, stackLines);
#ifdef AZ_TRAIT_COMPILER_USE_OUTPUT_DEBUG_STRING
        for (int i = 0; i < MAX_CALLSTACK_DEPTH; ++i)
        {
            OutputDebugString(stackLines[i]);
            OutputDebugString("\n");
        }
#endif
    }

    void HpAllocator::debug_record::record_stack()
    {
        AZ::Debug::StackRecorder::Record(mCallStack, MAX_CALLSTACK_DEPTH, 6);
    }

    void HpAllocator::debug_record::write_guard()
    {
        unsigned char guardByte = (unsigned char)rand();
        unsigned char* guard = (unsigned char*)mPtr + mSize;
        mGuardByte = guardByte;
        for (unsigned i = 0; i < MEMORY_GUARD_SIZE; i++)
        {
            guard[i] = guardByte++;
        }
    }

    bool HpAllocator::debug_record::check_guard() const
    {
        unsigned char guardByte = mGuardByte;
        unsigned char* guard = (unsigned char*)mPtr + mSize;
        for (unsigned i = 0; i < MEMORY_GUARD_SIZE; i++)
        {
            if (guardByte++ != guard[i])
            {
                return false;
            }
        }
        return true;
    }

    void HpAllocator::debug_record_map::initial_fill(void* ptr, size_t size)
    {
        unsigned char sFiller[] = {0xFF, 0xC0, 0xC0, 0xFF}; // QNAN (little OR big endian)
        unsigned char* p = (unsigned char*)ptr;
        for (size_t s = 0; s < size; s++)
        {
            p[s] = sFiller[s % sizeof(sFiller) / sizeof(sFiller[0])];
        }
    }

#define HPPA_ASSERT_PRINT_STACK(cond, it)   {if(!cond) it->print_stack(); HPPA_ASSERT(cond);}

    void HpAllocator::debug_record_map::add(void* ptr, size_t size, debug_source source)
    {
        const auto it = this->emplace(debug_record(ptr, size, source));
        // make sure this record is unique
        HPPA_ASSERT_PRINT_STACK(it.second, it.first);
        initial_fill(ptr, size);
    }

    HpAllocator::debug_info HpAllocator::debug_record_map::remove(void* ptr)
    {
        const_iterator it = this->find(debug_record(ptr));
        // if this asserts most likely the pointer was already deleted
        // or the pointer points to a static or a global variable.
        HPPA_ASSERT(it != this->end());
        // if this asserts then the memory was corrupted past the end of the block
        HPPA_ASSERT_PRINT_STACK(it->check_guard(), it);
        const size_t size = it->size();
        const debug_source source = it->source();
        initial_fill(ptr, size);
        this->erase(it);
        return debug_info(size, source);
    }

    HpAllocator::debug_info HpAllocator::debug_record_map::remove(void* ptr, size_t size)
    {
        const_iterator it = this->find(debug_record(ptr));
        // if this asserts most likely the pointer was already deleted
        // or the pointer points to a static or a global variable.
        HPPA_ASSERT(it != this->end());
        // make sure the free size matches the allocation size
        HPPA_ASSERT(size == it->size());
        // if this asserts then the memory was corrupted past the end of the block
        HPPA_ASSERT_PRINT_STACK(it->check_guard(), it);
        debug_source source = it->source();
        initial_fill(ptr, it->size());
        this->erase(it);
        return debug_info(size, source);
    }

    HpAllocator::debug_info HpAllocator::debug_record_map::replace(void* ptr, void* newPtr, size_t size, debug_source source)
    {
        const_iterator it = this->find(debug_record(ptr));
        // if this asserts most likely the pointer was already deleted
        HPPA_ASSERT(it != this->end());
        HPPA_ASSERT_PRINT_STACK(it->check_guard(), it);
        const size_t origSize = it->size();
        const debug_source origSource = it->source();
        this->erase(it);
        const auto itPair = this->emplace(newPtr, size, source); // this will record the stack and place a new guard
        HPPA_ASSERT(itPair.second);
        return debug_info(origSize, origSource);
    }

    HpAllocator::debug_info HpAllocator::debug_record_map::update(void* ptr, size_t size)
    {
        iterator it = this->find(debug_record(ptr));
        // if this asserts most likely the pointer was already deleted
        HPPA_ASSERT(it != this->end());
        const size_t origSize = it->size();
        HPPA_ASSERT_PRINT_STACK(it->check_guard(), it);
        it->set_size(size);
        it->record_stack();
        it->write_guard();
        return debug_info(origSize, it->source());
    }

    void HpAllocator::debug_record_map::check(void* ptr) const
    {
        const_iterator it = this->find(debug_record(ptr));
        // if this asserts most likely the pointer was already deleted
        HPPA_ASSERT(it != this->end());
        // if this asserts then the memory was corrupted past the end of the block
        HPPA_ASSERT_PRINT_STACK(it->check_guard(), it);
    }

    void HpAllocator::debug_record_map::purge()
    {
    }

    void* HpAllocator::debug_add(void* ptr, size_t size, debug_source source)
    {
        if (ptr)
        {
#ifdef MULTITHREADED
            AZStd::lock_guard<AZStd::mutex> lock(mDebugMutex);
#endif
            mDebugMap.add(ptr, size, source);
            if (source == DEBUG_SOURCE_BUCKETS)
            {
                mTotalRequestedSizeBuckets += size + MEMORY_GUARD_SIZE;
            }
            else
            {
                mTotalRequestedSizeTree += size + MEMORY_GUARD_SIZE;
            }
            HPPA_ASSERT(size <= this->size(ptr));
            return ptr;
        }
        return NULL;
    }

    void HpAllocator::debug_remove(void* ptr)
    {
    #ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::mutex> lock(mDebugMutex);
    #endif
        debug_info info = mDebugMap.remove(ptr);
        if (info.source == DEBUG_SOURCE_BUCKETS)
        {
            mTotalRequestedSizeBuckets -= info.size + MEMORY_GUARD_SIZE;
        }
        else
        {
            mTotalRequestedSizeTree -= info.size + MEMORY_GUARD_SIZE;
        }
    }

    void HpAllocator::debug_remove(void* ptr, size_t size)
    {
    #ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::mutex> lock(mDebugMutex);
    #endif
        debug_info info = mDebugMap.remove(ptr, size);
        if (info.source == DEBUG_SOURCE_BUCKETS)
        {
            mTotalRequestedSizeBuckets -= info.size + MEMORY_GUARD_SIZE;
        }
        else
        {
            mTotalRequestedSizeTree -= info.size + MEMORY_GUARD_SIZE;
        }
    }

    void HpAllocator::debug_replace(void* ptr, void* newPtr, size_t size, debug_source source)
    {
        if (!newPtr)
        {
            return;
        }
    #ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::mutex> lock(mDebugMutex);
    #endif
        debug_info info = mDebugMap.replace(ptr, newPtr, size, source);
        HPPA_ASSERT(size <= this->size(newPtr));
        if (info.source == DEBUG_SOURCE_BUCKETS)
        {
            mTotalRequestedSizeBuckets -= info.size + MEMORY_GUARD_SIZE;
        }
        else
        {
            mTotalRequestedSizeTree -= info.size + MEMORY_GUARD_SIZE;
        }
        if (source == DEBUG_SOURCE_BUCKETS)
        {
            mTotalRequestedSizeBuckets += size + MEMORY_GUARD_SIZE;
        }
        else
        {
            mTotalRequestedSizeTree += size + MEMORY_GUARD_SIZE;
        }
    }

    void HpAllocator::debug_update(void* ptr, size_t size)
    {
    #ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::mutex> lock(mDebugMutex);
    #endif
        debug_info info = mDebugMap.update(ptr, size);
        HPPA_ASSERT(size <= this->size(ptr));
        if (info.source == DEBUG_SOURCE_BUCKETS)
        {
            mTotalRequestedSizeBuckets += size - info.size;
        }
        else
        {
            mTotalRequestedSizeTree += size - info.size;
        }
    }

    void HpAllocator::debug_check(void* ptr) const
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::mutex> lock(mDebugMutex);
#endif
        mDebugMap.check(ptr);
    }

    void HpAllocator::debug_purge()
    {
    #ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::mutex> lock(mDebugMutex);
    #endif
        mDebugMap.purge();
    }

    void HpAllocator::check()
    {
    #ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::mutex> lock(mDebugMutex);
    #endif
        for (debug_record_map::const_iterator it = mDebugMap.begin(); it != mDebugMap.end(); ++it)
        {
            HPPA_ASSERT(it->size() <= size(it->ptr()));
            HPPA_ASSERT(it->check_guard());
        }
    }

    void HpAllocator::report()
    {
    #ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::mutex> lock(mDebugMutex);
    #endif
        AZ_TracePrintf("HPHA", "REPORT =================================================\n");
        AZ_TracePrintf("HPHA", "Total requested size=%zi bytes\n", mTotalRequestedSizeBuckets + mTotalRequestedSizeTree);
        AZ_TracePrintf("HPHA", "Total allocated size=%zi bytes\n", mTotalAllocatedSizeBuckets + mTotalAllocatedSizeTree);
        AZ_TracePrintf("HPHA", "Currently allocated blocks:\n");
        for (debug_record_map::const_iterator it = mDebugMap.begin(); it != mDebugMap.end(); ++it)
        {
            AZ_TracePrintf("HPHA", "ptr=%zX, size=%zi\n", (size_t)it->ptr(), it->size());
            it->print_stack();
        }
        AZ_TracePrintf("HPHA", "===========================================================\n");
    }
#endif // DEBUG_ALLOCATOR


    //=========================================================================
    // HphaScema
    // [2/22/2011]
    //=========================================================================
    HphaSchema::HphaSchema(const Descriptor& desc)
    {
#if defined(AZ_OS64)
        (void)m_pad;
#endif // AZ_OS64
        m_capacity = 0;

        m_desc = desc;
        m_ownMemoryBlock = false;

        if (m_desc.m_fixedMemoryBlockByteSize > 0)
        {
            AZ_Assert((m_desc.m_fixedMemoryBlockByteSize & (m_desc.m_pageSize - 1)) == 0, "Memory block size %d MUST be multiples of the of the page size %d!", m_desc.m_fixedMemoryBlockByteSize, m_desc.m_pageSize);
            if (m_desc.m_fixedMemoryBlock == NULL)
            {
                AZ_Assert(m_desc.m_subAllocator != NULL, "Sub allocator must point to a valid allocator if m_memoryBlock is NOT allocated (NULL)!");
                m_desc.m_fixedMemoryBlock = m_desc.m_subAllocator->Allocate(m_desc.m_fixedMemoryBlockByteSize, m_desc.m_memoryBlockAlignment, 0, "HphaSchema", __FILE__, __LINE__, 1);
                AZ_Assert(m_desc.m_fixedMemoryBlock != NULL, "Failed to allocate %d bytes!", m_desc.m_fixedMemoryBlockByteSize);
                m_ownMemoryBlock = true;
            }
            AZ_Assert((reinterpret_cast<size_t>(m_desc.m_fixedMemoryBlock) & static_cast<size_t>(desc.m_memoryBlockAlignment - 1)) == 0, "Memory block must be page size (%d bytes) aligned!", desc.m_memoryBlockAlignment);
            m_capacity = m_desc.m_fixedMemoryBlockByteSize;
        }
        else
        {
            m_capacity = desc.m_capacity;
        }

        AZ_Assert(sizeof(HpAllocator) <= sizeof(m_hpAllocatorBuffer), "Increase the m_hpAllocatorBuffer, we need %d bytes but we have %d bytes!", sizeof(HpAllocator), sizeof(m_hpAllocatorBuffer));
        // Fix SystemAllocator from growing in small chunks
        m_allocator = new (&m_hpAllocatorBuffer) HpAllocator(m_desc);
    }

    //=========================================================================
    // ~HphaSchema
    // [2/22/2011]
    //=========================================================================
    HphaSchema::~HphaSchema()
    {
        m_capacity = 0;
        m_allocator->~HpAllocator();

        if (m_ownMemoryBlock)
        {
            m_desc.m_subAllocator->DeAllocate(m_desc.m_fixedMemoryBlock, m_desc.m_fixedMemoryBlockByteSize, m_desc.m_memoryBlockAlignment);
            m_desc.m_fixedMemoryBlock = NULL;
        }
    }

    //=========================================================================
    // Allocate
    // [2/22/2011]
    //=========================================================================
    HphaSchema::pointer_type
    HphaSchema::Allocate(size_type byteSize, size_type alignment, int flags, const char* name, const char* fileName, int lineNum, unsigned int suppressStackRecord)
    {
        (void)flags;
        (void)name;
        (void)fileName;
        (void)lineNum;
        (void)suppressStackRecord;
        pointer_type address = m_allocator->alloc(byteSize, alignment);
        if (address == NULL)
        {
            GarbageCollect();
            address = m_allocator->alloc(byteSize, alignment);
        }
        return address;
    }

    //=========================================================================
    // pointer_type
    // [2/22/2011]
    //=========================================================================
    HphaSchema::pointer_type
    HphaSchema::ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment)
    {
        pointer_type address = m_allocator->realloc(ptr, newSize, newAlignment);
        if (address == NULL && newSize > 0)
        {
            GarbageCollect();
            address = m_allocator->realloc(ptr, newSize, newAlignment);
        }
        return address;
    }

    //=========================================================================
    // DeAllocate(pointer_type ptr,size_type size,size_type alignment)
    // [2/22/2011]
    //=========================================================================
    void
    HphaSchema::DeAllocate(pointer_type ptr, size_type size, size_type alignment)
    {
        if (ptr == 0)
        {
            return;
        }
        if (size == 0)
        {
            m_allocator->free(ptr);
        }
        else if (alignment == 0)
        {
            m_allocator->free(ptr, size);
        }
        else
        {
            m_allocator->free(ptr, size, alignment);
        }
    }

    //=========================================================================
    // Resize
    // [2/22/2011]
    //=========================================================================
    HphaSchema::size_type
    HphaSchema::Resize(pointer_type ptr, size_type newSize)
    {
        return m_allocator->resize(ptr, newSize);
    }

    //=========================================================================
    // AllocationSize
    // [2/22/2011]
    //=========================================================================
    HphaSchema::size_type
    HphaSchema::AllocationSize(pointer_type ptr)
    {
        return m_allocator->AllocationSize(ptr);
    }

    //=========================================================================
    // NumAllocatedBytes
    // [2/22/2011]
    //=========================================================================
    HphaSchema::size_type
    HphaSchema::NumAllocatedBytes() const
    {
        return m_allocator->allocated();
    }


    //=========================================================================
    // GetMaxAllocationSize
    // [2/22/2011]
    //=========================================================================
    HphaSchema::size_type
    HphaSchema::GetMaxAllocationSize() const
    {
        return m_allocator->GetMaxAllocationSize();
    }

    //=========================================================================
    // GetUnAllocatedMemory
    // [9/30/2013]
    //=========================================================================
    HphaSchema::size_type
    HphaSchema::GetUnAllocatedMemory(bool isPrint) const
    {
        return m_allocator->GetUnAllocatedMemory(isPrint);
    }

    //=========================================================================
    // GarbageCollect
    // [2/22/2011]
    //=========================================================================
    void
    HphaSchema::GarbageCollect()
    {
        m_allocator->purge();
    }
} // namspace AZ

#endif // #ifndef AZ_UNITY_BUILD
