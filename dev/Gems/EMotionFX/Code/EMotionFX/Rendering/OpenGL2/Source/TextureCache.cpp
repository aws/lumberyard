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

#include "GLInclude.h"
#include "TextureCache.h"
#include "GraphicsManager.h"
#include <MCore/Source/LogManager.h>
#include <MCore/Source/StringConversions.h>


namespace RenderGL
{
    // constructor
    Texture::Texture()
    {
        mTexture    = 0;
        mWidth      = 0;
        mHeight     = 0;
    }


    // constructor
    Texture::Texture(GLuint texID, uint32 width, uint32 height)
    {
        mTexture    = texID;
        mWidth      = width;
        mHeight     = height;
    }


    // destructor
    Texture::~Texture()
    {
        glDeleteTextures(1, &mTexture);
    }


    // constructor
    TextureCache::TextureCache()
    {
        mWhiteTexture           = nullptr;
        mDefaultNormalTexture   = nullptr;

        mEntries.SetMemoryCategory(MEMCATEGORY_RENDERING);
        mEntries.Reserve(128);
    }


    // destructor
    TextureCache::~TextureCache()
    {
        Release();
    }


    // initialize
    bool TextureCache::Init()
    {
        // create the white dummy texture
        const bool whiteTexture  = CreateWhiteTexture();
        const bool normalTexture = CreateDefaultNormalTexture();
        return (whiteTexture && normalTexture);
    }


    // release all textures
    void TextureCache::Release()
    {
        // delete all textures
        const uint32 numEntries = mEntries.GetLength();
        for (uint32 i = 0; i < numEntries; ++i)
        {
            delete mEntries[i].mTexture;
        }

        // clear all entries
        mEntries.Clear();

        // delete the white texture
        delete mWhiteTexture;
        mWhiteTexture = nullptr;

        delete mDefaultNormalTexture;
        mDefaultNormalTexture = nullptr;
    }


    // add the texture to the cache (assume there are no duplicate names)
    void TextureCache::AddTexture(const char* filename, Texture* texture)
    {
        mEntries.AddEmpty();
        mEntries.GetLast().mName    = filename;
        mEntries.GetLast().mTexture = texture;
    }


    // try to locate a texture based on its name
    Texture* TextureCache::FindTexture(const char* filename) const
    {
        // get the number of entries and iterate through them
        const uint32 numEntries = mEntries.GetLength();
        for (uint32 i = 0; i < numEntries; ++i)
        {
            if (AzFramework::StringFunc::Equal(mEntries[i].mName.c_str(), filename, false /* no case */)) // non-case-sensitive name compare
            {
                return mEntries[i].mTexture;
            }
        }

        // not found
        return nullptr;
    }


    // check if we have a given texture in the cache
    bool TextureCache::CheckIfHasTexture(Texture* texture) const
    {
        // get the number of entries and iterate through them
        const uint32 numEntries = mEntries.GetLength();
        for (uint32 i = 0; i < numEntries; ++i)
        {
            if (mEntries[i].mTexture == texture)
            {
                return true;
            }
        }

        return false;
    }


    // remove an item from the cache
    void TextureCache::RemoveTexture(Texture* texture)
    {
        const uint32 numEntries = mEntries.GetLength();
        for (uint32 i = 0; i < numEntries; ++i)
        {
            if (mEntries[i].mTexture == texture)
            {
                delete mEntries[i].mTexture;
                mEntries.Remove(i);
                return;
            }
        }
    }

    // create the white texture
    bool TextureCache::CreateWhiteTexture()
    {
        GLuint textureID;
        glGenTextures(1, &textureID);

        uint32 width = 2;
        uint32 height = 2;
        uint32 imageBuffer[4];
        for (uint32 i = 0; i < 4; ++i)
        {
            imageBuffer[i] = MCore::RGBA(255, 255, 255, 255); // actually abgr
        }
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageBuffer);
        glBindTexture(GL_TEXTURE_2D, 0);

        mWhiteTexture = new Texture(textureID, width, height);
        return true;

        /*
            uint32 whiteTextureID;

            glEnable( GL_TEXTURE_2D );
            glGenTextures( 1, &whiteTextureID );
            glBindTexture( GL_TEXTURE_2D, whiteTextureID );
            //glTexParameterf(GL_TEXTURE_2D,    GL_TEXTURE_ALLOCATION_HINT_SCE, GL_TEXTURE_SWIZZLED_GPU_SCE );
            //glTexParameteri(GL_TEXTURE_2D,    GL_TEXTURE_ALLOCATION_HINT_SCE, GL_TEXTURE_LINEAR_SYSTEM_SCE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

            // init texture data
            uint8* textureData = (uint8*)MCore::Allocate( 4 );
            textureData[0] = 255;   // init rgba to white
            textureData[1] = 255;
            textureData[2] = 255;
            textureData[3] = 255;

            // allocate texture
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, textureData );

            MCore::Free( textureData );

        //  assert( glGetError() == GL_NO_ERROR );
            if (glGetError() != GL_NO_ERROR)
            {
                MCore::LogWarning("[OpenGL] Failed to create white texture.");
                return false;
            }

            // create Texture object
            mWhiteTexture = new Texture(whiteTextureID, 1, 1);

            glDisable(GL_TEXTURE_2D);
            return true;
        */
    }


    // create the white texture
    bool TextureCache::CreateDefaultNormalTexture()
    {
        /*
            // load the texture
            uint32 width, height;
            uint32 flags = 0;

            AZStd::string filename;
            filename = AZStd::string::format("%sNormalTexture.png", GetGraphicsManager()->GetShaderPath() );

            GLuint textureID = 0;
            textureID = SOIL_load_OGL_texture( filename.AsChar(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, flags , &width, &height );

            // check if loading worked fine
            if (textureID == 0)
            {
                MCore::LogError("[OpenGL] Failed to load default normal texture from file '%s'.", filename.AsChar());
                return false;
            }
            else
            {
                LogDetailedInfo("[OpenGL] Successfully loaded default normal texture '%s'.", filename.AsChar());
                glBindTexture( GL_TEXTURE_2D, textureID );
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glBindTexture( GL_TEXTURE_2D, 0 );
            }
        */

        GLuint textureID;
        glGenTextures(1, &textureID);

        uint32 width = 2;
        uint32 height = 2;
        uint32 imageBuffer[4];
        for (uint32 i = 0; i < 4; ++i)
        {
            imageBuffer[i] = MCore::RGBA(255, 128, 128, 255); // opengl wants abgr
        }
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageBuffer);
        glBindTexture(GL_TEXTURE_2D, 0);

        mDefaultNormalTexture = new Texture(textureID, width, height);
        return true;

        /*
            uint32 textureID;

            glEnable( GL_TEXTURE_2D );
            glGenTextures( 1, &textureID );
            glBindTexture( GL_TEXTURE_2D, textureID );
            //glTexParameterf(GL_TEXTURE_2D,    GL_TEXTURE_ALLOCATION_HINT_SCE, GL_TEXTURE_SWIZZLED_GPU_SCE );
            //glTexParameteri(GL_TEXTURE_2D,    GL_TEXTURE_ALLOCATION_HINT_SCE, GL_TEXTURE_LINEAR_SYSTEM_SCE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

            // init texture data
            uint8* textureData = (uint8*)MCore::Allocate( 4 );

            // abgr
            textureData[0] = 255;   //
            textureData[1] = 255;
            textureData[2] = 128;
            textureData[3] = 128;

            // allocate texture
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, textureData );
            MCore::Free( textureData );

        //  assert( glGetError() == GL_NO_ERROR );
            if (glGetError() != GL_NO_ERROR)
            {
                MCore::LogWarning("[OpenGL] Failed to create white texture.");
                return false;
            }

            // create Texture object
            mDefaultNormalTexture = new Texture(textureID, 1, 1);

            glDisable(GL_TEXTURE_2D);
            return true;
        */
    }
} // namespace RenderGL
