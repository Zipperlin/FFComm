#pragma once

#include <vector>
#include <map>
#include <string.h>
using namespace std;

#include "CNCSCommClient.h"
#include "MIB.H"
#include "FFComm.h"
#include "CIniManager.h"

//////////////////////////////////////////////////////////////////////////
// 
#pragma pack(push, 1)

#define POSITIVE	1
#define NEGITIVE	-1

#define H1_INTERFACE_MAX_VCR_COUNT	128

#define MAX_ACCESS_NAME_LENGTH	36

#define INTF_LAYER_ID	0 // interface card generated
#define FMS_LAYER_ID	1
#define SM_LAYER_ID		3

// Custom service code for interface device access.
#define INTFDEV_READ					100
#define INTFDEV_WRITE					101
#define INTFDEV_DOWNLOAD_SEG			102
#define INTFDEV_LIVELIST_CHANGE_NOTIFY	103

// FMS service code.
#define FMS_READ				2
#define FMS_WRITE				3
#define FMS_GETOD				4
#define FMS_GENERIC_INITIATE_DOWNLOAD_SEQUENCE	31
#define FMS_GENERIC_DOWNLOAD_SEGMENT			32
#define FMS_GENERIC_TERMINATE_DOWNLOAD_SEQUENCE	33
#define FMS_INITIATE			40
#define FMS_ABORT				41
#define FMS_REJECT              42

// FMS Access Types
#define FMS_ACCESS_INDEX		0x0000
#define FMS_ACCESS_NAME			0x0001
#define FMS_ACCESS_NAME_LIST	0x0002

// OD access macro variable
#define FMS_INDEX_ACCESS			1
#define FMS_VAR_NAME_ACCESS			2
#define FMS_VAR_LIST_NAME_ACCESS	3
#define FMS_DOMAIN_NAME_ACCESS		4
#define FMS_PI_NAME_ACCESS			5
#define FMS_EVENT_NAME_ACCESS		6
#define FMS_START_INDEX_ACCESS		7

// SM service code.
#define SM_SVC_SET_PD_TAG		1
#define SM_SVC_SET_ADDRESS		2
#define SM_SVC_CLEAR_ADDRESS	3
#define SM_SVC_IDENTIFY			4

// Primitive Types
#define REQUEST_PRIMITIVE		0
#define RESPONSE_PRIMITIVE		1
#define INDICATION_PRIMITIVE	2
#define CONFIRM_PRIMITIVE		3

// FMS timeout
#define H1IO_TIMEOUT_FMS_INITIAL		3000
#define H1IO_TIMEOUT_FMS_READ			3000
#define H1IO_TIMEOUT_FMS_GETOD			3000
#define H1IO_TIMEOUT_FMS_WRITE			5000
#define H1IO_TIMEOUT_FMS_GEN_INIT_DL	3000
#define H1IO_TIMEOUT_FMS_GEN_DL_SEG		3000
#define H1IO_TIMEOUT_FMS_GEN_TERM_DL	3000
#define H1IO_TIMEOUT_FMS_ABORT			0		// needn't wait
// interface timeout
#define H1IO_TIMEOUT_INTF_READ			1000
#define H1IO_TIMEOUT_INTF_WRITE			1000
#define H1IO_TIMEOUT_INTF_DOWNLOAD_SEG	1000
// SM timeout
#define H1IO_TIMEOUT_SM_SET_PDTAG		4000
#define H1IO_TIMEOUT_SM_CLEAR_PDTAG		1000
#define H1IO_TIMEOUT_SM_SET_ADDRESS		60000
#define H1IO_TIMEOUT_SM_CLEAR_ADDRESS	5000
#define H1IO_TIMEOUT_SM_IDENTIFY		3000

#define NCS2_H1LINK_VCR_MAX_NUM		64

typedef struct _T_FIELDBUS_SERVICE_HEADER
{
	INT16	comm_ref;	// VCR ID
	int8	layer;
	int8	service;	//
	int8	primitive;	// Request(1) or Indication(2)
	unsigned int	invoke_id;  // ID helping to async transaction
	INT16	result;
} T_FIELDBUS_SERVICE_HEADER;

typedef struct _T_ACC_SPEC
{
	unsigned int	tag;
	unsigned int	dummy;
	unsigned short	index;
// 	union
// 	{
// 		unsigned short	index;
// 		char	name[MAX_ACCESS_NAME_LENGTH];
// 	} id;

} T_ACC_SPEC;

typedef struct _fms_user_read_req
{
	T_ACC_SPEC acc_spec;		// access specification
	unsigned int subindex;
	unsigned int dummy;
} FMS_USER_READ_REQ;

typedef struct _fms_user_read_cnf
{
	unsigned int dummy;
	unsigned int length;	// length of data field in octal??
	unsigned int value[MAX_DATA_FIELD_LEN];	// data field
} FMS_USER_READ_CNF;

// GetOD
typedef struct _fms_user_getod_req
{
	bool format;	// FB_TRUE = long, FB_FLASE = short format
	unsigned char dummy[3];
	T_ACC_SPEC acc_spec;	// access specification
} FMS_USER_GETOD_REQ;

typedef struct _fms_user_getod_cnf
{
	bool more_follows;	// further object descriptions follow
	unsigned char no_of_od_descr;	// number of object description
	char ll;
	char length;
	char length1;
	unsigned char packed_object_descr[MAX_DATA_FIELD_LEN];
	// packed array of object description
	// if the user sets result=POS, the application has
	// to set the no_of_od_descr to 0. the data block
	// is filled in by the communication software.
} FMS_USER_GETOD_CNF;

typedef struct _fms_user_write_req
{
	T_ACC_SPEC acc_spec;		// access specification
	unsigned int subindex;
	unsigned int length;				// length of data field in octects
	unsigned int value[MAX_DATA_FIELD_LEN];	//data field
} FMS_USER_WRITE_REQ;

// Download Domain
typedef struct _fms_user_gen_init_download_req
{
	T_ACC_SPEC acc_spec;
} FMS_USER_GEN_INIT_DOWNLOAD_REQ;

typedef struct _fms_user_gen_download_seg_req
{
	T_ACC_SPEC acc_spec;
    unsigned char more_follows;
	unsigned char data_len;
	unsigned char data[MAX_DATA_FIELD_LEN];
} FMS_USER_GEN_DOWNLOAD_SEG_REQ;

typedef struct _fms_user_gen_term_download_req
{
	T_ACC_SPEC acc_spec;
} FMS_USER_GEN_TERM_DOWNLOAD_REQ;


// Initial
typedef struct _fms_initial_req
{
	unsigned long	ulDLAddress; 		// remote DLCEP for open connections
	short	od_version;			// od_version
	unsigned char	profile_number[2];	// profile number
	bool	protection;			// access protection
	unsigned char	password;			// password
	unsigned char	access_groups; 		// access groups
	bool	indicate_buffer_sent;
	//publisher dlcep only; indicate when buffer was sent
	unsigned char	max_fms_pdu_sending_calling;
	unsigned char	max_fms_pdu_receiving_calling;
	unsigned char	fms_features_supported_calling[8];
	unsigned char	fms_max_scc_calling;
	unsigned char	fms_max_rcc_calling;
} FMS_INITIAL_REQ;

typedef struct _sm_set_address_req
{
	unsigned char	ucNodeAddr;
	unsigned char	ucClockInterval; // synchronization interval for application time
	unsigned char	ucPrimPublisher; // node address of primary time publisher
	char	szPdTag[FFH1_PDTAG_LEN]; // physical device tag of device
} SM_SET_ADDRESS_REQ;

typedef struct _sm_set_pd_tag_req
{
	unsigned char	ucNodeAddr;
	unsigned char	ucClear;	// point out the service other set or clear the address
	// TRUE: clear the PD tag of remote device and set its
	// SM state to UNINITIALIZED
	// FALSE:set the given PD tag for the remote device
	char	szPdTag[FFH1_PDTAG_LEN];
	char	szDevID[FFH1_DEVID_LEN];
} SM_SET_PD_TAG_REQ;

// Abort
#define FMS_ABORT_DETAIL_LENGTH		200	// define abort service's argument
typedef struct _fms_user_abort_req
{
	unsigned int local;	// local or remote generated( FB_TRUE = local )
	unsigned int abort_id;	// abort_identifier (user, fas, dll, fms)
	unsigned int reason;	// abort reason code
	unsigned int detail_length;
	char detail[FMS_ABORT_DETAIL_LENGTH];	// detail information about abort reason
} FMS_USER_ABORT_REQ;

//////////////////////////////////////////////////////////////////////////
// FF H1 SM Service Header
typedef struct _sm_identify_req
{
	unsigned char	ucNodeAddr;
	unsigned char	ucDummy;
} SM_IDENTIFY_REQ;

typedef struct _sm_identify_cnf
{
	char szPdTag[FFH1_PDTAG_LEN];
	char szDevID[FFH1_DEVID_LEN];
} SM_IDENTIFY_CNF;

typedef struct _sm_clear_address_req
{
	unsigned char	ucNodeAddr;
	char	szPdTag[FFH1_PDTAG_LEN];
	char	szDevID[FFH1_DEVID_LEN];
} SM_CLEAR_ADDRESS_REQ;

typedef struct _t_od_obj_descr_hdr
{
    unsigned short	index;
	bool	flag;
	unsigned char	length;
	bool	protection;
	short	version;
	short	l_st_od;
    unsigned short	first_index_s_od;
	short	l_s_od;
    unsigned short	f_i_dv_od;
	short	l_dv_od;
    unsigned short	f_i_dp_od;
	short	l_dp_od;
	unsigned long	int_addr;
	unsigned long	int_addr_st_od;
	unsigned long	int_addr_s_od;
	unsigned long	int_addr_dv_od;
	unsigned long	int_addr_dp_od;
} T_OD_OBJ_DESCR_HDR;

typedef struct _ff_block_info
{
	char	fbTag[BLOCK_TAG_LEN];
    uint32_t	ddName;
    uint32_t	ddItem;
	UINT16	ddRev;
	UINT16	profile;
	UINT16	profileRev;
    uint32_t	executionTime;
    uint32_t	periodExecution;
	UINT16	numParams;
	UINT16	nextFb;
	UINT16	startViewIndex;
	unsigned int	numView3;
	unsigned int	numView4;
} FF_BLOCK_INFO;

// NEW HEADER
#define FF_PACKET_HEAD_SIGNATURE	0xff
#define H1_DT_HOST_REQ  1
typedef struct __ff_packet_header
{
	uint8	head_signature; // FF_PACKET_HEAD_SIGNATURE
	uint8	data_len;		// include header
	uint8	link_index;		// 0 or 1
	uint8	packet_type;	// H1_DT_HOST_REQ(for HOST)
	int16	comm_ref;		// VCR ID
	int8	layer;
	int8	service;
	int8	primitive;
	uint8	invoke_id;
	uint16  result;
} FF_HEADER;

#pragma pack(pop)

//////////////////////////////////////////////////////////////////////////
/*
typedef struct __sync_req_info
{
	HANDLE event_handle;
	char* rsp_data_ptr;
	int rsp_data_len;
} SYNC_REQ_INFO;
*/

class FF_H1Module;
class FF_H1Link;

class FF_Block
{
public:
	FF_Block(uint16 start_index, FFBLOCK_TYPE type);
	void set_info(FF_BLOCK_INFO* info);
	FFBLOCK_TYPE get_type() { return m_block_type; }
	uint16 get_start_index() { return m_start_index; }
	const FF_BLOCK_INFO& get_info() { return m_info; }
	void copy_info(H1_BLOCK_INFO& info);

private:
	uint16 m_start_index;
	FFBLOCK_TYPE m_block_type;
	FF_BLOCK_INFO m_info;
};

class FF_Device
{
public:
	FF_Device(FF_H1Link* parent_link, const char* dev_id);
	~FF_Device();

	uint32 get_vendor_id() { return m_vendor_id; }
	uint16 get_device_type() { return m_device_type; }
	uint8 get_device_version() { return m_device_version; }
	uint8 get_dd_version() { return m_dd_version; }

	uint16 get_mib_vcr_index() { return m_vcr_index_mib; }
	uint16 get_vfd_vcr_index() { return m_vcr_index_vfd; }
//	void set_mib_vcr_index(uint16 index) { m_vcr_index_mib = index; }
//	void set_vfd_vcr_index(uint16 index) { m_vcr_index_vfd = index; }

	bool is_connected();

	FF_RESULT_CODE init_vcr(VFD_TYPE vt, uint16& vcr_index);

	void set_addr(uint8 addr) { m_address = addr; }
	uint8 get_addr() { return m_address; }

	FF_RESULT_CODE config_vcr(uint16 vcr_index, bool is_vfd, bool resp = false); // TODO: RESP?
	void reset_vcr();
	bool reset_vcr(uint16 vcr_index);
	void abort_vcr(uint16 vcr_index);
	void release_vcr_connection(VFD_TYPE vt);

	FF_H1Link* get_link() { return m_parent_link; }
	FF_H1Module* get_parent_module();

	FF_RESULT_CODE init_write_vcr();
	FF_RESULT_CODE init_static_block();
	FF_RESULT_CODE refresh_func_block_info();
	FF_RESULT_CODE get_block_list(H1_BLOCK_INFO* block_list, int* count);
	void release_func_block_array();

	FF_RESULT_CODE get_static_info();

	const char* get_device_id() { return m_device_id; }
	const char* get_pdtag() { return m_pdtag; }
    void set_pdtag(const char* pdtag) { memcpy(m_pdtag, pdtag, FFH1_PDTAG_LEN); }
	void set_device_id(const char* devid) { memcpy(m_device_id, devid, FFH1_DEVID_LEN); }
	FF_DEV_CLASS get_device_class() { return m_dev_class; }
	uint16* read_vfd_directory();

	FF_RESULT_CODE fms_init(uint16 vcr_index, VFD_TYPE vt);
	FF_RESULT_CODE fms_read(VFD_TYPE vfd_type, uint16 index, uint8 sub_index, FMS_USER_READ_CNF& cnfRead);
	FF_RESULT_CODE fms_write(VFD_TYPE vfd_type, uint16 index, uint8 sub_index, char* data, uint8 data_len);
	FF_RESULT_CODE fms_get_od(uint16 vcr_index, uint16 index, FMS_USER_GETOD_CNF& cnfGetOD);
	FF_RESULT_CODE fms_download_init_sequence(uint16 domain_index);
	FF_RESULT_CODE fms_download_terminate_sequence(uint16 domain_index);
	FF_RESULT_CODE fms_download_segment(uint16 domain_index, uint8 more_follow, char* data, uint8 data_len);

protected:
	FF_RESULT_CODE init_mib();
	FF_RESULT_CODE init_vfd();

	int close_mib();
	int close_vfd();

	FF_RESULT_CODE get_dd_params();

private:
	uint16 m_vcr_index_mib;
	uint16 m_vcr_index_vfd;
	VCR_STATIC_ENTRY m_vcr_mib;
	VCR_STATIC_ENTRY m_vcr_vfd;

	uint16 m_mib_max_dlsdu_size;
	uint16 m_vfd_max_dlsdu_size;

	char m_device_id[FFH1_DEVID_LEN];
	char m_pdtag[FFH1_PDTAG_LEN];

	uint8 m_address;
	uint8 m_vfd_address;
	FF_DEV_CLASS m_dev_class;

	uint32 m_vendor_id;
	uint16 m_device_type;
	uint8 m_device_version;
	uint8 m_dd_version;

	uint16 m_vcr_static_entry_index;
	uint32 m_vfd_ref;

	T_OD_OBJ_DESCR_HDR m_od_vfd;

	FF_H1Link* m_parent_link;

	FF_Block* m_res_block;
	vector<FF_Block*> m_trans_block_array;
	vector<FF_Block*> m_func_block_array;

};

class FF_H1Link
{
public:
	FF_H1Link(int nIndex, FF_H1Module* parent_module);
	virtual ~FF_H1Link();

	int get_index() { return m_nIndex; }
	int vcr_start_idx() { return m_nVCRStartIdx; } 
	FF_Device* find_dev(const char* dev_id);
	FF_Device* find_dev_by_pdtag(const char* pdtag);
	FF_Device* find_dev(uint8 dev_addr);

	FF_H1Module* get_parent_module() { return m_parent_module; }

	/*
	FF_RESULT_CODE get_dev_list(
		vector<H1_DEV_INFO>& dev_array,
		GET_DEV_LIST_OPTION opt);
	*/

	FF_RESULT_CODE get_dev_list(
		vector<H1_DEV_INFO>& dev_array,
		GET_DEV_RANGE range,
		GET_DEV_OPTION opt);

	FF_RESULT_CODE init_dev(
		uint8 dev_addr,
		uint32 option
		);

	uint16 get_user_vcr();
	void release_user_vcr(uint16 vcr_index);
	void release_local_vcr(uint16 vcr_index);

	//////////////////////////////////////////////////////////////////////////
	// Device access function
	FF_RESULT_CODE IntfDev_Read(
		unsigned short nIndex,
		unsigned char ucSubIndex,
		FMS_USER_READ_CNF& readCnf);

	FF_RESULT_CODE IntfDev_Write(
		unsigned short nIndex,
		unsigned char ucSubIndex,
		char * pszWriteBuf,
		unsigned char ucWriteLen);

	FF_RESULT_CODE IntfDev_DownloadSegment(
		unsigned short nDomainIndex,
		unsigned char ucMoreFollows,
		char* pszData,
		unsigned char ucDataLen);

	FF_RESULT_CODE SM_SetPdTag(
		unsigned char ucAddr,
		const char* szDevID,
		const char* szPdTag,
        bool bClear);

	FF_RESULT_CODE SM_SetAddress(
		unsigned char ucDevAddr,
		const char* szPdTag);

	FF_RESULT_CODE SM_ClearAddress(
		unsigned char ucDevAddr,
		const char* szDevID,
		const char* szPdTag);

	FF_RESULT_CODE SM_Identify(
		unsigned char ucDevAddr,
		char * szDevID,
		char * szPdTag);

    long FMS_Abort(int16 nVCRIndex);

	//////////////////////////////////////////////////////////////////////////
	// ported functions

	FF_RESULT_CODE init_link(bool reset_info = false);

	pthread_mutex_t* SMAccess()     { return &m_csSMAccess; }
    bool Refreshed() const           { return m_bRefreshed; }

private:
	FF_H1Module* m_parent_module;
	vector<FF_Device*> m_dev_array;
	map<uint16, bool> m_vcr_state_map;
	int	m_nIndex;	// Link index, 0 or 1

	int8* m_prev_live_list[32];

	int m_nVCRStartIdx; 
	// Critical section for VCR list manipulation.
	pthread_mutex_t	m_csVCRList;

	unsigned long	m_ulRefreshDevThread;

	unsigned char	m_ucInvokeId;
	// Critical section for invoke ID manipulation.
	pthread_mutex_t	m_csInvokeID;
	pthread_mutex_t	m_csAccess;
	// Critical section for SM access on the H1 link.
	// include device list refreshing.
	pthread_mutex_t	m_csSMAccess;

	// A flag to represent live list changed info.
	// 0   -- no change since last refresh;
	// > 0 -- one or more 'Live List Changed' message was received.
	// Must be reset to Zero as soon as the refreshing task is started.
	long	m_lLiveListFlag;

	// Store the invoke ID used by SM Set Address request.
	// Controller can't return the same invoke ID in the confirm packet, so
	// we must store it to find the pending request notify event.
	unsigned char	m_ucIvkID_SetAddr;

	void* m_hLiveListCheck;
	void* m_hLiveListChanged;

	// 2010-08-07
//	BOOL   m_bSendMsgToClient;
//	BOOL   m_bCallFromClient;
	void* m_hEventUpdating;
	void* m_hEventUpdateEnd;
	// 2012-09-11
    bool  m_bRefreshed;
	char  m_szLogID[16];
    timeval m_stSendReq; // diagnose info.
	unsigned char m_ucIvkIDSendReq; // diagnose info.

	uint16 m_live_list_index;

protected:
    static unsigned int thread_RefreshDevList(void* p);
//	BOOL _AddrInLiveList(unsigned char ucAddr, BYTE* pLiveList);

	void copy_cache_data(vector<H1_DEV_INFO>& dev_array);
};

class FF_H1Module // : public FFCommObject
{
public:
	FF_H1Module();
	~FF_H1Module();

	FF_H1Link* get_link(FF_LINK_INDEX id);

    void set_cncs_client(CNCSCommClient* cli) { m_cncs_client = cli; }

	FF_COMM_TYPE get_comm_type() { return m_comm_type; }
	void set_comm_type(FF_COMM_TYPE type) { m_comm_type = type; }

	FF_RESULT_CODE get_device_list(
		FF_LINK_INDEX link_index,
		H1_DEV_INFO* dev_array,
		int* count);

	uint16 get_slot_index() { return m_slot_index; }
	void set_slot_index(uint16 i) { m_slot_index = i; }

	uint8 get_new_invoke_id();

	FF_RESULT_CODE sync_request(
		unsigned char		ucLinkID,
		short		nVcrID,
		unsigned char		ucLayer,
		unsigned char		ucService,
		unsigned char		ucPrimitive,
	//	unsigned char		ucInvokeID,
        void *		pReqDataBuf,
		uint8		dwReqDataLen,
        unsigned long		dwTimeout,
        void *		pOutDataBuf,	// OUT
		unsigned char *		pucOutDataLen,	// OUT
		FF_HEADER& cnfSvcHeader, // OUT
        bool		bNeedResponse = true);

	FF_RESULT_CODE handle_notify(
		const char* data,
		int data_len);

private:
	FF_COMM_TYPE m_comm_type;
	uint16 m_slot_index;

	map<FF_LINK_INDEX, FF_H1Link*> m_map_link;

//	map<uint8, SYNC_REQ_INFO*> m_map_sync_req;
	pthread_mutex_t m_sync_req_lock;
	uint8 m_invoke_id_counter;

    CNCSCommClient* m_cncs_client;
};

class FFCommManager
{
public:
	FFCommManager(void);
	~FFCommManager(void);

	// RS232
	FF_MODULE_HANDLE open_module(
		int port_index,
		uint32 baud,
		uint8 data_bits,
		uint8 parity,
		uint8 stop_bits);

	// CNCS
	FF_MODULE_HANDLE open_module(
		uint8 dpu_addr,
		uint16 module_index,
		uint8 net1,
		uint8 net2,
		CNCS_BTYPE bb_type,
		uint16 udp_port,
		bool redundant_enabled);

	FF_H1Module* get_module(FF_MODULE_HANDLE h_module);

	static FFCommManager& inst()
	{
		if (!m_initialized) {
			init();
		}
		return m_comm_mgr;
	}
	
	static void handle_module_notify(
		CNCS_HEADER* cncs_header,
		const char* data,
		int data_len,
		uint8& err_sign,
		CTRL_ERR_CODE& err_code);

	static int fms_init_retry_count() { return m_fms_init_retry_count; }

private:
    static int GetModuleFileName( char* sModuleName, char* sFileName, int nSize);

	static bool init();
	static bool m_initialized;

//	FF_COMM_TYPE m_comm_type;
	map<FF_MODULE_HANDLE, FF_H1Module*> m_map_h1module;
    map<uint8, CNCSCommClient*> m_map_cncs_client;

	static FFCommManager m_comm_mgr;

	//////////////////////////////////////////////////////////////////////////
	// FF comm settings

	// fms_init_retry_count
	static int m_fms_init_retry_count;
    static CIniManager m_IniManager;
};

