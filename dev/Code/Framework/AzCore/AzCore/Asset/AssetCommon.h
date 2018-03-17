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
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/function/function_fwd.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/typetraits/is_base_of.h>
#include <AzCore/std/string/string.h>

#if defined(AZ_PLATFORM_LINUX) || defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_APPLE)
#   include <stdio.h> // for snprintf
#endif

namespace AZ
{
    class AssetSerializer;
    class AssetEventHandler;
    class SerializeContext;

    namespace Data
    {
        class AssetManager;
        class AssetData;
        class AssetEntry;

        template<class T>
        class Asset;

        typedef Uuid AssetType;

        namespace AssetInternal
        {
            bool IsValidAssetType(const AssetType& type, AZ::SerializeContext* serializeContext = nullptr);
        }

        /**
         * Asset ID types
         */
        typedef AssetData*  AssetPtr;

        struct AssetId
        {
            AZ_TYPE_INFO(AssetId, "{652ED536-3402-439B-AEBE-4A5DBC554085}");

            /// Create asset id in invalid state
            AssetId();
            AssetId(const Uuid& guid, u32 sudId = 0);

            bool IsValid() const;

            void SetInvalid(); // or just can just do m_assetId = AssetId();

            bool operator==(const AssetId& rhs) const;
            bool operator!=(const AssetId& rhs) const;

            template<class StringType>
            StringType ToString() const;

            template<class StringType>
            void ToString(StringType& result) const;

            Uuid m_guid;
            u32  m_subId;   ///< To allow easier and more consistent asset guid, we can provide asset sub ID. (i.e. Guid is a cubemap texture, subId is the index of the side)
        };

        /**
         * Constants
         */
        static const AssetType  s_invalidAssetType = AZ::Uuid::CreateNull();

        /**
         * Base class for all asset types.
         */
        class AssetData
        {
            template<class T>
            friend class Asset;
            friend class AssetManager;
            friend class AssetHandler;

        public:
            enum class AssetStatus : int
            {
                NotLoaded,          ///< Asset has not been loaded, and is not in the process of loading.
                Loading,            ///< Asset is currently in the process of loading.
                ReadyPreNotify,     ///< Asset is loaded and ready for use. AssetBus hasn't yet dispatched the OnAssetReady event on the main thread.
                Ready,              ///< Asset is loaded and ready for use.
                Error,              ///< Asset attempted to load, but it or a strict dependency failed.
            };

            AZ_CLASS_ALLOCATOR(AssetData, SystemAllocator, 0);
            AZ_RTTI(AssetData, "{AF3F7D32-1536-422A-89F3-A11E1F5B5A9C}");

            AssetData(const AssetId& assetId = AssetId(), AssetStatus status = AssetStatus::NotLoaded)
                : m_useCount(0)
                , m_status(static_cast<int>(status))
                , m_assetId(assetId)
            {}

            virtual ~AssetData()
            {}

            AZ_FORCE_INLINE void Acquire() { ++m_useCount; }
            AZ_FORCE_INLINE void Release()
            {
                AZ_Assert(m_useCount > 0, "Usecount is already 0!");
                if (--m_useCount == 0)
                {
                    RemoveFromDB();
                }
            }

            /// Asset is loaded and ready for use.
            /// Note that the asset may be ready for use before the OnAssetReady
            /// event has been dispatched by the AssetBus on the main thread.
            AZ_FORCE_INLINE bool IsReady() const
            {
                AssetStatus status = GetStatus();
                return (status == AssetStatus::Ready || status == AssetStatus::ReadyPreNotify);
            }

            AZ_FORCE_INLINE bool IsError() const { return GetStatus() == AssetStatus::Error; }
            AZ_FORCE_INLINE bool IsLoading() const { return GetStatus() == AssetStatus::Loading; }
            AZ_FORCE_INLINE AssetStatus GetStatus() const { return static_cast<AssetStatus>(m_status.load()); }
            AZ_FORCE_INLINE const AssetId& GetId() const { return m_assetId; }
            AZ_FORCE_INLINE const AssetType& GetType() const { return RTTI_GetType(); }
            AZ_FORCE_INLINE int GetUseCount() const { return m_useCount.load(); }

        protected:
            /**
             * Override this function do create a new Asset Instance for each AssetDatababse::GetAsset(),
             * Most assets should be read only and shared (thus registered in the database), make sure you understand
             * the consequences of changing behavior. When this function returns false, this asset will NOT be added to the
             * loaded assets map (which is why when we call AssetDatabase::GetAsset it will not be found a new will be create)
             */
            virtual bool IsRegisterReadonlyAndShareable() { return true; }

            void RemoveFromDB();

            // Workaround for VS2013
            // https://connect.microsoft.com/VisualStudio/feedback/details/800328/std-is-copy-constructible-is-broken
            AssetData(const AssetData&) = delete;

            AZStd::atomic_int m_useCount;
            AZStd::atomic_int m_status;
            AssetId m_assetId;
        };

        /**
         * Flags you can use for each instance (Asset<X>) to help with
         * systems that load/etc. the asset (i.e. ObjectStream)
         */
        enum class AssetFlags : u8
        {
            OBJECTSTREAM_PRE_LOAD = 0, ///< default, objectstream will pre-load the asset before returning the object to the user
            OBJECTSTREAM_QUEUE_LOAD = 1, ///< objectstream will queue a load and return the object to the user. User code will need to check if the asset is ready
            OBJECTSTREAM_NO_LOAD = 2, ///< object stream will load the ID. Asset loading will be left to the user.
            OBJECTSTREAM_MASK = 0x03,
        };

        /// Can be provided as a predicate for asset filtering or visiting.
        typedef AZStd::function<bool(const Data::Asset<Data::AssetData>& asset)> AssetFilterCB;

        /**
         * Asset is a smart pointer to an actual asset.
         * Asset users should always refer to assets using
         * this object.
         */
        template<class T>
        class Asset
        {
            typedef Asset ThisType;

            friend class AssetManager;
            friend class AZ::AssetSerializer;
            friend class AZ::AssetEventHandler;

            template<typename U>
            friend class Asset;

        public:
            AZ_TYPE_INFO(Asset, "{C891BF19-B60C-45E2-BFD0-027D15DDC939}", T);
            /// Create asset with default params (no asset bounded)
            /// By default, referenced assets will be preloaded during serialization.
            /// Use \ref AZ::Data::AssetFlags to control this behavior.
            Asset(u8 flags = static_cast<u8>(AssetFlags::OBJECTSTREAM_PRE_LOAD));
            /// Create an asset from a valid asset data (created asset), might not be loaded or currently loading.
            Asset(AssetData* assetData);
            /// Initialize asset pointer with id, type, and hint. No data construction will occur until QueueLoad is called.
            Asset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type, const AZStd::string& hint = AZStd::string());

            Asset(const Asset& rhs);
            template<typename U>
            Asset(const Asset<U>& rhs);

#if defined(AZ_HAS_RVALUE_REFS)
            Asset(Asset&& rhs);
            Asset& operator=(Asset&& rhs);
#endif
            ~Asset();

            Asset& operator=(const Asset& rhs);
            Asset& operator=(AssetData* assetData);

            explicit operator bool() const
            {
                return m_assetData != nullptr;
            }

            bool IsReady() const;               ///< Is the asset data ready (loaded)?
            bool IsError() const;               ///< Did an error occur when loading the asset?
            bool IsLoading() const;             ///< Is the asset currently loading?
            AssetData::AssetStatus GetStatus() const; ///< What is the current load status of the asset?

            const AssetId& GetId() const;       ///< Retrieve the Id of the referenced asset.
            const AssetType& GetType() const;   ///< Retrieve the type of the referenced asset (if available).
            const AZStd::string& GetHint() const; ///< Retrieve the last known path to the asset Id this ref is bound to

            /// Returns asset data - base class
            AssetData* GetData() const;

            /// Returns pointer to the asset type, or null if asset is not loaded
            T* Get() const;

            /// Return asset data as T type, uses azrtti for the cast
            template<class U>
            U* GetAs() const;

            /**
            * Create an asset with an ID type is inferred. If there is a current asset it will released;
            * \param id \ref AssetId
            * \param queueLoad if true it will call "QueueLoad" to load the asset data.
            */
            bool Create(const AssetId& id, bool queueLoad = false);

            /**
             * If the asset is not loaded and has a valid id, it will be queued for load.
             * \returns true if the load was queued (you can listen on the AssetBus or check with IsReady)
             * and false if the queue of loading did not happen. There are multiple reasons for this and you can
             * test in case of error. Current failure conditions:
             * - Asset already loaded
             * - Invalid id
             * \param assetLoadFilterCB - ObjectStream asset filter callback for dependent asset loads.
             */
            bool QueueLoad(const AssetFilterCB& assetLoadFilterCB = nullptr);

            /**
             * Releases reference on asset data, if one is held.
             * \return true if a reference was held, and therefore released.
             */
            bool Release();

            /**
             * Reloads an asset if an asset is create.
             * \returns true if reload is triggered, otherwise false if an asset is not created (ie. We don't asset ID to reload)
             */
            bool Reload();

            /**
             * Save asset, triggers a SaveAsset of possible \ref AssetDatabase::SaveAsset.
             */
            bool Save();

            /// Returns asset instance flags
            u8 GetFlags() const;

            /// You can change the flags only when we don't have an asset bound (ie. GetData() == null)
            bool SetFlags(u8 flags);

            /**
            * Upgrades legacy Ids/hints with the new canonical Ids/hints.
            * This will search for the assetId and hint in the catalog
            * and if found, will update it to be the new, canonical one(s)
            */
            void UpgradeAssetInfo();

        protected:

            void SetData(AssetData* assetData);

            void swap(Asset& rhs);

            AssetId m_assetId;      ///< Id of asset the pointer currently references. QueueLoad will acquire/load the actual asset.
            AssetType m_assetType;  ///< Referenced asset type.
            AssetData* m_assetData; ///< Pointer to the asset data, it's always present when we have bound an asset (it doesn't have to be loaded)
            u8 m_flags; ///< Flags per instance, used to control serializer/object stream loading behavior \ref AssetFlags
            AZStd::string m_assetHint; ///< Last known path to the asset m_assetId refers to
        };

        template<typename T, typename U>
        Asset<T> static_pointer_cast(const Asset<U>& p) { return Asset<T>(p); }


        namespace AssetInternal
        {
            Asset<AssetData> QueueAssetLoad(AssetData* assetData, const AZ::Data::AssetFilterCB& assetLoadFilterCB = nullptr);
            Asset<AssetData> GetAsset(const AssetId& id, const AssetType& type, bool queueLoad, bool isCreate = false);
            void UpdateAssetInfo(AssetId& id, AZStd::string& assetHint);
            bool ReloadAsset(AssetData* assetData);
            bool SaveAsset(AssetData* assetData);
            Asset<AssetData> GetAssetData(const AssetId& id);
            AssetId ResolveAssetId(const AssetId& id);
        }

        /**
         * AssetEvents
         * All parameters are passed by value because events can be queued.
         */
        class AssetEvents
            : public EBusTraits
        {
            friend class AssetManager;

        public:
            AZ_RTTI(AssetEvents, "{5EA1F709-A268-4C60-BEC0-7CFEED3BA114}");

            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            typedef AZStd::recursive_mutex  MutexType;
            typedef AssetId                 BusIdType;
            static const bool EnableEventQueue = true;

            /**
            * Custom connection policy to make sure all we are fully in sync
            */
            template <class Bus>
            struct AssetConnectionPolicy
                : public EBusConnectionPolicy<Bus>
            {
                static void Connect(typename Bus::BusPtr& busPtr, typename Bus::Context& context, typename Bus::HandlerNode& handler, const typename Bus::BusIdType& id = 0)
                {
                    // It's possible for users to open a level while the AP is still processing and as such not all legacy asset ids were known at the time
                    // of loading. Check now to see if the assets have been compiled and find the actual asset id to connect to.
                    typename Bus::BusIdType actualId = AssetInternal::ResolveAssetId(id);
                    EBusConnectionPolicy<Bus>::Connect(busPtr, context, handler, actualId);

                    // If the asset is ready, notify this handler manually.
                    // It is safe to do this because we know that if the asset
                    // has already been marked ready, then the event has already
                    // finished firing or otherwise we would not have been able
                    // to lock the bus mutex
                    Asset<AssetData> assetData(AssetInternal::GetAssetData(actualId));
                    if (assetData)
                    {
                        if (assetData.Get()->GetStatus() == AssetData::AssetStatus::Ready)
                        {
                            handler->OnAssetReady(assetData);
                        }
                        else if (assetData.Get()->IsError())
                        {
                            handler->OnAssetError(assetData);
                        }
                    }
                }
            };
            template<typename Bus>
            using ConnectionPolicy = AssetConnectionPolicy<Bus>;
            //////////////////////////////////////////////////////////////////////////

            virtual ~AssetEvents() {}

            /// Called when an asset is loaded, patched and ready to be used.
            virtual void OnAssetReady(Asset<AssetData> asset) { (void)asset; }
            /// Called when an asset has been moved (usually due to de-fragmentation/compaction), if possible the only data pointer is provided otherwise NULL.
            virtual void OnAssetMoved(Asset<AssetData> asset, void* oldDataPointer) { (void)asset; (void)oldDataPointer; }
            /// Called before an asset reload has started.
            virtual void OnAssetPreReload(Asset<AssetData> asset) { (void)asset; }
            /// Called when an asset has been reloaded (usually in tool mode and loose more). It should not be called in final build.
            virtual void OnAssetReloaded(Asset<AssetData> asset) { (void)asset; }
            /// Called when an asset failed to reload.
            virtual void OnAssetReloadError(Asset<AssetData> asset) { (void)asset; }
            /// Called when an asset has been saved. In general most assets can't be saved (in a game) so make sure you check the flag.
            virtual void OnAssetSaved(Asset<AssetData> asset, bool isSuccessful) { (void)asset; (void)isSuccessful; }
            /// Called when an asset is unloaded.
            virtual void OnAssetUnloaded(const AssetId assetId, const AssetType assetType) { (void)assetId; (void)assetType; }
            /// Called when an error happened with an asset. When this message is received the asset should be considered broken by default.
            virtual void OnAssetError(Asset<AssetData> asset) { (void)asset; }
        };

        typedef EBus<AssetEvents> AssetBus;

        /*
         * AssetBusCallbacks is a utility class that maps AssetBus events to user callbacks
         */
        class AssetBusCallbacks
            : public AssetBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(AssetBusCallbacks, AZ::SystemAllocator, 0);

            typedef AZStd::function<void (const Asset<AssetData>& /*asset*/, AssetBusCallbacks& /*callbacks*/)> AssetReadyCB;
            typedef AZStd::function<void (const Asset<AssetData>& /*asset*/, void* /*oldDataPointer*/, AssetBusCallbacks& /*callbacks*/)> AssetMovedCB;
            typedef AZStd::function<void (const Asset<AssetData>& /*asset*/, AssetBusCallbacks& /*callbacks*/)> AssetReloadedCB;
            typedef AZStd::function<void (const Asset<AssetData>& /*asset*/, bool /*isSuccessful*/, AssetBusCallbacks& /*callbacks*/)> AssetSavedCB;
            typedef AZStd::function<void (const AssetId& /*assetId*/, const AssetType& /*assetType*/, AssetBusCallbacks& /*callbacks*/)> AssetUnloadedCB;
            typedef AZStd::function<void (const Asset<AssetData>& /*asset*/, AssetBusCallbacks& /*callbacks*/)> AssetErrorCB;

            void SetCallbacks(const AssetReadyCB& readyCB, const AssetMovedCB& movedCB, const AssetReloadedCB& reloadedCB, const AssetSavedCB& savedCB, const AssetUnloadedCB& unloadedCB, const AssetErrorCB& errorCB);
            void ClearCallbacks();

            void OnAssetReady(Asset<AssetData> asset) override;
            void OnAssetMoved(Asset<AssetData> asset, void* oldDataPointer) override;
            void OnAssetReloaded(Asset<AssetData> asset) override;
            void OnAssetSaved(Asset<AssetData> asset, bool isSuccessful) override;
            void OnAssetUnloaded(const AssetId assetId, const AssetType assetType) override;
            void OnAssetError(Asset<AssetData> asset) override;

        protected:
            AssetReadyCB    m_onAssetReadyCB;
            AssetMovedCB    m_onAssetMovedCB;
            AssetReloadedCB m_onAssetReloadedCB;
            AssetSavedCB    m_onAssetSavedCB;
            AssetUnloadedCB m_onAssetUnloadedCB;
            AssetErrorCB    m_onAssetErrorCB;
        };

        /**
        * Implementations
        */

        /**
        * AssetID
        */

        //=========================================================================
        inline AssetId::AssetId()
            : m_guid(AZ::Uuid::CreateNull())
            , m_subId(0)
        {
        }

        //=========================================================================
        inline AssetId::AssetId(const Uuid& guid, u32 sudId)
            : m_guid(guid)
            , m_subId(sudId)
        {
        }

        //=========================================================================
        template<class StringType>
        inline StringType AssetId::ToString() const
        {
            StringType result;
            ToString(result);
            return result;
        }

        //=========================================================================
        template<class StringType>
        inline void AssetId::ToString(StringType& result) const
        {
            m_guid.ToString(result);
            char subIdBuffer[16];
            azsnprintf(subIdBuffer, AZ_ARRAY_SIZE(subIdBuffer), ":%x", m_subId);
            result += subIdBuffer;
        }

        //=========================================================================
        inline bool AssetId::IsValid() const
        {
            return m_guid != AZ::Uuid::CreateNull();
        }

        //=========================================================================
        inline void AssetId::SetInvalid()
        {
            m_guid = AZ::Uuid::CreateNull();
            m_subId = 0;
        }

        //=========================================================================
        inline bool AssetId::operator == (const AssetId& rhs) const
        {
            return m_guid == rhs.m_guid && m_subId == rhs.m_subId;
        }

        //=========================================================================
        inline bool AssetId::operator != (const AssetId& rhs) const
        {
            return m_guid != rhs.m_guid || m_subId != rhs.m_subId;
        }

        //=========================================================================
        template<class T, class U>
        inline bool operator==(const Asset<T>& lhs, const Asset<U>& rhs)
        {
            return lhs.GetId() == rhs.GetId(); /// Id is enough considering all ids are globally unique
        }

        //=========================================================================
        template<class T, class U>
        inline bool operator!=(const Asset<T>& lhs, const Asset<U>& rhs)
        {
            return lhs.GetId() != rhs.GetId(); /// Id is enough considering all ids are globally unique
        }

        /**
         * Asset<T>
         */
        //=========================================================================
        template<class T>
        Asset<T>::Asset(u8 flags)
            : m_assetType(azrtti_typeid<T>())
            , m_assetData(nullptr)
            , m_flags(flags)
        {
            AZ_STATIC_ASSERT((AZStd::is_base_of<AssetData, T>::value), "Can only specify desired type if asset type is AssetData");
        }

        //=========================================================================
        template<class T>
        Asset<T>::Asset(AssetData* assetData)
            : m_assetType(azrtti_typeid<T>())
            , m_assetData(nullptr)
            , m_flags(0)
        {
            SetData(assetData);
        }

        //=========================================================================
        template<class T>
        Asset<T>::Asset(const AssetId& id, const AZ::Data::AssetType& type, const AZStd::string& hint)
            : m_assetId(id)
            , m_assetType(type)
            , m_assetData(nullptr)
            , m_flags(0)
            , m_assetHint(hint)
        {
        }

        //=========================================================================
        template<class T>
        Asset<T>::Asset(const Asset& rhs)
            : m_assetId(rhs.m_assetId)
            , m_assetType(rhs.m_assetType)
            , m_assetData(nullptr)
            , m_flags(rhs.m_flags)
            , m_assetHint(rhs.m_assetHint)
        {
            SetData(rhs.m_assetData);
        }

        //=========================================================================
        template<class T>
        template<typename U>
        Asset<T>::Asset(const Asset<U>& rhs)
            : m_assetId(rhs.m_assetId)
            , m_assetType(rhs.m_assetType)
            , m_assetData(nullptr)
            , m_flags(rhs.m_flags)
            , m_assetHint(rhs.m_assetHint)
        {
            SetData(rhs.m_assetData);
        }

#if defined(AZ_HAS_RVALUE_REFS)
        //=========================================================================
        template<class T>
        Asset<T>::Asset(Asset&& rhs)
            : m_assetId(AZStd::move(rhs.m_assetId))
            , m_assetType(AZStd::move(rhs.m_assetType))
            , m_assetData(rhs.m_assetData)
            , m_flags(rhs.m_flags)
            , m_assetHint(AZStd::move(rhs.m_assetHint))
        {
            rhs.m_assetId = AssetId();
            rhs.m_assetType = AssetType::CreateNull();
            rhs.m_assetData = nullptr;
            rhs.m_flags = 0;
            rhs.m_assetHint = AZStd::string();
        }

        //=========================================================================
        template<class T>
        Asset<T>& Asset<T>::operator=(Asset&& rhs)
        {
            Release();

            m_assetId = AZStd::move(rhs.m_assetId);
            m_assetType = AZStd::move(rhs.m_assetType);
            m_assetData = rhs.m_assetData;
            m_flags = rhs.m_flags;
            m_assetHint = AZStd::move(rhs.m_assetHint);
            rhs.m_assetId = AssetId();
            rhs.m_assetType = AssetType::CreateNull();
            rhs.m_assetData = 0;
            rhs.m_flags = 0;
            rhs.m_assetHint = AZStd::string();
            return *this;
        }
#endif
        //=========================================================================
        template<class T>
        Asset<T>::~Asset()
        {
            if (m_assetData)
            {
                m_assetData->Release();
            }
        }

        //=========================================================================
        template<class T>
        Asset<T>& Asset<T>::operator=(const Asset& rhs)
        {
            ThisType(rhs).swap(*this);
            return *this;
        }

        //=========================================================================
        template<class T>
        Asset<T>& Asset<T>::operator=(AssetData* assetData)
        {
            SetData(assetData);

            if (!m_assetData)
            {
                // If assigning null, wipe the asset reference and reset the type to that of the pointer.
                m_assetId = AssetId();
                m_assetType = azrtti_typeid<T>();
                m_assetHint.clear();
            }

            return *this;
        }

        //=========================================================================
        template<class T>
        bool  Asset<T>::IsReady() const
        {
            return m_assetData && m_assetData->IsReady();
        }

        //=========================================================================
        template<class T>
        bool Asset<T>::IsError() const
        {
            return m_assetData && m_assetData->IsError();
        }

        //=========================================================================
        template<class T>
        bool Asset<T>::IsLoading() const
        {
            return m_assetData && m_assetData->IsLoading();
        }

        //=========================================================================
        template<class T>
        AssetData::AssetStatus Asset<T>::GetStatus() const
        {
            return m_assetData ? m_assetData->GetStatus() : AssetData::AssetStatus::NotLoaded;
        }

        //=========================================================================
        template<class T>
        const AssetId& Asset<T>::GetId() const
        {
            return m_assetId;
        }

        //=========================================================================
        template<class T>
        const AssetType& Asset<T>::GetType() const
        {
            return m_assetType;
        }

        //=========================================================================
        template<class T>
        AssetData* Asset<T>::GetData() const
        {
            return m_assetData;
        }

        template <class T>
        const AZStd::string& Asset<T>::GetHint() const
        {
            return m_assetHint;
        }

        //=========================================================================
        template<class T>
        T* Asset<T>::Get() const
        {
            if (m_assetData)
            {
                //return azrtti_cast<T*>(m_assetData);
                return static_cast<T*>(m_assetData); // Type is checked when we set the asset data
            }
            return nullptr;
        }

        //=========================================================================
        template<class T>
        template<class U>
        U* Asset<T>::GetAs() const
        {
            return azrtti_cast<U*>(m_assetData);
        }

        //=========================================================================
        template<class T>
        u8 Asset<T>::GetFlags() const
        {
            return m_flags;
        }

        //=========================================================================
        template<class T>
        void Asset<T>::UpgradeAssetInfo()
        {
            if (!m_assetId.IsValid())
            {
                return;
            }

            // called Internal to de-templatize and also avoid inclusion of AssetManager
            // into this header.
            AssetInternal::UpdateAssetInfo(m_assetId, m_assetHint);
        }

        //=========================================================================
        template<class T>
        bool Asset<T>::SetFlags(u8 flags)
        {
            if (!m_assetData)
            {
                m_flags = flags;
                return true;
            }
            return false;
        }

        //=========================================================================
        template<class T>
        void Asset<T>::SetData(AssetData* assetData)
        {
            if (m_assetData)
            {
                m_assetData->Release();
                m_assetData = nullptr;
            }

            if (assetData)
            {
                // Validate the data type matches or derives from T before assigning.
                if (assetData->RTTI_IsTypeOf(AzTypeInfo<T>::Uuid()))
                {
                    m_assetId = assetData->GetId();
                    m_assetType = assetData->RTTI_GetType();
                    assetData->Acquire();

                    UpgradeAssetInfo();
                }
                else
                {
#ifdef AZ_ENABLE_TRACING
                    char assetDataIdGUIDStr[Uuid::MaxStringBuffer];
                    char assetTypeIdGUIDStr[Uuid::MaxStringBuffer];
                    assetData->GetId().m_guid.ToString(assetDataIdGUIDStr, AZ_ARRAY_SIZE(assetDataIdGUIDStr));
                    AzTypeInfo<T>::Uuid().ToString(assetTypeIdGUIDStr, AZ_ARRAY_SIZE(assetTypeIdGUIDStr));
                    AZ_Error("AssetDatabase", false, "Asset of type %s:%x (%s) is not related to %s (%s)!",
                        assetData->GetType().ToString<AZStd::string>().c_str(), assetData->GetId().m_subId, assetDataIdGUIDStr,
                        AzTypeInfo<T>::Name(), assetTypeIdGUIDStr);
#endif // AZ_ENABLE_TRACING
                    m_assetId = AssetId();
                    m_assetType = azrtti_typeid<T>();
                    m_assetHint.clear();
                    return;
                }
            }
            m_assetData = assetData;
        }

        //=========================================================================
        template<class T>
        void Asset<T>::swap(Asset& rhs)
        {
            AZStd::swap(m_assetId, rhs.m_assetId);
            AZStd::swap(m_assetType, rhs.m_assetType);
            AZStd::swap(m_assetData, rhs.m_assetData);
            AZStd::swap(m_flags, rhs.m_flags);
            AZStd::swap(m_assetHint, rhs.m_assetHint);
        }

        //=========================================================================
        template<class T>
        bool Asset<T>::QueueLoad(const AZ::Data::AssetFilterCB& assetLoadFilterCB)
        {
            const u8 flags = GetFlags();

            if (!m_assetData && m_assetId.IsValid())
            {
                *this = AssetInternal::GetAsset(m_assetId, m_assetType, false);
                SetFlags(flags);
            }

            if (m_assetData && m_assetData->GetId().IsValid())
            {
                AssetData::AssetStatus status = m_assetData->GetStatus();
                if (status == AssetData::AssetStatus::Loading ||
                    status == AssetData::AssetStatus::Ready)
                {
                    return false;
                }
            }

            if (m_assetData)
            {
                const AZ::u8 loadFlags = GetFlags();
                *this = AssetInternal::QueueAssetLoad(m_assetData, assetLoadFilterCB);
                SetFlags(loadFlags);
            }

            return (m_assetData != nullptr);
        }

        //=========================================================================
        template<class T>
        bool Asset<T>::Release()
        {
            if (m_assetData)
            {
                SetData(nullptr);
                return true;
            }

            return false;
        }

        //=========================================================================
        template<class T>
        bool Asset<T>::Create(const AssetId& id, bool queueLoad)
        {
            if (id != GetId())
            {
                const AZ::u8 loadFlags = GetFlags();

                // Acquire new asset. This will release the current asset.
                *this = AssetInternal::GetAsset(id, m_assetType, queueLoad, true);

                SetFlags(loadFlags);
            }

            return true; // already created
        }

        //=========================================================================
        template<class T>
        bool Asset<T>::Reload()
        {
            if (m_assetData && m_assetData->GetId().IsValid())
            {
                return AssetInternal::ReloadAsset(m_assetData);
            }

            return false;
        }

        //=========================================================================
        template<class T>
        bool Asset<T>::Save()
        {
            if (m_assetData && m_assetData->GetId().IsValid())
            {
                return AssetInternal::SaveAsset(m_assetData);
            }

            return false;
        }
    }  // namespace Data
}   // namespace AZ

namespace AZStd
{
    // hash specialization
    template <>
    struct hash<AZ::Data::AssetId>
    {
        typedef AZ::Uuid    argument_type;
        typedef size_t      result_type;
        AZ_FORCE_INLINE size_t operator()(const AZ::Data::AssetId& id) const
        {
            return id.m_guid.GetHash();
        }
    };
}