// Scintilla source code edit control
/** @file LexKVIrc.cxx
 ** Lexer for KVIrc script.
 **/
// Copyright 2013 by OmegaPhil <OmegaPhil+scintilla@gmail.com>
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


/* Interface function called by Scintilla to request some text to be
 syntax highlighted? */
static void ColouriseKVIrcDoc(unsigned int startPos, int length,
                              int initStyle, WordList *keywordlists[],
                              Accessor &styler)
{
    // initStyle - Starting state

    // Identify characters of the same state, then at the end of the range call the colourto. What state do you pass in? 'The colouring'.

    // Test - colourise everything as comments, and see if geany acknowledges this lexer
    styler.ColourTo(startPos + length, SCE_KVIRC_COMMENT);
}

// For word classification, see classifyWordCmake

// Registering lexer
LexerModule lmKVIrc(SCLEX_KVIRC, ColouriseKVIrcDoc, "kvirc"); 
