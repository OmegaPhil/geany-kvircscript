// Scintilla source code edit control
/** @file LexKVIrc.cxx
 ** Lexer for KVIrc script.
 **/
// Copyright 2013 by OmegaPhil <OmegaPhil+scintilla@gmail.com>, based in
// part from LexPython, Copyright 1998-2002 by Neil Hodgson <neilh@scintilla.org>

// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif


// KVIrc Script syntactic rules: http://www.kvirc.net/doc/doc_syntactic_rules.html


/* Interface function called by Scintilla to request some text to be
 syntax highlighted */
static void ColouriseKVIrcDoc(unsigned int startPos, int length,
                              int initStyle, WordList *keywordlists[],
                              Accessor &styler)
{
    // Fetching style context
    //int endPos = startPos + length;
    StyleContext sc(startPos, length, initStyle, styler);

    // Accessing keywords
    WordList &keywords = *keywordlists[0];

    // Looping for all characters
    for( ; sc.More(); sc.Forward())
    {
        // Dealing with different states
        switch (sc.state)
        {
            case SCE_KVIRC_DEFAULT:

                // Detecting comments - only single-line comments exist
                if (sc.ch == '#')
                {
                    sc.SetState(SCE_KVIRC_COMMENT);
                    break;
                }

                // TODO: IsAWordChar
            case SCE_KVIRC_COMMENT:

                // Breaking out of comment when a newline is introduced
                if (sc.ch == '\r' || sc.ch == '\n')
                {
                    sc.SetState(SCE_KVIRC_DEFAULT);
                    break;
                }
    
        }
    }
    
    // TODO: In geany highlighting.c, more hardcoding needed - highlighting_is_string_style and highlighting_is_comment_style?

    sc.Complete();

    // Debug code
    //styler.ColourTo(length + startPos, SCE_KVIRC_COMMENT);
}

// For word classification, see classifyWordCmake

// Registering lexer and interface function
LexerModule lmKVIrc(SCLEX_KVIRC, ColouriseKVIrcDoc, "kvirc"); 
