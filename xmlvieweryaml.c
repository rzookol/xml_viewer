/* xmlvieweryaml.c - YAML handling helpers for XML Viewer
**
** Part of Plot.mcc MUI custom class package.
** (c) 2025 Michal Zukowski
*/

#include <string.h>
#include <stdio.h>

#include <proto/exec.h>
#include <proto/muimaster.h>
#include <mui/Listtree_mcc.h>
#include <yaml.h>

#include "xmlvieweryaml.h"
#include "xmlviewerdata.h"

/// helper to create a readable scalar string from a yaml scalar node
static const char *ScalarValue(const yaml_node_t *node)
{
    if (!node || node->type != YAML_SCALAR_NODE)
        return "";

    return (const char *)node->data.scalar.value;
}

/// recursive builder for YAML nodes
static BOOL InsertYamlNode(struct XMLTree *tree, yaml_document_t *doc, int node_index, const char *name)
{
    yaml_node_t *node;
    struct xml_data *node_data;
    LONG flags = 0;
    BOOL success = FALSE;
    BOOL pushed_depth = FALSE;

    if (!doc || !name || !tree)
        return FALSE;

    node = yaml_document_get_node(doc, node_index);
    if (!node)
        return FALSE;

    if (tree->depth + 1 >= (int)(sizeof(tree->tn) / sizeof(tree->tn[0])))
        return FALSE;

    if (node->type == YAML_MAPPING_NODE || node->type == YAML_SEQUENCE_NODE)
        flags = TNF_LIST;

    if (!(node_data = AllocXmlData(XML_VALUES)))
        return FALSE;

    tree->tn[tree->depth + 1] = InsertTreeNode(tree, name, node_data, tree->tn[tree->depth], flags);
    if (!tree->tn[tree->depth + 1])
    {
        FreeXmlData(node_data);
        return FALSE;
    }

    if (node->type == YAML_MAPPING_NODE)
    {
        yaml_node_pair_t *pair;

        pushed_depth = TRUE;
        tree->depth++;
        for (pair = node->data.mapping.pairs.start; pair < node->data.mapping.pairs.top; pair++)
        {
            yaml_node_t *key_node = yaml_document_get_node(doc, pair->key);
            const char *key_name = ScalarValue(key_node);
            if (!key_name || !key_name[0])
                key_name = "<key>";

            if (!InsertYamlNode(tree, doc, pair->value, key_name))
                goto cleanup;
        }
        tree->depth--;
        pushed_depth = FALSE;
    }
    else if (node->type == YAML_SEQUENCE_NODE)
    {
        yaml_node_item_t *item;
        int idx = 0;
        char idx_name[16];

        pushed_depth = TRUE;
        tree->depth++;
        for (item = node->data.sequence.items.start; item < node->data.sequence.items.top; item++)
        {
            snprintf(idx_name, sizeof(idx_name), "[%d]", idx++);
            if (!InsertYamlNode(tree, doc, *item, idx_name))
                goto cleanup;
        }
        tree->depth--;
        pushed_depth = FALSE;
    }
    else if (node->type == YAML_SCALAR_NODE)
    {
        AddAttribute(node_data, "value", ScalarValue(node));
    }

    success = TRUE;

cleanup:
    if (!success && pushed_depth && tree->depth > 0)
        tree->depth--;

    return success;
}

/// YamlToTree()
BOOL YamlToTree(struct XMLTree *tree, const char *filename, const char *yaml_text)
{
    yaml_parser_t parser;
    yaml_document_t document;
    struct xml_data *root_data;
    BOOL rc = FALSE;

    if (!tree || !filename || !yaml_text)
        return FALSE;

    if (!yaml_parser_initialize(&parser))
        return FALSE;

    yaml_parser_set_input_string(&parser, (const unsigned char *)yaml_text, strlen(yaml_text));

    if (!yaml_parser_load(&parser, &document))
    {
        yaml_parser_delete(&parser);
        return FALSE;
    }

    if (!(root_data = AllocXmlData(XML_VALUES)))
    {
        yaml_document_delete(&document);
        yaml_parser_delete(&parser);
        return FALSE;
    }

    tree->depth = 0;
    tree->tn[0] = InsertTreeNode(tree, filename, root_data, MUIV_Listtree_Insert_ListNode_Root, TNF_LIST);

    if (!tree->tn[0])
    {
        FreeXmlData(root_data);
        yaml_document_delete(&document);
        yaml_parser_delete(&parser);
        return FALSE;
    }

    {
        yaml_node_t *root_node = yaml_document_get_root_node(&document);
        if (root_node)
        {
            if (root_node->type == YAML_MAPPING_NODE)
            {
                yaml_node_pair_t *pair;
                for (pair = root_node->data.mapping.pairs.start; pair < root_node->data.mapping.pairs.top; pair++)
                {
                    const char *key_name = ScalarValue(yaml_document_get_node(&document, pair->key));
                    if (!key_name || !key_name[0])
                        key_name = "<key>";
                    if (!InsertYamlNode(tree, &document, pair->value, key_name))
                        goto cleanup;
                }
            }
            else if (root_node->type == YAML_SEQUENCE_NODE)
            {
                yaml_node_item_t *item;
                int idx = 0;
                char idx_name[16];

                for (item = root_node->data.sequence.items.start; item < root_node->data.sequence.items.top; item++)
                {
                    snprintf(idx_name, sizeof(idx_name), "[%d]", idx++);
                    if (!InsertYamlNode(tree, &document, *item, idx_name))
                        goto cleanup;
                }
            }
            else if (root_node->type == YAML_SCALAR_NODE)
            {
                AddAttribute(root_data, "value", ScalarValue(root_node));
            }
        }
    }

    rc = TRUE;

cleanup:
    yaml_document_delete(&document);
    yaml_parser_delete(&parser);
    return rc;
}

///
