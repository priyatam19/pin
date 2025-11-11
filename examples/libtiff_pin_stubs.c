#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "tiffiop.h"

/*
 * Minimal stub implementations so that single-object PIN harnesses can link
 * libtiff helper routines without linking the full library.  None of these
 * functions are exercised in the targeted scalar/utility APIs; they merely
 * satisfy linker requirements.
 */

int _TIFFMergeFields(TIFF *tif, const TIFFField info[], uint32 n)
{
    (void)tif;
    (void)info;
    (void)n;
    return 1;
}

void* _TIFFmalloc(tmsize_t size)
{
    return malloc((size_t)size);
}

void* _TIFFrealloc(void* ptr, tmsize_t size)
{
    return realloc(ptr, (size_t)size);
}

void _TIFFfree(void* ptr)
{
    free(ptr);
}

void _TIFFmemset(void* dest, int value, tmsize_t len)
{
    memset(dest, value, (size_t)len);
}

void _TIFFmemcpy(void* dest, const void* src, tmsize_t len)
{
    memcpy(dest, src, (size_t)len);
}

void _TIFFSetDefaultCompressionState(TIFF* tif)
{
    (void)tif;
}

void _TIFFNoPostDecode(TIFF* tif, uint8* buf, tmsize_t cc)
{
    (void)tif;
    (void)buf;
    (void)cc;
}

int TIFFSetField(TIFF* tif, uint32 tag, ...)
{
    (void)tif;
    (void)tag;
    return 1;
}

int TIFFFlushData1(TIFF* tif)
{
    (void)tif;
    return 1;
}

uint64 TIFFScanlineSize64(TIFF* tif)
{
    (void)tif;
    return 0;
}

tmsize_t TIFFScanlineSize(TIFF* tif)
{
    (void)tif;
    return 0;
}

uint64 TIFFTileRowSize64(TIFF* tif)
{
    (void)tif;
    return 0;
}

tmsize_t TIFFTileRowSize(TIFF* tif)
{
    (void)tif;
    return 0;
}

uint64 TIFFTileSize64(TIFF* tif)
{
    (void)tif;
    return 0;
}

tmsize_t TIFFTileSize(TIFF* tif)
{
    (void)tif;
    return 0;
}

void TIFFErrorExt(thandle_t client, const char* module, const char* fmt, ...)
{
    (void)client;
    (void)module;
    (void)fmt;
}
