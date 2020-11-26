//
// FFDef.h
//
#ifndef __FF_DEF_H__
#define __FF_DEF_H__

typedef unsigned char   uint8;
typedef char            int8;
typedef unsigned short  uint16;
typedef short           int16;
typedef unsigned int    uint32;
typedef int             int32;
typedef unsigned long long uint64;
typedef long long         int64;
typedef uint8           bool_t; 

typedef struct
{
    uint32  upper;
    uint32  lower;
} nifTime_t;

#define DEV_ID_SIZE		32
#define TAG_SIZE		32
#define IP_SIZE			16
#define NIF_NAME_LEN    66

// block types
#define FUNCTION_BLOCK   1
#define TRANSDUCER_BLOCK 2
#define RESOURCE_BLOCK   4

#define MAX_PATH 260

enum FF_RESULT_CODE
{
	FF_SUCCESS = 0,
	FF_ERR_INVALID_MODULE_HANDLE,
	FF_ERR_NOT_FOUND,
	FF_ERR_SYSTEM_ERROR,
	FF_ERR_INVALID_ARGUMENT,
	FF_ERR_TIMEOUT,
	FF_ERR_NEGATIVE_ECHO,
	FF_ERR_INVOKE_ID_MISMATCH,
	FF_ERR_INVALID_VCR_ID,
	FF_ERR_LIVELIST_HAS_NO_INTERFACE,
	FF_ERR_DEV_ABORT,
	FF_ERR_VCR_FULL,
	FF_ERR_VCR_MISMATCH,
	FF_ERR_INIT_VCR_FAILED,
	FF_ERR_DENIED_BY_INTFCARD,
	FF_ERR_BUFFER_TOO_SMALL,
	FF_ERR_DATA_TOO_LONG,
	FF_ERR_DATA_INCOMPLETE,
	FF_ERR_SOCKET_ERROR,
	FF_ERR_DEV_NOT_FOUND,
	FF_ERR_DEV_NOT_CONFIGURED,
	FF_ERR_LOAD_PROJECT_FAILED,
	FF_ERR_PARSE_LINK_FAILED,
	FF_ERR_DEVICE_NOT_SUPPORTED,
	FF_ERR_READ_DIRECTORY_ERROR,
	FF_ERR_READ_PARAMETER_FAILED,
	FF_ERR_DEV_NOT_INITIALIZED,
	FF_ERR_DEV_NOT_MAPPED,
	FF_ERR_DEV_INFO_ERROR,
	FF_ERR_DEV_BLOCK_NOT_FOUND,
	FF_ERR_DEV_BLOCK_MISMATCH,
	FF_ERR_DEV_INST_BLOCK_FAILED,
	FF_ERR_RS232_READ_NO_DATA,
	FF_ERR_CFG_FILE_ERROR,
	FF_ERR_DD_ERROR,

	FF_ERR_USER_CANCEL,

	// for CNCS
	FF_ERR_CNCS_STATION_ERROR_CODE_MASK = 0x1000,
	FF_ERR_CNCS_STATION_PACKET_LEN_MISMATCH = 0x1100,
	FF_ERR_CNCS_STATION_PACKET_CHECKSUM_ERROR,
	FF_ERR_CNCS_STATION_PACKET_HEAD_ERROR,
	FF_ERR_CNCS_STATION_TIMEOUT,
//	FF_ERR_STATION_BUS_ERROR,

	FF_ERR_CNCS_MODULE_TIMEOUT,
	FF_ERR_CNCS_MODULE_PACKET_LEN_MISMATCH,
	FF_ERR_CNCS_MODULE_PACKET_HEAD_ERROR,
};

enum FFBLOCK_TYPE
{
	FF_RES_BLOCK = 1,			// FF资源块
	FF_TRA_BLOCK = 2,			// FF转换块
	FF_FUNC_BLOCK = 4,			// FF功能块
};


enum VFD_TYPE
{
	H1_VFD_ALL,
	H1_VFD_MIB,
	H1_VFD_VFD,
};

#define E_MAX_WAIT_INTR					(-100)
// End of MicroCyber definitions.

#define E_COMM_ERROR_REMOTE		        (-1000)
#define E_CANNOT_GET_VFDLIST		    (-1001)
#define E_CANNOT_GET_DEVLIST		    (-1002)
#define E_FMS_ABORT						(-1003)

// from HSE init.
#define E_INVALID_VCR					0xff00



typedef long FF_RESULT;
typedef unsigned long FF_DESC;

typedef enum _PROTOCOL_TYPE
{
	PROTOCOL_NONE =		0x00,
	PROTOCOL_H1 =		0x01,
	PROTOCOL_HSE =		0x02,
} PROTOCOL_TYPE;

typedef enum _READ_OBJECT_TYPE
{
	READOBJ_BY_NAME = 0,
	READOBJ_BY_INDEX,
	READOBJ_BY_INDEX_SUBINDEX,
	READOBJ_BY_NAME_SUBINDEX,
	READOBJ_BY_BLOCK_INDEX,
	READOBJ_BY_BLOCK_INDEX_SUBINDEX,
	READOBJ_BY_BLOCK_NAME_SUBINDEX,
} READ_OBJECT_TYPE;

enum
{
	OPEN_DEV_BY_NAME = 0,
	OPEN_DEV_BY_ID = 1,
};

// #define DEFAULT_RETRY_NUMBER	5
#define MAX_READ_BUFFER			256

#ifdef __FFCFG_OLD_PROJ_SERIALIZE__

#pragma pack(push)
#pragma pack(1)

typedef struct {
    char deviceID[DEV_ID_SIZE + 1];
    char pdTag[TAG_SIZE + 1];
    uint8  nodeAddress;
    uint32 flags;
} __ffDeviceInfo_t;

typedef struct
{
    char	vfdTag[TAG_SIZE + 1];
    char    vendor[TAG_SIZE + 1];
    char    model[TAG_SIZE + 1];
    char    revision[TAG_SIZE + 1];
    WORD	ODVersion;
    WORD	numTransducerBlocks;
    WORD	numFunctionBlocks;
	WORD	action_object_start_index;
    WORD	numActionObjects;
	WORD	link_object_start_index;
    WORD	numLinkObjects;
	WORD	alert_object_start_index;
    WORD	numAlertObjects;
 	WORD	trend_object_start_index;
	WORD	numTrendObjects;
	WORD	domain_object_start_index;
    WORD	numDomainObjects;
    WORD	totalObjects;
    DWORD	flags;
} __ffVFDInfo_t;

typedef struct {
    char	fbTag[TAG_SIZE + 1];
    uint16  startIndex;
    uint32  ddName;
    uint32  ddItem;
    uint16  ddRev;
    uint16  profile;
    uint16  profileRev;
    uint32  executionTime;
    uint32  periodExecution;
    uint16  numParams;
    uint16  nextFb;
    uint16  startViewIndex;
    uint8   numView3;
    uint8   numView4;
    uint16  ordNum;
    uint8   blockType;
} __ffBlockInfo_t;

typedef struct {
    char            interfaceName[NIF_NAME_LEN];
    char            deviceID[DEV_ID_SIZE + 1];
} __ffInterfaceInfo_t;

#pragma pack(pop)

#endif // __FFCFG_OLD_PROJ_SERIALIZE__

#define FFH1_DEVID_LEN	32
#define FFH1_PDTAG_LEN	32
#define BLOCK_TAG_LEN	32

// new data struct type
typedef struct __h1_dev_info
{
	char pdtag[FFH1_PDTAG_LEN];
	char device_id[FFH1_DEVID_LEN];
	uint8 addr;
	uint32 manu_id;
	uint16 dev_type;
	uint8 dev_ver;
	uint8 dd_ver;
	uint8 dev_class;
//	DWORD dwStatus;
} H1_DEV_INFO;

typedef struct __h1_block_info
{
	char tag[32];
	uint16 start_index;
	uint16 start_view_index;
	uint8 block_type;
	uint32 exec_time;
	uint32 dd_item;
	uint16 profile;
	uint16 profile_rev;
} H1_BLOCK_INFO;

#endif // __FF_DEF_H__
