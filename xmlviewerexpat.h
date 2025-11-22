/* xmlviewerexpat.h - XML metafomat viewer
** expat.library interface header
**
** Part of Plot.mcc MUI custom class package.
** (c) 2008 Michal Zukowski
*/

#ifndef XMLVIEWEREXPAT_H
#define XMLVIEWEREXPAT_H


#include <proto/expat.h>
#include "api_MUI.h"
#include "resources.h"

#define XML_VALUES   1
#define XML_TEXT     2
#define XML_COMMENTS 4
#define XML_HEADER   8

struct XMLTree
{
    Object *tree;                               // tree list object
    char filename[102];                         // 102 DOS limitation
    struct MUIS_Listtree_TreeNode *tn[512];     // consider increasing if more objects appear
    int depth;
    BOOL has_utf8_bom;
};


struct DataNode
{
    struct MinNode Node;
    char *values;
    char *atributes;
};

struct xml_data
{
    struct MinList *attr_list;
    long type;
};

void SAVEDS default_hndl(void *, const XML_Char *, int);
void SAVEDS startElement(void *, const char *, const char **);
void SAVEDS endElement(void *, const char *);
void SAVEDS decl_hndl(void *, const XML_Char *, const XML_Char *, int);
void SAVEDS comment_hndl(void *, const XML_Char *);

#endif
