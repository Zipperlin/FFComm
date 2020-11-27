
#include "FFComm.h"
#include "CNCSCommClient.h"
#include "FFCommManager.h"

// CRC checksum
#define POLY16_CCITT 0x1021  // 该位为简式书写 实际为0x11021
#define PROTOCOL_BUF_LENGTH		512

uint16 g_CRCTable[256];

static uint16 CalCRCTable(uint16 data, uint16 genpoly)
{
	uint16 accum = 0;
	data <<= 8;
	for (int i = 8; i > 0; i--)	{
		if ((data^accum) & 0x8000) {
			accum = (accum<< 1) ^ genpoly;
		}
		else {
			accum <<= 1;
		}
		data <<= 1;
	}
	return (accum);
}

static void InitCRCTable(void)
{
	for (uint16 i = 0; i < 256; i++) {
		g_CRCTable[i] = CalCRCTable(i, POLY16_CCITT);
	}
}

static uint16 CalSFCRC(const char *pData, uint32 len)
{
	uint16	crc = 0xffff;
	uint32	i = 0;
	uint32	temp = 0;

	for (i = 0; i<len; ++i)
	{
		temp = (*pData++ ^ (crc>> 8)) & 0xff;
		crc = g_CRCTable[temp] ^ (crc<< 8);
	}
	crc = (crc^0);

	return crc;
}

TransactionInfo::TransactionInfo()
{
	m_used = false;
	m_state = TRANS_IDLE;
    pthread_mutex_init(&m_cs,NULL);
	// auto-reset event
    int ret=sem_init(&m_sem,0,0);
    if(ret)
    {
        cout<<"init sem failed!"<<endl;
    }
}

TransactionInfo::~TransactionInfo()
{
    pthread_mutex_destroy(&m_cs);
}

void TransactionInfo::init()
{
	lock();
	m_state = TRANS_WAIT_CONTROLLER_RSP;
    //ResetEvent(m_event);
    int ret=sem_init(&m_sem,0,0);
    if(ret){
        cout<<"init sem failed!"<<endl;
    }
	unlock();
}

void TransactionInfo::reset()
{
	lock();
	m_state = TRANS_IDLE;
    //ResetEvent(m_event);
    int ret=sem_init(&m_sem,0,0);
    if(ret){
        cout<<"init sem failed!"<<endl;
    }
	unlock();
}

void TransactionInfo::fill_data(char* data_ptr, int data_len)
{
	lock();
	if (m_state == TRANS_IDLE) {
		// 无法匹配的数据包，忽略或记录异常
		unlock();
		return;
	}
	memcpy(m_recv_buf, data_ptr, data_len);
	m_data_len = data_len;
    sem_post(&m_sem);
    //::SetEvent(m_event);
	unlock();
}

void TransactionInfo::get_data(
	char* data_ptr,
	int& data_len,
	TransactionState new_state)
{
	lock();
	memcpy(data_ptr, m_recv_buf, m_data_len);
	data_len = m_data_len;
	m_state = new_state;
	unlock();
}

uint32 CNCSCommClient::TIMEOUT_CTRL_CHECKSUM = 200;
uint32 CNCSCommClient::TIMEOUT_CTRL_RESPONSE = 1000;
bool CNCSCommClient::USE_H1_INVOKE_ID_AS_IDENTIFIER = true;

CNCSCommClient::CNCSCommClient(void)
{
    m_socket = SOCK_NONBLOCK;
	m_master_slave_enabled = false;
	m_using_master = true;
	m_protocol_revision = 0;
	m_dpu_addr = INVALID_DPU_ADDR;
	m_current_trans_id = 1; // 1 - 255
	m_recv_thread_handle = NULL;
    //m_exit_ev_handle = ::CreateEvent(NULL, TRUE, FALSE, NULL);
    set_trans_mode(MODE_NON_CYCLE);
	m_pfn_module_notify_handler = NULL;

    timmeout_ctrl_checksum.tv_nsec=TIMEOUT_CTRL_CHECKSUM*1000;
    timmeout_ctrl_response.tv_nsec=TIMEOUT_CTRL_RESPONSE*1000;

    pthread_mutex_init(&m_cs_trans_id,NULL);
}

CNCSCommClient::~CNCSCommClient(void)
{
    if (m_socket >0) {
        close(m_socket);
	}

    pthread_mutex_destroy(&m_cs_trans_id);
}

void CNCSCommClient::set_prot_rev(uint8 high_rev, uint8 mid_rev, uint8 low_rev)
{
	m_protocol_revision = low_rev;
	m_protocol_revision += (mid_rev << 3);
	m_protocol_revision += (high_rev << 6);
}

uint8 CNCSCommClient::get_prot_rev()
{
	return m_protocol_revision;
}

int CNCSCommClient::initialize(
	uint8 dpu_addr,
	uint8 net1,
	uint8 net2,
	CNCS_BTYPE bb_type,
	uint16 udp_port,
	bool redundant_enabled)
{
	// 暂时在这里设置，后续考虑其他方式
	set_prot_rev(1, 1, 1);

	InitCRCTable();

	if (dpu_addr == INVALID_DPU_ADDR) {
		return FF_ERR_INVALID_ARGUMENT;
	}

	m_dpu_addr = dpu_addr;
    string strnet1=to_string(net1);
    string strdpuaddr=to_string(dpu_addr);
    string strIP="192.168."+strnet1+"."+strdpuaddr;
    m_ctrl_ip_master[0] = inet_addr(strIP.c_str());
	m_redundant_enabled = redundant_enabled;
	m_udp_port = udp_port;

    m_socket=socket(AF_INET,SOCK_STREAM,0);
    if(m_socket==-1){
        return 0;
    }

    int m_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (m_socket <0) {
        return 0;
	}

    memset(&m_target_addr_master,0,sizeof(m_target_addr_master));

	m_target_addr_master.sin_family = AF_INET;
	m_target_addr_master.sin_port = htons(m_udp_port);
    m_target_addr_master.sin_addr.s_addr = htonl(m_ctrl_ip_master[0]);

	sockaddr_in sa_local;
	sa_local.sin_family = AF_INET;
	sa_local.sin_port = htons(0);
	sa_local.sin_addr.s_addr = htonl(INADDR_ANY); // 2013-03-19

	int nBindRet = bind(m_socket, (struct sockaddr *)&sa_local, sizeof(sa_local));
	if (nBindRet != 0)
	{
        return false;
	}

	// 启动数据接收线程
	uint32 _addr = 0;
    pthread_t mythread;
    int ret=pthread_create(&mythread,NULL,thread_recv_from_controller,&_addr);
    if(ret!=0){
        cout<<"pthread create failde!:error_code="<<ret<<endl;
    }

	return FF_SUCCESS;
}

uint8 CNCSCommClient::get_free_trans_id()
{
	// TODO: 
	// 如果有长周期等待，可能会出现id重复
	// 应考虑建立used id list，避开尚未返回的id

	SyncLock lock(&m_cs_trans_id);
	m_current_trans_id++;
	if (m_current_trans_id == 0) {
		m_current_trans_id = 1;
	}
	return m_current_trans_id;
}

// 透传请求，并等待回应
// TODO: 处理不需要回应的情况
int CNCSCommClient::request_ff_h1(
	uint16 slot_index,
	const char* req_data,
	int req_data_len,
	char* recv_data,
	int& recv_data_len,
	uint32 timeout,
	uint8 h1_invoke_id,
    bool need_response,int16 comm_ref)
{
    if (m_socket <0) {
        assert(false);
		return FF_ERR_SOCKET_ERROR;
	}

	if (req_data_len > (MAX_DATA_FIELD_LEN + 20)) { // TODO: find more proper size
		return FF_ERR_DATA_TOO_LONG;
	}

	char send_buf[MAX_DATA_LEN_TRANSPARENT];
	CNCS_HEADER* header = (CNCS_HEADER*)send_buf;
	header->prot_rev = get_prot_rev();
	header->src_code = SRC_FF_CFG;
	header->pkt_type = PKT_TRANSPARENT;
	header->cmd =
		(get_trans_mode() == MODE_CYCLE) ? PKT_CMD_FF_REQ_CYCLE : PKT_CMD_FF_REQ_NON_CYCLE;
	assert(m_dpu_addr != INVALID_DPU_ADDR);
	header->dpu_addr = m_dpu_addr;
	header->data_len = req_data_len;
	// 目前所有FF H1的数据包，长度均小于 MAX_DATA_LEN_TRANSPARENT 减去 header的长度（230 - 16）
	header->ordinal = ORDINAL_SINGLE_PACKET;
	header->slave_addr = slot_index;
	if (USE_H1_INVOKE_ID_AS_IDENTIFIER) {
		header->trans_id = h1_invoke_id;
	} else {
		header->trans_id = get_free_trans_id();
	}
	header->checksum = CalSFCRC(req_data, req_data_len);
	header->prot_custom_index = 0;
	header->prot_slot_index = 0;

//	DataFormatter::reverse_bytes(&header->slave_addr, 2);
	DataFormatter::reverse_bytes(&header->data_len, 2);
	DataFormatter::reverse_bytes(&header->ordinal, 2);
	DataFormatter::reverse_bytes(&header->checksum, 2);

	TransactionInfo* tr_inf = NULL;
	uint8 key_ivk_id = header->trans_id;
	if (m_trans_map.find(key_ivk_id) == m_trans_map.end()) {
		tr_inf = new TransactionInfo();
		m_trans_map[key_ivk_id] = tr_inf;
	} else {
		tr_inf = m_trans_map[key_ivk_id];
	}

    tr_inf->set_comm_ref(comm_ref);

	// 此对象用于TransactionInfo的自动reset
	TransactionGuard trg(tr_inf);
	
	int offset = sizeof(CNCS_HEADER);
	memcpy(send_buf + offset, req_data, req_data_len);
	int total_len = offset + req_data_len;
	int send_len = sendto(m_socket, send_buf, total_len, 0,
		(sockaddr*)&(m_using_master ? m_target_addr_master : m_target_addr_slave), 
		sizeof(sockaddr_in));
	if (send_len != total_len) {
		return FF_ERR_SOCKET_ERROR;
	}

	// 等待控制器的ACK
    //unsigned long wait_result = ::WaitForSingleObject(tr_inf->get_event(), TIMEOUT_CTRL_CHECKSUM);
    unsigned long wait_result=sem_timedwait(tr_inf->get_sem(),&timmeout_ctrl_checksum);
    if (wait_result == 0) {
		// 解析“控制器下行应答”数据包
		static int CTRL_RSP_PACKET_LEN = sizeof(CNCS_HEADER) + 2; // 头 + 错误标志 + 错误码
		char rsp_data[MAX_DATA_LEN_TRANSPARENT];
		int rsp_data_len = 0;
		tr_inf->get_data(rsp_data, rsp_data_len, TRANS_WAIT_MODULE_RSP);

		// 检查长度
		if (rsp_data_len != CTRL_RSP_PACKET_LEN) {
			return FF_ERR_CNCS_STATION_PACKET_LEN_MISMATCH;
		}

		// 检查头数据
		uint8 anticipate_pkt_cmd_ack = 
			(get_trans_mode() == MODE_CYCLE) ? PKT_CMD_CTRL_ACK_CYCLE : PKT_CMD_CTRL_ACK_NON_CYCLE;
		CNCS_HEADER* rsp_header = (CNCS_HEADER*)rsp_data;
		if ((rsp_header->pkt_type != PKT_TRANSPARENT) ||
			(rsp_header->cmd != anticipate_pkt_cmd_ack))
		{
			return FF_ERR_CNCS_STATION_PACKET_HEAD_ERROR;
		}

		// TODO: 检查CRC校验码. FF_ERR_STATION_PACKET_CHECKSUM_ERROR

		// 检查错误码
		char* data_bytes = rsp_data + sizeof(CNCS_HEADER);
		uint8 err_sign = data_bytes[0];
		if (err_sign == CTRL_RSP_ERROR) {
			uint8 err_code = data_bytes[1]; // enum CTRL_ERR_CODE
			return (FF_ERR_CNCS_STATION_ERROR_CODE_MASK | err_code);
		}
	}
    else if (wait_result == ETIMEDOUT) {
		return FF_ERR_CNCS_STATION_TIMEOUT;
	}
	else {
		assert(false);
		return FF_ERR_SYSTEM_ERROR;
	}

	if (!need_response) {
		return FF_SUCCESS;
	}

	// 等待FF模块的应答
    //wait_result = ::WaitForSingleObject(tr_inf->get_event(), timeout);
    wait_result = sem_timedwait(tr_inf->get_sem(), &timmeout_ctrl_response);
    if (wait_result == 0) {
		// 解析“控制器上行”命令
		char rsp_data[MAX_DATA_LEN_TRANSPARENT];
		int rsp_data_len = 0;
		tr_inf->get_data(rsp_data, rsp_data_len, TRANS_IDLE);

		CNCS_HEADER* rsp_header = (CNCS_HEADER*)rsp_data;
		DataFormatter::reverse_bytes(&rsp_header->data_len, 2);
		DataFormatter::reverse_bytes(&rsp_header->ordinal, 2);
		DataFormatter::reverse_bytes(&rsp_header->checksum, 2);

		// for reply packet
		uint8 err_sign = CTRL_RSP_SUCCESS;
		CTRL_ERR_CODE err_code = CTRL_ERR_OK;
		FF_RESULT_CODE ff_res = FF_SUCCESS;
		// 检查长度
		if (rsp_data_len != (rsp_header->data_len + sizeof(CNCS_HEADER))) {
			ff_res = FF_ERR_CNCS_MODULE_PACKET_LEN_MISMATCH;
			err_sign = CTRL_RSP_ERROR;
			err_code = CTRL_ERR_PACKET_LEN;
		}

		// 检查头数据
		if ((rsp_header->pkt_type != PKT_TRANSPARENT) ||
			(rsp_header->cmd != PKT_CMD_CTRL_RSP))
		{
			ff_res = FF_ERR_CNCS_MODULE_PACKET_HEAD_ERROR;
			err_sign = CTRL_RSP_ERROR;
			err_code = CTRL_ERR_PACKET_TYPE;
		}

		// TODO: 检查CRC校验码. FF_ERR_STATION_PACKET_CHECKSUM_ERROR

		char ack_data[2];
		ack_data[0] = err_sign;
		ack_data[1] = err_code;
		CNCS_HEADER* ack_header = (CNCS_HEADER*)send_buf;
		ack_header->prot_rev = get_prot_rev();
		ack_header->src_code = SRC_FF_CFG;
		ack_header->pkt_type = PKT_TRANSPARENT;
		ack_header->cmd = PKT_CMD_FF_ACK;
		ack_header->dpu_addr = m_dpu_addr;
		ack_header->data_len = 2;
		ack_header->ordinal = ORDINAL_SINGLE_PACKET;
		ack_header->slave_addr = slot_index;
		ack_header->trans_id = rsp_header->trans_id;
		ack_header->checksum = CalSFCRC(ack_data, 2);
		ack_header->prot_custom_index = 0;
		ack_header->prot_slot_index = 0;

		//	DataFormatter::reverse_bytes(&header->slave_addr, 2);
		DataFormatter::reverse_bytes(&ack_header->data_len, 2);
		DataFormatter::reverse_bytes(&ack_header->ordinal, 2);
		DataFormatter::reverse_bytes(&ack_header->checksum, 2);

		memcpy(send_buf + sizeof(CNCS_HEADER), ack_data, 2);

		int ack_total_len = sizeof(CNCS_HEADER) + 2;
		int send_ack_len = sendto(m_socket, send_buf, ack_total_len, 0,
			(sockaddr*)&(m_using_master ? m_target_addr_master : m_target_addr_slave), 
			sizeof(sockaddr_in));
		if (send_ack_len != ack_total_len) {
			return FF_ERR_SOCKET_ERROR;
		}

		if (ff_res != FF_SUCCESS) {
			return ff_res;
		}

		// 返回数据
		memcpy(recv_data, rsp_data, rsp_data_len);
		recv_data_len = rsp_data_len;
	}
    else if (wait_result == ETIMEDOUT) {
		return FF_ERR_CNCS_MODULE_TIMEOUT;
	}
	else {
		assert(false);
		return FF_ERR_SYSTEM_ERROR;
	}

	return FF_SUCCESS;
}

int CNCSCommClient::multiple_sem_wait(sem_t *sems, int num_sems,unsigned long timeout)
{
   while (true) {
      for (int i = 0; i < num_sems; ++i) {
         if (sem_trywait(&sems[i]) == 0) {
            return i;
         }
      }
   }
}

void CNCSCommClient::do_recv()
{
    //WSACreateEvent ol_data;
//	ol_data.hEvent = ::WSACreateEvent();

    int ret=sem_init(&m_mysem,0,0);
    ret=sem_init(&m_exit_ev_sem,0,0);
    sem_t event_array[2] =
    {
        m_mysem,
        m_exit_ev_sem,
    };

	for (;;) {
//        char cncs_pkt_buf[MAX_DATA_LEN_NON_TRANSPARENT];
//		WSABUF wsaBuf;
//		wsaBuf.len = MAX_DATA_LEN_NON_TRANSPARENT;
//		wsaBuf.buf = cncs_pkt_buf;

        char cncs_pkt_buf[MAX_DATA_LEN_NON_TRANSPARENT];;

		struct sockaddr_in  sa_from;
        int nFromLen = sizeof(sa_from);
        unsigned long cncs_pkt_len = 0;
        unsigned long __flag = 0;


        int ret  =  recv(m_socket,  cncs_pkt_buf,  sizeof(cncs_pkt_buf),  0);

        if ( (ret <=0))
		{
			break;
		}

        int dwWaitRes = multiple_sem_wait(event_array,2,10000000);
        if( dwWaitRes == 0 ) { // data packet
            bool get_result=true;
//            bool get_result = WSAGetOverlappedResult(
//                m_socket, &ol_data, &cncs_pkt_len, true, &__flag );
            if (get_result) {
                CNCS_HEADER* cncs_header = (CNCS_HEADER*)cncs_pkt_buf;
                uint8 key_id = cncs_header->trans_id;
                if (USE_H1_INVOKE_ID_AS_IDENTIFIER &&
                    (cncs_header->cmd == PKT_CMD_CTRL_RSP))
                {
                    // NOTE:
                    //  这里解析了传输数据中的FF信息，从结构上讲不合适，但目前只能这样，
                    //  否则无法匹配数据包。
                    //
                    // FF模块回应的数据，需要忽略CNCS_HEADER中的trans_id，使用FF数据包中的invoke_id
                    FF_HEADER* ff_header = (FF_HEADER*)((char*)cncs_pkt_buf + sizeof(CNCS_HEADER));
                    key_id = ff_header->invoke_id;
                    if (ff_header->service == FMS_ABORT &&
                        m_trans_map.find(key_id) != m_trans_map.end()) {
                        TransactionInfo* tr_inf = m_trans_map[key_id];
                        if (tr_inf->get_state() == TRANS_IDLE) {
                            // FF模块主动发送的Abort请求, 需要上层处理
                            on_module_notify(cncs_header,
                                (char*)cncs_pkt_buf + sizeof(CNCS_HEADER),
                                cncs_pkt_len - sizeof(CNCS_HEADER));
                        }
                    }
                }
                if (m_trans_map.find(key_id) == m_trans_map.end()) {
                //	assert(false);
                //	break;
                    // TODO: LOG UNKNOWN PACKET
                } else {
                    TransactionInfo* tr_inf = m_trans_map[key_id];
                    tr_inf->fill_data(cncs_pkt_buf, cncs_pkt_len);
                }
            } else {
                // TODO: log error
            // break;
            }
        } else if (dwWaitRes == 1) { // exit event
            break;
        } else {
            assert(false);
            break;
        }
    }
}

void CNCSCommClient::on_module_notify(CNCS_HEADER* cncs_header, const char* data, int data_len)
{
	if (m_pfn_module_notify_handler != NULL) {
		uint8 err_sign;
		CTRL_ERR_CODE err_code;
		(*m_pfn_module_notify_handler)(cncs_header, data, data_len, err_sign, err_code);
		send_ack(cncs_header, CTRL_RSP_SUCCESS, CTRL_ERR_OK);
	}
}

int CNCSCommClient::send_ack(
	CNCS_HEADER* cncs_header,
	uint8 err_sign,
	CTRL_ERR_CODE err_code)
{
	const int ACK_PACKET_LEN = sizeof(CNCS_HEADER) + 2; // 2 bytes data
	char pkt_buf[ACK_PACKET_LEN];
	CNCS_HEADER* ack_header = (CNCS_HEADER*)pkt_buf;
	memcpy(pkt_buf, cncs_header, sizeof(CNCS_HEADER));
	ack_header->cmd = PKT_CMD_FF_ACK;
	ack_header->data_len = 2;
	DataFormatter::reverse_bytes(&ack_header->data_len, 2);
	ack_header->prot_custom_index = 0;
	ack_header->prot_slot_index = 0;

	char* data_ptr = pkt_buf + sizeof(CNCS_HEADER);
	data_ptr[0] = err_sign;
	data_ptr[1] = err_code;

	ack_header->checksum = CalSFCRC(data_ptr, 2);
	DataFormatter::reverse_bytes(&ack_header->checksum, 2);

	int send_ack_len = sendto(m_socket, pkt_buf, ACK_PACKET_LEN, 0,
		(sockaddr*)&(m_using_master ? m_target_addr_master : m_target_addr_slave), 
		sizeof(sockaddr_in));
	if (send_ack_len != ACK_PACKET_LEN) {
		return FF_ERR_SOCKET_ERROR;
	}
	return FF_SUCCESS;
}


//
// 从控制器接收数据包的线程。
// 这里仅从当前的“主”控制器接收数据。
void* CNCSCommClient::thread_recv_from_controller(void* p)
{
	CNCSCommClient* cli = (CNCSCommClient*)p;
	cli->do_recv();
    //return 0;
}
