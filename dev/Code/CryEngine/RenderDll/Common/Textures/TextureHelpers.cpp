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

#include "StdAfx.h"
#include "TextureHelpers.h"
#include "StringUtils.h"

/* -----------------------------------------------------------------------
  * These functions are used in Cry3DEngine, CrySystem, CryRenderD3D11,
  * Editor, ResourceCompilerMaterial and more
  */

//////////////////////////////////////////////////////////////////////////
namespace
{
    static struct
    {
        EEfResTextures slot;
        int8 priority;
        CTexture** def;
        CTexture** neutral;
        const char* suffix;
    }
    s_TexSlotSemantics[] =
    {
        // NOTE: must be in order with filled holes to allow direct lookup
        { EFTT_DIFFUSE,          4, &CTexture::s_ptexNoTexture, &CTexture::s_ptexWhite,    "_diff"   },
        { EFTT_NORMALS,          2, &CTexture::s_ptexFlatBump,  &CTexture::s_ptexFlatBump, "_ddn"    },
        { EFTT_SPECULAR,         1, &CTexture::s_ptexWhite,     &CTexture::s_ptexWhite,    "_spec"   },
        { EFTT_ENV,              0, &CTexture::s_ptexWhite,     &CTexture::s_ptexWhite,    "_cm"     },
        { EFTT_DETAIL_OVERLAY,   3, &CTexture::s_ptexGray,      &CTexture::s_ptexWhite,    "_detail" },
        { EFTT_SECOND_SMOOTHNESS, 2, &CTexture::s_ptexWhite,    &CTexture::s_ptexWhite,    "" },
        { EFTT_HEIGHT,           2, &CTexture::s_ptexWhite,     &CTexture::s_ptexWhite,    "_displ"  },
        { EFTT_DECAL_OVERLAY,    3, &CTexture::s_ptexGray,      &CTexture::s_ptexWhite,    ""        },
        { EFTT_SUBSURFACE,       3, &CTexture::s_ptexWhite,     &CTexture::s_ptexWhite,    "_sss"    },
        { EFTT_CUSTOM,           4, &CTexture::s_ptexWhite,     &CTexture::s_ptexWhite,    ""        },
        { EFTT_CUSTOM_SECONDARY, 2, &CTexture::s_ptexFlatBump,  &CTexture::s_ptexFlatBump, ""        },
        { EFTT_OPACITY,          4, &CTexture::s_ptexWhite,     &CTexture::s_ptexWhite,    ""        },
        { EFTT_SMOOTHNESS,       2, &CTexture::s_ptexWhite,     &CTexture::s_ptexWhite,    "_ddna"   },
        { EFTT_EMITTANCE,        1, &CTexture::s_ptexWhite,     &CTexture::s_ptexWhite,    "_em"     },
        { EFTT_OCCLUSION,        4, &CTexture::s_ptexWhite,     &CTexture::s_ptexWhite,    ""        },
        { EFTT_SPECULAR_2,       4, &CTexture::s_ptexWhite,     &CTexture::s_ptexWhite,    "_spec" },

        // This is the terminator for the name-search
        { EFTT_UNKNOWN,          0, &CTexture::s_pTexNULL,      &CTexture::s_pTexNULL,     ""        },
    };

#if 0
    static class Verify
    {
    public:
        Verify()
        {
            for (int i = 0; s_TexSlotSemantics[i].def; i++)
            {
                if (s_TexSlotSemantics[i].slot != i)
                {
                    throw std::runtime_error("Invalid texture slot lookup array.");
                }
            }
        }
    }
    s_VerifyTexSlotSemantics;
#endif
}

namespace  TextureHelpers
{
    bool VerifyTexSuffix(EEfResTextures texSlot, const char* texPath)
    {
        assert((texSlot >= 0) && (texSlot < EFTT_MAX));
        return (strlen(texPath) > strlen(s_TexSlotSemantics[texSlot].suffix) && (CryStringUtils::stristr(texPath, s_TexSlotSemantics[texSlot].suffix)));
    }

    bool VerifyTexSuffix(EEfResTextures texSlot, const string& texPath)
    {
        assert((texSlot >= 0) && (texSlot < EFTT_MAX));
        return (texPath.size() > strlen(s_TexSlotSemantics[texSlot].suffix) && (CryStringUtils::stristr(texPath, s_TexSlotSemantics[texSlot].suffix)));
    }

    const char* LookupTexSuffix(EEfResTextures texSlot)
    {
        assert((texSlot >= 0) && (texSlot < EFTT_MAX));
        return s_TexSlotSemantics[texSlot].suffix;
    }

    int8 LookupTexPriority(EEfResTextures texSlot)
    {
        assert((texSlot >= 0) && (texSlot < EFTT_MAX));
        return s_TexSlotSemantics[texSlot].priority;
    }

    CTexture* LookupTexDefault(EEfResTextures texSlot)
    {
        assert((texSlot >= 0) && (texSlot < EFTT_MAX));
        return *s_TexSlotSemantics[texSlot].def;
    }

    CTexture* LookupTexNeutral(EEfResTextures texSlot)
    {
        assert((texSlot >= 0) && (texSlot < EFTT_MAX));
        return *s_TexSlotSemantics[texSlot].neutral;
    }
}
