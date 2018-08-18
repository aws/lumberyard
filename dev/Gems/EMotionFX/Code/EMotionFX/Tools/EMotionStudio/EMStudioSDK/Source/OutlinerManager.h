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

#include <AzCore/Memory/Memory.h>
#include <AzCore/std/containers/vector.h>
#include <MCore/Source/CommandGroup.h>
#include "EMStudioConfig.h"
#include <QString>
#include <QIcon>


// forward declarations
QT_FORWARD_DECLARE_CLASS(QMenu)


namespace EMStudio
{
    // forward declaration
    class OutlinerCategory;
    class OutlinerManager;


    // OutlinerCategoryItem struct
    struct OutlinerCategoryItem
    {
        AZ_CLASS_ALLOCATOR_DECL

        OutlinerCategoryItem()
        {
            mID         = MCORE_INVALIDINDEX32;
            mUserData   = nullptr;
            mCategory   = nullptr;
        }

        uint32              mID;
        void*               mUserData;
        OutlinerCategory*   mCategory;
    };


    // OutlinerCategoryCallback class
    class EMSTUDIO_API OutlinerCategoryCallback
    {
    public:
        virtual ~OutlinerCategoryCallback(){}
        virtual QString BuildNameItem(OutlinerCategoryItem* item) const = 0;
        virtual QString BuildToolTipItem(OutlinerCategoryItem* item) const = 0;
        virtual QIcon GetIcon(OutlinerCategoryItem* item) const = 0;
        virtual void OnRemoveItems(QWidget* parent, const AZStd::vector<OutlinerCategoryItem*>& items, MCore::CommandGroup* commandGroup) = 0;
        virtual void OnPostRemoveItems(QWidget* parent) = 0;
        virtual void OnLoadItem(QWidget* parent) = 0;
    };


    // OutlinerCategory class
    class EMSTUDIO_API OutlinerCategory
    {
    public:
        // constructor/destructor
        OutlinerCategory(OutlinerManager* manager, const QString& name, OutlinerCategoryCallback* callback) { mName = name; mCallback = callback; mManager = manager; }
        ~OutlinerCategory();

        // item
        void AddItem(uint32 mID, void* mUserData);
        void RemoveItem(uint32 ID);
        OutlinerCategoryItem* FindItemByID(uint32 ID);
        OutlinerCategoryItem* GetItem(size_t index)                                                         { return mItems[index]; }
        size_t GetNumItems() const                                                                          { return mItems.size(); }

        // gets
        QString GetName() const                                                                             { return mName; }
        OutlinerManager* GetManager() const                                                                 { return mManager; }

        // callback
        void SetCallback(OutlinerCategoryCallback* callback)                                                { mCallback = callback; }
        OutlinerCategoryCallback* GetCallback() const                                                       { return mCallback; }

    private:
        QString                             mName;
        OutlinerCategoryCallback*           mCallback;
        OutlinerManager*                    mManager;
        AZStd::vector<OutlinerCategoryItem*> mItems;
    };


    // OutlinerCallback class
    class EMSTUDIO_API OutlinerCallback
    {
    public:
        virtual ~OutlinerCallback(){}
        virtual void OnAddItem(OutlinerCategoryItem* item) = 0;
        virtual void OnRemoveItem(OutlinerCategoryItem* item) = 0;
        virtual void OnItemModified() = 0;
        virtual void OnRegisterCategory(OutlinerCategory* category) = 0;
        virtual void OnUnregisterCategory(const QString& name) = 0;
    };


    // OutlinerManager class
    class EMSTUDIO_API OutlinerManager
    {
    public:
        // constructor/destructor
        OutlinerManager()                                                                           { }
        ~OutlinerManager();

        // category
        OutlinerCategory* RegisterCategory(const QString& name, OutlinerCategoryCallback* callback);
        void UnregisterCategory(const QString& name);
        OutlinerCategory* GetCategory(size_t index)                                                 { return mCategories[index]; }
        size_t GetNumCategories() const                                                             { return mCategories.size(); }

        // callback
        void RegisterCallback(OutlinerCallback* callback);
        void UnregisterCallback(OutlinerCallback* callback);
        OutlinerCallback* GetCallback(size_t index) const                                           { return mCallbacks[index]; }
        size_t GetNumCallbacks() const                                                              { return mCallbacks.size(); }

        // fire the item modified event
        void FireItemModifiedEvent();

        void AddItemToCategory(const QString& name, uint32 mID, void* userData);
        void RemoveItemFromCategory(const QString& categoryName, uint32 mID);
    private:
        OutlinerCategory* FindCategoryByName(const QString& name) const;

        AZStd::vector<OutlinerCategory*> mCategories;
        AZStd::vector<OutlinerCallback*> mCallbacks;
    };
} // namespace EMStudio
