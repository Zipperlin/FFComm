//
// FFComm.h
//

#ifndef __FF_COMM_H__
#define __FF_COMM_H__

#include "FFDef.h"
#include <pthread.h>

#ifdef FFCOMM_EXPORTS
//#define FFCOMM_API extern "C" __declspec(dllexport)
//#else
//#define FFCOMM_API extern "C" __declspec(dllimport)
//#ifdef _DEBUG
//#pragma comment(lib, "FFCommD.lib")
//#else
//#pragma comment(lib, "FFComm.lib")
//#endif
#endif

#define MAX_DATA_FIELD_LEN 200
#define INVALID_MODULE_HANDLE   (0)
#define VCR_NONE	(0xffff)
#define	INTERFACE_DEV_ID	( "MC-INTERFACE-DEV                " ) // TODO:
#define DEV_TEMP_ADDR_START  0xF8
#define IS_TEMP_H1_ADDR(addr) (addr >= DEV_TEMP_ADDR_START)

// typedef uint8	CNCS_CONTROLLER_ID;
// typedef uint8	CNCS_SLOT_INDEX;
typedef uint16  CNCS_MODULE_KEY;

typedef int		RS232_MODULE_KEY;

typedef void*	FF_MODULE_HANDLE;
typedef uint8	FF_LINK_INDEX;

enum FF_DEV_CLASS
{
	DEV_CLASS_BASIC = 1,
	DEV_CLASS_LINK_MASTER = 2,
	DEV_CLASS_BRIDGE = 3,
};

enum FF_COMM_TYPE
{
	COMM_UNDEFINED = 0,
	COMM_RS232,
	COMM_CNCS,
};

enum GET_DEV_RANGE
{
	GDR_GET_CACHE,
	GDR_REFRESH_CHANGED,
	GDR_REFRESH_ALL,
};

enum GET_DEV_OPTION
{
	GDO_IDENTIFY_ONLY,
//	GDO_INIT_DEV_CONN,
	GDO_GET_DEV_INF,
};

#if 0
enum GET_DEV_LIST_OPTION
{
	GDLO_CACHE_ONLY = 0,	// 仅返回缓存中的设备列表
	GDLO_CHECK_LIVE_LIST,	// 检查网段的Live List，地址无变化则不更新，有变化则重新刷新（GDLO_REFRESH_ALL）
	GDLO_REFRESH_ALL,		// 清除缓存，重新刷新
};
#endif

enum GET_BLOCK_LIST_OPTION
{
	GBLO_CACHE_ONLY = 0,	// 仅返回缓存中的功能块列表
	GBLO_REFRESH_ALL,		// 清除缓存，重新刷新
};

/*
enum CNCS_REDUNDANT_MODE
{
	REDUNDANT_SINGLE = 0,
	MODE_REDUNDANT_CONTROLLER,
	MODE_REDUNDANT_NETWORK,
	MODE_REDUNDANT_BOTH,
};
*/

// CNCS背板类型
enum CNCS_BTYPE
{
	BACK_BOARD_DEC = 0,
	BACK_BOARD_HEX = 1,
};

// FFCOMM_API void ff_comm_init(
// 	FF_COMM_TYPE type
// 	);

// FFCOMM_API void ff_comm_uninit();

//
// net1,net2: 网段号
// redundant_enabled: 表示是否控制器冗余。默认始终有网络冗余。
//
FF_MODULE_HANDLE ff_open_h1module_cncs(
	uint8 dpu_addr,
	uint16 module_index,
	uint8 net1,
	uint8 net2,
	CNCS_BTYPE bb_type,
	uint16 udp_port,
	bool redundant_enabled
	);

FF_MODULE_HANDLE ff_open_h1module_rs232(
	int port_index,
	uint32 baud,
	uint8 data_bits,
	uint8 parity,
	uint8 stop_bits
	);

void ff_close_handle(
	FF_MODULE_HANDLE handle
	);

FF_RESULT_CODE ff_read_object(
	FF_MODULE_HANDLE ff_module_handle, // NULL means default unique module
	FF_LINK_INDEX link_index,
	const char* device_id,
	VFD_TYPE vfd_type,
	uint16 index,
	uint16 sub_index,
	char* data_buf_ptr, // out
	int* data_len // in, out
	);

FF_RESULT_CODE ff_write_object(
	FF_MODULE_HANDLE ff_module_handle,
	FF_LINK_INDEX link_index,
	const char* device_id,
	VFD_TYPE vfd_type,
	uint16 index,
	uint16 sub_index,
	char* data_buf_ptr, // in
	int data_len // in
	);

FF_RESULT_CODE ff_clear_dev_addr(
	FF_MODULE_HANDLE ff_module_handle,
	FF_LINK_INDEX link_index,
	const char* device_id,
	const char* device_pdtag,
	uint8 addr
	);

FF_RESULT_CODE ff_set_dev_addr(
	FF_MODULE_HANDLE ff_module_handle,
	FF_LINK_INDEX link_index,
	const char* device_pdtag,
	uint8 addr
	);

FF_RESULT_CODE ff_set_dev_pdtag(
	FF_MODULE_HANDLE ff_module_handle,
	FF_LINK_INDEX link_index,
	const char* device_id,
	const char* device_pdtag,
	uint8 addr,
	bool is_clear
	);

FF_RESULT_CODE ff_download_to_domain(
	FF_MODULE_HANDLE ff_module_handle,
	FF_LINK_INDEX link_index,
	const char* device_id,
	uint16 domain_index,
	const char* data,
	uint16 data_len
	);

#if 0
// 
// PARAM:
//   dev_array: [out] caller allocated array.
//   count: [in/out] in: array buffer length. out: device count
//
void* FF_RESULT_CODE ff_get_device_list(
	FF_MODULE_HANDLE ff_module_handle,
	FF_LINK_INDEX link_index,
	H1_DEV_INFO* dev_array,
	int* count,
	GET_DEV_LIST_OPTION opt
	);
#endif

FF_RESULT_CODE ff_get_device_list_2(
	FF_MODULE_HANDLE ff_module_handle,
	FF_LINK_INDEX link_index,
	H1_DEV_INFO* dev_array,
	int* count,
	GET_DEV_RANGE range,
	GET_DEV_OPTION opt
	);

FF_RESULT_CODE ff_init_device(
	FF_MODULE_HANDLE ff_module_handle,
	FF_LINK_INDEX link_index,
	uint8 dev_addr,
	uint32 option);

FF_RESULT_CODE ff_get_block_list(
	FF_MODULE_HANDLE module_handle,
	FF_LINK_INDEX link_index,
	const char* device_id,
	H1_BLOCK_INFO* block_array,
	int* count,
	GET_BLOCK_LIST_OPTION opt
	);

//////////////////////////////////////////////////////////////////////////
// task cancel utilities
typedef bool (*QUERY_CANCEL_FLAG)();
void ff_set_query_cancel_func(QUERY_CANCEL_FLAG pfn);
bool ff_query_cancel_flag();

//////////////////////////////////////////////////////////////////////////
// log utilities
enum LOG_OPTION
{
	LOG_NORMAL = 0,
	LOG_NO_TIMESTAMP = 0x0001,
	LOG_NO_NEW_LINE = 0x0002,
};

typedef void (*FF_LOG_INFO)(const char* info);
void ff_set_log_func(FF_LOG_INFO pfn);
void ff_log_info(LOG_OPTION opt, const char* fmt, ...);

#define FF_LOG(fmt, ...)			ff_log_info(LOG_NORMAL, fmt, #__VA_ARGS__)
#define FF_LOG_NT(fmt, ...)			ff_log_info(LOG_NO_TIMESTAMP, fmt, ##__VA_ARGS__)
#define FF_LOG_INLINE(fmt, ...)		ff_log_info(LOG_NO_NEW_LINE, fmt, ##__VA_ARGS__)
#define FF_LOG_NT_INLINE(fmt, ...)	ff_log_info((LOG_NO_TIMESTAMP | LOG_NO_NEW_LINE), fmt, ##__VA_ARGS__)


namespace comm_util
{
	class DataFormatter
	{
	public:
		static void reverse_bytes(void* buf, int len)
		{
            unsigned char* szBuf = (unsigned char*)buf;
		//	assert(szBuf != NULL);
			for (int i = 0; i < len / 2; i++) {
                unsigned char b = szBuf[i];
				szBuf[i] = szBuf[len - i - 1];
				szBuf[len - i - 1] = b;
			}
		};
	};

	class SyncLock
	{
	public:
        SyncLock(pthread_mutex_t* cs) : m_cs(cs)
		{
            pthread_mutex_lock(m_cs);
        }

		~SyncLock()
		{
            pthread_mutex_unlock(m_cs);
		}

	private:
        pthread_mutex_t* m_cs;
	};

}

#endif // __FF_COMM_H__
