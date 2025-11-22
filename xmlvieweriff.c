/* xmlvieweriff.c - IFF handling helpers for XML Viewer
**
** Part of Plot.mcc MUI custom class package.
** (c) 2025 Michal Zukowski
*/

#include <string.h>
#include <stdio.h>

#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/iffparse.h>
#include <mui/Listtree_mcc.h>

#include "xmlvieweriff.h"
#include "xmlviewerdata.h"

static void IdToString(ULONG id, char *buffer, size_t buffer_len)
{
    buffer[0] = (char)((id >> 24) & 0xFF);
    buffer[1] = (char)((id >> 16) & 0xFF);
    buffer[2] = (char)((id >> 8) & 0xFF);
    buffer[3] = (char)(id & 0xFF);
    buffer[4] = '\0';

    for (size_t i = 0; i < 4; ++i)
    {
        if (buffer[i] < 0x20 || buffer[i] > 0x7E)
            buffer[i] = '.';
    }
}

/// IffToTree()
BOOL IffToTree(struct XMLTree *tree, const char *filename, BPTR file)
{
    struct Library *IFFParseBase = NULL;
    struct IFFHandle *iff = NULL;
    struct xml_data *root_data;
    BOOL success = FALSE;

    if (!tree || !file || !filename)
        return FALSE;

    if (!(IFFParseBase = OpenLibrary("iffparse.library", 0)))
        return FALSE;

    if (!(iff = AllocIFF()))
    {
        CloseLibrary(IFFParseBase);
        return FALSE;
    }

    iff->iff_Stream = file;
    InitIFFasDOS(iff);

    if (OpenIFF(iff, IFFF_READ) != 0)
        goto cleanup;

    if (!(root_data = AllocXmlData(XML_VALUES)))
        goto cleanup;

    tree->depth = 0;
    tree->tn[0] = InsertTreeNode(tree, filename, root_data, MUIV_Listtree_Insert_ListNode_Root, TNF_LIST);
    if (!tree->tn[0])
    {
        FreeXmlData(root_data);
        goto cleanup;
    }

    while (1)
    {
        LONG err = ParseIFF(iff, IFFPARSE_RAWSTEP);
        if (err == IFFERR_EOF)
        {
            success = TRUE;
            break;
        }
        else if (err == IFFERR_EOC)
        {
            if (tree->depth > 0)
                tree->depth--;
            continue;
        }
        else if (err != 0)
        {
            break;
        }

        {
            struct ContextNode *cn = CurrentChunk(iff);
            struct xml_data *node_data;
            LONG flags = 0;
            char node_name[32];
            char type_name[8];
            BOOL is_container = FALSE;

            if (!cn)
                break;

            IdToString(cn->cn_Type, type_name, sizeof(type_name));
            IdToString(cn->cn_ID, node_name, sizeof(node_name));

            if ((cn->cn_Type == ID_FORM) || (cn->cn_Type == ID_LIST) || (cn->cn_Type == ID_CAT ))
            {
                char full_name[32];
                snprintf(full_name, sizeof(full_name), "%s %s", type_name, node_name);
                strcpy(node_name, full_name);
                flags = TNF_LIST;
                is_container = TRUE;
            }

            if (tree->depth + 1 >= (int)(sizeof(tree->tn) / sizeof(tree->tn[0])))
                break;

            if (!(node_data = AllocXmlData(XML_VALUES)))
                break;

            tree->tn[tree->depth + 1] = InsertTreeNode(tree, node_name, node_data, tree->tn[tree->depth], flags);
            if (!tree->tn[tree->depth + 1])
            {
                FreeXmlData(node_data);
                break;
            }

            if (is_container)
                tree->depth++;
        }
    }

cleanup:
    CloseIFF(iff);
    FreeIFF(iff);
    CloseLibrary(IFFParseBase);
    return success;
}

///
