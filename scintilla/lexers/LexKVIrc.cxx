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


/* KVIrc Script syntactic rules: http://www.kvirc.net/doc/doc_syntactic_rules.html */

/* Utility functions */
static inline bool IsAWordChar(int ch) {

    /* Keyword list includes modules, i.e. words including '.' */
    return (ch < 0x80) && (isalnum(ch) || ch == '_' || ch == '.');
}
static inline bool IsAWordStart(int ch) {

    /* Functions (start with '$') are treated separately to keywords */
    return (ch < 0x80) && (isalnum(ch) || ch == '_' );
}

/* Interface function called by Scintilla to request some text to be
 syntax highlighted */
static void ColouriseKVIrcDoc(unsigned int startPos, int length,
                              int initStyle, WordList *keywordlists[],
                              Accessor &styler)
{
    /* Fetching style context */
    StyleContext sc(startPos, length, initStyle, styler);

    /* Accessing keywords and function-marking keywords */
    WordList &keywords = *keywordlists[0];
    WordList &functionKeywords = *keywordlists[1];

    /* Looping for all characters - only automatically moving forward
     * when asked for (transitions leaving strings and keywords do this
     * already) */
    bool next;
    for( next = 1; sc.More(); next ? sc.Forward() : (void)0 )
    {
        /* Resetting next */
        next = 1;

        /* Dealing with different states */
        switch (sc.state)
        {
            case SCE_KVIRC_DEFAULT:

                /* Detecting single-line comments
                 * Unfortunately KVIrc script allows raw '#<channel
                 * name>' to be used, and appending # to an array returns
                 * its length...
                 * Going for a compromise where single line comments not
                 * starting on a newline are only allowed when they are
                 * preceeded and succeeded by a space
                 */
                if (
                    (sc.ch == '#' && sc.atLineStart) ||
                    (sc.ch == '#' && sc.chPrev == ' ' && sc.chNext == ' ')
                )
                {
                    sc.SetState(SCE_KVIRC_COMMENT);
                    break;
                }

                /* Detecting multi-line comments */
                if (sc.Match('/', '*'))
                {
                    sc.SetState(SCE_KVIRC_COMMENTBLOCK);
                    break;
                }

                /* Detecting strings */
                if (sc.ch == '"')
                {
                    sc.SetState(SCE_KVIRC_STRING);
                    break;
                }

                /* Detecting functions */
                if (sc.ch == '$')
                {
                    sc.SetState(SCE_KVIRC_FUNCTION);
                    break;
                }

                /* Detecting variables */
                if (sc.ch == '%')
                {
                    sc.SetState(SCE_KVIRC_VARIABLE);
                    break;
                }

                /* Detecting numbers */
                if (isdigit(sc.ch))
                {
                    sc.SetState(SCE_KVIRC_NUMBER);
                    break;
                }

                // Detecting words
                if (IsAWordStart(sc.ch) && IsAWordChar(sc.chNext))
                {
                    sc.SetState(SCE_KVIRC_WORD);
                    sc.Forward();
                    break;
                }

                /* Detecting operators */
                if (isoperator(sc.ch))
                {
                    sc.SetState(SCE_KVIRC_OPERATOR);
                    break;
                }

                break;

            case SCE_KVIRC_COMMENT:

                /* Breaking out of single line comment when a newline
                 * is introduced */
                if (sc.ch == '\r' || sc.ch == '\n')
                {
                    sc.SetState(SCE_KVIRC_DEFAULT);
                    break;
                }

                break;

            case SCE_KVIRC_COMMENTBLOCK:

                /* Detecting end of multi-line comment */
                if (sc.Match('*', '/'))
                {
                    // Moving the current position forward two characters
                    // so that '*/' is included in the comment
                    sc.Forward(2);
                    sc.SetState(SCE_KVIRC_DEFAULT);

                    /* Comment has been exited and the current position
                     * moved forward, yet the new current character
                     * has yet to be defined - loop without moving
                     * forward again */
                    next = 0;
                    break;
                }

                break;

            case SCE_KVIRC_STRING:

                /* Detecting end of string - closing speechmarks */
                if (sc.ch == '"')
                {
                    /* Allowing escaped speechmarks to pass */
                    if (sc.chPrev == '\\')
                        break;

                    /* Moving the current position forward to capture the
                     * terminating speechmarks, and ending string */
                    sc.ForwardSetState(SCE_KVIRC_DEFAULT);

                    /* String has been exited and the current position
                     * moved forward, yet the new current character
                     * has yet to be defined - loop without moving
                     * forward again */
                    next = 0;
                    break;
                }

                /* Breaking out of a string when a newline is introduced */
                if (sc.ch == '\r' || sc.ch == '\n')
                {
                    /* Allowing escaped newlines */
                    if (sc.chPrev == '\\')
                        break;

                    sc.SetState(SCE_KVIRC_DEFAULT);
                    break;
                }

                break;

            case SCE_KVIRC_FUNCTION:
            case SCE_KVIRC_VARIABLE:

                /* Detecting the end of a function/variable (word) */
                if (!IsAWordChar(sc.ch))
                {
                    sc.SetState(SCE_KVIRC_DEFAULT);

                    /* Word has been exited yet the current character
                     * has yet to be defined - loop without moving
                     * forward again */
                    next = 0;
                    break;
                }

                break;

            case SCE_KVIRC_NUMBER:

                /* Detecting the end of a number */
                if (!isdigit(sc.ch))
                {
                    sc.SetState(SCE_KVIRC_DEFAULT);

                    /* Number has been exited yet the current character
                     * has yet to be defined - loop without moving
                     * forward */
                    next = 0;
                    break;
                }

                break;

            case SCE_KVIRC_OPERATOR:

                // Detecting the end of an operator
                if (!isoperator(sc.ch))
                {
                    sc.SetState(SCE_KVIRC_DEFAULT);

                    /* Operator has been exited yet the current character
                     * has yet to be defined - loop without moving
                     * forward */
                    next = 0;
                    break;
                }

                break;

            case SCE_KVIRC_WORD:

                /* Detecting the end of a word */
                if (!IsAWordChar(sc.ch))
                {
                    /* Checking if the word was actually a keyword -
                     * fetching the current word, NULL-terminated like
                     * the keyword list */
                    char s[100];
                    int wordLen = sc.currentPos - styler.GetStartSegment();
                    if (wordLen > 100)
                        wordLen = 100;
                    int i;
                    for( i = 0; i < wordLen; ++i )
                    {
                        s[i] = styler.SafeGetCharAt( styler.GetStartSegment() + i );
                    }
                    s[wordLen] = '\0';

                    /* Actually detecting keywords and fixing the state */
                    if (keywords.InList(s))
                    {
                        /* The SetState call actually commits the
                         * previous keyword state */
                        sc.ChangeState(SCE_KVIRC_KEYWORD);
                    }
                    else if (functionKeywords.InList(s))
                    {
                        // Detecting function keywords and fixing the state
                        sc.ChangeState(SCE_KVIRC_FUNCTION_KEYWORD);
                    }

                    /* Transitioning to default and committing the previous
                     * word state */
                    sc.SetState(SCE_KVIRC_DEFAULT);

                    /* Word has been exited yet the current character
                     * has yet to be defined - loop without moving
                     * forward again */
                    next = 0;
                    break;
                }

                break;
        }
    }

    /* Indicating processing is complete */
    sc.Complete();
}

static void FoldKVIrcDoc(unsigned int startPos, int length, int /*initStyle - unused*/,
                      WordList *[], Accessor &styler)
{
    // TODO
}

/* Registering wordlists */
static const char *const kvircWordListDesc[] = {
	"primary",
	"function_keywords",
	0
};


/* Registering functions and wordlists */
LexerModule lmKVIrc(SCLEX_KVIRC, ColouriseKVIrcDoc, "kvirc", FoldKVIrcDoc,
                    kvircWordListDesc);
