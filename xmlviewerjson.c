/* xmlviewerjson.c - JSON handling helpers for XML Viewer
**
** Part of Plot.mcc MUI custom class package.
** (c) 2025 Michal Zukowski
*/

#include <string.h>
#include <stdio.h>

#include <proto/exec.h>
#include <proto/muimaster.h>
#include <mui/Listtree_mcc.h>

#include <cjson/cJSON.h>

#include "xmlviewerjson.h"

/// helper to allocate xml_data nodes with initialized lists
static struct xml_data *AllocXmlData(long type)
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
static void FreeXmlData(struct xml_data *data)
{
    if (!data)
        return;

    if (data->attr_list)
        FreeVec(data->attr_list);

    FreeVec(data);
}

/// append attribute/value to xml_data list
static BOOL AddAttribute(struct xml_data *data, const char *attr, const char *value)
{
    struct DataNode *dn;

    if (!data || !data->attr_list || !attr || !value)
        return FALSE;

    if ((dn = (struct DataNode *)AllocVec(sizeof(struct DataNode), MEMF_ANY)))
    {
        if ((dn->atributes = (char *)AllocVec(strlen(attr) + 1, MEMF_ANY)))
            strcpy(dn->atributes, attr);

        if ((dn->values = (char *)AllocVec(strlen(value) + 1, MEMF_ANY)))
            strcpy(dn->values, value);

        AddHead((struct List *)data->attr_list, (struct Node *)dn);
        return TRUE;
    }

    return FALSE;
}

/// convert primitive JSON values to attributes on the node
static void AttachPrimitiveValue(struct xml_data *data, cJSON *item)
{
    char buffer[64];
    const char *value = NULL;
    char *printed = NULL;

    if (cJSON_IsString(item) && item->valuestring)
        value = item->valuestring;
    else if (cJSON_IsNumber(item))
    {
        snprintf(buffer, sizeof(buffer), "%g", item->valuedouble);
        value = buffer;
    }
    else if (cJSON_IsBool(item))
        value = cJSON_IsTrue(item) ? "true" : "false";
    else if (cJSON_IsNull(item))
        value = "null";
    else
    {
        printed = cJSON_PrintUnformatted(item);
        value = printed;
    }

    if (value)
        AddAttribute(data, "value", value);

    if (printed)
        cJSON_free(printed);
}

/// recursive builder
static BOOL InsertJsonNode(struct XMLTree *tree, const char *name, cJSON *item)
{
    struct xml_data *node_data;
    LONG flags = 0;

    if (cJSON_IsObject(item) || cJSON_IsArray(item))
        flags = TNF_LIST;

    if (!(node_data = AllocXmlData(XML_VALUES)))
        return FALSE;

    tree->tn[tree->depth + 1] = (struct MUIS_Listtree_TreeNode *)DoMethod(tree->tree, MUIM_Listtree_Insert, name,
                                                                           (APTR)node_data, tree->tn[tree->depth],
                                                                           MUIV_Listtree_Insert_PrevNode_Tail, flags);
    if (!tree->tn[tree->depth + 1])
    {
        FreeXmlData(node_data);
        return FALSE;
    }

    if (cJSON_IsObject(item))
    {
        cJSON *child;
        tree->depth++;
        cJSON_ArrayForEach(child, item)
        {
            if (!InsertJsonNode(tree, child->string ? child->string : "<item>", child))
                return FALSE;
        }
        tree->depth--;
    }
    else if (cJSON_IsArray(item))
    {
        cJSON *child;
        int idx = 0;
        char idx_name[16];

        tree->depth++;
        cJSON_ArrayForEach(child, item)
        {
            snprintf(idx_name, sizeof(idx_name), "[%d]", idx++);
            if (!InsertJsonNode(tree, idx_name, child))
                return FALSE;
        }
        tree->depth--;
    }
    else
        AttachPrimitiveValue(node_data, item);

    return TRUE;
}

/// JsonToTree()
BOOL JsonToTree(struct XMLTree *tree, const char *filename, const char *json_text)
{
    cJSON *root;
    struct xml_data *root_data;
    BOOL rc = FALSE;

    if (!tree || !filename || !json_text)
        return FALSE;

    if (!(root = cJSON_Parse(json_text)))
        return FALSE;

    if (!(root_data = AllocXmlData(XML_VALUES)))
    {
        cJSON_Delete(root);
        return FALSE;
    }

    tree->depth = 0;
    tree->tn[0] = (struct MUIS_Listtree_TreeNode *)DoMethod(tree->tree, MUIM_Listtree_Insert, filename,
                                                            (APTR)root_data, MUIV_Listtree_Insert_ListNode_Root,
                                                            MUIV_Listtree_Insert_PrevNode_Tail, TNF_LIST);

    if (!tree->tn[0])
    {
        FreeXmlData(root_data);
        cJSON_Delete(root);
        return FALSE;
    }

    if (cJSON_IsObject(root))
    {
        cJSON *child;
        cJSON_ArrayForEach(child, root)
        {
            if (!InsertJsonNode(tree, child->string ? child->string : "<item>", child))
                goto cleanup;
        }
    }
    else if (cJSON_IsArray(root))
    {
        cJSON *child;
        int idx = 0;
        char idx_name[16];

        cJSON_ArrayForEach(child, root)
        {
            snprintf(idx_name, sizeof(idx_name), "[%d]", idx++);
            if (!InsertJsonNode(tree, idx_name, child))
                goto cleanup;
        }
    }
    else
        AttachPrimitiveValue(root_data, root);

    rc = TRUE;

cleanup:
    cJSON_Delete(root);
    return rc;
}

///
