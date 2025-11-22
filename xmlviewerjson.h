/* xmlviewerjson.h - JSON handling helpers for XML Viewer
**
** Part of Plot.mcc MUI custom class package.
** (c) 2025 Michal Zukowski
*/

#ifndef XMLVIEWERJSON_H
#define XMLVIEWERJSON_H

#include <exec/types.h>
#include "xmlviewerexpat.h"

BOOL JsonToTree(struct XMLTree *tree, const char *filename, const char *json_text);

#endif // XMLVIEWERJSON_H
