/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2026 The Open Watcom Contributors. All Rights Reserved.
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  Debugger stub functions.
*
****************************************************************************/


#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <process.h>
#define INCL_DOS
#include <os2.h>
#include "srcmgt.h"
#include "dbgdata.h"
#include "liteng.h"
#include "mad.h"
#include "madcli.h"
#include "dbgitem.h"
#include "dbgreg.h"
#include "dbgmad.h"
#include "dui.h"
#include "dbgvar.h"
#include "dbgstk.h"
#include "trapaccs.h"
#include "dbgscrn.h"
#include "strutil.h"
#include "dbgscan.h"
#include "dbgutil.h"
#include "dbgsrc.h"
#include "dbgexec.h"
#include "dbgexpr2.h"
#include "dbgmain.h"
#include "dbgbrk.h"
#include "dbgass.h"
#include "dbgpend.h"
#include "envlkup.h"
#include "dbgcmd.h"
#include "dbgtrace.h"
#include "trpld.h"
#include "dipimp.h"
#include "wvdipcli.h"
#include "dbgdot.h"
#include "dlgcmd.h"
#include "dbgwintr.h"
#include "dbgchopt.h"
#include "dbgsetfg.h"
#include "dbginsp.h"
#include "dbgwvar1.h"
#include "dlgfile.h"
#include "wndmenu1.h"
#include "dbgwset1.h"
#include "dlgbreak.h"


extern char             *CmdData;

/*************************************************/
/* TODO! review all these prototypes declaration if they are local(static) or external */

bool DlgNewWithSym( const char *title, char *buff, size_t buff_len );
bool DlgUpTheStack( void );
bool DlgAreYouNuts( unsigned long mult );
bool DlgBackInTime( bool warn );
bool DlgIncompleteUndo( void );
void ProcAccel( void );
void ProcDisplay( void );
void ProcFont( void );
void ProcHelp( void );
void ProcInternal( void );
void ProcPaint( void );
void ProcView( void );
void ProcConfigFile( void );
void ConfigDisp( void );
void ConfigFont( void );
void ConfigPaint( void );
int TabIntervalGet( void );
void TabIntervalSet( int new );
void PopErrBox( const char *buff );
/*************************************************/

unsigned                NumLines;
unsigned                NumColumns;

static bool             Done;

unsigned DUIConfigScreen( void )
{
    return( 0 );
}

bool DUIClose( void )
{
    Done = true;
    return( true );
}

#if 0
// The following routine is cut & pasted verbatim from dbgwvar.c
// (which we really don't want to drag in here)
var_node *VarGetDisplayPiece( var_info *i, int row, wnd_piece piece, int *pdepth, int *pinherit )
{
    var_node    *row_v;
    var_node    *v;

    if( piece >= VAR_PIECE_LAST )
        return( NULL );
    if( VarFirstNode( i ) == NULL )
        return( NULL );
    if( row >= VarRowTotal( i ) )
        return( NULL );
    row_v = VarFindRowNode( i, row );
    if( !row_v->value_valid ) {
        VarSetValue( row_v, LIT_ENG( Quest_Marks ) );
        row_v->value_valid = false;
    }
    if( !row_v->gadget_valid ) {
        VarSetGadget( row_v, VARGADGET_NONE );
        row_v->gadget_valid = false;
    }
    v = row_v;
    if( piece == VAR_PIECE_NAME ||
        ( piece == VAR_PIECE_GADGET && row_v->gadget_valid ) ||
        ( piece == VAR_PIECE_VALUE && row_v->value_valid ) ) {
        VarError = false;
    } else if( _IsOff( SW_TASK_RUNNING ) ) {
        if( row == i->exprsp_cacherow && i->exprsp_cache != NULL ) {
            VarError = false;
            v = i->exprsp_cache;
        } else if( row == i->exprsp_cacherow && i->exprsp_cache_is_error ) {
            VarError = true;
            v = NULL;
        } else {
            VarErrState();
            v = VarFindRow( i, row );
            VarOldErrState();
            i->exprsp_cacherow = row;
            i->exprsp_cache = v;
            i->exprsp_cache_is_error = VarError;
        }
        if( v == NULL ) {
            if( !VarError )
                return( NULL );
            v = row_v;
        }
        VarNodeInvalid( v );
        VarErrState();
        ExprValue( ExprSP );
        VarSetGadget( v, VarGetGadget( v ) );
        VarSetOnTop( v, VarGetOnTop( v ) );
        VarSetValue( v, VarGetValue( i, v ) );
        VarOldErrState();
        VarDoneRow( i );
    }
    VarGetDepths( i, v, pdepth, pinherit );
    return( v );
}
#endif

var_info        Locals;
HEV             Requestsem;
HEV             Requestdonesem;

static void DumpLocals( void )
{
    address     addr;

    if( _IsOff( SW_TASK_RUNNING ) ) {
        VarErrState();
        VarInfoRefresh( VAR_LOCALS, &Locals, &addr );
        VarOkToCache( &Locals, true );
    }
#if 0
    {
        int         row;
        var_node    *v;
        int         depth;
        int         inherit;

        for( row = 0; (v = VarGetDisplayPiece( &Locals, row, VAR_PIECE_GADGET, &depth, &inherit )) != NULL; ++row ) {
            v = VarGetDisplayPiece( &Locals, row, VAR_PIECE_NAME, &depth, &inherit );
            v = VarGetDisplayPiece( &Locals, row, VAR_PIECE_VALUE, &depth, &inherit );
            switch( v->gadget ) {
            case VARGADGET_NONE:
                printf( "  " );
                break;
            case VARGADGET_OPEN:
                printf( "+ " );
                break;
            case VARGADGET_CLOSED:
                printf( "- " );
                break;
            case VARGADGET_POINTS:
                printf( "->" );
                break;
            case VARGADGET_UNPOINTS:
                printf( "<-" );
                break;
            }
            VarBuildName( &Locals, v, true );
            printf( " %-20s %s\n", TxtBuff, v->value );
        }
    }
#endif
    if( _IsOff( SW_TASK_RUNNING ) ) {
        VarOkToCache( &Locals, false );
        VarOldErrState();
    }
}

static void DumpSource( void )
{
    char        buff[256];
    DIPHDL( cue, cueh );

    if( _IsOn( SW_TASK_RUNNING ) ) {
        printf( "I don't know where the task is. It's running\n" );
    }
    if( DeAliasAddrCue( NO_MOD, GetCodeDot(), cueh ) == SR_NONE ||
        !DUIGetSourceLine( cueh, buff, sizeof( buff ) ) ) {
        UnAsm( GetCodeDot(), buff, sizeof( buff ) );
    }
    printf( "%s\n", buff );
}

enum {
    REQ_NONE,
    REQ_BYE,
    REQ_GO,
    REQ_TRACE_OVER,
    REQ_TRACE_INTO
} Req = REQ_NONE;

bool RequestDone;

static void APIENTRY ControlFunc( ULONG parm )
{
    ULONG   ulCount;

    parm = parm;
    do {
        DosWaitEventSem( Requestsem, SEM_INDEFINITE_WAIT ); // wait for Request
        DosResetEventSem( Requestsem, &ulCount );
        switch( Req ) {
        case REQ_GO:
            Go( true );
            break;
        case REQ_TRACE_OVER:
            ExecTrace( TRACE_OVER, DbgLevel );
            break;
        case REQ_TRACE_INTO:
            ExecTrace( TRACE_INTO, DbgLevel );
            break;
        }
        DoInput();
        _SwitchOff( SW_TASK_RUNNING );
        DosPostEventSem( Requestdonesem );
    } while( Req != REQ_BYE );
    return; // thread over!
}

static void RunRequest( int req )
{
    ULONG   ulCount;

    if( _IsOn( SW_TASK_RUNNING ) )
        return;
    DosWaitEventSem( Requestdonesem, SEM_INDEFINITE_WAIT ); // wait for last request to finish
    DosResetEventSem( Requestdonesem, &ulCount );
    Req = req;
    _SwitchOn( SW_TASK_RUNNING );
    DosPostEventSem( Requestsem ); // tell worker to go
}

void DlgCmd( void )
{
    char        buff[256];

    printf( "DBG>" );
    fflush( stdout );   // not really necessary
    gets( buff );
    if( buff[0] != NULLCHAR && buff[1] == NULLCHAR ) {
        switch( tolower( buff[0] ) ) {
        case 'u':
            WndAsmInspect( GetCodeDot() );
            break;
        case 's':
            DumpSource();
            break;
        case 'l':
            DumpLocals();
            break;
        case 'i':
            RunRequest( REQ_TRACE_INTO );
            break;
        case 'o':
            RunRequest( REQ_TRACE_OVER );
            break;
        case 'g':
            RunRequest( REQ_GO );
            break;
        case 'x':
            if( _IsOn( SW_REMOTE_LINK ) ) {
                printf( "Can't break remote task!\n" );
            } else {
//
// NYI  OS/2 InterruptProgram must be implemented and exported from TRAP DLL
//          TRAP_EXTFUNC( InterruptProgram )() procedure must call this export
//
//                TRAP_EXTFUNC( InterruptProgram )();
            }
            // break the task
            break;
        default:
            printf( "Error - unrecognized command\n" );
        }
    } else {
        DoCmd( MemStrdupSafe( buff ) );
        DoInput();
    }
}

/*
 * td2ine-compatible command line autostep
 *
 *   dve [options] <program> [args...]
 *     --autostep N          auto-step through N instructions, print state
 *                           per step, then exit (like td2ine)
 *     --autostep-mode M     'into' (default) or 'over'
 *     --autostep-into-api   step into OS/2 API calls (default: step over them)
 *     --trace-calls         only print call/return instructions
 *     --loop-detect         detect repeated address sequences (report to stderr)
 *     --break-at SEG:OFF    run at native speed until PC hits SEG:OFF, then
 *                           report state (alias: --break-linear)
 *     --dump-linear SEG:OFF:LEN
 *                           dump guest memory at the stop (repeatable, max 16)
 *     --symbols <file>     accepted for CLI compatibility (ignored: symbols
 *                           come from the DWARF DIP)
 *     --help                print usage and exit
 */

#define MAX_AUTO_DUMPS  16
#define LOOP_HISTORY     1024
#define LOOP_MIN_SEQ     2
#define LOOP_MAX_SEQ     32

static unsigned        AutoStepCount;
static bool            AutoStepOver;
static bool            AutoStepIntoAPI;
static bool            AutoTraceCalls;
static bool            AutoLoopDetect;
static bool            AutoHaveBreak;
static address         AutoBreakAddr;
static address         AutoDumpAddrs[MAX_AUTO_DUMPS];
static size_t          AutoDumpLens[MAX_AUTO_DUMPS];
static unsigned        AutoDumpCount;

typedef struct {
    addr_seg            seg;
    addr_off            off;
} loop_addr;

static loop_addr       LoopHist[LOOP_HISTORY];
static unsigned        LoopHistCount;
static bool            LoopDetected;
static char            PrevSource[256];

static char *SkipBlanks( char *p )
{
    while( *p == ' ' || *p == '\t' )
        ++p;
    return( p );
}

static size_t TokenLen( char *p )
{
    size_t  len = 0;

    while( p[len] != NULLCHAR && p[len] != ' ' && p[len] != '\t' )
        ++len;
    return( len );
}

static bool TokenIs( char *p, const char *s )
{
    size_t  len = strlen( s );

    return( TokenLen( p ) == len && strncmp( p, s, len ) == 0 );
}

static void PrintUsage( void )
{
    printf( "Usage: dve [options] <program> [args...]\n" );
    printf( "  --autostep N              Auto-step through N instructions and print assembly\n" );
    printf( "  --autostep-mode M         Stepping mode for autostep: 'into' (default) or 'over'\n" );
    printf( "  --autostep-into-api        Step into OS/2 API calls (default: step over them)\n" );
    printf( "  --trace-calls             Only print call/return instructions (with symbols)\n" );
    printf( "  --loop-detect             Detect loops in autostep (repeated address sequences)\n" );
    printf( "  --break-at SEG:OFF         Run at native speed until PC hits SEG:OFF, then report state\n" );
    printf( "  --dump-linear SEG:OFF:LEN Dump guest memory at the stop (repeatable, max %u)\n",
            (unsigned)MAX_AUTO_DUMPS );
    printf( "  --symbols <file>          Accepted for compatibility (ignored: DWARF DIP is used)\n" );
    printf( "  --help                    Print this usage message and exit\n" );
}

static bool ParseHexVal( char **pp, unsigned long *val )
{
    char    *p = *pp;
    char    *end;

    if( p[0] == '0' && ( p[1] == 'x' || p[1] == 'X' ) )
        p += 2;
    *val = strtoul( p, &end, 16 );
    if( end == p )
        return( false );
    *pp = end;
    return( true );
}

static bool ParseSegOff( char **pp, address *addr )
{
    unsigned long   seg;
    unsigned long   off;

    if( !ParseHexVal( pp, &seg ) || **pp != ':' )
        return( false );
    ++*pp;
    if( !ParseHexVal( pp, &off ) )
        return( false );
    addr->mach.segment = (addr_seg)seg;
    addr->mach.offset = (addr_off)off;
    addr->sect_id = 0;
    addr->indirect = 0;
    return( true );
}

static char *BaseName( char *path )
{
    char    *p;

    p = strrchr( path, '\\' );
    if( p == NULL )
        p = strrchr( path, '/' );
    if( p == NULL )
        return( path );
    return( p + 1 );
}

static void ParseAutoStepOptions( char *cmd )
{
    char    *p = cmd;
    char    *end;
    size_t  len;
    char    mode[16];
    unsigned long n;
    unsigned long dumplen;

    for( ;; ) {
        p = SkipBlanks( p );
        if( TokenIs( p, "--autostep" ) ) {
            p += TokenLen( p );
            p = SkipBlanks( p );
            n = strtoul( p, &end, 10 );
            if( end == p ) {
                printf( "autostep: missing step count\n" );
            } else {
                AutoStepCount = (unsigned)n;
                p = end;
            }
        } else if( TokenIs( p, "--autostep-mode" ) ) {
            p += TokenLen( p );
            p = SkipBlanks( p );
            len = TokenLen( p );
            if( len == 0 ) {
                printf( "autostep: missing mode (use into|over)\n" );
            } else {
                if( len >= sizeof( mode ) )
                    len = sizeof( mode ) - 1;
                memcpy( mode, p, len );
                mode[len] = NULLCHAR;
                if( stricmp( mode, "over" ) == 0 ) {
                    AutoStepOver = true;
                } else if( stricmp( mode, "into" ) == 0 ) {
                    AutoStepOver = false;
                } else {
                    printf( "autostep: unknown mode '%s' (use into|over)\n", mode );
                }
                p += TokenLen( p );
            }
        } else if( TokenIs( p, "--autostep-into-api" ) ) {
            AutoStepIntoAPI = true;
            p += TokenLen( p );
        } else if( TokenIs( p, "--trace-calls" ) ) {
            AutoTraceCalls = true;
            p += TokenLen( p );
        } else if( TokenIs( p, "--loop-detect" ) ) {
            AutoLoopDetect = true;
            p += TokenLen( p );
        } else if( TokenIs( p, "--break-at" ) || TokenIs( p, "--break-linear" ) ) {
            p += TokenLen( p );
            p = SkipBlanks( p );
            if( !ParseSegOff( &p, &AutoBreakAddr ) ) {
                printf( "autostep: --break-at expects SEG:OFF (hex)\n" );
            } else {
                AutoHaveBreak = true;
            }
        } else if( TokenIs( p, "--dump-linear" ) ) {
            p += TokenLen( p );
            p = SkipBlanks( p );
            if( AutoDumpCount >= MAX_AUTO_DUMPS ) {
                printf( "autostep: too many --dump-linear (max %u)\n", (unsigned)MAX_AUTO_DUMPS );
                p += TokenLen( p );
            } else if( !ParseSegOff( &p, &AutoDumpAddrs[AutoDumpCount] ) || *p != ':' ) {
                printf( "autostep: --dump-linear expects SEG:OFF:LEN (hex)\n" );
                if( *p == ':' )
                    ++p;
                p += TokenLen( p );
            } else {
                ++p;
                if( !ParseHexVal( &p, &dumplen ) ) {
                    printf( "autostep: --dump-linear expects SEG:OFF:LEN (hex)\n" );
                } else {
                    AutoDumpLens[AutoDumpCount] = (size_t)dumplen;
                    ++AutoDumpCount;
                }
            }
        } else if( TokenIs( p, "--symbols" ) ) {
            p += TokenLen( p );
            p = SkipBlanks( p );
            p += TokenLen( p );
            printf( "autostep: --symbols ignored (symbols come from DWARF debug info)\n" );
        } else if( TokenIs( p, "--help" ) ) {
            PrintUsage();
            exit( 0 );
        } else {
            break;
        }
    }
    /* remove the consumed options from the engine command line */
    memmove( cmd, p, strlen( p ) + 1 );
}

/*
 * SymLookup - find the innermost procedure/code symbol containing addr and
 * return its source name plus the offset of addr from the symbol start
 * (like td2ine's symbol_map_lookup).
 */
static bool SymLookup( address addr, char *buff, size_t buff_size, unsigned long *poff )
{
    DIPHDL( sym, sh );
    sym_info        info;
    location_list   ll;
    address         start;
    size_t          nlen;

    if( DeAliasAddrSym( NO_MOD, addr, sh ) == SR_NONE )
        return( false );
    if( DIPSymInfo( sh, NULL, &info ) != DS_OK )
        return( false );
    if( info.kind != SK_PROCEDURE && info.kind != SK_CODE )
        return( false );
    if( DIPSymLocation( sh, NULL, &ll ) != DS_OK || ll.num == 0 )
        return( false );
    if( ll.e[0].type != LT_ADDR )
        return( false );
    start = ll.e[0].u.addr;
    nlen = DIPSymName( sh, NULL, SNT_SOURCE, buff, buff_size );
    if( nlen == 0 || nlen >= buff_size )
        return( false );
    *poff = (unsigned long)( addr.mach.offset - start.mach.offset );
    return( true );
}

/*
 * LoopRecord - record one step address and check whether the last steps form
 * a short sequence repeated three times (like td2ine --loop-detect).
 */
static void LoopRecord( address addr, unsigned step )
{
    loop_addr       *h;
    int             seqlen;
    int             start;
    int             r;
    int             j;
    bool            match;
    char            symbuff[256];
    unsigned long   symoff;
    address         a;

    if( LoopHistCount < LOOP_HISTORY ) {
        h = &LoopHist[LoopHistCount++];
    } else {
        memmove( &LoopHist[0], &LoopHist[1], ( LOOP_HISTORY - 1 ) * sizeof( LoopHist[0] ) );
        h = &LoopHist[LOOP_HISTORY - 1];
    }
    h->seg = addr.mach.segment;
    h->off = addr.mach.offset;
    if( LoopDetected || LoopHistCount < LOOP_MIN_SEQ * 3 )
        return;
    for( seqlen = LOOP_MIN_SEQ; seqlen <= LOOP_MAX_SEQ && !LoopDetected; ++seqlen ) {
        start = (int)LoopHistCount - seqlen * 3;
        if( start < 0 )
            continue;
        match = true;
        for( r = 0; r < 3 && match; ++r ) {
            for( j = 0; j < seqlen; ++j ) {
                if( LoopHist[start + r * seqlen + j].seg != LoopHist[start + j].seg
                  || LoopHist[start + r * seqlen + j].off != LoopHist[start + j].off ) {
                    match = false;
                    break;
                }
            }
        }
        if( !match )
            continue;
        LoopDetected = true;
        fprintf( stderr, "\n=== LOOP DETECTED (sequence length %d, starting at step %d) ===\n",
                 seqlen, (int)step - seqlen * 2 );
        for( j = 0; j < seqlen; ++j ) {
            a.mach.segment = LoopHist[start + j].seg;
            a.mach.offset = LoopHist[start + j].off;
            a.sect_id = 0;
            a.indirect = 0;
            if( SymLookup( a, symbuff, sizeof( symbuff ), &symoff ) ) {
                fprintf( stderr, "  %04X:%04lX  %s+0x%lX\n", a.mach.segment,
                         (unsigned long)a.mach.offset, symbuff, symoff );
            } else {
                fprintf( stderr, "  %04X:%04lX  (unknown)\n", a.mach.segment,
                         (unsigned long)a.mach.offset );
            }
        }
        fprintf( stderr, "=== Last %d steps before loop ===\n", seqlen * 3 );
    }
}

static void DumpRegisters( void )
{
    const mad_reg_set_data    *rsd;
    const mad_reg_info        *ri;
    const char                *descript;
    size_t                    max_descript;
    size_t                    max_value;
    mad_type_handle           mth;
    mad_radix                 radix, old_radix;
    item_mach                 value;
    char                      valbuff[64];
    unsigned                  piece;
    unsigned                  per_line;
    unsigned                  n;

    if( DbgRegs == NULL )
        return;
    rsd = NULL;
    RegFindData( MTK_INTEGER, &rsd );
    if( rsd == NULL )
        return;
    per_line = MADRegSetDisplayGrouping( rsd );
    if( per_line == 0 )
        per_line = 4;
    n = 0;
    for( piece = 0; ; ++piece ) {
        if( MADRegSetDisplayGetPiece( rsd, &DbgRegs->mr, piece, &descript, &max_descript, &ri, &mth, &max_value ) != MS_OK )
            break;
        if( ri == NULL ) {
            if( n != 0 ) {
                printf( "\n" );
                n = 0;
            }
            printf( "  %s\n", descript == NULL ? "" : descript );
            continue;
        }
        radix = MADTypePreferredRadix( mth );
        old_radix = NewCurrRadix( radix );
        RegValue( &value, ri, DbgRegs );
        max_value = sizeof( valbuff );
        MADTypeHandleToString( radix, mth, &value, valbuff, &max_value );
        NewCurrRadix( old_radix );
        printf( "%s%s=%s", n == 0 ? "  " : " ", descript == NULL ? "?" : descript, valbuff );
        ++n;
        if( n >= per_line ) {
            printf( "\n" );
            n = 0;
        }
    }
    if( n != 0 )
        printf( "\n" );
}

static void PrintStep( unsigned step, address addr, mad_disasm_data *dd, const char *text )
{
    address             sp;
    unsigned_8          bytes[16];
    size_t              got;
    unsigned            nbytes;
    char                sbuff[256];
    char                fbuf[256];
    char                symbuff[256];
    char                *ir;
    char                *base;
    unsigned long       symoff;
    unsigned long       line;
    DIPHDL( cue, cueh );
    unsigned            i;

    printf( "--- Step %u ---\n", step );
    if( SymLookup( addr, symbuff, sizeof( symbuff ), &symoff ) ) {
        if( symoff == 0 ) {
            printf( "  CS:IP=%04X:%04lX  %s\n", addr.mach.segment,
                     (unsigned long)addr.mach.offset, symbuff );
        } else {
            printf( "  CS:IP=%04X:%04lX  %s+0x%lX\n", addr.mach.segment,
                     (unsigned long)addr.mach.offset, symbuff, symoff );
        }
    } else {
        printf( "  CS:IP=%04X:%04lX\n", addr.mach.segment, (unsigned long)addr.mach.offset );
    }
    printf( "  Bytes: " );
    nbytes = 0;
    if( dd != NULL ) {
        nbytes = MADDisasmInsSize( dd );
        if( nbytes > 8 )
            nbytes = 8;
    }
    got = 0;
    if( nbytes > 0 )
        got = MADCliReadMem( addr, nbytes, bytes );
    if( nbytes > 0 && got == nbytes ) {
        for( i = 0; i < nbytes; ++i ) {
            printf( "%02X ", bytes[i] );
        }
        for( i = nbytes; i < 8; ++i ) {
            printf( "   " );
        }
    } else {
        printf( "???????? " );
    }
    printf( "\n" );
    if( dd != NULL ) {
        printf( "  ASM:   %s\n", text );
    } else {
        printf( "  ASM:   (decode failed)\n" );
    }
    DumpRegisters();
    sp = GetRegSP();
    got = MADCliReadMem( sp, 16, bytes );
    printf( "  Stack: SS:SP=%04X:%04lX", sp.mach.segment, (unsigned long)sp.mach.offset );
    for( i = 0; i + 1 < got; i += 2 ) {
        printf( "  [%04lX]=%02X%02X", (unsigned long)( sp.mach.offset + i ), bytes[i + 1], bytes[i] );
    }
    printf( "\n" );
    if( DeAliasAddrCue( NO_MOD, addr, cueh ) != SR_NONE
      && DUIGetSourceLine( cueh, sbuff, sizeof( sbuff ) ) ) {
        DIPCueFile( cueh, fbuf, sizeof( fbuf ) );
        line = DIPCueLine( cueh );
        base = BaseName( fbuf );
        if( PrevSource[0] == NULLCHAR || strcmp( PrevSource, base ) != 0 ) {
            printf( "  >>> Source: %s <<<\n", base );
            strncpy( PrevSource, base, sizeof( PrevSource ) - 1 );
            PrevSource[sizeof( PrevSource ) - 1] = NULLCHAR;
        }
        printf( "  DWARF: %s:%lu\n", fbuf, line );
        printf( "  %4lu  %s\n", line, sbuff );
        ir = strstr( sbuff, "; IR:" );
        if( ir != NULL ) {
            ir += 5;
            while( *ir == ' ' || *ir == '\t' )
                ++ir;
            printf( "  IR:  %s\n", ir );
        }
    }
}

static void AutoStepRun( void )
{
    address             addr;
    address             target;
    mad_disasm_data     *dd;
    mad_disasm_control   ctrl;
    mad_disasm_control   type;
    mod_handle          mh;
    trace_cmd_type      trace;
    char                buff[256];
    bool                decoded;
    unsigned            step;

    printf( "=== Auto-Step: %u steps (mode: %s, OS/2 API: %s%s%s) ===\n\n", AutoStepCount,
            AutoStepOver ? "step-over" : "step-into",
            AutoStepIntoAPI ? "step-into" : "step-over",
            AutoTraceCalls ? ", trace-calls" : "",
            AutoLoopDetect ? ", loop-detect" : "" );
    PrevSource[0] = NULLCHAR;
    LoopHistCount = 0;
    LoopDetected = false;
    _AllocA( dd, MADDisasmDataSize() );
    for( step = 0; step < AutoStepCount; ++step ) {
        if( _IsOff( SW_HAVE_TASK ) ) {
            printf( "  *** task terminated ***\n" );
            break;
        }
        addr = GetCodeDot();
        if( AutoLoopDetect )
            LoopRecord( addr, step );
        memset( dd, 0, MADDisasmDataSize() );
        decoded = MADDisasm( dd, &addr, 0 ) == MS_OK;
        if( decoded ) {
            MADDisasmFormat( dd, MDP_ALL, CurrRadix, buff, sizeof( buff ) );
            ctrl = MADDisasmControl( dd, &DbgRegs->mr );
        } else {
            buff[0] = NULLCHAR;
            ctrl = 0;
        }
        type = ctrl & MDC_TYPE_MASK;
        if( !AutoTraceCalls
          || type == MDC_CALL || type == MDC_RET || type == MDC_SYSRET ) {
            PrintStep( step, addr, decoded ? dd : NULL, buff );
        }
        if( step + 1 < AutoStepCount ) {
            trace = TRACE_INTO;
            if( AutoStepOver ) {
                trace = TRACE_OVER;
            } else if( !AutoStepIntoAPI && decoded ) {
                if( type == MDC_SYSCALL ) {
                    /* int XX: skip the interrupt handler */
                    trace = TRACE_OVER;
                } else if( type == MDC_CALL
                  && MADDisasmInsNext( dd, &DbgRegs->mr, &target ) == MS_OK
                  && DeAliasAddrMod( target, &mh ) == SR_NONE ) {
                    /* call into code not covered by any module (OS/2 API): skip it */
                    trace = TRACE_OVER;
                }
            }
            ExecTrace( trace, LEVEL_ASM );
            DoInput();
        }
        printf( "\n" );
    }
    printf( "=== Auto-Step complete ===\n" );
}

/*
 * RunBreakMode - run the task at native speed until PC hits the requested
 * address, then report the state (like td2ine --break-linear).
 */
static void RunBreakMode( void )
{
    brkp                *bp;
    unsigned            conditions;
    address             addr;
    address             sp;
    mad_disasm_data     *dd;
    unsigned_8          bytes[16];
    size_t              got;
    unsigned            size;
    unsigned            ins;
    char                buff[256];
    char                sbuff[256];
    char                fbuf[256];
    DIPHDL( cue, cueh );
    unsigned            i;

    bp = AddBreak( AutoBreakAddr );
    if( bp == NULL ) {
        printf( "autostep: --break-at: cannot set breakpoint at %04X:%04lX\n",
                AutoBreakAddr.mach.segment, (unsigned long)AutoBreakAddr.mach.offset );
        return;
    }
    for( ;; ) {
        conditions = Go( true );
        DoInput();
        if( ( conditions & ( COND_BREAK | COND_USER | COND_STOP
                           | COND_TERMINATE | COND_EXCEPTION ) ) != 0 )
            break;
        /* ignore library-load and other non-stop conditions */
    }
    RemoveBreak( AutoBreakAddr );
    if( _IsOff( SW_HAVE_TASK ) ) {
        printf( "=== break-at: task terminated before hitting %04X:%04lX ===\n",
                AutoBreakAddr.mach.segment, (unsigned long)AutoBreakAddr.mach.offset );
        return;
    }
    addr = GetCodeDot();
    printf( "=== break at %04X:%04lX ===\n", addr.mach.segment, (unsigned long)addr.mach.offset );
    DumpRegisters();
    _AllocA( dd, MADDisasmDataSize() );
    for( ins = 0; ins < 6; ++ins ) {
        memset( dd, 0, MADDisasmDataSize() );
        if( MADDisasm( dd, &addr, 0 ) != MS_OK )
            break;
        MADDisasmFormat( dd, MDP_ALL, CurrRadix, buff, sizeof( buff ) );
        printf( "  %04X:%04lX: %s\n", addr.mach.segment, (unsigned long)addr.mach.offset, buff );
        if( DeAliasAddrCue( NO_MOD, addr, cueh ) != SR_NONE
          && DUIGetSourceLine( cueh, sbuff, sizeof( sbuff ) ) ) {
            DIPCueFile( cueh, fbuf, sizeof( fbuf ) );
            printf( "    DWARF: %s:%lu\n", fbuf, DIPCueLine( cueh ) );
        }
        size = MADDisasmInsSize( dd );
        if( size == 0 )
            break;
        addr.mach.offset += size;
    }
    sp = GetRegSP();
    got = MADCliReadMem( sp, 16, bytes );
    printf( "  Stack: SS:SP=%04X:%04lX", sp.mach.segment, (unsigned long)sp.mach.offset );
    for( i = 0; i + 1 < got; i += 2 ) {
        printf( "  [%04lX]=%02X%02X", (unsigned long)( sp.mach.offset + i ), bytes[i + 1], bytes[i] );
    }
    printf( "\n" );
}

/*
 * RunDumps - hexdump the requested guest memory ranges (like td2ine
 * --dump-linear).
 */
static void RunDumps( void )
{
    unsigned_8          buf[16];
    address             addr;
    size_t              len;
    size_t              chunk;
    size_t              got;
    unsigned            d;
    unsigned            i;

    for( d = 0; d < AutoDumpCount; ++d ) {
        printf( "=== dump %04X:%04lX len 0x%X ===\n",
                AutoDumpAddrs[d].mach.segment,
                (unsigned long)AutoDumpAddrs[d].mach.offset,
                (unsigned)AutoDumpLens[d] );
        addr = AutoDumpAddrs[d];
        len = AutoDumpLens[d];
        while( len > 0 ) {
            chunk = ( len > 16 ) ? 16 : len;
            got = MADCliReadMem( addr, chunk, buf );
            if( got < chunk ) {
                printf( "  %04X:%04lX: <unreadable>\n", addr.mach.segment,
                        (unsigned long)addr.mach.offset );
            } else {
                printf( "  %04X:%04lX: ", addr.mach.segment, (unsigned long)addr.mach.offset );
                for( i = 0; i < chunk; ++i ) {
                    printf( "%02X ", buf[i] );
                }
                printf( "|" );
                for( i = 0; i < chunk; ++i ) {
                    printf( "%c", ( buf[i] >= 32 && buf[i] < 127 ) ? buf[i] : '.' );
                }
                printf( "|\n" );
            }
            addr.mach.offset += chunk;
            len -= chunk;
        }
    }
}

int main( int argc, char **argv )
{
    char        cmd_line[256];
    TID         tid;
    APIRET      rc;

    /* unused parameters */ (void)argc; (void)argv;

    /* unbuffered output: engine error paths exit via DosExit (KillDebugger),
     * which skips stdio flushing -- with buffering their messages are lost */
    setvbuf( stdout, NULL, _IONBF, 0 );
    setvbuf( stderr, NULL, _IONBF, 0 );

    MemInit();
    _bgetcmd( cmd_line, sizeof( cmd_line ) );
    ParseAutoStepOptions( cmd_line );
    CmdData = cmd_line;
    DebugMain();
    DoInput();
    VarInitInfo( &Locals );
    DosCreateEventSem( NULL, &Requestsem, 0, false );
    DosCreateEventSem( NULL, &Requestdonesem, 0, false );
    DosPostEventSem( Requestdonesem ); // signal req done
    rc = DosCreateThread( &tid, ControlFunc, 0, 0, 32768 );
    if( rc != 0 ) {
        printf( "Stubugger: Error creating thread!\n" );
    }
    if( AutoHaveBreak || AutoDumpCount > 0 ) {
        /* td2ine ordering: break/dump modes never fall through to autostep */
        if( _IsOn( SW_HAVE_TASK ) ) {
            if( AutoHaveBreak )
                RunBreakMode();
            RunDumps();
        } else {
            printf( "autostep: no task loaded\n" );
        }
        Done = true;
    } else if( AutoStepCount > 0 ) {
        if( _IsOn( SW_HAVE_TASK ) ) {
            AutoStepRun();
        } else {
            printf( "autostep: no task loaded\n" );
        }
        Done = true;
    }
    while( !Done ) {
        DlgCmd();
    }
    RunRequest( REQ_BYE );
    DosCloseEventSem( Requestsem );
    DosCloseEventSem( Requestdonesem );
    DebugFini();
    MemFini();
    return( 0 );
}

// Minimalist DUI callback routines

void DUIMsgBox( const char *text )
{
    printf( "MSG %s\n", text );
}

bool DUIDlgTxt( const char *text )
{
    printf( "DLG %s\n", text );
    return( true );
}

void DUIInfoBox( const char *text )
{
    printf( "INF %s\n", text );
}

void DUIErrorBox( const char *text )
{
    printf( "ERR %s\n", text );
}

void DUIStatusText( const char *text )
{
    if( text != NULL && text[0] != NULLCHAR ) {
        printf( "STA %s\n", text );
    }
}

bool DUIDlgGivenAddr( const char *title, address *value )
{
    /* unused parameters */ (void)title; (void)value;

    // needed when segment's don't map (from new/sym command)
    return( false );
}

bool DlgNewWithSym( const char *title, char *buff, size_t buff_len )
{
    /* unused parameters */ (void)title; (void)buff; (void)buff_len;

    // used by print command with no arguments
    return( true );
}

bool DlgUpTheStack( void )
{
    // used when trying to trace, but we've unwound the stack a bit
    return( false );
}

bool DlgAreYouNuts( unsigned long mult )
{
    /* unused parameters */ (void)mult;

    // used when too many break on write points are set
    return( false );
}

bool DlgBackInTime( bool warn )
{
    /* unused parameters */ (void)warn;

    // used when trying to trace, but we've backed up over a call or asynch
    return( false );
}

bool DlgIncompleteUndo( void )
{
    // used when trying to trace, but we've backed up over a call or asynch
    return( false );
}

bool DlgBreak( address addr )
{
    /* unused parameters */ (void)addr;

    // used when an error occurs in the break point expression or it is entered wrong
    return( false );
}

bool DUIInfoRelease( void )
{
    // used when we're low on memory
    return( false );
}

void DUIUpdate( update_flags flags )
{
    /* unused parameters */ (void)flags;

    // flags indicates what conditions have changed.  They should be saved
    // until an appropriate time, then windows updated accordingly
}

void DUIStop( void )
{
    // close down the UI - we're about to change modes.
}

void DUIFini( void )
{
    // finish up the UI
}

void DUIInit( void )
{
    // Init the UI
}

void DUIFreshAll( void )
{
    // refresh all screens - initialization has been done
    UpdateFlags = 0;
}

bool DUIStopRefresh( bool stop )
{
    /* unused parameters */ (void)stop;

    // temporarily turn off/on screen refreshing, cause we're going to run a
    // big command file and we don't want flashing.
    return( false );
}

void DUIShow( void )
{
    // show the main screen - the splash page has been closed
}

void DUIWndUser( void )
{
    // switch to the program screen
}

void DUIWndDebug( void )
{
    // switch to the debugger screen
}

void DUIShowLogWindow( void )
{
    // bring up the log window, cause some printout is coming
}

int DUIGetMonitorType( void )
{
    // stub for old UI
    return( 1 );
}

int DUIScreenSizeY( void )
{
    // stub for old UI
    return( 0 );
}

int DUIScreenSizeX( void )
{
    // stub for old UI
    return( 0 );
}

void DUIArrowCursor( void )
{
    // we're about to suicide, so restore the cursor to normal
}

bool DUIAskIfAsynchOk( void )
{
    // we're about to try to replay across an asynchronous event.  Ask user
    return( false );
}

void DUIFlushKeys( void )
{
    // we're about to suicide - clear the keyboard typeahead
}

void DUIPlayDead( bool dead )
{
    /* unused parameters */ (void)dead;

    // the app is about to run - make the debugger play dead
}

void DUISysEnd( bool pause )
{
    /* unused parameters */ (void)pause;

    // done calling system();
}

void DUISysStart( void )
{
    // about to call system();
}

void DUIRingBell( void )
{
    // ring ring (error)
}

bool DUIDisambiguate( const ambig_info *ambig, int num_items, int *choice )
{
    /* unused parameters */ (void)ambig; (void)num_items;

    // the expression processor detected an ambiguous symbol.  Ask user which one
    *choice = 0;
    return( true );
}

void ProcAccel( void )
{
    // stub for old UI
    FlushEOC();
}

void ProcDisplay( void )
{
    // stub for old UI
    FlushEOC();
}

void ProcFont( void )
{
    // stub for old UI
    FlushEOC();
}

void ProcHelp( void )
{
    // stub for old UI
    FlushEOC();
}

void ProcInternal( void )
{
    // stub for old UI
    FlushEOC();
}

void ProcPaint( void )
{
    // stub for old UI
    FlushEOC();
}

void ProcView( void )
{
    // stub for old UI
    FlushEOC();
}

void DUIProcWindow( void )
{
    // stub for old UI
    FlushEOC();
}

void ProcConfigFile( void )
{
    // called when main config file processed
    FlushEOC();
}

void ConfigDisp( void )
{
    // stub for old UI
}

void ConfigFont( void )
{
    // stub for old UI
}

void ConfigPaint( void )
{
    // stub for old UI
}

void DClickSet( void )
{
    // stub for old UI
    FlushEOC();
}

void DClickConf( void )
{
    // stub for old UI
}

void InputSet( void )
{
    // stub for old UI
    FlushEOC();
}

void InputConf( void )
{
    // stub for old UI
}

void MacroSet( void )
{
    // stub for old UI
    FlushEOC();
}

void MacroConf( void )
{
    // stub for old UI
}

void    FiniMacros( void )
{
    // stub for old UI
}
int TabIntervalGet( void )
{
    // stub for old UI
    return( 0 );
}
void TabIntervalSet( int new )
{
    /* unused parameters */ (void)new;

    // stub for old UI
}

void TabSet( void )
{
    // stub for old UI
    FlushEOC();
}

void TabConf( void )
{
    // stub for old UI
}

void SearchSet( void )
{
    // stub for old UI
    FlushEOC();
}

void SearchConf( void )
{
    // stub for old UI
}

void DUIFingOpen( void )
{
    // open a splash page
}

void DUIFingClose( void )
{
    // close the splash page
}

void AsmChangeOptions( void )
{
    // assembly window options changed
}

void RegChangeOptions( void )
{
    // reg window options changed
}

void VarChangeOptions( void )
{
    // var window options changed
}

void FuncChangeOptions( void )
{
    // func window options changed
}

void GlobChangeOptions( void )
{
    // glob window options changed
}

void ModChangeOptions( void )
{
    // mod window options changed
}

void WndVarInspect( const char *buff )
{
    /* unused parameters */ (void)buff;
}

//void *WndAsmInspect( address addr )
void WndAsmInspect( address addr )
{
    // used by examine/assembly command
    int         i;
    char        buff[256];
    mad_disasm_data     *dd;

    _AllocA( dd, MADDisasmDataSize() );
    // must be zeroed: on a failed decode MADDisasmFormat would run on
    // stale ins data (garbage opcode name index -> crash in MAD)
    memset( dd, 0, MADDisasmDataSize() );
    for( i = 0; i < 10; ++i ) {
        if( MADDisasm( dd, &addr, 0 ) != MS_OK ) {
            printf( "disasm failed\n" );
            break;
        }
        MADDisasmFormat( dd, MDP_ALL, CurrRadix, buff, sizeof( buff ) );
        InsMemRef( dd );
        printf( "%-40s%s\n", buff, TxtBuff );
    }
//    return( NULL );
}

//void *WndSrcInspect( address addr )
void WndSrcInspect( address addr )
{
    /* unused parameters */ (void)addr;

    // used by examine/source command
//    return( NULL );
}

void WndMemInspect( address addr, char *next, unsigned len, mad_type_handle mth )
{
    /* unused parameters */ (void)addr; (void)next; (void)len; (void)mth;

    // used by examine/byte/word/etc command
}

void WndIOInspect( address *addr, mad_type_handle mth )
{
    /* unused parameters */ (void)addr; (void)mth;

    // used by examine/iobyte/ioword/etc command
}

void WndTmpFileInspect( const char *file )
{
    /* unused parameters */ (void)file;

    // used by capture command
}

void GraphicDisplay( void )
{
    // used by print/window command
}

void VarUnMapScopes( image_entry *img )
{
    /* unused parameters */ (void)img;

    // unmap variable scopes - prog about to restart
}

void VarReMapScopes( image_entry *img )
{
    /* unused parameters */ (void)img;

    // remap variable scopes - prog about to restart
}

void VarFreeScopes( void )
{
    // free variable scope info
}

void SetLastExe( const char *name )
{
    /* unused parameters */ (void)name;

    // remember last exe debugged name
}

void DUIProcPendingPaint( void )
{
    // a paint command was issued - update the screen (stub)
}

void PopErrBox( const char *buff )
{
    printf( "%s: %s\n", buff, LIT_ENG( Debugger_Startup_Error ) );
//    MessageBox( (HWND) NULL, buff, LIT_ENG( Debugger_Startup_Error ),
//            MB_OK | MB_ICONHAND | MB_SYSTEMMODAL );
}

void DUIEnterCriticalSection( void )
{
}

void DUIExitCriticalSection( void )
{
}

bool DUIGetSourceLine( cue_handle *cueh, char *buff, size_t len )
{
    void        *viewhndl;

    viewhndl = OpenSrcFile( cueh );
    if( viewhndl == NULL )
        return( false );
    len = FReadLine( viewhndl, DIPCueLine( cueh ), 0, buff, len );
    if( len == FREADLINE_ERROR )
        len = 0;
    buff[len] = NULLCHAR;
    FDoneSource( viewhndl );
    return( true );
}

bool DUIIsDBCS( void )
{
    return( false );
}

size_t DUIEnvLkup( const char *name, char *buff, size_t buff_len )
{
    return( EnvLkup( name, buff, buff_len ) );
}

void DUIDirty( void )
{
}


void DUISrcOrAsmInspect( address addr )
{
    /* unused parameters */ (void)addr;
}

void DUIAddrInspect( address addr )
{
    /* unused parameters */ (void)addr;
}

void DUIRemoveBreak( brkp *bp )
/*****************************/
{
    RemovePoint( bp );
}

void SetMADMenuItems( void )
/**************************/
{
}

void FPUChangeOptions( void )
/***************************/
{
}

void MMXChangeOptions( void )
/***************************/
{
}

void XMMChangeOptions( void )
/***************************/
{
}

bool DUIImageLoaded( image_entry *image, bool load,
                     bool already_stopping, bool *force_stop )
/************************************************************/
{
    char buff[256];

    already_stopping=already_stopping;
    force_stop= force_stop;
    if( load ) {
        sprintf( buff, "%s '%s'", LIT_ENG( DLL_Loaded ), image->image_name );
    } else {
        sprintf( buff, "%s '%s'", LIT_ENG( DLL_UnLoaded ), image->image_name );
    }
    DUIDlgTxt( buff );
    return( false );
}

void DUICopySize( void *cookie, unsigned long size )
/**************************************************/
{
    /* unused parameters */ (void)cookie; (void)size;
}

void DUICopyCopied( void *cookie, unsigned long size )
/****************************************************/
{
    /* unused parameters */ (void)cookie; (void)size;
}

bool DUICopyCancelled( void * cookie )
/************************************/
{
    /* unused parameters */ (void)cookie;

    return( false );
}

unsigned DUIDlgAsyncRun( void )
/*****************************/
{
    return( 0 );
}

void DUISetNumLines( int num )
{
    /* unused parameters */ (void)num;
}

void DUISetNumColumns( int num )
{
    /* unused parameters */ (void)num;
}

void DUIInitRunThreadInfo( void )
{
}

void DUIScreenOptInit( void )
{
}

bool DUIScreenOption( const char *start, unsigned len, int pass )
{
    /* unused parameters */ (void)start; (void)len; (void)pass;

    return( true );
}

#if defined( GUI_IS_GUI )
unsigned OnAnotherThreadAccess( trap_elen in_num, in_mx_entry_p in_mx, trap_elen out_num, mx_entry_p out_mx )
{
    return( TrapAccess( in_num, in_mx, out_num, out_mx ) );
}

unsigned OnAnotherThreadSimpleAccess( trap_elen in_len, in_data_p in_data, trap_elen out_len, out_data_p out_data )
{
    return( TrapSimpleAccess( in_len, in_data, out_len, out_data ) );
}
#endif
