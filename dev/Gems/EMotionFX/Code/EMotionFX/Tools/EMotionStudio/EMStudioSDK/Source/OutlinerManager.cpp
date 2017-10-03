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

// include required headers
#include "OutlinerManager.h"


namespace EMStudio
{
    OutlinerCategory::~OutlinerCategory()
    {
        const size_t numItems = mItems.size();
        for (size_t i = 0; i < numItems; ++i)
        {
            delete mItems[i];
        }
    }


    void OutlinerCategory::AddItem(OutlinerCategoryItem* item)
    {
        // find the ID to avoid duplicate
        if (FindItemByID(item->mID))
        {
            return;
        }

        // set the category of the item
        item->mCategory = this;

        // add the item
        mItems.push_back(item);

        // fire the add item event
        if (mManager)
        {
            const size_t numCallbacks = mManager->GetNumCallbacks();
            for (size_t i = 0; i < numCallbacks; ++i)
            {
                mManager->GetCallback(i)->OnAddItem(item);
            }
        }
    }


    void OutlinerCategory::RemoveItem(uint32 ID)
    {
        const size_t numItems = mItems.size();
        for (size_t i = 0; i < numItems; ++i)
        {
            if (mItems[i]->mID == ID)
            {
                // fire the add item event
                if (mManager)
                {
                    const size_t numCallbacks = mManager->GetNumCallbacks();
                    for (size_t j = 0; j < numCallbacks; ++j)
                    {
                        mManager->GetCallback(j)->OnRemoveItem(mItems[i]);
                    }
                }

                // remove the item
                delete mItems[i];
                mItems.erase(mItems.begin() + i);

                // done
                return;
            }
        }
    }


    OutlinerCategoryItem* OutlinerCategory::FindItemByID(uint32 ID)
    {
        const size_t numItems = mItems.size();
        for (size_t i = 0; i < numItems; ++i)
        {
            if (mItems[i]->mID == ID)
            {
                return mItems[i];
            }
        }
        return nullptr;
    }


    OutlinerManager::~OutlinerManager()
    {
        const size_t numCategories = mCategories.size();
        for (size_t i = 0; i < numCategories; ++i)
        {
            delete mCategories[i];
        }
    }


    OutlinerCategory* OutlinerManager::RegisterCategory(const QString& name, OutlinerCategoryCallback* callback)
    {
        // find if the category is already registered
        const size_t numCategories = mCategories.size();
        for (size_t i = 0; i < numCategories; ++i)
        {
            if (mCategories[i]->GetName() == name)
            {
                return mCategories[i];
            }
        }

        // add the category in the array
        OutlinerCategory* category = new OutlinerCategory(this, name, callback);
        mCategories.push_back(category);

        // fire the register category event
        const size_t numCallbacks = mCallbacks.size();
        for (size_t i = 0; i < numCallbacks; ++i)
        {
            mCallbacks[i]->OnRegisterCategory(name);
        }

        // return the category
        return category;
    }


    void OutlinerManager::UnregisterCategory(const QString& name)
    {
        const size_t numCategories = mCategories.size();
        for (size_t i = 0; i < numCategories; ++i)
        {
            if (mCategories[i]->GetName() == name)
            {
                // remove the category from the array
                delete mCategories[i];
                mCategories.erase(mCategories.begin() + i);

                // fire the unregister category event
                const size_t numCallbacks = mCallbacks.size();
                for (size_t j = 0; j < numCallbacks; ++j)
                {
                    mCallbacks[j]->OnUnregisterCategory(name);
                }

                // done
                return;
            }
        }
    }


    OutlinerCategory* OutlinerManager::FindCategoryByName(const QString& name)
    {
        const size_t numCategories = mCategories.size();
        for (size_t i = 0; i < numCategories; ++i)
        {
            if (mCategories[i]->GetName() == name)
            {
                return mCategories[i];
            }
        }
        return nullptr;
    }


    void OutlinerManager::RegisterCallback(OutlinerCallback* callback)
    {
        mCallbacks.push_back(callback);
    }


    void OutlinerManager::UnregisterCallback(OutlinerCallback* callback)
    {
        mCallbacks.erase(AZStd::remove(mCallbacks.begin(), mCallbacks.end(), callback), mCallbacks.end());
    }


    void OutlinerManager::FireItemModifiedEvent()
    {
        const size_t numCallbacks = mCallbacks.size();
        for (size_t j = 0; j < numCallbacks; ++j)
        {
            mCallbacks[j]->OnItemModified();
        }
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/OutlinerManager.moc>