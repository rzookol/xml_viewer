/* xmlviewerformat.h - file format detection helpers for XML Viewer
**
** Part of Plot.mcc MUI custom class package.
** (c) 2025 Michal Zukowski
*/

#ifndef XMLVIEWERFORMAT_H
#define XMLVIEWERFORMAT_H

#include <exec/types.h>
#include <proto/dos.h>

enum FileFormat
{
    FILE_FORMAT_UNKNOWN = 0,
    FILE_FORMAT_XML,
    FILE_FORMAT_JSON,
    FILE_FORMAT_YAML,
    FILE_FORMAT_IFF,
};

struct FileFormatInfo
{
    enum FileFormat format;
    BOOL has_utf8_bom;
    BOOL guessed_from_suffix;
};

BOOL DetectFileFormat(const char *filename, BPTR file, struct FileFormatInfo *info);

#endif // XMLVIEWERFORMAT_H
