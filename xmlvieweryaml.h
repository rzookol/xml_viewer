/* xmlvieweryaml.h - YAML handling helpers for XML Viewer
**
** Part of Plot.mcc MUI custom class package.
** (c) 2025 Michal Zukowski
*/

#ifndef XMLVIEWERYAML_H
#define XMLVIEWERYAML_H

#include <exec/types.h>
#include "xmlviewerexpat.h"

BOOL YamlToTree(struct XMLTree *tree, const char *filename, const char *yaml_text);

#endif // XMLVIEWERYAML_H
