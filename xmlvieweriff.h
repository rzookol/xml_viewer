/* xmlvieweriff.h - IFF handling helpers for XML Viewer
**
** Part of Plot.mcc MUI custom class package.
** (c) 2025 Michal Zukowski
*/

#ifndef XMLVIEWERIFF_H
#define XMLVIEWERIFF_H

#include <exec/types.h>
#include "xmlviewerexpat.h"

BOOL IffToTree(struct XMLTree *tree, const char *filename, BPTR file);

#endif // XMLVIEWERIFF_H
