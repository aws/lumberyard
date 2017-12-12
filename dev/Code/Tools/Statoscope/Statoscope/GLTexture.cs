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

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Drawing;
using System.Drawing.Imaging;
using CsGL.OpenGL;

namespace Statoscope
{
  class GLTexture : IDisposable
  {
    uint m_texture = 0;

    public bool Valid
    {
      get { return m_texture != 0; }
    }

    public GLTexture()
    {
    }

    public void Update(Bitmap bm)
    {
      if (bm != null)
      {
        BitmapData bd = bm.LockBits(new Rectangle(0, 0, bm.Width, bm.Height), ImageLockMode.ReadOnly, PixelFormat.Format32bppArgb);

        if (bd.Stride != bd.Width * 4)
          throw new ApplicationException("Stride not a multiple of width");

        unsafe
        {
          uint tex = m_texture;
          if (tex == 0)
            OpenGL.glGenTextures(1, &tex);
          m_texture = tex;

          OpenGL.glBindTexture(OpenGL.GL_TEXTURE_2D, m_texture);
          OpenGL.glTexParameteri(OpenGL.GL_TEXTURE_2D, OpenGL.GL_TEXTURE_MIN_FILTER, (int)OpenGL.GL_LINEAR_MIPMAP_LINEAR);
          OpenGL.glTexParameteri(OpenGL.GL_TEXTURE_2D, OpenGL.GL_TEXTURE_MAG_FILTER, (int)OpenGL.GL_LINEAR_MIPMAP_LINEAR);
          OpenGL.glPixelStorei(OpenGL.GL_UNPACK_ALIGNMENT, 1);

          //OpenGL.glTexImage2D(OpenGL.GL_TEXTURE_2D, 0, (int)OpenGL.GL_RGBA8, (int)m_atlasWidth, (int)m_atlasHeight, 0, OpenGL.GL_BGRA_EXT, OpenGL.GL_UNSIGNED_BYTE, bd.Scan0.ToPointer());
          GLU.gluBuild2DMipmaps(OpenGL.GL_TEXTURE_2D, (int)OpenGL.GL_RGBA8, (int)bm.Width, (int)bm.Height, OpenGL.GL_BGRA_EXT, OpenGL.GL_UNSIGNED_BYTE, bd.Scan0.ToPointer());
        }

        bm.UnlockBits(bd);
      }
    }

    public void Bind()
    {
      OpenGL.glBindTexture(OpenGL.GL_TEXTURE_2D, m_texture);
    }

    public void Unbind()
    {
      OpenGL.glBindTexture(OpenGL.GL_TEXTURE_2D, 0);
    }

    public void Dispose()
    {
      if (m_texture != 0)
      {
        unsafe
        {
          uint tex = m_texture;
          OpenGL.glDeleteTextures(1, &tex);
          m_texture = 0;
        }
      }
    }
  }
}
