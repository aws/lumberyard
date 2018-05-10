#pragma once

#include <AzCore/Asset/AssetCommon.h>

namespace AZ
{

/** IAssetBlendKey used in AssetBlend animation track.
*/
struct IAssetBlendKey
    : public ITimeRangeKey
{
    AZ::Data::AssetId m_assetId;    //!< Asset Id
    AZStd::string m_description;    //!< Description (filename part of path)

    IAssetBlendKey()
        : ITimeRangeKey()
    {
    }
};

    AZ_TYPE_INFO_SPECIALIZE(IAssetBlendKey, "{15B82C3A-6DB8-466F-AF7F-18298FCD25FD}");
}