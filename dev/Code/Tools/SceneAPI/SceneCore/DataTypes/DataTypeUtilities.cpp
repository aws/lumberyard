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

#include <AzFramework/StringFunc/StringFunc.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IGroup.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            namespace Utilities
            {
                bool IsNameAvailable(const AZStd::string& name, const Containers::SceneManifest& manifest, const Uuid& type)
                {
                    for (AZStd::shared_ptr<const IManifestObject> object : manifest.GetValueStorage())
                    {
                        if (object->RTTI_IsTypeOf(IGroup::TYPEINFO_Uuid()) && object->RTTI_IsTypeOf(type))
                        {
                            const IGroup* group = azrtti_cast<const IGroup*>(object.get());
                            if (AzFramework::StringFunc::Equal(group->GetName().c_str(), name.c_str()))
                            {
                                return false;
                            }
                        }
                    }
                    return true;
                }

                AZStd::string CreateUniqueName(const AZStd::string& baseName, const Containers::SceneManifest& manifest, const Uuid& type)
                {
                    int highestIndex = -1;
                    for (AZStd::shared_ptr<const IManifestObject> object : manifest.GetValueStorage())
                    {
                        if (object->RTTI_IsTypeOf(IGroup::TYPEINFO_Uuid()) && object->RTTI_IsTypeOf(type))
                        {
                            const IGroup* group = azrtti_cast<const IGroup*>(object.get());
                            const AZStd::string& groupName = group->GetName();
                            if (groupName.length() < baseName.length())
                            {
                                continue;
                            }

                            if (AzFramework::StringFunc::Equal(groupName.c_str(), baseName.c_str(), false, baseName.length()))
                            {
                                if (groupName.length() == baseName.length())
                                {
                                    highestIndex = AZStd::max(0, highestIndex);
                                }
                                else if (groupName[baseName.length()] == '-')
                                {
                                    int index = 0;
                                    if (AzFramework::StringFunc::LooksLikeInt(groupName.c_str() + baseName.length() + 1, &index))
                                    {
                                        highestIndex = AZStd::max(index, highestIndex);
                                    }
                                }
                            }
                        }
                    }

                    if (highestIndex == -1)
                    {
                        return baseName;
                    }
                    else
                    {
                        return AZStd::string::format("%s-%i", baseName.c_str(), highestIndex + 1);
                    }
                }
            } // Utilities
        } // DataTypes
    } // SceneAPI
} // AZ