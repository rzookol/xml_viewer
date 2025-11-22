/* xmlviewer.c - XML metafomat viewer
**
** Part of Plot.mcc MUI custom class package.
** (c) 2008-2025 Michal Zukowski
*/

#define VERSION_TAG "$VER: XML Viewer 0.17 (21.11.2025)"

///
/// INCLUDES

//#define USE_INLINE_STDARG

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/expat.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/asl.h>
#include <proto/locale.h>

#include <mui/Listtree_mcc.h>
#include <clib/alib_protos.h>
#include <exec/lists.h>

#include "xmlviewerlist.h"
#include "xmlviewerexpat.h"
#include "xmlviewerjson.h"
#include "xmlvieweryaml.h"
#include "xmlvieweriff.h"
#include "xmlviewerformat.h"
#include "xmlviewerdata.h"
#include "xmlviewertree.h"
#include "xmlviewerabout.h"
#include "xmlviewerlocale.h"
#include "api_MUI.h"

#ifdef MEMTRACK
#include "resources.h"
#endif


///
/// DEFINES

#ifndef MAKE_ID
#define MAKE_ID(a,b,c,d) ((ULONG) (a)<<24 | (ULONG) (b)<<16 | (ULONG) (c)<<8 | (ULONG) (d))
#endif

#define MUIM_App_Prefs	    OM_Dummy+1000
#define MUIM_App_Load	    OM_Dummy+1001
#define MUIM_App_AslReq	    OM_Dummy+1002
#define MUIM_App_Save	    OM_Dummy+1003
#define MUIM_App_CloseAll   OM_Dummy+1004
#define MUIM_App_Search     OM_Dummy+1005

#ifndef __MORPHOS__
#define REG(a) __asm(#a)
#define reg(a) __asm(#a)
#define OLDCALL __saveds  __stdargs

#else
#define OLDCALL
#endif
#define DBG(x)  (x)

///
/// MyApp structures

struct MUI_CustomClass *MyAppClass = NULL;
struct MUI_CustomClass *xmlviewertreeClass = NULL;

DISPATCHER_DEF( MyApp_Class_ )

struct MyApp_Data
{
    ULONG data;
    char last_dir[102];
    char last_file[102];
    int last_search;

};

///

#ifndef __MORPHOS__

struct Library *MUIMasterBase;

OLDCALL APTR  DoSuperNew( struct IClass *cl, APTR obj, ULONG tag1, ... )
{
    return ( ( APTR )DoSuperMethod( cl, obj, OM_NEW, &tag1, NULL ) );
}

#endif

struct MUIP_App_Filename
{
    ULONG methodid;
    ULONG filename;
};
struct MUIP_App_FileReq
{
    ULONG methodid;
    ULONG mode;
};

#define MODE_SAVE 1
#define MODE_LOAD 2
#define MODE_SAVEFORMATTED 3

#define TEMPLATE "FILE/A"
BOOL load_xml_tree( Object *obj, BPTR fileXML, struct XMLTree *tree, XML_Parser parser, char *error_buf, size_t error_buf_len );
BOOL load_json_tree( Object *obj, BPTR fileXML, struct XMLTree *tree, char *error_buf, size_t error_buf_len );
BOOL load_yaml_tree( Object *obj, BPTR fileXML, struct XMLTree *tree, char *error_buf, size_t error_buf_len );
BOOL load_iff_tree( Object *obj, BPTR fileXML, struct XMLTree *tree, char *error_buf, size_t error_buf_len );

void handle_arguments( int argc, char *argv[] )
{
    struct RDArgs *rdargs = NULL;
    long opts[2];

    if( ( rdargs = ReadArgs( TEMPLATE, opts, NULL ) ) )
    {
        printf( "file %s\n", ( char* )opts[0] );

        FreeArgs( ( struct RDArgs * )rdargs );
    }
    else
    {
        printf( "Syntax: %s %s\n", argv[0], TEMPLATE );
    }
}

/// CreateAppClass()

struct MUI_CustomClass *CreateAppClass( void )
{
    struct MUI_CustomClass *cl;
    cl = MUI_CreateCustomClass( NULL, MUIC_Application, NULL, sizeof( struct MyApp_Data ), ( APTR )DISPATCHER_REF( MyApp_Class_ ) );
    return cl;
}

///
/// checkmark enums
enum
{
    CHM_NAMES = 0,
    CHM_VALUES,
    CHM_ATTRIBUTES,
    CHM_OBJECTCOUNT
};
///
/// GLOBAL VARS

#ifdef __MORPHOS__

static STRPTR UsedClasses[]  = {MUIC_Urltext, MUIC_Listtree, MUIC_Aboutbox, 0};

#endif

struct Library *ExpatBase;
struct Catalog *Cat;
Object  *app,  *abt, *window;
static Object  *bt_open_nodes, *bt_close_nodes, *lt_nodes, *search_prev,  *search_next, *search_string;
Object checkmarks[CHM_OBJECTCOUNT];
Object *list;
char *filename = NULL;
XML_Parser parser;
struct XMLTree m_tree;

///
/// CreateMenuitems()

Object *CreateMenuitemAbout()
{
    Object *obj;

    obj = MUI_NewObject( MUIC_Menuitem,
                         MUIA_Menuitem_Title, GetCatalogStr( Cat, MSG_MENU_ABOUT, "About..." ),
                         MUIA_Menuitem_Shortcut, ( ULONG )"?",
                         TAG_END );

    return obj;
}

Object *CreateMenuitemLoad()
{
    Object *obj;

    obj = MUI_NewObject( MUIC_Menuitem,
                         MUIA_Menuitem_Title, GetCatalogStr( Cat, MSG_MENU_OPEN, "Open..." ),
                         MUIA_Menuitem_Shortcut, ( ULONG )"O",
                         TAG_END );

    return obj;
}

Object *CreateMenuitemQuit()
{
    Object *obj;

    obj = MUI_NewObject( MUIC_Menuitem,
                         MUIA_Menuitem_Title, GetCatalogStr( Cat, MSG_MENU_QUIT, "Quit" ),
                         MUIA_Menuitem_Shortcut, ( ULONG )"Q",
                         TAG_END );

    return obj;
}

Object *CreateMenuitemSave()
{
    Object *obj;

    obj = MUI_NewObject( MUIC_Menuitem,
                         MUIA_Menuitem_Title, GetCatalogStr( Cat, MSG_MENU_SAVE, "Save..." ),
                         MUIA_Menuitem_Shortcut, ( ULONG )"S",
                         TAG_END );

    return obj;
}

Object *CreateMenuitemSaveFormatted()
{
    Object *obj;

    obj = MUI_NewObject( MUIC_Menuitem,
                         MUIA_Menuitem_Title,  GetCatalogStr( Cat, MSG_MENU_SAVEFORM, "Save formatted..." ),
                         MUIA_Menuitem_Enabled, FALSE,
                         TAG_END );

    return obj;
}

///
/// menustrip enums
enum
{
    MS_MENU = 0,
    MS_OPEN,
    MS_SAVE,
    MS_SAVEFORMATTED,
    MS_ABOUT,
    MS_QUIT,
    MS_OBJECTCOUNT
};
///
/// MyApp_New()

ULONG MyApp_New( struct IClass *cl, Object *obj, struct opSet *msg )
{

    Object *menustrip_w[MS_OBJECTCOUNT];

    obj = DoSuperNew( cl, obj,
                      MUIA_Application_Title, 	"XML Viewer",
                      MUIA_Application_Copyright, 	"(c) 2008-2025 Michal 'rzookol' Zukowski",
                      MUIA_Application_Author, 	"Michal Zukowski",
                      MUIA_Application_Description, 	"XML metaformat viewer",
#ifdef __MORPHOS__
                      MUIA_Application_UsedClasses,   UsedClasses,
#endif
                      MUIA_Application_Version,   	VERSION_TAG,
                      MUIA_Application_Window, abt =  CreateAboutWindow(),
                      MUIA_Application_Menustrip, menustrip_w[MS_MENU] =  MUI_NewObject( MUIC_Menustrip,
                              MUIA_Family_Child, ( ULONG )MUI_NewObject( MUIC_Menu,
                                      MUIA_Menu_Title,  GetCatalogStr( Cat, MSG_MENU_PROJECT, "Project" ),
                                      MUIA_Family_Child, menustrip_w[MS_OPEN]     = CreateMenuitemLoad(),
                                      MUIA_Family_Child, menustrip_w[MS_SAVE]     = CreateMenuitemSave(),
                                      MUIA_Family_Child, menustrip_w[MS_SAVEFORMATTED]     = CreateMenuitemSaveFormatted(),
                                      MUIA_Family_Child, menustrip_w[MS_ABOUT]    = CreateMenuitemAbout(),
                                      MUIA_Family_Child, menustrip_w[MS_QUIT]     = CreateMenuitemQuit(),
                                      TAG_END ),
                              TAG_END ),

                      SubWindow, window  = WindowObject,
                      MUIA_Window_Title, "XML Viewer",
                      MUIA_Window_ID, MAKE_ID( 'T', 'R', 'E', 'E' ),
                      MUIA_Window_AppWindow, TRUE,
                      WindowContents, VGroup,
                        // top buttons
                      Child, HGroup,
                      Child, bt_open_nodes = KeyButton( GetCatalogStr( Cat, MSG_OPENALL, "Open All" ), 'o' ),
                      Child, bt_close_nodes = KeyButton( GetCatalogStr( Cat, MSG_CLOSEALL, "Close All" ), 'a' ),
                      End,

                        // tree view and attribute list
                      Child, HGroup,
                      Child, ListviewObject,
                      MUIA_Listview_List, lt_nodes = NewObject( xmlviewertreeClass->mcc_Class, NULL, TAG_DONE ),
                      End,
                      Child,  MUI_NewObject( "Balance.mui", TAG_END ),
                      Child,  list = MUI_NewObject( MUIC_Listview,
                                     MUIA_Listview_Input, FALSE,
                                     MUIA_Listview_List, MUI_NewObject( MUIC_List,
                                             MUIA_List_Format, "P=\033l,P=\033l",
                                             MUIA_Frame, MUIV_Frame_ReadList,
                                             MUIA_List_Title, TRUE,
                                             MUIA_List_ConstructHook,	( APTR )&DataConstructor_hook,
                                             MUIA_List_DestructHook,	( APTR )&DataDestructor_hook,
                                             MUIA_List_DisplayHook,	( APTR )&DataDisplayer_hook,
                                             TAG_END ),
                                     TAG_END ),
                      End,

                        // search
                      Child, MUI_NewObject( MUIC_Group,
                                            GroupFrame,
                                            MUIA_FrameTitle, GetCatalogStr( Cat, MSG_SEARCH_LABEL, "Search" ),
                                            MUIA_Background, MUII_GroupBack,
                                            MUIA_Group_Horiz, FALSE,
                                            Child, MUI_NewObject( MUIC_Group,
                                                    MUIA_Group_Horiz, TRUE,
                                                    Child, MUI_NewObject( MUIC_Text,
                                                            MUIA_Text_Contents, ( long )GetCatalogStr( Cat, MSG_SEARCH_NAMES, "Element names" ),
                                                            MUIA_FixWidthTxt, GetCatalogStr( Cat, MSG_SEARCH_NAMES, "Element names" ),
                                                            TAG_END ),
                                                    Child, checkmarks[CHM_NAMES] = MUI_NewObject( MUIC_Image,
                                                            MUIA_InputMode,               MUIV_InputMode_Toggle,
                                                            MUIA_Image_Spec,              MUII_CheckMark,
                                                            MUIA_ShowSelState,            TRUE,
                                                            MUIA_Selected,                TRUE,
                                                            MUIA_Frame,                   MUIV_Frame_ImageButton,
                                                            TAG_END ),

                                                    Child, MUI_NewObject( MUIC_Text,
                                                            MUIA_Text_Contents, ( long )GetCatalogStr( Cat, MSG_SEARCH_VALUES, "Element values" ),
                                                            MUIA_FixWidthTxt, GetCatalogStr( Cat, MSG_SEARCH_VALUES, "Element values" ),
                                                            TAG_END ),
                                                    Child, checkmarks[CHM_VALUES] = MUI_NewObject( MUIC_Image,
                                                            MUIA_InputMode,               MUIV_InputMode_Toggle,
                                                            MUIA_Image_Spec,              MUII_CheckMark,
                                                            MUIA_ShowSelState,            TRUE,
                                                            MUIA_Selected,                TRUE,
                                                            MUIA_Frame,                   MUIV_Frame_ImageButton,
                                                            TAG_END ),
                                                    Child, MUI_NewObject( MUIC_Text,
                                                            MUIA_Text_Contents, ( long )GetCatalogStr( Cat, MSG_SEARCH_ATTRIBS, "Attributes (name & values)" ),
                                                            MUIA_FixWidthTxt, GetCatalogStr( Cat, MSG_SEARCH_ATTRIBS, "Attributes (name & values)" ),
                                                            TAG_END ),
                                                    Child, checkmarks[CHM_ATTRIBUTES] = MUI_NewObject( MUIC_Image,
                                                            MUIA_InputMode,               MUIV_InputMode_Toggle,
                                                            MUIA_Image_Spec,              MUII_CheckMark,
                                                            MUIA_ShowSelState,            TRUE,
                                                            MUIA_Selected,                TRUE,
                                                            MUIA_Frame,                   MUIV_Frame_ImageButton,
                                                            TAG_END ),

                                                    Child, MUI_NewObject( MUIC_Rectangle,
                                                            TAG_END ),
                                                    TAG_END ),
                                            Child, MUI_NewObject( MUIC_Group,
                                                    MUIA_Group_Horiz, TRUE,
                                                    Child, search_string = StringObject,
                                                    StringFrame,
                                                    MUIA_String_MaxLen, 128,
                                                    End,
                                                    MUIA_Group_Child,  search_prev = MUI_NewObject( MUIC_Group,
                                                            MUIA_ShortHelp,  GetCatalogStr( Cat, MSG_SEARCH_PREV_HELP, "Search previous element." ),
                                                            MUIA_Frame, MUIV_Frame_Button,
                                                            MUIA_Background, MUII_ButtonBack,
                                                            MUIA_Group_Horiz, TRUE,
                                                            MUIA_InputMode, MUIV_InputMode_RelVerify,
                                                            MUIA_Group_Child, ImageObject, MUIA_Image_Spec, MUII_TapePlayBack, End,
                                                            MUIA_Group_Child, MUI_NewObject( MUIC_Text,
                                                                    MUIA_Font, MUIV_Font_Button,
                                                                    MUIA_Text_Contents, ( long )GetCatalogStr( Cat, MSG_SEARCH_PREV, "Prev" ),
                                                                    MUIA_FixWidthTxt, GetCatalogStr( Cat, MSG_SEARCH_PREV, "Prev" ),
                                                                    TAG_END ),
                                                            TAG_END ),
                                                    MUIA_Group_Child,  search_next = MUI_NewObject( MUIC_Group,
                                                            MUIA_ShortHelp, GetCatalogStr( Cat, MSG_SEARCH_NEXT_HELP, "Search next element." ),
                                                            MUIA_Frame, MUIV_Frame_Button,
                                                            MUIA_Background, MUII_ButtonBack,
                                                            MUIA_Group_Horiz, TRUE,
                                                            MUIA_InputMode, MUIV_InputMode_RelVerify,
                                                            MUIA_Group_Child, MUI_NewObject( MUIC_Text,
                                                                    MUIA_Font, MUIV_Font_Button,
                                                                    MUIA_Text_Contents, ( long )GetCatalogStr( Cat, MSG_SEARCH_NEXT, "Next" ),
                                                                    MUIA_FixWidthTxt, GetCatalogStr( Cat, MSG_SEARCH_NEXT, "Next" ),
                                                                    TAG_END ),
                                                            MUIA_Group_Child, ImageObject, MUIA_Image_Spec, MUII_TapePlay, End,
                                                            TAG_END ),
                                                    TAG_END ),
                                            TAG_END ),
                      End,
                      End,
                      End;


                      if( obj )
{
    struct MyApp_Data   *data = INST_DATA( cl, obj );

        DoMethod( menustrip_w[MS_QUIT], MUIM_Notify,  MUIA_Menuitem_Trigger, MUIV_EveryTime, ( ULONG )obj, 2,
                  MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit );
        DoMethod( menustrip_w[MS_ABOUT], MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, abt, 3, MUIM_Set, MUIA_Window_Open, TRUE );

        DoMethod( menustrip_w[MS_OPEN], MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, obj, 2, MUIM_App_AslReq, MODE_LOAD );

        DoMethod( menustrip_w[MS_SAVE], MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, obj, 2, MUIM_App_AslReq, MODE_SAVE );


        DoMethod( window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,  obj, 2,
                  MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit );

        DoMethod( bt_open_nodes, MUIM_Notify, MUIA_Pressed, FALSE, lt_nodes, 4,
                  MUIM_Listtree_Open, MUIV_Listtree_Open_ListNode_Root, MUIV_Listtree_Open_TreeNode_All, 0 );

        DoMethod( bt_close_nodes, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_App_CloseAll );


        DoMethod( lt_nodes, MUIM_Notify, MUIA_Listtree_Active, MUIV_EveryTime, lt_nodes, 3,
                  MUIM_CallHook, &active_hook, MUIV_TriggerValue );

        DoMethod( search_prev, MUIM_Notify, MUIA_Pressed, FALSE,  obj, 2, MUIM_App_Search, 0 );
        DoMethod( search_next, MUIM_Notify, MUIA_Pressed, FALSE,  obj, 2, MUIM_App_Search, 1 );


        SetAttrs( window, MUIA_Window_Open, TRUE, TAG_END );
        data->last_file[0] = 0;
        data->last_dir[0] = 0;
        data->last_search = -10; // do not activate search right now

        if( filename && strlen( filename ) > 0 )
        {
            DoMethod( obj, MUIM_App_Load,  filename );
        }

        return( ( ULONG )obj );
    }

    return( FALSE );
}

///
/// MyApp_AslReq()

ULONG MyApp_AslReq( struct IClass *cl, Object *obj, struct MUIP_App_FileReq* msg )
{
    struct FileRequester *smr = 0;
    char buffer[1024];
    struct Window *window2;
    struct MyApp_Data   *data = INST_DATA( cl, obj );

    get( window, MUIA_Window, &window2 );

    smr = MUI_AllocAslRequestTags( ASL_FileRequest, TAG_DONE );
    if( smr )
    {
        if( ( MUI_AslRequestTags( smr,
                                  ASLFR_Window, window2,
                                  ASLFR_DoSaveMode, ( msg->mode == MODE_LOAD ) ? FALSE : TRUE,
                                  ASLFR_DoMultiSelect, FALSE,
                                  ASLFR_InitialDrawer, data->last_dir,
                                  ASLFR_InitialFile, data->last_file,

                                  ASLFR_TitleText, GetCatalogStr( Cat, MSG_ASLSELECTFILE, "Select XML File:" ), TAG_DONE ) ) )
        {

            if( !( smr->fr_Drawer ) )
            {
                strcpy( buffer, "PROGDIR:" );
            }
            else
            {
                strcpy( buffer, smr->fr_Drawer );
            }
            AddPart( buffer, smr->fr_File, sizeof( buffer ) );

            strcpy( data->last_file, smr->fr_File );
            strcpy( data->last_dir, smr->fr_Drawer );

            if( msg->mode == MODE_LOAD )
            {
                DoMethod( obj, MUIM_App_Load, buffer );
            }
            else if( msg->mode == MODE_SAVE )
            {
                DoMethod( obj, MUIM_App_Save, buffer );
            }
        }
        else
        {
            return FALSE;
        }

        MUI_FreeAslRequest( smr );
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}


///
/// MyApp_Dispose()

ULONG MyApp_Dispose( struct IClass *cl, Object *obj, Msg msg )
{
    struct MyApp_Data *data;

    data = INST_DATA( cl, obj );

    SetAttrs( window, MUIA_Window_Open, FALSE, TAG_END );

    return( DoSuperMethodA( cl, obj, msg ) );
}

///
/// MyApp_Load()

ULONG MyApp_Load( struct IClass *cl, Object *obj,  struct MUIP_App_Filename *msg )
{
    struct MyApp_Data   *data = INST_DATA( cl, obj );
    char error_buf[100];
    BPTR fileXML;
    struct FileFormatInfo format_info;
    BOOL success = FALSE;

    if( msg->filename )
    {
        if( fileXML  = Open( ( STRPTR )msg->filename, MODE_OLDFILE ) )
        {
            if( !DetectFileFormat( ( const char* )msg->filename, fileXML, &format_info ) )
            {
                snprintf( error_buf,  100, "%s", GetCatalogStr( Cat, MSG_FORMAT_UNKNOWN, "Unable to detect file format" ) );
                MUI_RequestA( obj, window, 0, "XML Viewer Error", "*OK", error_buf, NULL );
                Close( fileXML );
                return TRUE;
            }

            DoMethod( list,  MUIM_Set, MUIA_List_Quiet, TRUE );
            DoMethod( lt_nodes,  MUIM_Set, MUIA_Listtree_Quiet, TRUE );
            DoMethod( lt_nodes, MUIM_Listtree_Remove, MUIV_Listtree_Remove_ListNode_Root, MUIV_Listtree_Remove_TreeNode_All, 0 );

            m_tree.tree = lt_nodes;
            m_tree.depth = 0;
            m_tree.has_utf8_bom = format_info.has_utf8_bom;
            memset( m_tree.tn, 0, sizeof( m_tree.tn ) );
            data->last_search = -1; // reset any active search

            strcpy( m_tree.filename, FilePart( msg->filename ) );

            switch( format_info.format )
            {
                case FILE_FORMAT_JSON:
                    success = load_json_tree( obj, fileXML, &m_tree, error_buf, sizeof( error_buf ) );
                    break;
                case FILE_FORMAT_YAML:
                    success = load_yaml_tree( obj, fileXML, &m_tree, error_buf, sizeof( error_buf ) );
                    break;
                case FILE_FORMAT_IFF:
                    success = load_iff_tree( obj, fileXML, &m_tree, error_buf, sizeof( error_buf ) );
                    break;
                case FILE_FORMAT_XML:
                default:
                    success = load_xml_tree( obj, fileXML, &m_tree, parser, error_buf, sizeof( error_buf ) );
                    break;
            }

            DoMethod( lt_nodes, MUIM_Set,  MUIA_Listtree_Quiet, FALSE );
            DoMethod( list,  MUIM_Set,  MUIA_List_Quiet, FALSE );

            if( success )
            {
                DoMethod( lt_nodes, MUIM_Listtree_Open, MUIV_Listtree_Open_ListNode_Root, MUIV_Listtree_Open_TreeNode_All, 0 );
            }

            Close( fileXML );
        }
        else
        {
            snprintf( error_buf,  100, "%s %s!!!", GetCatalogStr( Cat, MSG_UNABLETOOPENFILE, "Error opening" ), ( char* )msg->filename );
            MUI_RequestA( obj, window, 0, "XML Viewer Error", "*OK", error_buf, NULL );
        }
    }
    return TRUE;
}


///
/// MyApp_Search()

#define ASIZE 256   /* alphabet size */

static void preQsBc( UBYTE *x, LONG m, LONG qsBc[] )
{
    LONG i;

    for( i = 0; i < ASIZE; ++i )
    {
        qsBc[i] = m + 1;
    }
    for( i = 0; i < m; ++i )
    {
        qsBc[x[i]] = m - i;
    }
}


static LONG QuickSearch( UBYTE *x, LONG m, UBYTE *y, LONG n )
{
    LONG j, qsBc[ASIZE];

    /* Preprocessing */
    preQsBc( x, m, qsBc );

    /* Searching */
    j = 0;
    while( j <= n - m )
    {
        if( memcmp( x, y + j, m ) == 0 )
        {
            return j;
        }
        j += qsBc[y[j + m]];               /* shift */
    }

    return -1;
}


ULONG MyApp_Search( struct IClass *cl, Object *obj,  struct MUIP_App_FileReq *msg )
{
    struct MyApp_Data   *data = INST_DATA( cl, obj );

    struct MUIS_Listtree_TreeNode *treenode;
    char *string = NULL;
    int pos, found, attribute_index, entry_count;
    int is_values = 0, is_names = 0, is_attributes = 0;
    int Nr;
    struct DataNode *dn;
    struct MinList *tmp_list;
    int strlen_string;
    struct xml_data *tmp;

    if( data->last_search == -10 )
    {
        return FALSE;
    }
    // get text to be searched
    get( search_string, MUIA_String_Contents, &string );

    if( string && (strlen_string=strlen(string)))
    {
        
		// get address of the active node
		treenode = ( struct MUIS_Listtree_TreeNode * )DoMethod( lt_nodes, MUIM_Listtree_GetEntry,
				   MUIV_Listtree_GetEntry_ListNode_Root, MUIV_Listtree_GetEntry_Position_Active, 0 );
		// get number of the tree node
		Nr = DoMethod( lt_nodes, MUIM_Listtree_GetNr, treenode, 0 );
		// if no node selected, try to find from start
		if( Nr < 0 )
		{
			data->last_search = -1;
		}
		
		// pos=last%Nr;
		pos = data->last_search;
                  // determine whether to search forward or backward
		if( msg->mode )
		{
			pos++;
		}
		else
		{
			pos--;
		}

		get( checkmarks[CHM_VALUES], MUIA_Selected, &is_values );
		get( checkmarks[CHM_ATTRIBUTES], MUIA_Selected, &is_attributes );
		get( checkmarks[CHM_NAMES], MUIA_Selected, &is_names );

                  // do not search if no checkmarks are selected
		if( is_values || is_attributes || is_names )
			//KPrintF("Nr: %d, last:%d\n", Nr, pos);
			while( 1 )
			{
				if( ( treenode = ( struct MUIS_Listtree_TreeNode * )DoMethod( lt_nodes, MUIM_Listtree_GetEntry,  MUIV_Listtree_GetEntry_ListNode_Root, pos, 0 ) ) )
				{
					if( is_names )
						if( treenode->tn_Flags & TNF_LIST )
						{
							if( QuickSearch( string, strlen_string,  treenode->tn_Name, strlen( treenode->tn_Name ) ) != -1 )
							{
								data->last_search = pos;
								break;
							}
						}
					if( is_values )
						if( !( treenode->tn_Flags & TNF_LIST ) )
						{
							if( QuickSearch( string, strlen_string,  treenode->tn_Name, strlen( treenode->tn_Name ) ) != -1 )
							{
								data->last_search = pos;
								break;
							}
						}

					if( is_attributes )
					{

						// get attribs list
						if( ( tmp = ( struct xml_data* )( treenode->tn_User ) ) )
							if( ( tmp_list = tmp->attr_list ) )
							{
								found = 0;
                                                                  attribute_index = 0;
								for( dn = ( struct DataNode* )tmp_list->mlh_Head;  dn->Node.mln_Succ;  dn = ( struct DataNode* )dn->Node.mln_Succ )
								{
									if( dn )
									{
										if( QuickSearch( string, strlen_string,  dn->values, strlen( dn->values ) ) != -1 )
										{
											found = 1;
											break;
										}
										if( QuickSearch( string, strlen_string,  dn->atributes, strlen( dn->atributes ) ) != -1 )
										{
											found = 1;
											break;
										}
                                                                                  attribute_index++;
									}
									else
									{
										break;
									}
								}
							}
						if( found )
						{
							data->last_search = pos;
							break;
						}
					}

                                          // determine whether to search forward or backward
					if( msg->mode )
					{
						pos++;
					}
					else
					{
						pos--;
					}
				}
                                  else     // reached the end, restart from the beginning
				{
					data->last_search = -1;
					break;
				}
			}
                  // open the located node
		DoMethod( lt_nodes, MUIM_Listtree_Open, MUIV_Listtree_Open_ListNode_Parent, treenode, 0 );

                  // make the located node active
		set( lt_nodes, MUIA_Listtree_Active, treenode );

		if( found )
		{
                          get( list, MUIA_List_Entries, &entry_count );
                          set( list, MUIA_List_Active, entry_count - attribute_index - 1 );
		}
		return TRUE;

    }

    return FALSE;
}

///
/// MyApp_CloseAll()

ULONG MyApp_CloseAll( struct IClass *cl, Object *obj,  struct MUIP_App_Filename *msg )
{
    //struct MyApp_Data   *data = INST_DATA(cl,obj);

    DoMethod( list,  MUIM_Set, MUIA_List_Quiet, TRUE );
    DoMethod( lt_nodes,  MUIM_Set, MUIA_Listtree_Quiet, TRUE );

    // close all elements
    DoMethod( lt_nodes, MUIM_Listtree_Close, MUIV_Listtree_Close_ListNode_Root, MUIV_Listtree_Close_TreeNode_All, 0 );

    DoMethod( lt_nodes, MUIM_Set,  MUIA_Listtree_Quiet, FALSE );
    DoMethod( list,  MUIM_Set,  MUIA_List_Quiet, FALSE );

    return TRUE;
}

///
/// MyApp_Save()

ULONG MyApp_Save( struct IClass *cl, Object *obj,  struct MUIP_App_Filename *msg )
{
    //struct MyApp_Data   *data = INST_DATA(cl,obj);
    LONG ret_val = FALSE;

    if( msg->filename )
    {
        BPTR fp;
        if( fp = Open( ( STRPTR )msg->filename, MODE_NEWFILE ) )

        {
            DoMethod( lt_nodes, MUIM_LTree_Save, fp, MUIV_Listtree_GetEntry_ListNode_Root, MODE_SAVE );
            Close( fp );
            ret_val = TRUE;
        }
    }

    return ret_val;
}

///
/// MyApp_Class_Dispatcher()

DISPATCHER( MyApp_Class_ )
switch( msg->MethodID )
{
	case OM_NEW:
		return ( MyApp_New( cl, obj, ( struct opSet* )msg ) );
	case OM_DISPOSE:
		return ( MyApp_Dispose( cl, obj, ( APTR )msg ) );
	case MUIM_App_AslReq:
		return ( MyApp_AslReq( cl, obj, ( struct MUIP_App_FileReq* )msg ) );
	case MUIM_App_Load:
		return ( MyApp_Load( cl, obj, ( struct MUIP_App_Filename* )msg ) );
	case MUIM_App_Save:
		return ( MyApp_Save( cl, obj, ( struct MUIP_App_Filename* )msg ) );
	case MUIM_App_Search:
		return ( MyApp_Search( cl, obj, ( struct MUIP_App_FileReq * )msg ) );
	case MUIM_App_CloseAll:
		return ( MyApp_CloseAll( cl, obj, ( APTR )msg ) );
	default:
		return ( DoSuperMethodA( cl, obj, msg ) );
}
DISPATCHER_END

///
/// Main()

int main( int argc, char **argv )
{
    ULONG signals;

    if( argc == 2 )
    {
        filename = argv[1];
    }


#ifndef __MORPHOS__
    MUIMasterBase = OpenLibrary( "muimaster.library", 16 );
#endif

    if( ( ExpatBase = OpenLibrary( "expat.library", 4 ) ) )
    {
        Cat = OpenCatalog( NULL, "xmlviewer.catalog", TAG_END );

        if( ExpatBase->lib_Version == 12 )
        {
            Printf( GetCatalogStr( Cat, MSG_WRONGEXPTATVERS, "Wrong version of \"expat.library\"!\n" ) );
            CloseLibrary( ExpatBase );
            return 0;
        }

        if( !( MyAppClass = CreateAppClass() ) )
        {
            Printf( GetCatalogStr( Cat, MSG_CUSTCLASSFAIL, "Failed to create custom class.\n" ) );
        }
        else
        {
            // tree class
            if( ( xmlviewertreeClass = CreatexmlviewertreeClass() ) )
            {

                parser = XML_ParserCreate( NULL );
                if( ( app = NewObject( MyAppClass->mcc_Class, NULL, TAG_DONE ) ) )
                {
                    while( DoMethod( app, MUIM_Application_NewInput, &signals ) != MUIV_Application_ReturnID_Quit )
                    {
                        if( signals )
                        {
                            signals = Wait( signals | SIGBREAKF_CTRL_C );

                            if( signals & SIGBREAKF_CTRL_C )
                            {
                                break;
                            }
                        }
                    }

                    // remove application
                    MUI_DisposeObject( app );
                }
                else
                {
                    Printf( GetCatalogStr( Cat, MSG_APPFAIL, "Failed to create Application.\n" ) );
                }

                XML_ParserFree( parser );
                if( xmlviewertreeClass )
                {
                    MUI_DeleteCustomClass( xmlviewertreeClass );
                }
            }
            else
            {
                Printf( GetCatalogStr( Cat, MSG_CUSTCLASSFAIL, "Failed to create custom class.\n" ) );
            }
            if( MyAppClass )
            {
                MUI_DeleteCustomClass( MyAppClass );
            }
        }

        if( Cat )
        {
            CloseCatalog( Cat );
        }
        CloseLibrary( ExpatBase );
    }
    else
    {
        Printf( GetCatalogStr( Cat, MSG_EXPATFAIL, "Unable to open \"expat.library\" >= 4.0\n" ) );
    }

    return( 0 );
}


BOOL load_json_tree( Object *obj, BPTR fileXML, struct XMLTree *tree, char *error_buf, size_t error_buf_len )
{
    LONG file_size;
    char *json_buf = NULL;
    BOOL success = FALSE;
    LONG start_offset = tree && tree->has_utf8_bom ? 3 : 0;

    if( ( file_size = Seek( fileXML, 0, OFFSET_END ) ) > 0 )
    {
        if( Seek( fileXML, 0, OFFSET_BEGINNING ) != -1 )
        {
            if( ( json_buf = ( char* )AllocVec( file_size + 1, MEMF_ANY ) ) )
            {
                if( Read( fileXML, json_buf, file_size ) == file_size )
                {
                    json_buf[file_size] = '\0';

                    if( file_size >= start_offset )
                    {
                        if( JsonToTree( tree, tree->filename, json_buf + start_offset ) )
                        {
                            success = TRUE;
                        }
                        else
                        {
                            snprintf( error_buf, error_buf_len, "%s", GetCatalogStr( Cat, MSG_JSON_PARSE_ERROR, "Error parsing JSON file" ) );
                        }
                    }
                    else
                    {
                        snprintf( error_buf, error_buf_len, "%s", GetCatalogStr( Cat, MSG_JSON_EMPTY_ERROR, "JSON file is empty" ) );
                    }
                }
                else
                {
                    snprintf( error_buf, error_buf_len, "%s", GetCatalogStr( Cat, MSG_JSON_READ_ERROR, "Error reading JSON file" ) );
                }

                FreeVec( json_buf );
            }
            else
            {
                snprintf( error_buf, error_buf_len, "%s", GetCatalogStr( Cat, MSG_JSON_ALLOC_ERROR, "Unable to allocate buffer for JSON file" ) );
            }
        }
        else
        {
            snprintf( error_buf, error_buf_len, "%s", GetCatalogStr( Cat, MSG_JSON_REWIND_ERROR, "Unable to rewind JSON file" ) );
        }
    }
    else
    {
        snprintf( error_buf, error_buf_len, "%s", GetCatalogStr( Cat, MSG_JSON_EMPTY_ERROR, "JSON file is empty" ) );
    }

    if( !success )
    {
        MUI_RequestA( obj, window, 0, "XML Viewer Error", "*OK", error_buf, NULL );
    }

    return success;
}

BOOL load_yaml_tree( Object *obj, BPTR fileXML, struct XMLTree *tree, char *error_buf, size_t error_buf_len )
{
    LONG file_size;
    char *yaml_buf = NULL;
    BOOL success = FALSE;
    LONG start_offset = tree && tree->has_utf8_bom ? 3 : 0;

    if( ( file_size = Seek( fileXML, 0, OFFSET_END ) ) > 0 )
    {
        if( Seek( fileXML, 0, OFFSET_BEGINNING ) != -1 )
        {
            if( ( yaml_buf = ( char* )AllocVec( file_size + 1, MEMF_ANY ) ) )
            {
                if( Read( fileXML, yaml_buf, file_size ) == file_size )
                {
                    yaml_buf[file_size] = '\0';

                    if( file_size >= start_offset )
                    {
                        if( YamlToTree( tree, tree->filename, yaml_buf + start_offset ) )
                        {
                            success = TRUE;
                        }
                        else
                        {
                            snprintf( error_buf, error_buf_len, "%s", GetCatalogStr( Cat, MSG_YAML_PARSE_ERROR, "Error parsing YAML file" ) );
                        }
                    }
                    else
                    {
                        snprintf( error_buf, error_buf_len, "%s", GetCatalogStr( Cat, MSG_YAML_EMPTY_ERROR, "YAML file is empty" ) );
                    }
                }
                else
                {
                    snprintf( error_buf, error_buf_len, "%s", GetCatalogStr( Cat, MSG_YAML_READ_ERROR, "Error reading YAML file" ) );
                }

                FreeVec( yaml_buf );
            }
            else
            {
                snprintf( error_buf, error_buf_len, "%s", GetCatalogStr( Cat, MSG_YAML_ALLOC_ERROR, "Unable to allocate buffer for YAML file" ) );
            }
        }
        else
        {
            snprintf( error_buf, error_buf_len, "%s", GetCatalogStr( Cat, MSG_YAML_REWIND_ERROR, "Unable to rewind YAML file" ) );
        }
    }
    else
    {
        snprintf( error_buf, error_buf_len, "%s", GetCatalogStr( Cat, MSG_YAML_EMPTY_ERROR, "YAML file is empty" ) );
    }

    if( !success )
    {
        MUI_RequestA( obj, window, 0, "XML Viewer Error", "*OK", error_buf, NULL );
    }

    return success;
}

BOOL load_iff_tree( Object *obj, BPTR fileXML, struct XMLTree *tree, char *error_buf, size_t error_buf_len )
{
    BOOL success = FALSE;

    if( IffToTree( tree, tree->filename, fileXML ) )
    {
        success = TRUE;
    }
    else
    {
        snprintf( error_buf, error_buf_len, "%s", GetCatalogStr( Cat, MSG_IFF_PARSE_ERROR, "Error parsing IFF file" ) );
        MUI_RequestA( obj, window, 0, "XML Viewer Error", "*OK", error_buf, NULL );
    }

    return success;
}

BOOL load_xml_tree( Object *obj, BPTR fileXML, struct XMLTree *tree, XML_Parser parser, char *error_buf, size_t error_buf_len )
{
    char buf[BUFSIZ];
    int done;
    BOOL success = TRUE;

    XML_ParserReset( parser, 0 );
    XML_SetUserData( parser, tree );
    XML_SetElementHandler( parser, startElement, endElement );
    XML_SetCharacterDataHandler( parser,  default_hndl );
    XML_SetXmlDeclHandler( parser, decl_hndl );
    XML_SetCommentHandler( parser, comment_hndl );

    do
    {
        size_t len =  Read( fileXML, buf, BUFSIZ ); //fread(buf, 1, sizeof(buf), fileXML);
        done = len < BUFSIZ;
        if( XML_Parse( parser, buf, len, done ) == XML_STATUS_ERROR )
        {
            snprintf( error_buf, error_buf_len,  "Error:\n\n%s at line %d",
                      XML_ErrorString( XML_GetErrorCode( parser ) ), XML_GetCurrentLineNumber( parser ) );
            MUI_RequestA( obj, window, 0, "XML Viewer Error", "*OK", error_buf, NULL );
            success = FALSE;
            break;
        }

    }
    while( !done );

    return success;
}


///


