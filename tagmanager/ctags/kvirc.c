/*
*   Copyright (c) 2000-2003, Darren Hiebert
*   Copyright (c) 2013, OmegaPhil (OmegaPhil+geany@gmail.com)
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License.
*
*   This module contains functions for generating tags for KVIrc Script
*   language files.
*/
/*
*   INCLUDE FILES
*/
#include "general.h"  /* must always come first */

#include <string.h>

#include "entry.h"
#include "options.h"
#include "read.h"
#include "main.h"
#include "vstring.h"
#include "nestlevel.h"


/*
*   DATA DEFINITIONS
*/
typedef enum {
    K_ALIAS,
    K_EVENT,
    K_VARIABLE
} kvircKind;

/* Based on s_tag_type_names values from tm_tag.c */
static kindOption KVIrcKinds[] = {
    {TRUE, 'a', "function", "alias"},
    {TRUE, 'e', "namespace", "event"},
    {TRUE, 'v', "variable", "variable"}
};

/*
*   FUNCTION DEFINITIONS
*/

#define vStringLast(vs) ((vs)->buffer[(vs)->length - 1])

static boolean isIdentifierCharacter (int c, boolean allowNamespaces)
{
    /* Hack to allow ':' character when there is a possibility of the
     * 'identifier' including 2nd level or higher namespaces */
    return (boolean) ( isalnum (c) || c == '_' ||
                     ( allowNamespaces && c == ':' ));
}

static const char *skipSpace (const char *cp)
{
    while (isspace ((int) *cp))
        ++cp;
    return cp;
}

/* Starting at ''cp'', parse an identifier into ''identifier''. */
static const char *parseIdentifier (const char *cp,
                                    vString *const identifier,
                                    boolean allowNamespaces)
{
    vStringClear (identifier);
    while (isIdentifierCharacter ((int) *cp, allowNamespaces))
    {
        vStringPut (identifier, (int) *cp);
        ++cp;
    }
    vStringTerminate (identifier);
    return cp;
}

static void findKVIrcTags (void)
{
    /* Preparing vStrings */
    vString *const name = vStringNew ();
    vString *const aliasName = vStringNew ();
    vString *const handlerName = vStringNew ();
    vString *const marker = vStringNewInit ( "*" );  // Used for global variables

    /* String list to record where events have been come across and
     * therefore already added to the list
     * String list is also used in js.c */
    stringList* seenEvents = stringListNew();

    /* Same for alias namespaces and global variables*/
    stringList* seenAliasNamespaces = stringListNew();
    stringList* seenGlobalVariables = stringListNew();

    /* Looping for all lines */
    const char *line;
    while(( line = (const char *)fileReadLine()) != NULL )
    {
        const char *cp = line;

        /* Ignoring leading whitespace */
        cp = skipSpace( cp );

        /* Skipping blank line */
        if( *cp == '\0' )
            continue;

        /* Skipping comments */
        if( *cp == '#' || strncmp( cp, "/*", 2 ) == 0 )
            continue;

        /* Detecting aliases
         * alias(<alias name>)
         * Example: 'alias(ChannelServicesScript::Startup)' */
        if( strncmp( cp, "alias(", 6 ) == 0 )
        {
            /* Obtaining the alias' name from the next identifier - cp
             * is moved on as appropriate */
            cp += 6;
            cp = parseIdentifier( cp, name, FALSE );

            /* This might actually be the alias' namespace, terminated
             * with '::' */
            if( strncmp( cp, "::", 2 ) == 0 )
            {
                /* It is - extracting the actual alias name. Hack to
                 * allow multi-level namespaces in the remaining alias
                 * name */
                cp += 2;
                cp = parseIdentifier( cp, aliasName, TRUE );

                /* If the alias namespace hasn't been registered already,
                 * adding */
                if ( !stringListHas( seenAliasNamespaces,
                                     vStringValue( name ) ))
                {
                    stringListAdd( seenAliasNamespaces,
                                   vStringNewCopy( name ) );
                    makeSimpleTag( name, KVIrcKinds, K_ALIAS );
                }

                /* Registering 'scoped' alias under its namespace */
                makeSimpleScopedTag( aliasName, KVIrcKinds, K_ALIAS,
                                     "function", vStringValue( name ), NULL );

                /* Debug code */
                /*printf( "Registered alias: Namespace: %s, name: %s\n",
                            vStringValue( name ), vStringValue( aliasName ) );*/

                /* Clearing up */
                vStringClear( aliasName );
            }
            else
            {
                /* No namespace detected - registering plain alias */
                makeSimpleTag( name, KVIrcKinds, K_ALIAS );

                /* Debug code */
                //printf( "Registered alias: %s\n", vStringValue( name ) );
            }

            /* Clearing up */
            vStringClear( name );

            /* Moving to next line */
            continue;
        }

        /* Detecting events
         * event(<event name>,<handler name>)
         * Example: 'event(OnKVIrcStartup,ChannelServicesScript)' */
        if( strncmp( cp, "event(", 6) == 0 )
        {
            /* Obtaining the event's name from the next identifier - cp
             * is moved on as appropriate */
            cp += 6;
            cp = parseIdentifier( cp, name, FALSE );

            /* Hack to stop geany using inappropriate parents in scoped
             * tags - events have one space appended */
            vStringCatS( name, " " );

            /* Skipping invalid event handlers */
            cp = skipSpace( cp );
            if( *cp != ',' )
                continue;

            /* Obtaining event handler name */
            ++cp;
            cp = skipSpace( cp );
            cp = parseIdentifier( cp, handlerName, FALSE );

            /* If a handler of the event type hasn't been registered
             * already, adding the parent node */
            if ( !stringListHas( seenEvents, vStringValue( name ) ))
            {
                stringListAdd( seenEvents, vStringNewCopy( name ) );
                makeSimpleTag( name, KVIrcKinds, K_EVENT );
            }

            /* Registering 'scoped' event handler - since you can have
             * many handlers for the same event */
            makeSimpleScopedTag( handlerName, KVIrcKinds, K_EVENT, "namespace",
                                 vStringValue( name ), NULL );

            /* Debug code */
            /*printf( "Registered event: %s, %s\n", vStringValue( name ),
                    vStringValue( handlerName ) );*/

            /* Clearing up */
            vStringClear( name );
            vStringClear( handlerName );

            /* Moving to next line */
            continue;
        }

        /* Detecting assumed assignments to global variables */
        if( *cp == '%' && isupper( *(cp + 1) ))
        {
            /* Obtaining the variable name from the next identifier - cp
             * is moved on as appropriate */
            ++cp;
            cp = parseIdentifier( cp, name, FALSE );

            /* Hack to stop geany using inappropriate parents in scoped
             * tags - variables have two spaces appended */
            vStringCatS( name, "  " );

            /* If an assignment to the global variable hasn't been
             * registered already, adding */
            if ( !stringListHas( seenGlobalVariables, vStringValue( name ) ))
            {
                stringListAdd( seenGlobalVariables, vStringNewCopy( name ) );
                makeSimpleTag( name, KVIrcKinds, K_VARIABLE );
            }

            /* Registering this instance of assignment to the global
             * variable - text is just '*' as its just a marker */
            makeSimpleScopedTag( marker, KVIrcKinds, K_VARIABLE, "variable",
                                 vStringValue( name ), NULL );

            /* Debug code */
            //printf( "Registered variable: %s\n", vStringValue( name ) );

            /* Clearing up */
            vStringClear( name );

            /* Moving to next line */
            continue;
        }
    }

    /* Clean up all memory we allocated. */
    vStringDelete( aliasName );
    vStringDelete( handlerName );
    vStringDelete( marker );
    vStringDelete( name );
    stringListDelete( seenAliasNamespaces );
    stringListDelete( seenEvents );
    stringListDelete( seenGlobalVariables );
}

extern parserDefinition *KVIrcParser (void)
{
    static const char *const extensions[] = { "kvs", NULL };
    parserDefinition *def = parserNew ("KVIrc");
    def->kinds = KVIrcKinds;
    def->kindCount = KIND_COUNT (KVIrcKinds);
    def->extensions = extensions;
    def->parser = findKVIrcTags;
    return def;
}
