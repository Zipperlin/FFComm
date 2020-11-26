#include "MIB.H"
#include "FFCommManager.h"

FFCommManager::FFCommManager(void)
{
//	m_comm_type = COMM_UNDEFINED;
}

FFCommManager FFCommManager::m_comm_mgr;
bool FFCommManager::m_initialized = false;
int FFCommManager::m_fms_init_retry_count = 0;

FFCommManager::~FFCommManager(void)
{
	map<FF_MODULE_HANDLE, FF_H1Module*>::iterator it_h1 = m_map_h1module.begin();
	for ( ; it_h1 != m_map_h1module.end(); it_h1++) 
		delete it_h1->second;
	m_map_h1module.clear();

	map<uint8, CNCSCommClient*>::iterator it_cncs = m_map_cncs_client.begin();
	for ( ; it_cncs != m_map_cncs_client.end(); it_cncs++) 
		delete it_cncs->second;
	m_map_cncs_client.clear();
}

FF_H1Module* FFCommManager::get_module(FF_MODULE_HANDLE h_module)
{
	map<FF_MODULE_HANDLE, FF_H1Module*>::const_iterator it = m_map_h1module.find(h_module);
	if (it == m_map_h1module.end()) {
		return NULL;
	}
	return it->second;
}

FF_H1Link* FF_H1Module::get_link(FF_LINK_INDEX index)
{
	if (index != 0 && index != 1) {
		return NULL;
	}
	map<FF_LINK_INDEX, FF_H1Link*>::const_iterator it = m_map_link.find(index);
	if (it == m_map_link.end()) {
		m_map_link[index] = new FF_H1Link(index, this);
		return m_map_link[index];
	} else {
		return it->second;
	}
}

// CNCS
FF_MODULE_HANDLE FFCommManager::open_module(
	uint8 dpu_addr,
	uint16 module_index,
	uint8 net1,
	uint8 net2,
	CNCS_BTYPE bb_type,
	uint16 udp_port,
	bool redundant_enabled)
{
	if (m_map_cncs_client.find(dpu_addr) == m_map_cncs_client.end()) {
		CNCSCommClient* cncs_client = new CNCSCommClient();
		int init_result = cncs_client->initialize(dpu_addr, net1, net2, bb_type, udp_port, redundant_enabled);
		if (init_result != FF_SUCCESS) {
			delete cncs_client;
			return INVALID_MODULE_HANDLE;
		}
		cncs_client->set_module_notify_handler(handle_module_notify);
		m_map_cncs_client[dpu_addr] = cncs_client;
	}

    //FF_MODULE_HANDLE h_cncs = (FF_MODULE_HANDLE)MAKELONG(dpu_addr, module_index);
    long addr=0;
    FF_MODULE_HANDLE h_cncs = (FF_MODULE_HANDLE)addr;

	if (m_map_h1module.find(h_cncs) != m_map_h1module.end()) {
		return h_cncs;
	}

	FF_H1Module* module = new FF_H1Module();
	module->set_slot_index(module_index);
	module->set_comm_type(COMM_CNCS);
	module->set_cncs_client(m_map_cncs_client[dpu_addr]);
	m_map_h1module[h_cncs] = module;

	return h_cncs;
}

bool FFCommManager::init()
{
    char szPath[MAX_PATH];
//	if (::GetModuleFileName(NULL, szPath, MAX_PATH) == 0) {
//        //unsigned long dwErr = ::errno();
//		assert(false);
//		return false;
//	}

	char* pos = strrchr(szPath, '\\');
	pos[0] = 0;
	strcat(szPath, "\\conf\\cfg-comm.ini");
    //m_fms_init_retry_count = ::GetPrivateProfileInt("FF_SETTINGS", "fms_init_retry_count", 0, szPath);
	
	m_initialized = true;
	return true;
}

void FFCommManager::handle_module_notify(
	CNCS_HEADER* cncs_header,
	const char* data,
	int data_len,
	uint8& err_sign,
	CTRL_ERR_CODE& err_code)
{
    //FF_MODULE_HANDLE h_cncs = (FF_MODULE_HANDLE)MAKELONG(cncs_header->dpu_addr, cncs_header->slave_addr);

    long addr=0;
    FF_MODULE_HANDLE h_cncs = (FF_MODULE_HANDLE)addr;


	FF_H1Module* ff_module = FFCommManager::inst().get_module(h_cncs);
	if (ff_module == NULL) {
		FF_LOG("未知的FF模块： DPU=%d, ADDR=%x", cncs_header->dpu_addr, cncs_header->slave_addr);
		return;
	}
	ff_module->handle_notify(data, data_len);
	// TODO: 完善错误检查，填充返回值
	err_sign = CTRL_RSP_SUCCESS;
	err_code = CTRL_ERR_OK;
}

FF_MODULE_HANDLE FFCommManager::open_module(
	int port_index,
	uint32 baud,
	uint8 data_bits,
	uint8 parity,
	uint8 stop_bits)
{
	FF_MODULE_HANDLE h_rs232 = (FF_MODULE_HANDLE)port_index;
	if (m_map_h1module.find(h_rs232) != m_map_h1module.end()) {
		return h_rs232;
	}

//    void* h_comm = CSerialPort::OpenPort(port_index, baud, data_bits, (CSerialPort::Parity)parity, (CSerialPort::StopBits)stop_bits);
//	if (h_comm == INVALID_HANDLE_VALUE) {
//		return INVALID_MODULE_HANDLE;
//	}
	FF_H1Module* module = new FF_H1Module();
//	module->get_port_rs232().Attach(h_comm);
//	module->set_comm_type(COMM_RS232);

	m_map_h1module[h_rs232] = module;
	return h_rs232;
}

FF_H1Module::FF_H1Module()
{
	m_cncs_client = NULL;
    pthread_mutex_init(&m_sync_req_lock,nullptr);
	m_invoke_id_counter = 1;
}

FF_H1Module::~FF_H1Module()
{
    ::pthread_mutex_destroy(&m_sync_req_lock);
}

FF_RESULT_CODE FF_H1Module::get_device_list(
	FF_LINK_INDEX link_index,
	H1_DEV_INFO* dev_array,
	int* count)
{
	return FF_SUCCESS;
}

uint8 FF_H1Module::get_new_invoke_id()
{
	// TODO:
	// 如果是CNCS多模块并行访问模式，则应在控制器级别保证唯一。
	//
	m_invoke_id_counter++;
	if (m_invoke_id_counter == 0)
		m_invoke_id_counter = 1;
	return m_invoke_id_counter;
}

// 向FF模块发送透传访问请求，并等待返回结果。
FF_RESULT_CODE FF_H1Module::sync_request(
    unsigned char		ucLinkID,
    short		nVcrID,
    unsigned char		ucLayer,
    unsigned char		ucService,
    unsigned char		ucPrimitive,
//	UCHAR		ucInvokeID,
    void *		pReqDataBuf,
	uint8		req_data_len,
    unsigned long		dwTimeout,
    void *		pOutDataBuf,	// OUT
    unsigned char *		pucOutDataLen,	// IN, OUT
	FF_HEADER& cnfSvcHeader, // OUT
	bool		bNeedResponse)
{
	if (bNeedResponse && (pucOutDataLen == NULL || pOutDataBuf == NULL)) {
        assert(false);
		return FF_ERR_INVALID_ARGUMENT;
	}

	char send_buf[255];

	FF_HEADER* ff_header = (FF_HEADER*)send_buf;
	ff_header->head_signature = FF_PACKET_HEAD_SIGNATURE;
	ff_header->link_index = ucLinkID;
	ff_header->packet_type = H1_DT_HOST_REQ;
	ff_header->comm_ref = nVcrID;
	ff_header->layer = ucLayer;
	ff_header->service = ucService;
	ff_header->primitive = ucPrimitive;
	ff_header->data_len = req_data_len; // + sizeof(FF_REQ_HEADER);
	ff_header->invoke_id = get_new_invoke_id(); // DONT'T call this function elsewhere
	memcpy(send_buf + sizeof(FF_HEADER), pReqDataBuf, req_data_len);
	if (m_comm_type == COMM_RS232) {
//        unsigned long wlen = m_serial_port.Write(send_buf, ff_header->data_len + sizeof(FF_HEADER));
//		if (!bNeedResponse) {
//			Sleep(100);
//			return FF_SUCCESS;
//		}
//        unsigned long res = m_serial_port.Read(&cnfSvcHeader, sizeof(FF_HEADER));
//		if (res == 0) {
//			FF_LOG("\t未能读取数据");
//			return FF_ERR_RS232_READ_NO_DATA;
//		}
//		if (cnfSvcHeader.invoke_id != ff_header->invoke_id) {
//			FF_LOG("\tInvoke ID 不匹配：(req/rsp) %d / %d",
//				ff_header->invoke_id, cnfSvcHeader.invoke_id);
//			return FF_ERR_INVOKE_ID_MISMATCH;
//		}
//		int dlen = cnfSvcHeader.data_len - sizeof(FF_HEADER);
//        unsigned long rlen = m_serial_port.Read(pOutDataBuf, dlen);
//		if (rlen != dlen) {
//			return FF_ERR_DATA_INCOMPLETE;
//		}
//		*pucOutDataLen = (uint8)rlen;
	}
	else if (m_comm_type == COMM_CNCS) {
		char recv_data_buf[MAX_COMM_DATA_BUFFER];
		int rsp_data_len = MAX_COMM_DATA_BUFFER;
		int req_result = m_cncs_client->request_ff_h1(
			m_slot_index, send_buf,
			req_data_len + sizeof(FF_HEADER), recv_data_buf,
			rsp_data_len, dwTimeout,
			ff_header->invoke_id, bNeedResponse,
			ff_header->comm_ref);

		if (req_result != FF_SUCCESS) {
			return (FF_RESULT_CODE)req_result;
		}

		if (bNeedResponse) {
			if (rsp_data_len > *pucOutDataLen) {
				return FF_ERR_BUFFER_TOO_SMALL;
			}
			int ff_data_len = rsp_data_len - sizeof(CNCS_HEADER) - sizeof(FF_HEADER);
			*pucOutDataLen = ff_data_len;
			char* ptr_ff_header = recv_data_buf + sizeof(CNCS_HEADER);
			memcpy((char*)&cnfSvcHeader, ptr_ff_header, sizeof(FF_HEADER));
			memcpy(pOutDataBuf, ptr_ff_header + sizeof(FF_HEADER), ff_data_len);
		}
	}

	return FF_SUCCESS;
}

FF_RESULT_CODE FF_H1Module::handle_notify(
	const char* data,
	int data_len)
{
	FF_HEADER* ff_header = (FF_HEADER*)data;
	if (ff_header->service == FMS_ABORT) {
		FF_H1Link* link_ob = get_link(ff_header->link_index);
		uint16 vcr_index = ff_header->comm_ref;
	//	DataFormatter::reverse_bytes(&vcr_index, sizeof(vcr_index));
	//	link_ob->release_local_vcr(vcr_index);

		// 更新该Abort信息相关设备的VCR连接状态
		link_ob->release_user_vcr(vcr_index);
	}
	return FF_SUCCESS;
}

FF_H1Link::FF_H1Link(int nIndex, FF_H1Module* parent_module)
	: m_nIndex(nIndex)
	, m_parent_module(parent_module)
	, m_live_list_index(0)
{
	memset(m_prev_live_list, 0, 32);
    ::pthread_mutex_init(&m_csAccess,nullptr);
}

FF_H1Link::~FF_H1Link()
{
    ::pthread_mutex_destroy(&m_csAccess);
}

// 读取VCR等信息，用于后续的通信连接
FF_RESULT_CODE FF_H1Link::init_link(bool reset_info)
{
	memset(m_prev_live_list, 0, 32);
	m_dev_array.clear();

	FMS_USER_READ_CNF readCnf;
    long lReadRes = IntfDev_Read(257, 0, readCnf);
	if (lReadRes != FF_SUCCESS) {
		return (FF_RESULT_CODE)lReadRes;
	}

    unsigned short* pDirIndex = (unsigned short*)readCnf.value;
	m_live_list_index = pDirIndex[12];
	DataFormatter::reverse_bytes(&m_live_list_index, 2);
	m_live_list_index += 3;
	FF_LOG("livelist index = %d", m_live_list_index);

	m_nVCRStartIdx = 0;
	memcpy(&m_nVCRStartIdx, readCnf.value + 16, 2);
	DataFormatter::reverse_bytes(&m_nVCRStartIdx, 2);
	m_nVCRStartIdx += 2;

	// Get VCR number
// 	lReadRes = IntfDev_Read(m_nVCRStartIdx - 1, 2, readCnf);
// 	if (lReadRes != FF_SUCCESS) {
// 		return (FF_RESULT_CODE)lReadRes;
// 	}
	int16 _nVcrNum = NCS2_H1LINK_VCR_MAX_NUM;
//	memcpy(&_nVcrNum, readCnf.value, 2);
//	DataFormatter::reverse_bytes(&_nVcrNum, 2);
//	assert(_nVcrNum == NCS2_H1LINK_VCR_MAX_NUM);

	FMS_Abort(0xff);

	for (size_t i = 0; i < m_dev_array.size(); i++) {
		m_dev_array[i]->reset_vcr();
	}

	for (int i = 0; i < _nVcrNum; i++) {
		if (i >= 3 && i <= 34) {
			m_vcr_state_map[i] = false;
		//	continue;
		} else {
			m_vcr_state_map[i] = true;
		}
	}

	return FF_SUCCESS;
}

static bool addr_has_device(unsigned char ucAddr, char *pLiveList)
{
    assert(pLiveList != NULL);
	return (pLiveList[ucAddr >> 3] & (0x80 >> (ucAddr & 7))) != 0;
}

void FF_H1Link::copy_cache_data(vector<H1_DEV_INFO>& dev_array)
{
	for (size_t i = 0; i < m_dev_array.size(); i++) {
		H1_DEV_INFO dev_inf;
		FF_Device* dev = m_dev_array[i];
		memcpy(dev_inf.device_id, dev->get_device_id(), FFH1_DEVID_LEN);
		memcpy(dev_inf.pdtag, dev->get_pdtag(), FFH1_PDTAG_LEN);
		dev_inf.addr = dev->get_addr();
		bool is_temp = IS_TEMP_H1_ADDR(dev_inf.addr);
		dev_inf.manu_id = is_temp ? 0 : dev->get_vendor_id();
		dev_inf.dev_type = is_temp ? 0 : dev->get_device_type();
		dev_inf.dev_ver = is_temp ? 0 : dev->get_device_version();
		dev_inf.dd_ver = is_temp ? 0 : dev->get_dd_version();
		dev_array.push_back(dev_inf);
	}
}

FF_RESULT_CODE FF_H1Link::init_dev(
	uint8 dev_addr,
	uint32 option
	)
{
	FF_LOG("初始化设备[0x%02x]...", dev_addr);
	H1_DEV_INFO dev_inf;
	FF_RESULT_CODE idt_result = SM_Identify(dev_addr, dev_inf.device_id, dev_inf.pdtag);
	// TODO: CHECK EMPTY DEVICE_ID
	if (idt_result != FF_SUCCESS) {
		FF_LOG("设备[0x%02x] Identify失败。错误码：%d", dev_addr, idt_result);
		return idt_result;
	}
	FF_LOG("设备[0x%02x] Identify完成。", dev_addr);

	char __buf[33]; memset(__buf, 0, 33);
	memcpy(__buf, dev_inf.device_id, FFH1_DEVID_LEN);
	FF_LOG("\t DEV_ID: %s", __buf);

	FF_Device* the_dev = find_dev(dev_addr);
//	_ASSERT(the_dev != NULL);

	if (the_dev == NULL) {
		the_dev = new FF_Device(this, dev_inf.device_id);
		the_dev->set_addr(dev_addr);
		the_dev->set_pdtag(dev_inf.pdtag);
		m_dev_array.push_back(the_dev); // internal cache
	} else {
		the_dev->set_device_id(dev_inf.device_id);
		the_dev->set_pdtag(dev_inf.pdtag);
		if (the_dev->is_connected()) {
			// TODO: check option
			the_dev->release_vcr_connection(H1_VFD_ALL);
			//return FF_SUCCESS;
		}
	}

	if (!IS_TEMP_H1_ADDR(dev_addr)) {
		FF_LOG("\t获取设备[0x%02x]基本信息...", dev_addr);
		// 获取在线设备的基本信息 (InitialDevStaticInfo)
		FF_RESULT_CODE result = the_dev->get_static_info();
		if (result != FF_SUCCESS) {
			FF_LOG("\t获取设备[0x%02x]基本信息失败。错误码：%d", dev_addr, result);
			return result;
		}
		FF_LOG("\t获取设备[0x%02x]基本信息完成", dev_addr);
	}

	return FF_SUCCESS;
}

FF_RESULT_CODE FF_H1Link::get_dev_list(
	vector<H1_DEV_INFO>& dev_array,
	GET_DEV_RANGE range,
	GET_DEV_OPTION opt)
{
	if (range == GDR_GET_CACHE) {
		// ignore other options
		copy_cache_data(dev_array);
		return FF_SUCCESS;
	}

	if (range == GDR_REFRESH_ALL ||
		m_live_list_index == 0) {
		FF_LOG("初始化网段...");
		FF_RESULT_CODE result = init_link();
		if (result != FF_SUCCESS) {
			FF_LOG("初始化网段失败，错误码：%d", result);
			return result;
		}
		FF_LOG("初始化网段完成");
	}

	// 读取 live list
	FF_LOG("读取livelist...");
	if (m_live_list_index == 0) {
		FF_LOG("live list索引无效，请重新初始化网段。");
		return FF_ERR_INVALID_ARGUMENT;
	}

	FMS_USER_READ_CNF cnfRead;
	FF_RESULT_CODE rr = IntfDev_Read(m_live_list_index, 0, cnfRead);
	if (rr != FF_SUCCESS) {
		FF_LOG("读取livelist失败，错误码：%d", rr);
		return rr;
	}
	FF_LOG("读取livelist完成。");

	uint8* livelist_bytes = (uint8*)cnfRead.value;
	// TODO: VALIDATE READ RESULT (_ValidLiveList)

#define PRINT_LIVELIST_ENABLED
#ifdef PRINT_LIVELIST_ENABLED
	int cc = 0;
	int sc = 0;
	char byte_str[128];
	sprintf(byte_str + sc, "\r\n\t");
	sc += 3;
	while (cc < 32) {
		sprintf(byte_str + sc, " %02X", (uint8)livelist_bytes[cc]);
		sc += 3;
		cc++;
		if ((cc % 16) == 0 || cc == 32) {
			sprintf(byte_str + sc, "\r\n\t");
			sc += 3;
		}
	}
	FF_LOG(byte_str);
#endif

    if (!addr_has_device(0x10, (char*)livelist_bytes)) {
        FF_LOG("错误：未找到接口卡设备. ", );
		return FF_ERR_LIVELIST_HAS_NO_INTERFACE;
	}

	bool live_list_changed = (memcmp(m_prev_live_list, livelist_bytes, 32) != 0);
	FF_LOG("livelist %s变化。", live_list_changed ? "有" : "无");

	if (live_list_changed) {
        for (unsigned char ucNodeAddr = 0x11; ucNodeAddr < 0xff; ucNodeAddr++) {
			// 删除不在列表里的设备
            if (!addr_has_device(ucNodeAddr, (char*)livelist_bytes)) {
				vector<FF_Device*>::iterator it = m_dev_array.begin();
				for ( ; it != m_dev_array.end(); it++) {
					FF_Device* dev = *it;
					if (dev->get_addr() == ucNodeAddr) {
						m_dev_array.erase(it);
						FF_LOG("设备（0x%02x）下线。", ucNodeAddr);
						break;
					}
				}
				continue;
			}

			FF_LOG("设备（0x%02x）在线", ucNodeAddr);
			H1_DEV_INFO dev_inf;
			FF_RESULT_CODE idt_result = SM_Identify(ucNodeAddr, dev_inf.device_id, dev_inf.pdtag);
			// TODO: CHECK EMPTY DEVICE_ID
			if (idt_result != FF_SUCCESS) {
				FF_LOG("设备（0x%02x）Identify失败。错误码：%d", ucNodeAddr, idt_result);
				continue;
			//	return idt_result;
			}
			FF_LOG("设备（0x%02x）Identify完成。", ucNodeAddr);
			char __buf[33]; memset(__buf, 0, 33);
			memcpy(__buf, dev_inf.device_id, FFH1_DEVID_LEN);
			FF_LOG("\t DEV_ID: %s", __buf);

			FF_Device* the_dev = find_dev(ucNodeAddr);
			if (range == GDR_REFRESH_ALL) {
                assert(the_dev == NULL);
			}

			if (the_dev == NULL) {
				the_dev = new FF_Device(this, dev_inf.device_id);
				the_dev->set_addr(ucNodeAddr);
				the_dev->set_pdtag(dev_inf.pdtag);
				m_dev_array.push_back(the_dev); // internal cache
			} else {
			//	the_dev->release_vcr_connection(H1_VFD_ALL);
				the_dev->set_device_id(dev_inf.device_id);
				the_dev->set_pdtag(dev_inf.pdtag);
				if (the_dev->is_connected()) {
					continue;
				}
			}

			if (opt == GDO_IDENTIFY_ONLY) {
				continue;
			}

			if (!IS_TEMP_H1_ADDR(ucNodeAddr)) {
				FF_LOG("\t获取设备（0x%02x）基本信息...", ucNodeAddr);
				// 获取在线设备的基本信息 (InitialDevStaticInfo)
				FF_RESULT_CODE result = the_dev->get_static_info();
				if (result != FF_SUCCESS) {
					// TODO: 
					FF_LOG("\t获取设备（0x%02x）基本信息失败。错误码：%d", ucNodeAddr, result);
					continue;
				//	return result;
				}
				FF_LOG("\t获取设备（0x%02x）基本信息完成", ucNodeAddr);
			}
		}
		memcpy(m_prev_live_list, livelist_bytes, 32);
	}

	FF_LOG("更新设备列表完成。");
	copy_cache_data(dev_array);
	
	return FF_SUCCESS;
}

/*
FF_RESULT_CODE FF_H1Link::get_dev_list(
	vector<H1_DEV_INFO>& dev_array,
	GET_DEV_LIST_OPTION opt)
{
	if (opt != GDLO_CACHE_ONLY) {
		// Get the live list
		FF_LOG("读取257...");
		FMS_USER_READ_CNF cnfRead;
		FF_RESULT_CODE lReadRes = IntfDev_Read(257, 0, cnfRead);
		if ( lReadRes != FF_SUCCESS) {
			FF_LOG("读取257失败，错误码：%d", lReadRes);
			return lReadRes;
		}
		FF_LOG("读取257完成。");

		USHORT* pDirIndex = (USHORT*)cnfRead.value;
		USHORT nLiveListStatus = pDirIndex[12];
		DataFormatter::reverse_bytes(&nLiveListStatus, 2);
		nLiveListStatus += 3;

		FF_LOG("读取livelist...");
		lReadRes = IntfDev_Read(nLiveListStatus, 0, cnfRead);
		if (lReadRes != FF_SUCCESS) {
			FF_LOG("读取livelist失败，错误码：%d", lReadRes);
			return (FF_RESULT_CODE)lReadRes;
		}
		FF_LOG("读取livelist完成。");

		uint8* livelist_bytes = (uint8*)cnfRead.value;

		// TODO: VALIDATE READ RESULT (_ValidLiveList)

		if (!addr_has_device(0x10, livelist_bytes)) {
			FF_LOG("错误：未找到接口卡设备. ", );
			return FF_ERR_LIVELIST_HAS_NO_INTERFACE;
		}

		bool live_list_changed = (memcmp(m_prev_live_list, livelist_bytes, 32) != 0);
		FF_LOG("livelist %s变化。", live_list_changed ? "有" : "无");

		if (live_list_changed || opt == GDLO_REFRESH_ALL) {
			m_dev_array.clear();
			// node address 0x11 is for backup interface device
			for (UCHAR ucNodeAddr = 0x11; ucNodeAddr < 0xff; ucNodeAddr++) {
				if (!addr_has_device(ucNodeAddr, livelist_bytes)) {
					continue;
				}
				FF_LOG("设备（0x%02x）在线", ucNodeAddr);
				H1_DEV_INFO dev_inf;
				FF_RESULT_CODE idt_result = SM_Identify(ucNodeAddr, dev_inf.device_id, dev_inf.pdtag);
				// TODO: CHECK EMPTY DEVICE_ID
				if (idt_result != FF_SUCCESS) {
					FF_LOG("设备（0x%02x）Identify失败。错误码：%d", ucNodeAddr, idt_result);
					return idt_result;
				}
				FF_LOG("设备（0x%02x）Identify完成。", ucNodeAddr);
				char __buf[33]; memset(__buf, 0, 33);
				memcpy(__buf, dev_inf.device_id, FFH1_DEVID_LEN);
				FF_LOG("\t DEV_ID: %s", __buf);

				FF_Device* dev = new FF_Device(this, dev_inf.device_id);
				dev->set_addr(ucNodeAddr);
				dev->set_pdtag(dev_inf.pdtag);

				if (!IS_TEMP_H1_ADDR(ucNodeAddr)) {
					FF_LOG("\t获取设备（0x%02x）基本信息...", ucNodeAddr);
					// 获取在线设备的基本信息 (InitialDevStaticInfo)
					int result = dev->get_static_info();
					if (result != FF_SUCCESS) {
						// TODO: 
						FF_LOG("\t获取设备（0x%02x）基本信息失败。错误码：%d", ucNodeAddr, result);
						//	continue;
					}
					FF_LOG("\t获取设备（0x%02x）基本信息完成", ucNodeAddr);
				}
				m_dev_array.push_back(dev); // internal cache
			}
			memcpy(m_prev_live_list, livelist_bytes, 32);
		}
	}

	for (size_t i = 0; i < m_dev_array.size(); i++) {
		H1_DEV_INFO dev_inf;
		FF_Device* dev = m_dev_array[i];
		memcpy(dev_inf.device_id, dev->get_device_id(), FFH1_DEVID_LEN);
		memcpy(dev_inf.pdtag, dev->get_pdtag(), FFH1_PDTAG_LEN);
		dev_inf.addr = dev->get_addr();
		bool is_temp = IS_TEMP_H1_ADDR(dev_inf.addr);
		dev_inf.manu_id = is_temp ? 0 : dev->get_vendor_id();
		dev_inf.dev_type = is_temp ? 0 : dev->get_device_type();
		dev_inf.dev_ver = is_temp ? 0 : dev->get_device_version();
		dev_inf.dd_ver = is_temp ? 0 : dev->get_dd_version();
		dev_array.push_back(dev_inf);
	}

	FF_LOG("更新设备列表完成。");
	return FF_SUCCESS;
}
*/

FF_Device* FF_H1Link::find_dev(const char* dev_id)
{
	for (size_t i = 0; i < m_dev_array.size(); i++) {
		FF_Device* dev = m_dev_array[i];
        int len = min((int)strlen(dev_id), 32);
		if (strncmp(dev->get_device_id(), dev_id, len) == 0) {
			return dev;
		}
	}
	return NULL;
}

FF_Device* FF_H1Link::find_dev_by_pdtag(const char* pdtag)
{
	for (size_t i = 0; i < m_dev_array.size(); i++) {
		FF_Device* dev = m_dev_array[i];
        int len = min((int)strlen(pdtag), 32);
		len = min(len, (int)strlen(dev->get_pdtag()));
		if (strncmp(dev->get_pdtag(), pdtag, len) == 0) {
			return dev;
		}
	}
	return NULL;
}

FF_Device* FF_H1Link::find_dev(uint8 dev_addr)
{
	for (size_t i = 0; i < m_dev_array.size(); i++) {
		FF_Device* dev = m_dev_array[i];
		if (dev->get_addr() == dev_addr) {
			return dev;
		}
	}
	return NULL;
}

void FF_H1Link::release_local_vcr(uint16 vcr_index)
{
	if (m_vcr_state_map.find(vcr_index) == m_vcr_state_map.end()) {
		return;
	}
	m_vcr_state_map[vcr_index] = false; // set to unused
}

void FF_H1Link::release_user_vcr(uint16 vcr_index)
{
	if (m_vcr_state_map.find(vcr_index) == m_vcr_state_map.end()) {
		return;
	}
	m_vcr_state_map[vcr_index] = false; // set to unused

	for (size_t i = 0; i < m_dev_array.size(); i++) {
		if (m_dev_array[i]->reset_vcr(vcr_index)) {
			return;
		}
	}
}

// 在本地缓存中查找，获取一个尚未使用的接口卡中的VCR。
// 如果已满（都已连接使用），则释放第一个。（正常情况下不应该发生）
uint16 FF_H1Link::get_user_vcr()
{
//	CCS lock(&m_csVCRList);

	map<uint16, bool>::iterator it = m_vcr_state_map.begin();
	for ( ; it != m_vcr_state_map.end(); it++) {
		bool used = it->second;
		if (!used) {
			it->second = true;
			return it->first;
		}
	}

	// all used, release the first
	it = m_vcr_state_map.begin();
	uint16 vcr_index = it->first;
	it->second = true;
	release_user_vcr(vcr_index);
	FMS_Abort(vcr_index);

	FF_LOG("VCR已满，释放VCR[%d]之前的连接。\n", vcr_index);

	return vcr_index;
}

long FF_H1Link::FMS_Abort(int16 nVCRIndex)
{
	FMS_USER_ABORT_REQ reqAbort;
    reqAbort.local = true;
	reqAbort.abort_id = 1;
	reqAbort.reason = 0;
	reqAbort.detail_length = 0;
	::memset(reqAbort.detail, 0, sizeof(reqAbort.detail));
    int offset = (int)(reqAbort.detail - (char*)&reqAbort);

//	ATLTRACE("abort VCR (%d) ... ", nVCRIndex);
	FF_HEADER cnfHeader;
    long lRes = get_parent_module()->sync_request(
		m_nIndex, nVCRIndex,
		FMS_LAYER_ID, FMS_ABORT, REQUEST_PRIMITIVE,
		&reqAbort, offset + reqAbort.detail_length,
		H1IO_TIMEOUT_FMS_ABORT, NULL, NULL, cnfHeader,
        false // needn't response
		);

	if (lRes != FF_SUCCESS) {
	//	ATLTRACE(" failed. 0x%x \r\n", lRes);
		return lRes;
	}
	return FF_SUCCESS;
}

// 注意：函数填充的szDevID和szPdTag，不会自动在结尾添加 '\0'
//
FF_RESULT_CODE FF_H1Link::SM_Identify(unsigned char ucDevAddr, char *szDevID, char *szPdTag)
{
//	CCS lock(&m_csSMAccess);
//	CCS lockLink(&m_csAccess);
	SM_IDENTIFY_REQ reqSMIdentify;
	reqSMIdentify.ucNodeAddr = ucDevAddr;
	reqSMIdentify.ucDummy = 0;

    char szBuf[255];
    unsigned char ucOutLen = 255;
	FF_HEADER cnfHeader;

// #ifdef _DEBUG
// 	clock_t s = clock();
// #endif

	FF_RESULT_CODE lRes = get_parent_module()->sync_request(
		m_nIndex, 0,
		SM_LAYER_ID, SM_SVC_IDENTIFY, REQUEST_PRIMITIVE,
		&reqSMIdentify, sizeof(reqSMIdentify),
		H1IO_TIMEOUT_SM_IDENTIFY, szBuf, &ucOutLen, cnfHeader);

	if (lRes != 0)
	{
	//	szDevID[0] = '\0';
	//	szPdTag[0] = '\0';
		m_ucIvkIDSendReq = szBuf[0];
        ::memcpy(&m_stSendReq, szBuf + 1, sizeof(timeval));
		return lRes;
	}

	SM_IDENTIFY_CNF cnf;
	memcpy(&cnf, szBuf, ucOutLen);

	memcpy(szDevID, cnf.szDevID, FFH1_DEVID_LEN);
//	szDevID[FFH1_DEVID_LEN] = '\0';
	memcpy(szPdTag, cnf.szPdTag, FFH1_PDTAG_LEN);
//	szPdTag[FFH1_PDTAG_LEN] = '\0';

// #ifdef _DEBUG
// 	s -= clock();
// 	ATLTRACE(_T("Identify H1 device (%d, %s) uses %d msec.\n"), ucDevAddr, szDevID, -s);
// #endif

	return FF_SUCCESS;
}

FF_RESULT_CODE FF_H1Link::IntfDev_Read(
	uint16 nIndex,
	uint8 ucSubIndex,
	FMS_USER_READ_CNF& readCnf)
{
	FMS_USER_READ_REQ readReq;
	readReq.acc_spec.tag = FMS_ACCESS_INDEX;
#ifdef _DEBUG
	readReq.acc_spec.dummy = 0xab;
//	readReq.acc_spec.dummy[1] = 0xab;
//	readReq.acc_spec.dummy[2] = 0xab;
#endif // _DEBUG
	readReq.acc_spec.index = nIndex;
	readReq.subindex = ucSubIndex;

	FF_HEADER cnfHeader;
	char _szBuf[255];
    unsigned char _ucLen = 255;

	SyncLock __lock(&m_csAccess);

	FF_RESULT_CODE result = get_parent_module()->sync_request(
		m_nIndex, 0, FMS_LAYER_ID, INTFDEV_READ, REQUEST_PRIMITIVE,
		&readReq, sizeof(readReq), H1IO_TIMEOUT_INTF_READ,
        _szBuf, &_ucLen, cnfHeader, true);
	if (result != FF_SUCCESS) {
		return result;
	}
	if (cnfHeader.result != POSITIVE) {
		return FF_ERR_NEGATIVE_ECHO; // NCS_E_NEGATIVE_ECHO;
	}

	memcpy(&readCnf, _szBuf, _ucLen);
    assert(readCnf.length == (_ucLen - 2));

	return FF_SUCCESS;
}

FF_RESULT_CODE FF_H1Link::IntfDev_Write(
    unsigned short nIndex,
    unsigned char ucSubIndex,
    char * pszWriteBuf,
    unsigned char ucWriteLen)
{
    assert(pszWriteBuf != 0);
    assert(ucWriteLen > 0);
    assert(ucWriteLen <= MAX_DATA_FIELD_LEN);

	FMS_USER_WRITE_REQ reqWrite;
	reqWrite.acc_spec.tag = FMS_ACCESS_INDEX;
	reqWrite.acc_spec.index = nIndex;
	reqWrite.subindex = ucSubIndex;
	reqWrite.length = ucWriteLen;

	memcpy(reqWrite.value, pszWriteBuf, ucWriteLen);
//	CCS lockLink(&m_csAccess);
    char _szBuf[255];
    unsigned char _ucLen = 255;
	FF_HEADER cnfHeader;
	uint8 data_len = sizeof(reqWrite) - (MAX_DATA_FIELD_LEN - ucWriteLen);
	FF_RESULT_CODE lRes = get_parent_module()->sync_request(
		m_nIndex, 0, FMS_LAYER_ID, INTFDEV_WRITE, REQUEST_PRIMITIVE,
		&reqWrite, data_len,
		H1IO_TIMEOUT_INTF_WRITE, _szBuf, &_ucLen, cnfHeader);
	if (lRes != FF_SUCCESS) {
		m_ucIvkIDSendReq = _szBuf[0];
        ::memcpy(&m_stSendReq, _szBuf + 1, sizeof(timeval));
		return lRes;
	}

	if (cnfHeader.result != POSITIVE) {
		return FF_ERR_NEGATIVE_ECHO;
	}

	return FF_SUCCESS;
}

FF_RESULT_CODE FF_H1Link::IntfDev_DownloadSegment(
    unsigned short nDomainIndex,
    unsigned char ucMoreFollows,
    char* pszData,
    unsigned char ucDataLen)
{
    assert(ucDataLen <= MAX_DATA_FIELD_LEN);

	FMS_USER_GEN_DOWNLOAD_SEG_REQ reqGenDLSeg;
	memset(&reqGenDLSeg, 0, sizeof(reqGenDLSeg));
	reqGenDLSeg.acc_spec.tag = 0;
	reqGenDLSeg.acc_spec.index = nDomainIndex;
	reqGenDLSeg.more_follows = ucMoreFollows;
	reqGenDLSeg.data_len = ucDataLen;
	memcpy(&reqGenDLSeg.data, pszData, ucDataLen);
//	CCS lockLink(&m_csAccess);
    char _szBuf[255];
    unsigned char _ucLen = 255;
	FF_HEADER cnfHeader;
	FF_RESULT_CODE lRes = get_parent_module()->sync_request(
		m_nIndex, 0, FMS_LAYER_ID, INTFDEV_DOWNLOAD_SEG, REQUEST_PRIMITIVE,
		&reqGenDLSeg, sizeof(reqGenDLSeg),
		H1IO_TIMEOUT_INTF_DOWNLOAD_SEG, _szBuf, &_ucLen, cnfHeader);
	if (lRes != FF_SUCCESS) {
		m_ucIvkIDSendReq = _szBuf[0];
        ::memcpy(&m_stSendReq, _szBuf + 1, sizeof(timeval));
		return lRes;
	}

	if (cnfHeader.result != POSITIVE) {
		return FF_ERR_NEGATIVE_ECHO;
	}

	return FF_SUCCESS;
}

FF_RESULT_CODE FF_H1Link::SM_ClearAddress(unsigned char ucDevAddr, const char* szDevID, const char* szPdTag)
{
	SM_CLEAR_ADDRESS_REQ reqClearAddr;
	reqClearAddr.ucNodeAddr = ucDevAddr;
	memcpy(reqClearAddr.szDevID, szDevID, FFH1_DEVID_LEN);
	memcpy(reqClearAddr.szPdTag, szPdTag, FFH1_PDTAG_LEN);

	FF_Device* dev_ob = find_dev(ucDevAddr);
    assert(dev_ob != NULL);
	dev_ob->release_vcr_connection(H1_VFD_ALL);

    char _szBuf[255];
    unsigned char _ucLen = 255;
	FF_HEADER cnfHeader;
	FF_RESULT_CODE lRes = get_parent_module()->sync_request(
		m_nIndex, 0, SM_LAYER_ID, SM_SVC_CLEAR_ADDRESS, REQUEST_PRIMITIVE,
		&reqClearAddr, sizeof(reqClearAddr),
		H1IO_TIMEOUT_SM_CLEAR_ADDRESS + 1000, _szBuf, &_ucLen, cnfHeader);
	if (lRes != FF_SUCCESS) {
		m_ucIvkIDSendReq = _szBuf[0];
        ::memcpy(&m_stSendReq, _szBuf + 1, sizeof(timeval));
		return lRes;
	}
	if (cnfHeader.result != POSITIVE) {
		return FF_ERR_NEGATIVE_ECHO;
	}

	return FF_SUCCESS;
}

FF_RESULT_CODE FF_H1Link::SM_SetAddress(
    unsigned char ucDevAddr,
	const char* szPdTag)
{
	// Get parameters for set address ... 
	FMS_USER_READ_CNF cnfRead;
	FF_RESULT_CODE lReadRes = IntfDev_Read(256, 0, cnfRead);
	if (lReadRes != FF_SUCCESS) {
		return lReadRes;
	}

    unsigned short* pArray = (unsigned short*)cnfRead.value;

	/*
	// T1, T2, T3
	USHORT _nIdxSMSupport = pArray[6];
	fmtBytesReverse(&_nIdxSMSupport, 2);
	USHORT _nIdxT1 = _nIdxSMSupport + 1;
	//	USHORT _nIdxT2 = _nIdxSMSupport + 2;
	USHORT _nIdxT3 = _nIdxSMSupport + 3;
	lReadRes = IntfDev_Read(_nIdxT1, 0, cnfRead);
	if (lReadRes != NCS_E_OK)
	{
		return lReadRes;
	}
	_ASSERT(cnfRead.length == 4);
	UINT nT1 = *((UINT*)cnfRead.value);
	fmtBytesReverse(&nT1, 4);

	lReadRes = IntfDev_Read(_nIdxT3, 0, cnfRead);
	if (lReadRes != NCS_E_OK)
	{
		return lReadRes;
	}
	_ASSERT(cnfRead.length == 4);
	UINT nT3 = *((UINT*)cnfRead.value);
	fmtBytesReverse(&nT3, 4);
	*/

	// AP_CLOCK_SYNC_INTERVAL and PRIMARY_AP_TIME_PUBLISHER
    unsigned short _nIdxCurTime = pArray[8];
	DataFormatter::reverse_bytes(&_nIdxCurTime, 2);
    unsigned short _nIdxSyncIntv = _nIdxCurTime + 2;
    unsigned short _nIdxPrimApTimePub = _nIdxCurTime + 4;

	lReadRes = IntfDev_Read(_nIdxSyncIntv, 0, cnfRead);
	if (lReadRes != FF_SUCCESS) {
		return lReadRes;
	}
    assert(cnfRead.length == 1);
    unsigned char ucClockIntv = cnfRead.value[0];

	lReadRes = IntfDev_Read(_nIdxPrimApTimePub, 0, cnfRead);
	if (lReadRes != FF_SUCCESS) {
		return lReadRes;
	}
    assert(cnfRead.length == 1);
    unsigned char ucPrimTimePub = cnfRead.value[0];

	FF_Device* dev_ob = find_dev_by_pdtag(szPdTag);

	SM_SET_ADDRESS_REQ reqSetAddr;
//	CCS lockLink(&m_csAccess);
	reqSetAddr.ucNodeAddr = ucDevAddr;
	reqSetAddr.ucClockInterval = ucClockIntv;
	reqSetAddr.ucPrimPublisher = ucPrimTimePub;
	memcpy(reqSetAddr.szPdTag, szPdTag, FFH1_PDTAG_LEN);

    char _szBuf[255];
    unsigned char _ucLen = 255;
	FF_HEADER cnfHeader;
//	_ASSERT(m_ucIvkID_SetAddr == 0);
//	m_ucIvkID_SetAddr = GetUniqueInvokeID();
	FF_RESULT_CODE lRes = get_parent_module()->sync_request(
		m_nIndex, 0, SM_LAYER_ID, SM_SVC_SET_ADDRESS, REQUEST_PRIMITIVE,
		&reqSetAddr, sizeof(reqSetAddr),
		H1IO_TIMEOUT_SM_SET_ADDRESS, _szBuf, &_ucLen, cnfHeader);

//	m_ucIvkID_SetAddr = 0;

	if (lRes != FF_SUCCESS) {
		m_ucIvkIDSendReq = _szBuf[0];
        ::memcpy(&m_stSendReq, _szBuf + 1, sizeof(timeval));
		return lRes;
	}

	if (cnfHeader.result != POSITIVE) {
		return FF_ERR_NEGATIVE_ECHO;
	}

	if (dev_ob != NULL) {
		dev_ob->set_addr(ucDevAddr);
	}

	return FF_SUCCESS;
}

FF_RESULT_CODE FF_H1Link::SM_SetPdTag(
    unsigned char ucAddr,
	const char* szDevID,
	const char* szPdTag,
    bool bClear)
{
	SM_SET_PD_TAG_REQ reqSetPdTag;
	reqSetPdTag.ucClear = (bClear ? 0xFF : 0x00);
	reqSetPdTag.ucNodeAddr = ucAddr;
	if (bClear) {
		memcpy(reqSetPdTag.szPdTag, szPdTag, FFH1_PDTAG_LEN);
	} else {
		memset(reqSetPdTag.szPdTag, ' ', FFH1_PDTAG_LEN);
        memcpy(reqSetPdTag.szPdTag, szPdTag, min((int)strlen(szPdTag), 32));
	}
	memcpy(reqSetPdTag.szDevID, szDevID, FFH1_DEVID_LEN);

	FF_Device* dev_ob = find_dev(ucAddr);

    unsigned long _dwTimeout =
		(bClear ?  H1IO_TIMEOUT_SM_CLEAR_PDTAG : H1IO_TIMEOUT_SM_SET_PDTAG);

    char _szBuf[255];
    unsigned char _ucLen = 255;
	FF_HEADER cnfHeader;
	FF_RESULT_CODE lRes = get_parent_module()->sync_request(
		m_nIndex, 0, SM_LAYER_ID, SM_SVC_SET_PD_TAG, REQUEST_PRIMITIVE,
		&reqSetPdTag, sizeof(reqSetPdTag),
		_dwTimeout, _szBuf, &_ucLen, cnfHeader);
	if (lRes != FF_SUCCESS) {
		m_ucIvkIDSendReq = _szBuf[0];
        ::memcpy(&m_stSendReq, _szBuf + 1, sizeof(timeval));
		return lRes;
	}

	if (cnfHeader.result != POSITIVE) {
		return FF_ERR_NEGATIVE_ECHO;
	}

	if (!bClear) {
		dev_ob->set_pdtag(szPdTag);
	}

	return FF_SUCCESS;
}

FF_Block::FF_Block(uint16 start_index, FFBLOCK_TYPE type)
	: m_block_type(type), m_start_index(start_index)
{

}

void FF_Block::set_info(FF_BLOCK_INFO* info)
{
	memcpy(&m_info, info, sizeof(FF_BLOCK_INFO));

	DataFormatter::reverse_bytes( &(m_info.ddItem), sizeof(m_info.ddItem) );
	DataFormatter::reverse_bytes( &(m_info.ddName), sizeof(m_info.ddName) );
	DataFormatter::reverse_bytes( &(m_info.ddRev), sizeof(m_info.ddRev) );
	DataFormatter::reverse_bytes( &(m_info.executionTime), sizeof(m_info.executionTime) );
	DataFormatter::reverse_bytes( &(m_info.nextFb), sizeof(m_info.nextFb) );
	DataFormatter::reverse_bytes( &(m_info.numParams), sizeof(m_info.numParams) );
	DataFormatter::reverse_bytes( &(m_info.periodExecution), sizeof(m_info.periodExecution) );
	DataFormatter::reverse_bytes( &(m_info.profile), sizeof(m_info.profile) );
	DataFormatter::reverse_bytes( &(m_info.profileRev), sizeof(m_info.profileRev) );
	DataFormatter::reverse_bytes( &(m_info.startViewIndex), sizeof(m_info.startViewIndex) );
}

void FF_Block::copy_info(H1_BLOCK_INFO& info)
{
	info.dd_item = m_info.ddItem;
	info.exec_time = m_info.executionTime;
	info.start_index = m_start_index;
	info.start_view_index = m_info.startViewIndex;
	memcpy(info.tag, m_info.fbTag, BLOCK_TAG_LEN);
	info.block_type = m_block_type;
	info.profile = m_info.profile;
	info.profile_rev = m_info.profileRev;
}

FF_Device::FF_Device(FF_H1Link* parent_link, const char* dev_id)
	: m_parent_link(parent_link)
{
	m_vcr_index_vfd = 0;
	m_vcr_index_mib = 0;
	m_mib_max_dlsdu_size = 0;
	m_vfd_max_dlsdu_size = 0;
	m_address = 0;
	m_vfd_address = 0;
	m_vcr_static_entry_index = 0;
	m_vfd_ref = 0;
	m_res_block = NULL;
	memcpy(m_device_id, dev_id, FFH1_DEVID_LEN);
	memset(&m_vcr_vfd, 0, sizeof(m_vcr_vfd));
}

FF_Device::~FF_Device()
{
	release_func_block_array();
}

FF_H1Module* FF_Device::get_parent_module()
{
	return m_parent_link->get_parent_module();
}

FF_RESULT_CODE FF_Device::init_write_vcr()
{
	// 初始化 MIB 连接
	FF_LOG("\t初始化MIB连接...");
	FF_RESULT_CODE init_mib_result = init_mib();
	if (init_mib_result != FF_SUCCESS) {
		FF_LOG("\t初始化MIB连接失败。错误码：%d", init_mib_result);
		return init_mib_result;
	}
	FF_LOG("\t初始化MIB连接完成。");

	// 获取 OD 索引
	FF_LOG("\t获取 OD 0 信息...");
	FMS_USER_GETOD_CNF cnf_get_od;
	FF_RESULT_CODE get_od_result = fms_get_od(m_vcr_index_mib, 0, cnf_get_od);
	if (get_od_result != FF_SUCCESS) {
		close_mib();
		FF_LOG("\t获取 OD 0 信息失败。错误码：%d", get_od_result);
		return get_od_result;
	}
	FF_LOG("\t获取 OD 0 信息完成。");

	T_OD_OBJ_DESCR_HDR* pODHeader = (T_OD_OBJ_DESCR_HDR*)cnf_get_od.packed_object_descr;
	uint16 od_index = pODHeader->first_index_s_od;
	DataFormatter::reverse_bytes(&od_index, 2);
	FF_LOG("\tOD索引：%d", od_index);

	//////////////////////////////////////////////////////////////////////////
	FF_LOG("\tFMS读取 %d ...", od_index);
	FMS_USER_READ_CNF cnf_read;
	FF_RESULT_CODE read_result = fms_read(H1_VFD_MIB, od_index, 0, cnf_read);
	if (read_result != FF_SUCCESS) {
		close_mib();
		FF_LOG("\tFMS读取 %d 失败。错误码：%d", od_index, read_result);
		return read_result;
	}
	FF_LOG("\tFMS读取 %d 完成", od_index);

	// 获取目录索引列表
	uint16* dir_index_array = (uint16*)cnf_read.value;

	// 确认 VFD 个数
	uint16 vfd_num = dir_index_array[13];
	DataFormatter::reverse_bytes(&vfd_num, 2);
	if (vfd_num != 2) {
		assert(false);
		close_mib();
		return FF_ERR_DEVICE_NOT_SUPPORTED;
	}
	FF_LOG("\tVFD数量：%d", vfd_num);

	// read FmsVfdld
	uint16 vfd_ref_entry = dir_index_array[12];
	DataFormatter::reverse_bytes(&vfd_ref_entry, 2);
	FMS_USER_READ_CNF cnf_read_vfd_ref;
	read_result = fms_read(H1_VFD_MIB, vfd_ref_entry + 1, 0, cnf_read_vfd_ref);
	if (read_result != FF_SUCCESS) {
		close_mib();
		return read_result;
	}
	VFD_REF_ENTRY* vfd_ref_entry_ptr = (VFD_REF_ENTRY*)cnf_read_vfd_ref.value;
	uint32 vfd_ref = vfd_ref_entry_ptr->VfdRef;
	m_vcr_vfd.FmsVfdId = vfd_ref;
	DataFormatter::reverse_bytes(&vfd_ref, sizeof(vfd_ref));
	m_vfd_ref = vfd_ref;

	//////////////////////////////////////////////////////////////////////////
	FMS_USER_READ_CNF cnf_read_od;
	read_result = fms_read(H1_VFD_MIB, od_index + 1, 0, cnf_read_od);
	if (read_result != FF_SUCCESS) {
		close_mib();
		return read_result;
	}
	// TODO: 
	// 1. 这里需要调试确认，N4K中的指针数组用法不好，不应用一个指针指向多次读取的结果。
	// 
	uint16* dir_vcr_index_array = (uint16*)cnf_read_od.value;
	uint16 vcr_entry_index = dir_vcr_index_array[8];
	DataFormatter::reverse_bytes(&vcr_entry_index, 2);
	uint16 dlme_basic_char = dir_vcr_index_array[10];
	DataFormatter::reverse_bytes(&dlme_basic_char, 2);
	m_vcr_static_entry_index = vcr_entry_index;

	FMS_USER_READ_CNF cnf_read_dlme;
	read_result = fms_read(H1_VFD_MIB, dlme_basic_char, 0, cnf_read_dlme);
	if (read_result != FF_SUCCESS) {
		close_mib();
		FF_LOG("\t读取 DLME_BASIC_CHAR 失败。错误码 %d", read_result);
		return read_result;
	}
	assert(cnf_read_dlme.length == 7);
	m_dev_class = (FF_DEV_CLASS)cnf_read_dlme.value[2];

	uint16 wVCRTicsIdx = m_vcr_static_entry_index + 1;
	uint16 wVCRStartIdx = m_vcr_static_entry_index + 2;
	// Read VCR_LIST_CHARACTERISTICS
    VCR_LIST_CHARACTERISTICS Characteristic;
	FMS_USER_READ_CNF cnfRead;
	FF_RESULT_CODE lRes = fms_read(H1_VFD_MIB, wVCRTicsIdx, 0, cnfRead);
	if (lRes != FF_SUCCESS ||
		(cnfRead.length != sizeof(VCR_LIST_CHARACTERISTICS)) )
	{
		FF_LOG("\t读取 VCR_LIST_CHARACTERISTICS 状态失败。错误码：%d", lRes);
		return FF_ERR_READ_DIRECTORY_ERROR;
	}
	::memcpy(&Characteristic, cnfRead.value, cnfRead.length);
	DataFormatter::reverse_bytes(&Characteristic.Version,
		sizeof(Characteristic.Version));
	DataFormatter::reverse_bytes(&Characteristic.MaxEntries,
		sizeof(Characteristic.MaxEntries));
	DataFormatter::reverse_bytes(&Characteristic.NumPermanentEntries, 
		sizeof(Characteristic.NumPermanentEntries));
	DataFormatter::reverse_bytes(&Characteristic.NumCurrentlyConfigured, 
		sizeof(Characteristic.NumCurrentlyConfigured));
	DataFormatter::reverse_bytes(&Characteristic.FirstUnconfiguredEntry, 
		sizeof(Characteristic.FirstUnconfiguredEntry));
	DataFormatter::reverse_bytes(&Characteristic.NumOfStatisticsEntries, 
		sizeof(Characteristic.NumOfStatisticsEntries));

    char ucLocalAddr = 0;
    char ucLocalAddrUsed[256];
	VCR_STATIC_ENTRY VCRInf[4], *pCurVCR = NULL;
	memset(VCRInf, 0, sizeof(VCRInf));
	memset(ucLocalAddrUsed, 0, sizeof(ucLocalAddrUsed));
    ucLocalAddrUsed[0xF8] = true;
    unsigned long ulVfdRef = (m_vfd_ref > 0) ? m_vfd_ref : 1;
	DataFormatter::reverse_bytes(&ulVfdRef, sizeof(ulVfdRef));

	// Read VCRs
    unsigned int wCfgCnt = 0;
    for ( unsigned long wIdx = 0; wIdx < Characteristic.MaxEntries - 2; wIdx++ )
	{
		lRes = fms_read(H1_VFD_MIB, wIdx + wVCRStartIdx, 0, cnfRead);
		if (lRes != FF_SUCCESS) {
			return lRes;
		}
		if (cnfRead.length != sizeof(VCR_STATIC_ENTRY))	{
			return FF_ERR_DATA_INCOMPLETE;
		}

		pCurVCR = VCRInf + ((wCfgCnt > 3) ? 3 : wCfgCnt);
		::memcpy(pCurVCR, cnfRead.value, sizeof(VCR_STATIC_ENTRY));
		if ( pCurVCR->FasArTypeAndRole == 0 ) {
			continue; 
		}

		wCfgCnt++; 
		if ( wIdx == 0 )
		{	// VCR for MIB
			if ( pCurVCR->FasArTypeAndRole != 0x32 ) { // H1_SERVICE_VCR
				return FF_ERR_VCR_MISMATCH;
			}
            assert(1 == wCfgCnt);
			::memcpy(&m_vcr_mib, cnfRead.value, sizeof(m_vcr_mib));
			FF_LOG("\tMIB VCR OD_INDEX: %d / 0x%x", wIdx + wVCRStartIdx, wIdx + wVCRStartIdx);
			continue;
		}

		// Search VCR for VFD
		DataFormatter::reverse_bytes(&pCurVCR->FasDllLocalAddr,
			sizeof(pCurVCR->FasDllLocalAddr));
        ucLocalAddr = (char)(pCurVCR->FasDllLocalAddr & 0xFF);
        ucLocalAddrUsed[ucLocalAddr] = true;
		FF_LOG("\t VCR OD_INDEX: %d, TYPE=0x%x, ADDR=0x%x, VFD_ID=%x/%x",
			wIdx + wVCRStartIdx, pCurVCR->FasArTypeAndRole, ucLocalAddr,
			pCurVCR->FmsVfdId, ulVfdRef);
		if ( pCurVCR->FasArTypeAndRole == 0x32 )
		{
			::memcpy(&m_vcr_vfd, cnfRead.value, sizeof(m_vcr_vfd));
			if ( pCurVCR->FmsVfdId == ulVfdRef )
			{
				m_vfd_address = ucLocalAddr;
				FF_LOG("\tVFD VCR OD_INDEX: %d", wIdx + wVCRStartIdx);
				return FF_SUCCESS;
/*
				FF_RESULT_CODE ir = init_vfd();
				if (ir == FF_SUCCESS) {
					return ir;
				}
				m_vfd_address = 0;
*/
			}
		}
		if ( wCfgCnt >= Characteristic.NumCurrentlyConfigured )
		{
			break;
		}
	}

	if ( m_vcr_vfd.FasArTypeAndRole != 0x32 )
	{
		::memcpy(&m_vcr_vfd, &m_vcr_mib, sizeof(m_vcr_vfd));
		m_vcr_vfd.FmsVfdId = ulVfdRef; 
	}

	// Write VCR for VFD
    for ( unsigned int wIdx = 1; wIdx <= 2; wIdx++ )
	{
		pCurVCR = VCRInf + 1;
		::memcpy(pCurVCR, VCRInf, sizeof(VCR_STATIC_ENTRY));
		pCurVCR->FmsVfdId = ulVfdRef;
		// Search unused Local address
		for ( ucLocalAddr = 0xF8; ucLocalAddr > 0; ucLocalAddr-- )
		{
			if ( ! ucLocalAddrUsed[ucLocalAddr] )
			{
				pCurVCR->FasDllLocalAddr = ucLocalAddr;
                ucLocalAddrUsed[ucLocalAddr] = true;
				break;
			}
		}
		if ( wIdx == 1 )
		{
			pCurVCR->FasDllResidualActivitySupported = 0xff;
			pCurVCR->FmsMaxOutstandingServicesCalling = 0;
			pCurVCR->FmsMaxOutstandingServicesCalled = 1;
		}

		DataFormatter::reverse_bytes(&pCurVCR->FasDllLocalAddr, sizeof(pCurVCR->FasDllLocalAddr));
		pCurVCR->FasDllMaxDlsduSize = m_vcr_vfd.FasDllMaxDlsduSize; // 0x100 
		pCurVCR->FasDllMaxConfirmDelayOnData = m_vcr_vfd.FasDllMaxConfirmDelayOnData; // 0x7530 
		pCurVCR->FasDllMaxConfirmDelayOnConnect = m_vcr_vfd.FasDllMaxConfirmDelayOnConnect;
		::memcpy(pCurVCR->FmsFeaturesSupported, m_vcr_vfd.FmsFeaturesSupported, 8);
	//	LOG_MSG(0, "\tWrite VCR for VFD (index = %d, Local address = %d).\n",
	//		Characteristic.FirstUnconfiguredEntry, ucLocalAddr);
		lRes = fms_write(H1_VFD_MIB, Characteristic.FirstUnconfiguredEntry, 0,
            (char*)pCurVCR, sizeof(VCR_STATIC_ENTRY));
		if (lRes != FF_SUCCESS)	{
			return lRes;
		}
		::memcpy(&m_vcr_vfd, pCurVCR, sizeof(m_vcr_vfd));
		m_vfd_address = ucLocalAddr;
		FF_LOG("\tVFD VCR OD_INDEX: %d, addr=%d", Characteristic.FirstUnconfiguredEntry, m_vfd_address);
		break;
	}
	return (m_vfd_address ? FF_SUCCESS : FF_ERR_VCR_MISMATCH);
}

FF_RESULT_CODE FF_Device::get_static_info()
{
	FF_LOG("\t初始化写入 VCR ...");
	FF_RESULT_CODE iwr = init_write_vcr();
	if (iwr != FF_SUCCESS) {
		FF_LOG("\t初始化写入 VCR 失败。错误码 %d", iwr);
		return iwr;
	}
	FF_LOG("\t初始化写入 VCR 完成");

	FF_RESULT_CODE ivr = init_vfd();
	if (ivr != FF_SUCCESS) {
		return ivr;
	}

	// 读取资源块等信息
	FF_RESULT_CODE init_result = init_static_block();
	if (init_result != FF_SUCCESS) {
		close_mib();
		FF_LOG("\t读取资源块信息失败。错误码 %d", init_result);
		return init_result;
	}

	// DD 参数
	FF_RESULT_CODE get_dd_result = get_dd_params();
	if (get_dd_result != FF_SUCCESS) {
		close_mib();
		FF_LOG("\t获取DD信息失败。错误码 %d", get_dd_result);
		return get_dd_result;
	}

	return FF_SUCCESS;
}

void FF_Device::release_func_block_array()
{
	for (size_t i = 0; i < m_func_block_array.size(); i++) {
		delete m_func_block_array[i];
	}
	m_func_block_array.clear();
}

FF_RESULT_CODE FF_Device::refresh_func_block_info()
{
	uint16* vfd_dir_array = read_vfd_directory();
	if (vfd_dir_array == NULL) {
		close_vfd();
		return FF_ERR_READ_DIRECTORY_ERROR;
	}

	release_func_block_array();

	// Get function blocks info.
	uint16 nComDirIndex = vfd_dir_array[4];
	DataFormatter::reverse_bytes(&nComDirIndex, 2);
	uint16 nBlockDirIndex = vfd_dir_array[nComDirIndex + 3];
	DataFormatter::reverse_bytes(&nBlockDirIndex, 2);
	uint16 nBlockNum = vfd_dir_array[nComDirIndex + 4];
	DataFormatter::reverse_bytes(&nBlockNum, 2);

	FMS_USER_READ_CNF cnfRead;
	for (int i = 0; i < nBlockNum; i++) {
        unsigned short nBlkIndex = vfd_dir_array[nBlockDirIndex + (2 * i) - 1];
		DataFormatter::reverse_bytes(&nBlkIndex, 2);
		FF_RESULT_CODE lRes = fms_read(H1_VFD_VFD, nBlkIndex, 0, cnfRead);
		if (lRes != FF_SUCCESS) {
			close_vfd();
			delete [] vfd_dir_array;
			return lRes;
		}

		assert( cnfRead.length == sizeof(FF_BLOCK_INFO) );

		FF_Block* func_block = new FF_Block(nBlkIndex, FF_FUNC_BLOCK);
		func_block->set_info((FF_BLOCK_INFO*) cnfRead.value);
		m_func_block_array.push_back(func_block);
	}
	return FF_SUCCESS;
}

FF_RESULT_CODE FF_Device::get_block_list(H1_BLOCK_INFO* block_list, int* count)
{
	int total_count = m_func_block_array.size() + m_trans_block_array.size() + 1;
	if (*count < total_count) {
		return FF_ERR_BUFFER_TOO_SMALL;
	}
	*count = total_count;

	int idx = 0;
	m_res_block->copy_info(block_list[0]);
	idx++;

	for (size_t i = 0; i < m_trans_block_array.size(); i++) {
		m_trans_block_array[i]->copy_info(block_list[idx++]);
	}

	for (size_t i = 0; i < m_func_block_array.size(); i++) {
		m_func_block_array[i]->copy_info(block_list[idx++]);
	}

	return FF_SUCCESS;
}

FF_RESULT_CODE FF_Device::init_static_block()
{
// 	if (init_vcr(H1_VFD_VFD) == VCR_NONE) {
// 		return FF_ERR_INIT_VCR_FAILED;
// 	}
	FMS_USER_GETOD_CNF cnf_getod;
	FF_RESULT_CODE getod_result = fms_get_od(m_vcr_index_vfd, 0, cnf_getod);
	if (getod_result != FF_SUCCESS) {
		return getod_result;
	}

	memcpy(&m_od_vfd, cnf_getod.packed_object_descr, sizeof(m_od_vfd));
	DataFormatter::reverse_bytes(&m_od_vfd.first_index_s_od, 2);

	uint16* vfd_dir_array = read_vfd_directory();
	if (vfd_dir_array == NULL) {
		close_vfd();
		return FF_ERR_READ_DIRECTORY_ERROR;
	}

	// Get resource block start index
    unsigned short nComDirIndex = vfd_dir_array[4];
	DataFormatter::reverse_bytes(&nComDirIndex, 2);
    unsigned short nResBlockDirIndex = vfd_dir_array[nComDirIndex - 1];
	DataFormatter::reverse_bytes(&nResBlockDirIndex, 2);
    unsigned short nResBlockStartIndex = vfd_dir_array[nResBlockDirIndex - 1];
	DataFormatter::reverse_bytes(&nResBlockStartIndex, 2);

	if (m_res_block == NULL) {
		m_res_block = new FF_Block(nResBlockStartIndex, FF_RES_BLOCK);
	}

	// Read resource block info.
	FMS_USER_READ_CNF cnfRead;
	FF_RESULT_CODE result = fms_read(H1_VFD_VFD, nResBlockStartIndex, 0, cnfRead);
	if (result != FF_SUCCESS) {
		delete [] vfd_dir_array;
		return result;
	}
	assert(cnfRead.length == sizeof(FF_BLOCK_INFO));
	m_res_block->set_info( (FF_BLOCK_INFO*) cnfRead.value );

	// TODO: get transducer blocks info

	return FF_SUCCESS;
}

uint16* FF_Device::read_vfd_directory()
{
	if (m_od_vfd.first_index_s_od == 0) {
		assert(false);
		return NULL;
	}

	FMS_USER_READ_CNF cnf_read;
	int read_result = fms_read(H1_VFD_VFD, m_od_vfd.first_index_s_od, 0, cnf_read);
	if (read_result != FF_SUCCESS) {
		return NULL;
	}
	uint16* _array  = (uint16*)cnf_read.value;
	uint16 nDirNum = _array[2];
	DataFormatter::reverse_bytes(&nDirNum, 2);

	int total_len = (cnf_read.length / 2) * nDirNum;
	uint16* dir_array = new uint16[total_len];
	memcpy(dir_array, cnf_read.value, cnf_read.length);
	uint16 nDataLen = cnf_read.length;

	for (int i = 0; i < nDirNum; i++) {
		if (i == 0) {
			continue;
		}
		read_result = fms_read(H1_VFD_VFD, m_od_vfd.first_index_s_od + i, 0, cnf_read);
		if (read_result != FF_SUCCESS) {
			delete [] dir_array;
			return NULL;
		}
        memcpy((char*)dir_array + nDataLen, cnf_read.value, cnf_read.length);
		nDataLen += cnf_read.length;
	}
	return dir_array;
}

// Resource block DD related parameters offset.
#define RB_OFFSET_MANUFACTURER_ID	10
#define RB_OFFSET_DEVICE_TYPE		11
#define RB_OFFSET_DEVICE_REVISION	12
#define RB_OFFSET_DD_TYPE			13
FF_RESULT_CODE FF_Device::get_dd_params()
{
	if (m_res_block == NULL) {
		return FF_ERR_DEV_NOT_INITIALIZED;
	}
	uint16 res_block_index = m_res_block->get_start_index();
	assert(m_vcr_index_vfd > 0);

	FMS_USER_READ_CNF cnfRead;
	// Manufacturer ID.
	FF_RESULT_CODE result = fms_read(H1_VFD_VFD, res_block_index + RB_OFFSET_MANUFACTURER_ID, 0, cnfRead);
	if (result != FF_SUCCESS) {
		return result;
	}

	if (cnfRead.length == 4) {
		memcpy(&m_vendor_id, cnfRead.value, 4);
		DataFormatter::reverse_bytes(&m_vendor_id, 4);
	}
	else {
		assert(false);
		// N4K: m_dwManuID = FFGetManufacturerID((LPCSTR)cnfRead.value, cnfRead.length);
	}

	// Device Type.
	result = fms_read(H1_VFD_VFD, res_block_index + RB_OFFSET_DEVICE_TYPE, 0, cnfRead);
	if (result != FF_SUCCESS) {
		return result;
	}
    assert( cnfRead.length == 2 );
	memcpy(&m_device_type, cnfRead.value, 2);
	DataFormatter::reverse_bytes(&m_device_type, 2);

	// Device Revision.
	result = fms_read(H1_VFD_VFD, res_block_index + RB_OFFSET_DEVICE_REVISION, 0, cnfRead);
	if (result != FF_SUCCESS) {
		return result;
	}
    assert( cnfRead.length == 1 );
	memcpy(&m_device_version, cnfRead.value, 1);

	// DD Revision.
	result = fms_read(H1_VFD_VFD, res_block_index + RB_OFFSET_DD_TYPE, 0, cnfRead);
	if (result != FF_SUCCESS) {
		return result;
	}
    assert( cnfRead.length == 1 );
	memcpy(&m_dd_version, cnfRead.value, 1);

	return FF_SUCCESS;
}

FF_RESULT_CODE FF_Device::fms_init(uint16 vcr_index, VFD_TYPE vfd_type)
{
//	abort_vcr(vcr_index);
    const char ucFMSFeatureVFD[8] = { 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    const char ucFMSFeatureMIB[8] = { 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	FMS_INITIAL_REQ reqInitial;
	memset(&reqInitial, 0, sizeof(reqInitial));
    unsigned long _ulDLAddr = 0;
	if (vfd_type == H1_VFD_VFD) {
        assert(m_vfd_address > 0);
		_ulDLAddr = (get_addr() << 8) + m_vfd_address;
	}
	else if (vfd_type == H1_VFD_MIB) {
		_ulDLAddr = (get_addr() << 8) + 0xF8;
	}
	else {
        assert(false);
	}

	reqInitial.ulDLAddress = _ulDLAddr;
	reqInitial.access_groups = 0;
	reqInitial.password = 0;
	reqInitial.od_version = 0;
    reqInitial.protection = false;
	if (vfd_type == H1_VFD_MIB) {
		reqInitial.profile_number[0] = 'G';
		reqInitial.profile_number[1] = 'M';
		memcpy(reqInitial.fms_features_supported_calling, ucFMSFeatureMIB, 8);
	}
	else if (vfd_type == H1_VFD_VFD) {
		reqInitial.profile_number[0] = 0;
		reqInitial.profile_number[1] = 0;
		memcpy(reqInitial.fms_features_supported_calling, ucFMSFeatureVFD, 8);
	}

    char _szBuf[255];
    unsigned char _ucLen = 255;
	FF_HEADER cnfHeader;
	FF_RESULT_CODE lRes = get_parent_module()->sync_request(
		get_link()->get_index(), vcr_index,
		FMS_LAYER_ID, FMS_INITIATE, REQUEST_PRIMITIVE,
		&reqInitial, sizeof(reqInitial),
		H1IO_TIMEOUT_FMS_INITIAL, _szBuf, &_ucLen, cnfHeader);

 	// 2006-3-6 10:09, KK
 	// Close the VCR connection.
 	if (lRes == FF_ERR_DENIED_BY_INTFCARD) {
 		abort_vcr(vcr_index);
 	}

	if ( lRes == FF_ERR_NEGATIVE_ECHO) {
		return lRes; 
	}
	// TODO:
	// NCS_E_TIMEOUT
	// check timeout. abort vcr ...
	if (lRes != FF_SUCCESS) {
		// NOTE:
		// Caller should reset the VCR info.
	//	ATLTRACE("error (0x%x) \r\n", lRes);
		return lRes;
	}

	if (cnfHeader.service == FMS_ABORT) {
		// TODO: parse the returned code.
	//	ATLTRACE("error abort.\r\n ");
		return FF_ERR_DEV_ABORT;
	}

	if (cnfHeader.result != POSITIVE) {
	//	ATLTRACE("error negative.\r\n ");
		return FF_ERR_NEGATIVE_ECHO;
	}

//	ATLTRACE("OK (%d). \r\n ", nVCRIndex);
	return FF_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//
// CH1Device::FMS_Read:
//
// HISTORY:
// 2005-12-2 11:08, KK
//
// 2005-12-8 19:24, KK
//
// 2006-3-3 11:21, KK
//		Close the VCR connection. for NCS_E_H1_DENIED_BY_INTFCARD...
//
//	DESC:
//		Read device block or MIB data.
//
FF_RESULT_CODE FF_Device::fms_read(
	VFD_TYPE vfd_type,
	uint16 index,
	uint8 sub_index,
	FMS_USER_READ_CNF& cnfRead)
{
	if (index == 0xffff) {
        assert(false); // shouldn't goes here in CNCS
		switch (vfd_type)
		{
		case H1_VFD_MIB: 
			::memcpy(cnfRead.value, &m_vcr_mib, sizeof(VCR_STATIC_ENTRY));
			cnfRead.length = sizeof(VCR_STATIC_ENTRY);
			break;
		case H1_VFD_VFD: 
			::memcpy(cnfRead.value, &m_vcr_vfd, sizeof(VCR_STATIC_ENTRY));
			cnfRead.length = sizeof(VCR_STATIC_ENTRY);
			break;
		default:
			cnfRead.length = 0;
			break;
		}
		return FF_SUCCESS; 
	}

	uint16 vcr_index = 0;
	FF_RESULT_CODE init_result = init_vcr(vfd_type, vcr_index);
	if (init_result != FF_SUCCESS) {
		FF_LOG("初始化VCR失败。错误码：%d", init_result);
		return init_result;
	}
    assert(vcr_index > 0);

	FMS_USER_READ_REQ readReq;
	readReq.acc_spec.tag = FMS_ACCESS_INDEX;
#ifdef _DEBUG
	readReq.acc_spec.dummy = 0xab;
#endif // _DEBUG
	readReq.acc_spec.index = index;
	readReq.subindex = sub_index;

#define FMS_READ_RETRY_ENABLED
#ifdef FMS_READ_RETRY_ENABLED
	int retry_count = 1;
retry_read:
#endif

    char _szBuf[255];
    unsigned char _ucLen = 255;
	FF_HEADER cnfHeader;
//	UCHAR _ivkid = GetParentLink()->GetUniqueInvokeID();
	//	ATLTRACE("Read Object use VCR %d (%d)... ", nVCRIndex, _ivkid);
	FF_RESULT_CODE lRes = get_parent_module()->sync_request(
		get_link()->get_index(), vcr_index,
		FMS_LAYER_ID, FMS_READ, REQUEST_PRIMITIVE,
		&readReq, sizeof(readReq),
		H1IO_TIMEOUT_FMS_READ, _szBuf, &_ucLen, cnfHeader);

	// 2006-3-3 11:21, KK
	// Close the VCR connection.
	if (lRes == FF_ERR_DENIED_BY_INTFCARD) {
		release_vcr_connection(vfd_type);
	}
	if (lRes != FF_SUCCESS) {
#ifdef FMS_READ_RETRY_ENABLED
		// 针对模块响应超时，添加重试机制
		if (lRes == FF_ERR_CNCS_MODULE_TIMEOUT) {
			if (retry_count > 0) {
				retry_count--;
				goto retry_read;
			}
		}
#endif
		//ATLTRACE(" error(0x%x). \r\n", lRes);
		return lRes;
	}
	if (cnfHeader.service == FMS_ABORT) {
		//	ATLTRACE(" error abort (%d). \r\n", _ivkid);
		release_vcr_connection(vfd_type);
		return FF_ERR_DEV_ABORT;
	}
	if (cnfHeader.result != POSITIVE) {
		//	ATLTRACE(" error negative. \r\n");
		return FF_ERR_NEGATIVE_ECHO;
	}

	memcpy(&cnfRead, _szBuf, _ucLen);
    assert(cnfRead.length == (_ucLen - 2));
	return FF_SUCCESS;
}

FF_RESULT_CODE FF_Device::fms_write(
	VFD_TYPE vfd_type,
	uint16 index,
	uint8 sub_index,
	char* data,
	uint8 data_len)
{
    assert(data != NULL);
    assert(data_len > 0);
    assert(data_len <= MAX_DATA_FIELD_LEN);

	uint16 vcr_index = 0;
	FF_RESULT_CODE init_result = init_vcr(vfd_type, vcr_index);
	if (init_result != FF_SUCCESS) {
		FF_LOG("初始化VCR失败。错误码：%d", init_result);
		return init_result;
	}
    assert(vcr_index > 0);

	FMS_USER_WRITE_REQ reqWrite;
	reqWrite.acc_spec.tag = FMS_ACCESS_INDEX;
	reqWrite.acc_spec.index = index;
	reqWrite.subindex = sub_index;
	reqWrite.length = data_len;
	memcpy(reqWrite.value, data, data_len);

    char _szBuf[255];
    unsigned char _ucLen = 255;
	FF_HEADER cnfHeader;
	FF_RESULT_CODE lRes = get_parent_module()->sync_request(
		get_link()->get_index(), vcr_index,
		FMS_LAYER_ID, FMS_WRITE, REQUEST_PRIMITIVE,
		&reqWrite, sizeof(reqWrite),
		H1IO_TIMEOUT_FMS_WRITE, _szBuf, &_ucLen, cnfHeader);

	// Close the VCR connection.
	if (lRes == FF_ERR_DENIED_BY_INTFCARD) {
		release_vcr_connection(vfd_type);
	}

	if (lRes != FF_SUCCESS) {
		return lRes;
	}

	if (cnfHeader.service == FMS_ABORT) {
		release_vcr_connection(vfd_type);
		return FF_ERR_DEV_ABORT;
	}

	if (cnfHeader.result != POSITIVE) {
		return FF_ERR_NEGATIVE_ECHO;
	}

	return FF_SUCCESS;
}

FF_RESULT_CODE FF_Device::fms_get_od(
	uint16 vcr_index,
	uint16 index,
	FMS_USER_GETOD_CNF& cnfGetOD)
{
	FMS_USER_GETOD_REQ reqGetOD;
	reqGetOD.acc_spec.tag = FMS_INDEX_ACCESS;
	reqGetOD.acc_spec.index = index;
    reqGetOD.format = false; // short format

    char szBuf[255];
    unsigned char ucBufLen = 255;

	FF_HEADER cnfHeader;
	FF_RESULT_CODE lRes = get_parent_module()->sync_request(
		get_link()->get_index(), vcr_index,
		FMS_LAYER_ID, FMS_GETOD, REQUEST_PRIMITIVE,
		&reqGetOD, sizeof(reqGetOD),
		H1IO_TIMEOUT_FMS_GETOD, szBuf, &ucBufLen, cnfHeader);
	
	if (lRes != FF_SUCCESS) {
		return lRes;
	}
	if (cnfHeader.service == FMS_ABORT) {
		return FF_ERR_DEV_ABORT;
	}
	if (cnfHeader.result != POSITIVE) {
		return FF_ERR_NEGATIVE_ECHO;
	}

	memcpy(&cnfGetOD, szBuf, sizeof(cnfGetOD));

	return FF_SUCCESS;
}

FF_RESULT_CODE FF_Device::fms_download_init_sequence(uint16 domain_index)
{
	uint16 mib_vcr_index = 0;
	FF_RESULT_CODE init_result = init_vcr(H1_VFD_MIB, mib_vcr_index);
	if (init_result != FF_SUCCESS) {
		FF_LOG("初始化VCR失败。错误码：%d", init_result);
		return init_result;
	}
    assert(mib_vcr_index > 0);

	FMS_USER_GEN_INIT_DOWNLOAD_REQ reqGenInitDL;
	memset(&reqGenInitDL, 0, sizeof(reqGenInitDL));
	reqGenInitDL.acc_spec.tag = 0;
	reqGenInitDL.acc_spec.index = domain_index;

    char _szBuf[255];
    unsigned char _ucLen = 255;
	FF_HEADER cnfHeader;
	FF_RESULT_CODE lRes = get_parent_module()->sync_request(
		get_link()->get_index(), mib_vcr_index,
		FMS_LAYER_ID, FMS_GENERIC_INITIATE_DOWNLOAD_SEQUENCE, REQUEST_PRIMITIVE,
		&reqGenInitDL, sizeof(reqGenInitDL),
		H1IO_TIMEOUT_FMS_GEN_INIT_DL,
		_szBuf, &_ucLen, cnfHeader);
	if (lRes != FF_SUCCESS) {
		return lRes;
	}
	if (cnfHeader.service == FMS_ABORT) {
		// TODO: parse the returned code.
		return FF_ERR_DEV_ABORT;
	}
	if (cnfHeader.result != POSITIVE) {
		return FF_ERR_NEGATIVE_ECHO;
	}
	return FF_SUCCESS;
}

FF_RESULT_CODE FF_Device::fms_download_terminate_sequence(uint16 domain_index)
{
	uint16 mib_vcr_index = 0;
	FF_RESULT_CODE init_result = init_vcr(H1_VFD_MIB, mib_vcr_index);
	if (init_result != FF_SUCCESS) {
		FF_LOG("初始化VCR失败。错误码：%d", init_result);
		return init_result;
	}
    assert(mib_vcr_index > 0);

	FMS_USER_GEN_TERM_DOWNLOAD_REQ reqGenTermDL;
	memset(&reqGenTermDL, 0, sizeof(reqGenTermDL));
	reqGenTermDL.acc_spec.tag = 0;
	reqGenTermDL.acc_spec.index = domain_index;

    char _szBuf[255];
    unsigned char _ucLen = 255;
	FF_HEADER cnfHeader;
	FF_RESULT_CODE lRes = get_parent_module()->sync_request(
		get_link()->get_index(), mib_vcr_index,
		FMS_LAYER_ID, FMS_GENERIC_TERMINATE_DOWNLOAD_SEQUENCE, REQUEST_PRIMITIVE,
		&reqGenTermDL, sizeof(reqGenTermDL),
		H1IO_TIMEOUT_FMS_GEN_TERM_DL,
		_szBuf, &_ucLen, cnfHeader);
	if (lRes != FF_SUCCESS) {
		return lRes;
	}
	if (cnfHeader.service == FMS_ABORT) {
		// TODO: parse the returned code.
		return FF_ERR_DEV_ABORT;
	}
	if (cnfHeader.result != POSITIVE) {
		return FF_ERR_NEGATIVE_ECHO;
	}
	return FF_SUCCESS;
}

FF_RESULT_CODE FF_Device::fms_download_segment(
	uint16 domain_index,
	uint8 more_follow,
	char* data,
	uint8 data_len)
{
	int res = init_mib();
	if (res != FF_SUCCESS) {
		return (FF_RESULT_CODE)res;
	}
	assert(data_len <= MAX_DATA_FIELD_LEN);

	FMS_USER_GEN_DOWNLOAD_SEG_REQ reqGenDLSeg;
	memset(&reqGenDLSeg, 0, sizeof(reqGenDLSeg));
	reqGenDLSeg.acc_spec.tag = 0;
	reqGenDLSeg.acc_spec.index = domain_index;
	reqGenDLSeg.more_follows = more_follow;
	reqGenDLSeg.data_len = data_len;
	memcpy(&reqGenDLSeg.data, data, data_len);

    char _szBuf[255];
    unsigned char _ucLen = 255;
	FF_HEADER cnfHeader;
	FF_RESULT_CODE lRes = get_parent_module()->sync_request(
		get_link()->get_index(), m_vcr_index_mib,
		FMS_LAYER_ID, FMS_GENERIC_DOWNLOAD_SEGMENT, REQUEST_PRIMITIVE,
		&reqGenDLSeg, sizeof(reqGenDLSeg),
		H1IO_TIMEOUT_FMS_GEN_DL_SEG,
		_szBuf, &_ucLen, cnfHeader);

	if (lRes != FF_SUCCESS) {
		return lRes;
	}
	if (cnfHeader.service == FMS_ABORT) {
		// TODO: parse the returned code.
		return FF_ERR_DEV_ABORT;
	}
	if (cnfHeader.result != POSITIVE) {
		return FF_ERR_NEGATIVE_ECHO;
	}
	return FF_SUCCESS;
}

bool FF_Device::is_connected()
{
	return (m_vcr_index_mib != 0 || m_vcr_index_vfd != 0);
}

FF_RESULT_CODE FF_Device::init_vcr(VFD_TYPE vt, uint16& vcr_index)
{
	if (vt == H1_VFD_VFD) {
		FF_RESULT_CODE res = init_vfd();
		if (res != FF_SUCCESS) {
			return res;
		}
		vcr_index = m_vcr_index_vfd;
	} else if (vt == H1_VFD_MIB) {
		FF_RESULT_CODE res = init_mib();
		if (res != FF_SUCCESS) {
			return res;
		}
		vcr_index = m_vcr_index_mib;
	} else {
        assert(false);
	}
	return FF_SUCCESS;
}

void FF_Device::reset_vcr()
{
	m_vcr_index_mib = 0;
	m_vcr_index_vfd = 0;
}

bool FF_Device::reset_vcr(uint16 index)
{
	if (m_vcr_index_mib == index) {
		m_vcr_index_mib = 0;
		return true;
	} else if (m_vcr_index_vfd == index) {
		m_vcr_index_vfd = 0;
		return true;
	}
	return false;
}

// 2006-3-6 10:35, KK
void FF_Device::abort_vcr(uint16 vcr_index)
{
    long lRes = get_link()->FMS_Abort(vcr_index);
	get_link()->release_user_vcr(vcr_index);
}

void FF_Device::release_vcr_connection(VFD_TYPE vt)
{
	if (vt == H1_VFD_ALL || vt == H1_VFD_MIB) {
		close_mib();
	}

	if (vt == H1_VFD_ALL || vt == H1_VFD_VFD) {
		close_vfd();
	}
}

int FF_Device::close_mib()
{
	if (m_vcr_index_mib == 0) { // not connected
		return FF_SUCCESS;
	}
	abort_vcr(m_vcr_index_mib);
	// reset MIB VCR index ...
	m_vcr_index_mib = 0;
	return FF_SUCCESS;
}

int FF_Device::close_vfd()
{
	if (m_vcr_index_vfd == 0) { // not connected
		return FF_SUCCESS;
	}
	abort_vcr(m_vcr_index_vfd);
	// reset VFD VCR index ...
	m_vcr_index_vfd = 0;
	return FF_SUCCESS;
}

FF_RESULT_CODE FF_Device::config_vcr(uint16 vi, bool is_vfd, bool resp) // TODO: RESP?
{
	// 接口卡设备中用于和仪表通信的C/S VCR，由接口卡自行初始化，无需上位机配置。
    assert(false);

	// Set adaptive parameters 2014-12-02 
	VCR_STATIC_ENTRY& vcr_entry = (is_vfd ? m_vcr_vfd : m_vcr_mib); 
	uint16 max_dlsdu_size = (is_vfd ? m_vfd_max_dlsdu_size: m_mib_max_dlsdu_size);

	FMS_USER_READ_CNF readCnf;
	uint16 vcr_index = vi + get_link()->vcr_start_idx();
	VCR_STATIC_ENTRY* pVCR = (VCR_STATIC_ENTRY*)readCnf.value;
	FF_RESULT_CODE lRes = get_link()->IntfDev_Read(vcr_index, 0, readCnf);
	if ( lRes != FF_SUCCESS) {
		return lRes; 
	}

	if (resp) { // TODO: ??
		/*
		if ( (1 != m_InitResp.ucError) && (2 != m_InitResp.ucError) )
		{
			return S_FALSE; // DON'T known how to handle the error 
		}
		USHORT usMaxPduSize = 
			max(m_InitResp.ucSendPduSize, m_InitResp.ucRecvPduSize) + 1;
		fmtBytesReverse(&usMaxPduSize, sizeof(usMaxPduSize));
		if ((0 == usMaxPduSize) || 
			(pVCR->FasDllMaxDlsduSize == usMaxPduSize) &&
			(memcmp(pVCR->FmsFeaturesSupported, m_InitResp.ucFeatures, 8) == 0) )
		{
			return NCS_E_OK; 
		}
		pVCR->FasDllMaxDlsduSize = usMaxPduSize;
		memcpy(pVCR->FmsFeaturesSupported, m_InitResp.ucFeatures, 8); 
		*/
	}
	else if ( vcr_entry.FasArTypeAndRole != 0x32 ) {
		// 2015-06-17 cache FasDllMaxDlsduSize 
        static unsigned short TEST_DLSDU_SIZE = 0x8000; //fmtBytesReverse(0x0080)
		if ( max_dlsdu_size == 0 ) {
			if ( pVCR->FasDllMaxDlsduSize == TEST_DLSDU_SIZE ) {
				vcr_entry.FasDllMaxDlsduSize = TEST_DLSDU_SIZE; 
				return FF_SUCCESS; 
			}
			pVCR->FasDllMaxDlsduSize = TEST_DLSDU_SIZE; 
		}
		else if ( pVCR->FasDllMaxDlsduSize == max_dlsdu_size ) {
			return FF_SUCCESS; 
		}
		else {
			pVCR->FasDllMaxDlsduSize = max_dlsdu_size; 
		}
	}
	else if ((pVCR->FasDllMaxDlsduSize != vcr_entry.FasDllMaxDlsduSize) ||
		(pVCR->FasDllMaxConfirmDelayOnData != vcr_entry.FasDllMaxConfirmDelayOnData) || 
		(pVCR->FasDllMaxConfirmDelayOnConnect != vcr_entry.FasDllMaxConfirmDelayOnConnect) )
	{
		pVCR->FasDllMaxDlsduSize = vcr_entry.FasDllMaxDlsduSize;
		pVCR->FasDllMaxConfirmDelayOnData = vcr_entry.FasDllMaxConfirmDelayOnData;
		pVCR->FasDllMaxConfirmDelayOnConnect = vcr_entry.FasDllMaxConfirmDelayOnConnect;
	}
	else {
		return FF_SUCCESS; 
	}
	lRes = get_link()->IntfDev_Write(
        vcr_index, 0, (char*)readCnf.value, (unsigned char)readCnf.length);
	if ( lRes != FF_SUCCESS) {
		return lRes; 
	}
	vcr_entry.FasDllMaxDlsduSize = pVCR->FasDllMaxDlsduSize; 

	return FF_SUCCESS;
}

FF_RESULT_CODE FF_Device::init_mib()
{
	if (m_vcr_index_mib > 0) {
		return FF_SUCCESS;
	}

	uint16 vcr_index = get_link()->get_user_vcr();

	/*
	// 2015-06-17 set default FasDllMaxDlsduSize = 0x80 
	if ((m_mib_max_dlsdu_size > 0) &&
		(m_mib_max_dlsdu_size == m_vcr_mib.FasDllMaxDlsduSize))
	{
		FF_RESULT_CODE __result = FF_SUCCESS;
		if ((__result = config_vcr(vcr_index, false)) != FF_SUCCESS) {
			return __result;
		}
	}
	*/

	FF_LOG("\tFMS Init...");
    assert(vcr_index >= 3 && vcr_index <= 63);


	int try_count = FFCommManager::fms_init_retry_count();
retry_init:
	FF_RESULT_CODE res = fms_init(vcr_index, H1_VFD_MIB);
	if (res != FF_SUCCESS) {
		FF_LOG("\tFMS Init失败。错误码：%d", res);
		if (try_count > 0) {
			FF_LOG("\t重试...");
			try_count--;
			goto retry_init;
		} else {
			get_link()->release_user_vcr(vcr_index);
			return res;
		}
	}
	FF_LOG("\tFMS Init完成。");

	m_vcr_index_mib = vcr_index;
	m_mib_max_dlsdu_size = m_vcr_mib.FasDllMaxDlsduSize;

	return FF_SUCCESS;
}

FF_RESULT_CODE FF_Device::init_vfd()
{
	if (m_vcr_index_vfd > 0) {
		return FF_SUCCESS;
	}

	if (m_vfd_address == 0) {
		FF_RESULT_CODE iwvr = init_write_vcr();
		if (iwvr != FF_SUCCESS) {
			return iwvr;
		}
	}

	uint16 vcr_index = get_link()->get_user_vcr();
	/*
	FF_RESULT_CODE res = config_vcr(vcr_index, true);
	if (res != FF_SUCCESS) {
		return res;
	}
	*/

	int try_count = FFCommManager::fms_init_retry_count();
retry_init:
	FF_RESULT_CODE res = fms_init(vcr_index, H1_VFD_VFD);
	if (res != FF_SUCCESS) {
		if (try_count > 0) {
			FF_LOG("\t重试...");
			try_count--;
			goto retry_init;
		} else {
			get_link()->release_user_vcr(vcr_index);
			return res;
		}
	}

	m_vcr_index_vfd = vcr_index;
	m_vfd_max_dlsdu_size = m_vcr_vfd.FasDllMaxDlsduSize; // TODO: ??

	return FF_SUCCESS;
}

