
#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/parallel/atomic.h>

namespace Physics
{
    /// Common types & constants.
    using MaterialId = unsigned short;  ///< Material Id; index into a given material set.
    static const MaterialId kDefaultMaterialId = static_cast<MaterialId>(~0);   ///< Reserved Id for default material in the material set.

    /**
     * Base class for AzFramework::Physics::Ptr<> managed objects.
     * Contains necessary API for intrusive ref-counting.
     */
    class ReferenceBase
    {
        friend class AZStd::intrusive_ptr<ReferenceBase>;

    public:

        AZ_CLASS_ALLOCATOR(ReferenceBase, AZ::SystemAllocator, 0);
        AZ_RTTI(ReferenceBase, "{51239399-42DF-47C6-886D-F75E2862174C}");

        ReferenceBase()
            : m_refCount(0)
        {}

        void add_ref()
        { 
            ++m_refCount; 
        }

        AZ::u32 release()
        { 
            AZ_Assert(m_refCount > 0, "Attempting to dereference an object whose refcount is already 0."); 

            const AZ::u32 refCount = m_refCount.fetch_sub(1, AZStd::memory_order_acq_rel);

            if (1 == refCount)
            {
                delete this;
                return 0;
            }

            return refCount - 1;
        }
            
        AZ::u32 GetReferenceCount() const { return m_refCount; }

        virtual AZ::Crc32 GetNativeType() const { return AZ::Crc32(); }
        virtual void* GetNativePointer() const { return nullptr; }

    protected:

        virtual ~ReferenceBase() 
        {}

    private:

        AZStd::atomic<AZ::u32> m_refCount;
    };

    /**
     * Intrusive smart pointer for managing shared-ownership objects within the physics API.
     * Objects must inherit from \ref ReferenceBase to be usable with AzFramework::Physics::Ptr<>.
     */
    template <typename T>
    using Ptr = AZStd::intrusive_ptr<T>;

    template<typename T, typename U>
    Ptr<T> PtrCast(const Ptr<U>& other)
    {
        U* otherPtr = other.get();
        return Ptr<T>(azrtti_cast<T*>(otherPtr));
    }

    template<typename T>
    Ptr<T> PtrCast(ReferenceBase* other)
    {
        return Ptr<T>(azrtti_cast<T*>(other));
    }

    /**
     * Structure for internal user data storage to reference physics API objects given raw physics
     * engine objects. High level game user data is stored here, since we consume the user data
     * field on the underlying physics engine with this structure.
     */
    class InternalUserData : public ReferenceBase
    {
    public:

        AZ_CLASS_ALLOCATOR(InternalUserData, AZ::SystemAllocator, 0);

        /// To identify, given raw physics user data, whether or not it is physics API-owned user data.
        enum { kMagic = 0x85349762 };

        InternalUserData(ReferenceBase* object, void* customUserData = nullptr)
            : m_magicId(static_cast<AZ::u32>(kMagic))
            , m_object(object)
            , m_customUserData(customUserData)
        {}
              
        InternalUserData(void* customUserData = nullptr)
            : m_magicId(static_cast<AZ::u32>(kMagic))
            , m_object(nullptr)
            , m_customUserData(customUserData)
        {}

        inline bool IsValid() const
        {
            return (m_magicId == kMagic);
        }

        AZ::u32             m_magicId;
        ReferenceBase*      m_object;           // InternalUserData is owned by an object, and should therefore not hold a reference on that object.
        void*               m_customUserData;
    };

} // namespace Physics
