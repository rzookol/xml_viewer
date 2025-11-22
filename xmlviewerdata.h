/* xmlviewerdata.h - shared listtree helpers for XML Viewer
**
** Part of Plot.mcc MUI custom class package.
** (c) 2025 Michal Zukowski
*/

#ifndef XMLVIEWERDATA_H
#define XMLVIEWERDATA_H

#include <exec/types.h>
#include "xmlviewerexpat.h"

struct xml_data *AllocXmlData(long type);
void FreeXmlData(struct xml_data *data);
BOOL AddAttribute(struct xml_data *data, const char *attr, const char *value);
struct MUIS_Listtree_TreeNode *InsertTreeNode(struct XMLTree *tree, const char *name, struct xml_data *node_data, APTR parent, LONG flags);

#endif // XMLVIEWERDATA_H
