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

#include "cJSON.h"

#include "xmlviewerjson.h"
#include "xmlviewerdata.h"

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
    BOOL success = FALSE;
    BOOL pushed_depth = FALSE;

    if (tree->depth + 1 >= (int)(sizeof(tree->tn) / sizeof(tree->tn[0])))
        return FALSE;

    if (cJSON_IsObject(item) || cJSON_IsArray(item))
        flags = TNF_LIST;

    if (!(node_data = AllocXmlData(XML_VALUES)))
        return FALSE;

    tree->tn[tree->depth + 1] = InsertTreeNode(tree, name, node_data, tree->tn[tree->depth], flags);
    if (!tree->tn[tree->depth + 1])
    {
        FreeXmlData(node_data);
        return FALSE;
    }

    if (cJSON_IsObject(item))
    {
        cJSON *child;
        pushed_depth = TRUE;
        tree->depth++;
        cJSON_ArrayForEach(child, item)
        {
            if (!InsertJsonNode(tree, child->string ? child->string : "<item>", child))
                goto cleanup;
        }
        tree->depth--;
        pushed_depth = FALSE;
    }
    else if (cJSON_IsArray(item))
    {
        cJSON *child;
        int idx = 0;
        char idx_name[16];

        pushed_depth = TRUE;
        tree->depth++;
        cJSON_ArrayForEach(child, item)
        {
            snprintf(idx_name, sizeof(idx_name), "[%d]", idx++);
            if (!InsertJsonNode(tree, idx_name, child))
                goto cleanup;
        }
        tree->depth--;
        pushed_depth = FALSE;
    }
    else
        AttachPrimitiveValue(node_data, item);

    success = TRUE;

cleanup:
    if (!success && pushed_depth && tree->depth > 0)
        tree->depth--;

    return success;
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
    tree->tn[0] = InsertTreeNode(tree, filename, root_data, MUIV_Listtree_Insert_ListNode_Root, TNF_LIST);

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
