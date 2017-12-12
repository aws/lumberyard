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

#ifndef CRYINCLUDE_CRYCOMMON_DEBUGUTILS_H
#define CRYINCLUDE_CRYCOMMON_DEBUGUTILS_H

#pragma once

#include <stdio.h>
#include <IRenderer.h>
#include <ITextModeConsole.h>

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

/**
 * Helper for debug text printing, with colorization and formatting options.
 *
 * Example:
 *
 *  DebugTextHelper text( gEnv->pRenderer, 100, 100 ); // At screen pos (100,100)
 *  text.SetAutoNewlined( true );
 *  text.SetMonospaced( true );
 *  text.SetColor( Col_Yellow );
 *  text.SetSize( 2.f );
 *  text.AddText( "This is a yellow title" );
 *  text.SetSize( 1.5f );
 *  text.AddText( "This is some detail." );
 */

class DebugTextHelper
{
public:
    static const int kTextModeRowSize = 10;
    static const int kTextModeColSize = 10;
    static const int kTextModeColCount = 128;   // WINDOWS_CONSOLE_WIDTH
    static const int kTextModeRowCount = 48;    // WINDOWS_CONSOLE_HEIGHT - 2

    DebugTextHelper(IRenderer* r,
        float x = 0.f, float y = 0.f,
        float fontSize = 1.5f,
        ColorF defaultColor = Col_White)
        : m_pos(x, y, 0.f)
        , m_defaultColor(defaultColor)
        , m_fontSize(fontSize)
        , m_flags(kFlag_AutoNewline | kFlag_Monospaced | kFlag_TextModeConsole)
        , m_renderer(r)
    {
    }

    ~DebugTextHelper() {};

    enum
    {
        kMaxLabelSize = 512
    };

    void AddText(const char* format, ...)
    {
        va_list argList;
        char buffer[ kMaxLabelSize ];
        va_start(argList, format);
        const int len = vsnprintf_s(buffer, sizeof(buffer), sizeof(buffer) - 1, format, argList);
        buffer[ sizeof(buffer) - 1 ] = '\0';
        va_end(argList);

        AddText(m_defaultColor, buffer);
    }

    void AddText(ColorF color, const char* format, ...)
    {
        va_list argList;
        char buffer[ kMaxLabelSize ];
        va_start(argList, format);
        const int len = vsnprintf_s(buffer, sizeof(buffer), sizeof(buffer) - 1, format, argList);
        buffer[ sizeof(buffer) - 1 ] = '\0';
        va_end(argList);

        const uint32 drawFlags = IsMonospaced() ?
            (eDrawText_2D | eDrawText_800x600 | eDrawText_FixedSize | eDrawText_Monospace) :
            (eDrawText_2D | eDrawText_800x600 | eDrawText_FixedSize);

        m_renderer->Draw2dLabelWithFlags(m_pos.x, m_pos.y, m_fontSize, color, drawFlags, buffer);

        if (m_flags & kFlag_TextModeConsole)
        {
            if (ITextModeConsole* textConsole = gEnv->pSystem->GetITextModeConsole())
            {
                const int posX = static_cast<int>(m_pos.x) / kTextModeColSize;
                const int posY = static_cast<int>(m_pos.y) / kTextModeRowSize;

                if (posX < kTextModeColCount && posY < kTextModeRowCount)
                {
                    textConsole->PutText(posX, posY, buffer);
                }
            }
        }

        if (m_flags & kFlag_AutoNewline)
        {
            Newline();
        }
    }

    void ClearLines(float startY, float height)
    {
        const int startRow = std::min(static_cast<int>(startY) / kTextModeRowSize, int(kTextModeRowCount));
        const int numRows = std::min(static_cast<int>(height) / kTextModeRowSize, int(kTextModeRowCount));

        // Text mode console requires wiping the frame buffer to maintain a reasonable
        // quality display. The frame profiler doesn't do this, and as a result is
        // sometimes completely unreadable. It's very slow to set characters in the
        // console, so for now, we're wiping in scanline fashion.
        if (ITextModeConsole* textConsole = gEnv->pSystem->GetITextModeConsole())
        {
            static char s_emptyLine[ kTextModeColCount + 1 ] = { '\0' };
            if (0 == s_emptyLine[ 0 ])
            {
                for (int col = 0; col < kTextModeColCount; ++col)
                {
                    s_emptyLine[ col ] = ' ';
                }

                s_emptyLine[ kTextModeColCount ] = 0;
            }

            for (int row = startRow; row < numRows; ++row)
            {
                textConsole->PutText(0, row, s_emptyLine);
            }
        }
    }

    inline ColorF GetDefaultColor() const { return m_defaultColor; }
    inline void SetDefaultColor(ColorF color) { m_defaultColor = color; }

    inline Vec3 GetPosition() const { return m_pos; }
    inline void SetPosition(const Vec3& pos) { m_pos = pos; }

    inline float GetFontSize() const { return m_fontSize; }
    inline void SetFontSize(float fontSize) { m_fontSize = fontSize; }

    inline void Newline() { m_pos.y += m_fontSize * 10.f; }

    inline bool IsAutoNewlined() const { return !!(m_flags & kFlag_AutoNewline); }
    inline bool IsMonospaced() const { return !!(m_flags & kFlag_Monospaced); }

    inline void SetAutoNewlined(bool set)
    {
        if (set)
        {
            m_flags |= kFlag_AutoNewline;
        }
        else
        {
            m_flags &= ~kFlag_AutoNewline;
        }
    }

    inline void SetMonospaced(bool set)
    {
        if (set)
        {
            m_flags |= kFlag_Monospaced;
        }
        else
        {
            m_flags &= ~kFlag_Monospaced;
        }
    }

    inline void SetTextModeConsole(bool set)
    {
        if (set)
        {
            m_flags |= kFlag_TextModeConsole;
        }
        else
        {
            m_flags &= ~kFlag_TextModeConsole;
        }
    }

private:

    Vec3            m_pos;
    ColorF          m_defaultColor;

    float           m_fontSize;

    enum Flags
    {
        kFlag_AutoNewline       = (1 << 0),     // Auto advance to next line after AddText().
        kFlag_Monospaced        = (1 << 1),     // Print out using monospaced font.
        kFlag_TextModeConsole   = (1 << 2),     // Write to "text mode console" (dedicated server).
    };

    uint32          m_flags;

    IRenderer*      m_renderer;
};


#endif // CRYINCLUDE_CRYCOMMON_DEBUGUTILS_H
