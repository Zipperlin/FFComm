/////////////////////////////////////////////////////////////////////////////
//
//  Filename   :  DDSWrap.h
//
//  Description:  Header file for Device Descriptions Services DLL	
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __DDS_H__              
#define __DDS_H__

/////////////////////////////////////////////////////////////////////////////

#include "ddi_lib.h"

/////////////////////////////////////////////////////////////////////////////

#define MAX_HELP_LEN	255
#define MAX_LABEL_LEN	32

typedef struct
{
	CHAR *			unit;			//
	ULONG			read_time_out;	//
	ULONG			write_time_out;	//
	RANGE_DATA_LIST	min_val;		//
	RANGE_DATA_LIST	max_val;		//
	EXPR			scale;			// scale factor 
	ULONG			valid;			// validity 
} 
PARA_MISC;

typedef struct
{
	USHORT			evaled;
	ULONG			val;
	CHAR *			desc;
	CHAR *			help;
	ULONG			func_class;		// functional class 
	ULONG			status;
	ITEM_ID         actions;
}
PARA_ENUM_VALUE;

typedef struct
{
	USHORT				count;
	PARA_ENUM_VALUE *	list;
} 
ENUM_LIST;

typedef struct
{
	ITEM_ID		para_id;
	ITEM_ID		para_nameid;
	CHAR *		para_name;
	CHAR *		label;
	CHAR *		help;
	ENUM_LIST	enums;
	PARA_MISC	misc;
	TYPE_SIZE	type_size;
	int			ReadWrite;	// Read:1,Write:2
	UCHAR *		Val;
	ULONG		class_attr; // parameter type INPUT/OUTPUT/ALARM...
} 
FB_SUB_PARA_LIST;

typedef struct
{
	ITEM_ID				para_nameid;
	CHAR *				para_name;
	ITEM_ID				para_id;
	CHAR *				label;
	CHAR *				help;
	ULONG				class_attr;	// parameter type INPUT/OUTPUT/ALARM...
	ULONG				para_type;	// variable,record,array...
	int					count;		// number of sub-parameters 
	FB_SUB_PARA_LIST *	subpara_list;
} 
FB_PARA_LIST;

typedef struct
{
	DESC_REF	ref;	
	CHAR *		member;
}
MEMBERLIST;

typedef struct
{
	ITEM_ID			id;
	ITEM_ID			nameid;	// Item ID for this item 
	CHAR *			view_name;
	int				member_count;
	MEMBERLIST *	member_list;
	CHAR *			help;
	CHAR *			label;
} 
FB_VIEW_LIST;

typedef struct
{
	CHAR			block_tag[MAX_LABEL_LEN];
	ITEM_ID			block_id;
	int				count;
	int				block_type;
	CHAR *			help;
	CHAR *			desc;
	FB_PARA_LIST *	para_list;
	int				view_count;
	FB_VIEW_LIST *	view_list;
} 
FB_PARA;

///////////////////////////////////////////////////////////////////////////

#ifdef DDS_EXPORTS
	#define DDS_API __declspec(dllexport)
#else
#define DDS_API __declspec(dllimport)
	#ifdef _DEBUG
		#pragma comment(lib, "ddswrapd.lib")
	#else
		#pragma comment(lib, "ddswrap.lib")
	#endif
#endif	//DDS_EXPORTS

///////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif	//__cplusplus

DDS_API int dds_cm_init(LPSTR pszPath);

DDS_API int dds_cm_end();

DDS_API int dds_build_dev_table(DD_DEVICE_ID * pDeviceID);

DDS_API int dds_get_block_para(LPSTR pszBlockTag, FB_PARA * pPara);

DDS_API int dds_free_block_memory(FB_PARA * pPara);

DDS_API int dds_get_block_list(DD_DEVICE_ID *	pDeviceID,
							   BLOCK_INFO *		pBlockList,
							   int *			pBlockCount);

DDS_API int dds_open_block(LPCSTR pszBlockTag);

#ifdef __cplusplus
}
#endif	//__cplusplus

//////////////////////////////////////////////////////////////////////////

#endif	//__DDS_H__

//////////////////////////////////////////////////////////////////////////
