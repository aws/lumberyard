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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYACTION_SERIALIZATION_XMLCPBIN_WRITER_XMLCPB_BUFFERWRITER_H
#define CRYINCLUDE_CRYACTION_SERIALIZATION_XMLCPBIN_WRITER_XMLCPB_BUFFERWRITER_H
#pragma once

#include "../XMLCPB_Common.h"
#include "IPlatformOS.h"

namespace XMLCPB {
    class CWriter;

    class CBufferWriter
    {
    public:
        struct SAddr
        {
            uint16  bufferOffset;
            uint16  bufferIndex;

            SAddr() { Reset(); }

            void Reset()
            {
                bufferOffset = 0xffff;
                bufferIndex = 0xffff;
            }

            bool IsValid()
            {
                return bufferOffset != 0xffff || bufferIndex != 0xffff;
            }
        };

    public:

        CBufferWriter(int bufferSize);
        void Init(CWriter* pWriter, bool useStreaming);

        void AddData(const void* pSource, int sizeToAdd);
        template<class T>
        void AddDataEndianAware(T val);
        void AddDataNoSplit(SAddr& addr, const void* pSource, int sizeToAdd);
        void GetCurrentAddr(SAddr& addr) const;
        uint32 GetUsedMemory() const;
        FlatAddr ConvertSegmentedAddrToFlatAddr(const SAddr& addr);
        void WriteToFile();
        void WriteToMemory(uint8*& rpData, uint32& outWriteLoc);
        uint32 GetDataSize() const;

        ILINE uint8* GetPointerFromAddr(const SAddr& addr) const
        {
            assert(!m_usingStreaming);
            //if (m_usingStreaming)
            //  return NULL;

            assert(addr.bufferIndex < m_buffers.size());
            return m_buffers[addr.bufferIndex]->GetPointer(addr.bufferOffset);
        }

    private:

        void AddBuffer();

        struct SSimpleBuffer
            : public _reference_target_t
        {
        public:
            SSimpleBuffer(int size)
                : m_pBuffer(NULL)
                , m_size(size)
                , m_used(0)
            {
                m_pBuffer = new uint8[size];
            }

            ~SSimpleBuffer()
            {
                FreeBuffer();
            }

            bool        CanAddData(int sizeToAdd) const { return m_used + sizeToAdd <= m_size; }
            bool        IsFull() const { return m_used == m_size; }
            void        AddData(const uint8* pSource, int sizeToAdd);
            int         GetFreeSpaceSize() const { return m_size - m_used; }
            uint32  GetUsed() const { return m_used; }
            uint8*  GetPointer(uint32 offset) const { assert(offset < m_used); return GetBasePointer() + offset; }
            void        WriteToFile(CWriter& Writer);
            void        WriteToMemory(CWriter& Writer, uint8*& rpData, uint32& outWriteLoc);
            uint32  GetDataSize() const { return m_used; }
            uint8*  GetBasePointer() const { return m_pBuffer; }
            void        FreeBuffer() { SAFE_DELETE_ARRAY(m_pBuffer); }

        private:
            uint8* m_pBuffer;
            uint32 m_used;
            uint32 m_size;
        };



    private:
        typedef _smart_ptr<SSimpleBuffer> SSimpleBufferT;
        std::vector<SSimpleBufferT >        m_buffers;
        CWriter*                                                m_pWriter;
        int                                                         m_bufferSize;
        bool                                                        m_usingStreaming;
    };

    template<class T>
    inline void CBufferWriter::AddDataEndianAware(T val)
    {
        SwapIntegerValue(val);
        AddData(&val, sizeof(val));
    }

    inline void CBufferWriter::SSimpleBuffer::AddData(const uint8* pSource, int sizeToAdd)
    {
        assert(CanAddData(sizeToAdd));

        memcpy(GetBasePointer() + m_used, pSource, sizeToAdd);

        m_used += sizeToAdd;
    }

    class AttrStringAllocatorImpl
    {
        static const int k_numPerBucket = 4096;
        static const int k_maxItems = 1024 * 1024;
        static uint64* s_buckets[k_maxItems / k_numPerBucket];
        static uint64* s_ptr;
        static uint64* s_end;
        static int s_currentBucket;
        static int s_poolInUse;
    public:
        static void LockPool()
        {
#ifndef _RELEASE
            if (gEnv->mMainThreadId != CryGetCurrentThreadId())
            {
                __debugbreak();
            }
#endif
            s_poolInUse++;
        }
        static void* AttrStringAlloc()
        {
#ifndef _RELEASE
            if (gEnv->mMainThreadId != CryGetCurrentThreadId())
            {
                __debugbreak();
            }
#endif
            uint64* ptr = s_ptr;
            if (ptr)
            {
                s_ptr++;
                if (s_ptr == s_end)
                {
                    s_ptr = NULL;
                }
            }
            else
            {
#ifndef _RELEASE
                if (s_currentBucket == sizeof(s_buckets) / sizeof(s_buckets[0]))
                {
                    __debugbreak();
                }
#endif
                ptr = (uint64*)malloc(k_numPerBucket * sizeof(uint64));
                assert(s_currentBucket >= 0 && s_currentBucket < sizeof(s_buckets) / sizeof(s_buckets[0]));
                s_buckets[s_currentBucket++] = ptr;
                s_ptr = ptr + 1;
                s_end = ptr + k_numPerBucket;
            }
            return ptr;
        }
        static void CleanPool()
        {
#ifndef _RELEASE
            if (gEnv->mMainThreadId != CryGetCurrentThreadId())
            {
                __debugbreak();
            }
#endif
            s_poolInUse--;
            if (!s_poolInUse)
            {
                for (int i = 0; i < s_currentBucket; i++)
                {
                    free(s_buckets[i]);
                }
                s_currentBucket = 0;
                s_ptr = NULL;
            }
        }
    };

    //template <class T>
    //class AttrStringAllocator
    //{
    //public:
    //  typedef size_t    size_type;
    //  typedef ptrdiff_t difference_type;
    //  typedef T*        pointer;
    //  typedef const T*  const_pointer;
    //  typedef T&        reference;
    //  typedef const T&  const_reference;
    //  typedef T         value_type;
    //
    //    typedef AZStd::false_type   allow_memory_leaks;
    //    typedef T*                  pointer_type;
    //
    //
    //  template <class U> struct rebind
    //  {
    //      typedef AttrStringAllocator<U> other;
    //  };
    //
    //  AttrStringAllocator() throw()
    //  {
    //  }
    //
    //  AttrStringAllocator(const AttrStringAllocator&) throw()
    //  {
    //  }
    //
    //  template <class U> AttrStringAllocator(const AttrStringAllocator<U>) throw()
    //  {
    //  }
    //
    //  ~AttrStringAllocator() throw()
    //  {
    //  }
    //
    //  pointer address(reference x) const
    //  {
    //      return &x;
    //  }
    //
    //  const_pointer address(const_reference x) const
    //  {
    //      return &x;
    //  }
    //
    //  pointer allocate(size_type n = 1, const void* hint = 0)
    //  {
    //      if (sizeof(T)==8 && n==1)
    //      {
    //          return (T*)AttrStringAllocatorImpl::AttrStringAlloc();
    //      }
    //      else
    //      {
    //          return (T*)malloc(n*sizeof(T));
    //      }
    //  }
    //
    //  void deallocate(pointer p, size_type n = 1)
    //  {
    //      if (sizeof(T)!=8 || n!=1)
    //      {
    //          free(p);
    //      }
    //  }
    //
    //    void deallocate(pointer_type ptr, size_type byteSize, size_type alignment)
    //    {
    //        deallocate(ptr, byteSize);
    //    }
    //
    //  size_type max_size() const throw()
    //  {
    //      return INT_MAX;
    //  }
    //
    //  void construct(pointer p, const T& val)
    //  {
    //      new(static_cast<void*>(p)) T(val);
    //  }
    //
    //  void construct(pointer p)
    //  {
    //      new(static_cast<void*>(p)) T();
    //  }
    //
    //  void destroy(pointer p)
    //  {
    //      p->~T();
    //  }
    //
    //  pointer new_pointer()
    //  {
    //      return new(allocate()) T();
    //  }
    //
    //  pointer new_pointer(const T& val)
    //  {
    //      return new(allocate()) T(val);
    //  }
    //
    //  void delete_pointer(pointer p)
    //  {
    //      p->~T();
    //      deallocate(p);
    //  }
    //
    //  bool operator==(const AttrStringAllocator&) {return true;}
    //  bool operator!=(const AttrStringAllocator&) {return false;}
    //};

#include <AzCore/RTTI/TypeInfo.h>

    template<class T>
    class AZStdAttrStringAllocator
    {
    public:
        AZ_TYPE_INFO(AZStdAttrStringAllocator, "{1C93D797-C8A1-4A2D-BD65-74CE3200AAE5}");

        typedef void*               pointer_type;
        typedef AZStd::size_t       size_type;
        typedef AZStd::ptrdiff_t    difference_type;
        typedef AZStd::false_type   allow_memory_leaks;

        AZ_FORCE_INLINE AZStdAttrStringAllocator(const char* name = "AZStdAttrStringAllocator") {}
        AZ_FORCE_INLINE AZStdAttrStringAllocator(const AZStdAttrStringAllocator& rhs){}
        AZ_FORCE_INLINE AZStdAttrStringAllocator(const AZStdAttrStringAllocator& rhs, const char* name) { (void)rhs; (void)name; }

        AZ_FORCE_INLINE AZStdAttrStringAllocator& operator=(const AZStdAttrStringAllocator& rhs)        { return *this; }

        AZ_FORCE_INLINE const char*  get_name() const                   { return "AZStdAttrStringAllocator"; }
        AZ_FORCE_INLINE void         set_name(const char* name)         { (void)name; }

        pointer_type    allocate(size_type byteSize, size_type alignment, int flags = 0)
        {
            (void)alignment;
            (void)flags;
            if (sizeof(T) == 8 && byteSize == 8)
            {
                return AttrStringAllocatorImpl::AttrStringAlloc();
            }
            else
            {
                return malloc(byteSize);
            }
        }
        void            deallocate(pointer_type ptr, size_type byteSize, size_type alignment)
        {
            (void)alignment;
            if (sizeof(T) != 8 || byteSize != 8)
            {
                free(ptr);
            }
        }
        size_type       resize(pointer_type ptr, size_type newSize)
        {
            (void)ptr;
            (void)newSize;
            return 0;
        }
        size_type       get_max_size() const
        {
            return INT_MAX;
        }
        size_type       get_allocated_size() const
        {
            return 0;
        }

        AZ_FORCE_INLINE bool is_lock_free()                             { return false; }
        AZ_FORCE_INLINE bool is_stale_read_allowed()                    { return false; }
        AZ_FORCE_INLINE bool is_delayed_recycling()                     { return false; }

    private:
    };

    template<class T>
    bool operator==(const AZStdAttrStringAllocator<T>& a, const AZStdAttrStringAllocator<T>& b)
    {
        (void)a;
        (void)b;
        return true;
    }

    template<class T>
    bool operator!=(const AZStdAttrStringAllocator<T>& a, const AZStdAttrStringAllocator<T>& b)
    {
        (void)a;
        (void)b;
        return false;
    }
}  // end namespace


#endif // CRYINCLUDE_CRYACTION_SERIALIZATION_XMLCPBIN_WRITER_XMLCPB_BUFFERWRITER_H
