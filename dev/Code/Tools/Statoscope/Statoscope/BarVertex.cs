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

namespace Statoscope
{
  [System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Explicit)]
  struct BarVertex
  {
    public BarVertex(float x, float y, float z, uint color)
    {
      this.x = x;
      this.y = y;
      this.z = z;
      this.color = color;
    }

    [System.Runtime.InteropServices.FieldOffset(0)]
    public float x;
    [System.Runtime.InteropServices.FieldOffset(4)]
    public float y;
    [System.Runtime.InteropServices.FieldOffset(8)]
    public float z;
    [System.Runtime.InteropServices.FieldOffset(12)]
    public uint color;
  }
}
