#pragma once

#include <string>
#include <map>
#include <sys/socket.h>
#include <semaphore.h>
#include <pthread.h>
#include <assert.h>
#include <arpa/inet.h>
#include <iostream>
#include <semaphore.h>
using namespace std;

#include "FFDef.h"
#include "CommUtil.h"
#include "FFComm.h"
using namespace comm_util;

#pragma pack(push, 1)

enum SRC_CODE
{
	SRC_FF_CFG = 1,
	SRC_CNCS_CFG = 2,
	SRC_AMS = 3,
	SRC_CTRL = 4,
};

enum PKT_TYPE
{
	PKT_NON_TRANSPARENT = 0,
	PKT_TRANSPARENT = 1,
};

/* OLD DEF
enum PKT_COMMAND
{
	// transparent packet
	PKT_CMD_FF_REQ = 0x01,
	PKT_CMD_CTRL_ACK = 0x81,
	PKT_CMD_CTRL_RSP = 0x82,
	PKT_CMD_FF_ACK = 0x02,
	// non-transparent packet
	PKT_CMD_FF_DOWNLOAD_REQ = 0x03,
	PKT_CMD_CTRL_DOWNLOAD_RSP = 0x83,
	PKT_CMD_FF_UPLOAD_REQ = 0x04,
	PKT_CMD_CTRL_UPLOAD_RSP = 0x84,
};
*/

enum PKT_COMMAND
{
	// transparent packet
	PKT_CMD_FF_REQ_NON_CYCLE = 0x01,
	PKT_CMD_FF_REQ_CYCLE = 0x03,
	PKT_CMD_CTRL_ACK_NON_CYCLE = 0x81,
	PKT_CMD_CTRL_ACK_CYCLE = 0x83,
	PKT_CMD_CTRL_RSP = 0x82,
	PKT_CMD_FF_ACK = 0x02,
	// non-transparent packet
	PKT_CMD_FF_DOWNLOAD_REQ = 0x03,
	PKT_CMD_CTRL_DOWNLOAD_RSP = 0x83,
	PKT_CMD_FF_UPLOAD_REQ = 0x04,
	PKT_CMD_CTRL_UPLOAD_RSP = 0x84,
};

enum CNCS_TRANS_MODE
{
	MODE_CYCLE,
	MODE_NON_CYCLE,
};

/*
enum PKT_COMMAND_TRANSPARENT
{
	// transparent packet
	PKT_CMD_REQ_DOWNLOAD = 0x01,
	PKT_CMD_REQ_RT_MON = 0x03,
	PKT_CMD_ACK_DOWNLOAD = 0x81,
	PKT_CMD_ACK_RT_MON = 0x83,
	PKT_CMD_RSP_CTRL_DATA = 0x82,
	PKT_CMD_ACK_CTRL_DATA = 0x02,
};

enum PKT_COMMAND_NON_TRANSPARENT
{
	// non-transparent packet
	PKT_CMD_FF_DOWNLOAD_REQ = 0x03,
	PKT_CMD_CTRL_DOWNLOAD_RSP = 0x83,
	PKT_CMD_FF_UPLOAD_REQ = 0x04,
	PKT_CMD_CTRL_UPLOAD_RSP = 0x84,
};
*/

// for PKT_CMD_CTRL_ACK
#define CTRL_RSP_SUCCESS  0
#define CTRL_RSP_ERROR    1
enum CTRL_ERR_CODE
{
	CTRL_ERR_OK = 0,
	CTRL_ERR_CHECKSUM,
	CTRL_ERR_CTRL_ADDR,
	CTRL_ERR_PACKET_LEN,
	CTRL_ERR_BUS,
	CTRL_ERR_ORDINAL_DISCONTINUOUS,
	CTRL_ERR_PACKET_TYPE,
};

#define INVALID_COMMAND		0

#define INVALID_DPU_ADDRESS 0

#define SLAVE_ADDR_CONTROLLER  0

#define MAX_DATA_LEN_NON_TRANSPARENT	1400
#define MAX_DATA_LEN_TRANSPARENT		230
#define MAX_COMM_DATA_BUFFER			1400

#define ORDINAL_SINGLE_PACKET		0
#define ORDINAL_MULTI_PACKET_END	0xffff

// 16 bytes
typedef struct __cncs_header
{
	uint8 prot_rev; // 1.1.1 format (bit-67, bit-345, bit-12)
	uint8 src_code; // enum SRC_CODE
	uint8 pkt_type; // PKT_TYPE: transparent or not
	uint8 cmd;		// ???
	uint8 dpu_addr; // controller address, 1-255
	uint8 trans_id;
	uint16 slave_addr; // for controller, set to 0
	uint16 data_len;
	uint16 ordinal; // for multi-packet message
	uint16 checksum; // crc
	uint8 prot_slot_index;
	uint8 prot_custom_index;
} CNCS_HEADER;

#pragma pack(pop)

#define INVALID_DPU_ADDR  0

enum TransactionState
{
	TRANS_IDLE,
	TRANS_WAIT_CONTROLLER_RSP,
	TRANS_WAIT_MODULE_RSP,
};

class TransactionInfo
{
public:
	TransactionInfo();
	~TransactionInfo();
//	bool is_used() { return m_used; }
//	void set_used(bool b = true) { m_used = b; }
//	CRITICAL_SECTION* get_critical_section() { return &m_cs; }
    sem_t* get_sem() { return &m_sem; }
//	char* get_buffer() { return m_recv_buf; }
//	void set_data_len(uint16 len) { m_data_len = len; }
//	uint16 get_data_len() { return m_data_len; }

	void init();
	void reset();

	void fill_data(char* data_ptr, int data_len);
	void get_data(char* data_ptr, int& data_len, TransactionState new_state);

	TransactionState get_state() { return m_state; }
	void set_state(TransactionState st) { m_state = st; }

    int16 get_comm_ref() { return m_comm_ref; }
    void set_comm_ref(int16 v) { m_comm_ref = v; }

protected:
        void lock() { ::pthread_mutex_lock(&m_cs); }
        void unlock() { ::pthread_mutex_unlock(&m_cs); }

private:
	bool m_used;
    sem_t m_sem;
	char m_recv_buf[MAX_DATA_LEN_NON_TRANSPARENT];
	uint16 m_data_len;
	TransactionState m_state;
    pthread_mutex_t m_cs;
    int16 m_comm_ref;
};

class TransactionGuard
{
public:
	TransactionGuard(TransactionInfo* tr) : m_tr(tr)
	{
		m_tr->init();
	}

	~TransactionGuard()
	{
		m_tr->reset();
	}

private:
	TransactionInfo* m_tr;
};

typedef void (*MODULE_NOTIFY_HANDLER)(
	CNCS_HEADER* cncs_header,
	const char* data,
	int data_len,
	uint8& err_sign,
	CTRL_ERR_CODE& err_code
	);

class CNCSCommClient
{
public:
	CNCSCommClient(void);
	~CNCSCommClient(void);

	int initialize(
		uint8 dpu_addr,
		uint8 net1,
		uint8 net2,
		CNCS_BTYPE bb_type,
		uint16 udp_port,
		bool redundant_enabled);

	int request_ff_h1(
		uint16 slot_index,
		const char* req_data,
		int req_data_len,
		char* rsp_data,
		int& rsp_data_len,
		uint32 timeout,
		uint8 h1_invoke_id,
        bool need_response,
            int16 comm_ref);

	// 非透传请求，并等待回应
	int request_cncs_store_cfg(
		uint8 slot_index,
		char* cfg_data,
		uint32 data_len
		);

	int request_cncs_load_cfg(
		uint8 slot_index,
		char* cfg_data_buffer,
		uint32 data_len
		);

	void set_prot_rev(
		uint8 high_rev,
		uint8 mid_rev,
		uint8 low_rev);
	uint8 get_prot_rev();

	CNCS_TRANS_MODE get_trans_mode()
	{
		return m_cncs_trans_mode;
	}

	void set_trans_mode(CNCS_TRANS_MODE mode)
	{ 
		m_cncs_trans_mode = mode;
	}

	uint8 get_free_trans_id();

	void set_module_notify_handler(MODULE_NOTIFY_HANDLER pfn)
	{
		m_pfn_module_notify_handler = pfn;
	}

	void on_module_notify(
		CNCS_HEADER* cncs_header,
		const char* data,
		int data_len);

	int send_ack(
		CNCS_HEADER* cncs_header,
		uint8 err_sign,
		CTRL_ERR_CODE err_code);

private:
	void do_recv();
    static void* thread_recv_from_controller(void* p);

	uint8 m_dpu_addr;
	CNCS_TRANS_MODE m_cncs_trans_mode;
	
	// IP address: 0/1: network 1/2
	string m_local_ip[2];
        unsigned long m_ctrl_ip_master[2];
        unsigned long  m_ctrl_ip_slave[2];
        struct sockaddr_in m_target_addr_master;
        struct sockaddr_in m_target_addr_slave;

        int m_socket;

//	CNCS_REDUNDANT_MODE m_redundant_mode;
	bool m_redundant_enabled;

	bool m_master_slave_enabled;
	bool m_using_master;

	uint16 m_udp_port;
//	uint8 m_slot_index;

	uint8 m_protocol_revision;

	uint8 m_current_trans_id;
        pthread_mutex_t m_cs_trans_id;

	map<uint8, TransactionInfo*> m_trans_map;

	uintptr_t m_recv_thread_handle;
    //    void* m_exit_ev_handle;

	MODULE_NOTIFY_HANDLER m_pfn_module_notify_handler;

    static uint32 TIMEOUT_CTRL_CHECKSUM;
    static uint32 TIMEOUT_CTRL_RESPONSE;
    timespec timmeout_ctrl_checksum;
    timespec timmeout_ctrl_response;
//	static uint32 TIMEOUT_MODULE_RESPONSE;

	static bool USE_H1_INVOKE_ID_AS_IDENTIFIER;
};
