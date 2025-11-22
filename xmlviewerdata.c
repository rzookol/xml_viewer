/* xmlviewerdata.c - shared listtree helpers for XML Viewer
**
** Part of Plot.mcc MUI custom class package.
** (c) 2025 Michal Zukowski
*/

#include <string.h>
#include <proto/exec.h>
#include <proto/muimaster.h>
#include <mui/Listtree_mcc.h>

#include "xmlviewerdata.h"

/// helper to allocate xml_data nodes with initialized lists
struct xml_data *AllocXmlData(long type)
{
    struct xml_data *tmp;

    if ((tmp = (struct xml_data *)AllocVec(sizeof(struct xml_data), MEMF_ANY)))
    {
        if ((tmp->attr_list = (struct MinList *)AllocVec(sizeof(struct MinList), MEMF_ANY)))
        {
#ifdef __MORPHOS__
            NewMinList(tmp->attr_list);
#else
            NewList(tmp->attr_list);
#endif
            tmp->type = type;
            return tmp;
        }
        FreeVec(tmp);
    }

    return NULL;
}

/// helper to free xml_data nodes when insertion fails
void FreeXmlData(struct xml_data *data)
{
    if (!data)
        return;

    if (data->attr_list)
        FreeVec(data->attr_list);

    FreeVec(data);
}

/// append attribute/value to xml_data list
BOOL AddAttribute(struct xml_data *data, const char *attr, const char *value)
{
    struct DataNode *dn;

    if (!data || !data->attr_list || !attr || !value)
        return FALSE;

    if ((dn = (struct DataNode *)AllocVec(sizeof(struct DataNode), MEMF_ANY)))
    {
        if ((dn->atributes = (char *)AllocVec(strlen(attr) + 1, MEMF_ANY)))
            strcpy(dn->atributes, attr);
        else
        {
            FreeVec(dn);
            return FALSE;
        }

        if ((dn->values = (char *)AllocVec(strlen(value) + 1, MEMF_ANY)))
        {
            strcpy(dn->values, value);
            AddHead((struct List *)data->attr_list, (struct Node *)dn);
            return TRUE;
        }

        FreeVec(dn->atributes);
        FreeVec(dn);
    }

    return FALSE;
}

/// helper to insert a node into the listtree using a duplicated name
struct MUIS_Listtree_TreeNode *InsertTreeNode(struct XMLTree *tree, const char *name, struct xml_data *node_data, APTR parent, LONG flags)
{
    struct MUIS_Listtree_TreeNode *result = NULL;
    char *node_name;

    if (!tree || !tree->tree || !name || !node_data)
        return NULL;

    if (!(node_name = (char *)AllocVec(strlen(name) + 1, MEMF_ANY)))
        return NULL;

    strcpy(node_name, name);
    result = (struct MUIS_Listtree_TreeNode *)DoMethod(tree->tree, MUIM_Listtree_Insert, node_name, (APTR)node_data, parent,
                                                      MUIV_Listtree_Insert_PrevNode_Tail, flags);

    /*
     * Keep the duplicated name alive for the listtree node lifetime. Unlike
     * the XML codepath where the parser supplies stable strings, the JSON/YAML/IFF
     * names come from transient buffers (e.g. array indices). Freeing them
     * immediately resulted in empty/garbled labels and invisible nodes.
     */
    if (!result)
        FreeVec(node_name);

    return result;
}

///
