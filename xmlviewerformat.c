/* xmlviewerformat.c - file format detection helpers for XML Viewer
**
** Part of Plot.mcc MUI custom class package.
** (c) 2025 Michal Zukowski
*/

#include <string.h>
#include <ctype.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include "xmlviewerformat.h"

#define BOM0 0xEF
#define BOM1 0xBB
#define BOM2 0xBF

static enum FileFormat GuessFromExtension(const char *filename)
{
    const char *ext;

    if (!filename)
        return FILE_FORMAT_UNKNOWN;

    if ((ext = strrchr(filename, '.')))
    {
        if (Stricmp(ext, ".xml") == 0)
            return FILE_FORMAT_XML;
        if (Stricmp(ext, ".json") == 0)
            return FILE_FORMAT_JSON;
        if ((Stricmp(ext, ".yaml") == 0) || (Stricmp(ext, ".yml") == 0))
            return FILE_FORMAT_YAML;
        if (Stricmp(ext, ".iff") == 0)
            return FILE_FORMAT_IFF;
    }

    return FILE_FORMAT_UNKNOWN;
}

static const char *SkipWhitespace(const char *ptr, size_t len)
{
    while (len > 0 && *ptr && isspace((int)(unsigned char)*ptr))
    {
        ptr++;
        len--;
    }

    return ptr;
}

static BOOL MatchesFormat(enum FileFormat format, const char *header, size_t len)
{
    const char *ptr = header;

    if (!header || len == 0)
        return FALSE;

    ptr = SkipWhitespace(ptr, len);

    switch (format)
    {
        case FILE_FORMAT_XML:
            return (len >= 4 && Strnicmp(ptr, "<xml", 4) == 0);
        case FILE_FORMAT_JSON:
            return (*ptr == '{' || *ptr == '[');
        case FILE_FORMAT_YAML:
            return (len >= 3 && strncmp(ptr, "---", 3) == 0);
        case FILE_FORMAT_IFF:
            return (len >= 4 && ((Strnicmp(ptr, "FORM", 4) == 0) || (Strnicmp(ptr, "CAT ", 4) == 0)));
        default:
            return FALSE;
    }
}

BOOL DetectFileFormat(const char *filename, BPTR file, struct FileFormatInfo *info)
{
    enum FileFormat guessed;
    enum FileFormat try_order[4];
    int try_count = 0, i;
    ULONG pos;
    char header[16];
    LONG read_len = 0;

    if (!file || !info)
        return FALSE;

    memset(info, 0, sizeof(struct FileFormatInfo));

    if ((pos = Seek(file, 0, OFFSET_CURRENT)) == (ULONG)-1)
        return FALSE;

    Seek(file, 0, OFFSET_BEGINNING);
    read_len = Read(file, header, sizeof(header) - 1);
    if (read_len < 0)
        goto restore;

    header[read_len] = '\0';

    if ((read_len >= 3) && ((unsigned char)header[0] == BOM0) && ((unsigned char)header[1] == BOM1) && ((unsigned char)header[2] == BOM2))
    {
        info->has_utf8_bom = TRUE;
        memmove(header, header + 3, read_len - 3);
        read_len -= 3;
        header[read_len] = '\0';
    }

    guessed = GuessFromExtension(filename);
    if (guessed != FILE_FORMAT_UNKNOWN)
        try_order[try_count++] = guessed;

    if (guessed != FILE_FORMAT_XML)
        try_order[try_count++] = FILE_FORMAT_XML;
    if (guessed != FILE_FORMAT_JSON)
        try_order[try_count++] = FILE_FORMAT_JSON;
    if (guessed != FILE_FORMAT_YAML)
        try_order[try_count++] = FILE_FORMAT_YAML;
    if (guessed != FILE_FORMAT_IFF)
        try_order[try_count++] = FILE_FORMAT_IFF;

    for (i = 0; i < try_count; ++i)
    {
        if (MatchesFormat(try_order[i], header, (size_t)read_len))
        {
            info->format = try_order[i];
            info->guessed_from_suffix = (info->format == guessed);
            goto restore;
        }
    }

    if (guessed != FILE_FORMAT_UNKNOWN)
    {
        info->format = guessed;
        info->guessed_from_suffix = TRUE;
    }

restore:
    Seek(file, pos, OFFSET_BEGINNING);
    return info->format != FILE_FORMAT_UNKNOWN;
}

///
