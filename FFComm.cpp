// FFComm.cpp : Defines the exported functions for the DLL application.
//

#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <stdarg.h>
#include <algorithm>

#include "CNCSCommClient.h"
#include "MIB.H"
#include "FFComm.h"
#include "FFCommManager.h"

// #define SIM_COMM_MODE

// FFCOMM_API void ff_comm_init(
// 	FF_COMM_TYPE type)
// {
// 	if (FFCommManager::inst().get_comm_type() != COMM_UNDEFINED) {
// 	//	assert(FALSE);
// 	}
// 	FFCommManager::inst().set_comm_type(type);
// }

void* ff_open_h1module_cncs(
	uint8 dpu_addr,
	uint16 module_index,
	uint8 net1,
	uint8 net2,
	CNCS_BTYPE bb_type,
	uint16 udp_port,
	bool redundant_enabled
	)
{
	return FFCommManager::inst().open_module(dpu_addr, module_index, net1, net2, bb_type, udp_port, redundant_enabled);
}

FF_MODULE_HANDLE ff_open_h1module_rs232(
	int port_index,
	uint32 baud,
	uint8 data_bits,
	uint8 parity,
	uint8 stop_bits)
{
//	assert(FFCommManager::inst().get_comm_type() == COMM_RS232);
	return FFCommManager::inst().open_module(port_index, baud, data_bits, parity, stop_bits);
}

FF_RESULT_CODE ff_init_device(
	FF_MODULE_HANDLE ff_module_handle,
	FF_LINK_INDEX link_index,
	uint8 dev_addr,
	uint32 option)
{
	FF_H1Module* module = FFCommManager::inst().get_module(ff_module_handle);
	if (module == NULL) {
		FF_LOG("获取模块失败。");
		return FF_ERR_INVALID_MODULE_HANDLE;
	}

	FF_H1Link* link_ob = module->get_link(link_index);
	assert(link_ob != NULL);

	return link_ob->init_dev(dev_addr, option);
}

FF_RESULT_CODE ff_get_device_list_2(
	FF_MODULE_HANDLE ff_module_handle,
	FF_LINK_INDEX link_index,
	H1_DEV_INFO* dev_array,
	int* count,
	GET_DEV_RANGE range,
	GET_DEV_OPTION opt
	)
{
	FF_LOG("刷新网段 %d 设备...", link_index + 1);
	FF_H1Module* module = FFCommManager::inst().get_module(ff_module_handle);
	if (module == NULL) {
		FF_LOG("获取模块失败。");
		return FF_ERR_INVALID_MODULE_HANDLE;
	}

	FF_H1Link* link_ob = module->get_link(link_index);
	assert(link_ob != NULL);

	vector<H1_DEV_INFO> dev_vector;
	FF_RESULT_CODE res = link_ob->get_dev_list(dev_vector, range, opt);
	if (res != FF_SUCCESS) {
		return res;
	}

	if (*count < (int)dev_vector.size()) {
		return FF_ERR_BUFFER_TOO_SMALL;
	}

	for (size_t i = 0; i < dev_vector.size(); i++) {
		dev_array[i] = dev_vector[i];
	}

	*count = dev_vector.size();

	return FF_SUCCESS;
}

#if 0

FFCOMM_API FF_RESULT_CODE ff_get_device_list(
	FF_MODULE_HANDLE ff_module_handle,
	FF_LINK_INDEX link_index,
	H1_DEV_INFO* dev_array,
	int* count,
	GET_DEV_LIST_OPTION opt
	)
{
#ifdef SIM_COMM_MODE
#pragma warning(disable : 4996)

	for (int i = 0; i < 5; i++) {
		dev_array[i].addr = 0x14 + i * 2;
		sprintf(dev_array[i].pdtag, "TEST_H1DEV_%04d", i);
		sprintf(dev_array[i].device_id, "MC-IF-%04x", i);
		dev_array[i].manu_id = 0x105;
		dev_array[i].dev_type = 1;
		dev_array[i].dev_ver = 1;
		dev_array[i].dd_ver = 1;
	}
	*count = 5;
	return FF_SUCCESS;
#else
	FF_LOG("刷新网段 %d 设备...", link_index + 1);
	FF_H1Module* module = FFCommManager::inst().get_module(ff_module_handle);
	if (module == NULL) {
		return FF_ERR_INVALID_MODULE_HANDLE;
	}

	FF_H1Link* link_ob = module->get_link(link_index);
	assert(link_ob != NULL);

	if (opt != GDLO_CACHE_ONLY) {
		FF_LOG("初始化网段...");
		FF_RESULT_CODE result = link_ob->init_link();
		if (result != FF_SUCCESS) {
			FF_LOG("初始化网段失败，错误码：%d", result);
			return result;
		}
		FF_LOG("初始化网段完成");
	}

	vector<H1_DEV_INFO> dev_vector;
	FF_RESULT_CODE res = link_ob->get_dev_list(dev_vector, opt);
	if (res != FF_SUCCESS) {
		return res;
	}

	if (*count < (int)dev_vector.size()) {
		return FF_ERR_BUFFER_TOO_SMALL;
	}

	for (size_t i = 0; i < dev_vector.size(); i++) {
		dev_array[i] = dev_vector[i];
	}

	*count = dev_vector.size();

	return FF_SUCCESS;
#endif
}
#endif

static bool _is_interface_device_id(const char* dev_id)
{
	return ((dev_id == NULL) || (strlen(dev_id) == 0) ||
		memcmp(dev_id, INTERFACE_DEV_ID, FFH1_DEVID_LEN) == 0);
}

FF_RESULT_CODE ff_read_object(
	FF_MODULE_HANDLE ff_module_handle, // NULL means default unique module
	FF_LINK_INDEX link_index,
	const char* device_id,
	VFD_TYPE vfd_type,
	uint16 index,
	uint16 sub_index,
	char* data_buf_ptr, // out
	int* data_len // in, out
	)
{
	FF_H1Module* module = FFCommManager::inst().get_module(ff_module_handle);
	if (module == NULL) {
		return FF_ERR_INVALID_MODULE_HANDLE;
	}

	FF_H1Link* link_ob = module->get_link(link_index);
	assert(link_ob != NULL);

	FMS_USER_READ_CNF cnfRead;
	memset(&cnfRead, 0, sizeof(cnfRead));
	if (_is_interface_device_id(device_id)) { // interface device
		FF_RESULT_CODE res = link_ob->IntfDev_Read(index, (uint8)sub_index, cnfRead);
		if (res != FF_SUCCESS) {
			return res;
		}
	}
	else {
		FF_Device* dev = link_ob->find_dev(device_id);
		if (dev == NULL) {
			return FF_ERR_DEV_NOT_FOUND;
		}
		int rres = dev->fms_read(vfd_type, index, (uint8)sub_index, cnfRead);
		if (rres != FF_SUCCESS) {
			return (FF_RESULT_CODE)rres;
		}
	}
	if (*data_len < cnfRead.length) {
		*data_len = cnfRead.length; // indicate the correct length
		return FF_ERR_BUFFER_TOO_SMALL;
	}
	*data_len = cnfRead.length;
	memcpy(data_buf_ptr, cnfRead.value, cnfRead.length);

	return FF_SUCCESS;
}

FF_RESULT_CODE ff_write_object(
	FF_MODULE_HANDLE ff_module_handle,
	FF_LINK_INDEX link_index,
	const char* device_id,
	VFD_TYPE vfd_type,
	uint16 index,
	uint16 sub_index,
	char* data_buf_ptr, // in
	int data_len // in
	)
{
	if (data_len > MAX_DATA_FIELD_LEN) {
		return FF_ERR_DATA_TOO_LONG;
	}

	FF_H1Module* module = FFCommManager::inst().get_module(ff_module_handle);
	if (module == NULL) {
		return FF_ERR_INVALID_MODULE_HANDLE;
	}

	FF_H1Link* link_ob = module->get_link(link_index);
	assert(link_ob != NULL);

	if (_is_interface_device_id(device_id)) { // interface device
		FF_RESULT_CODE res = link_ob->IntfDev_Write(index, (uint8)sub_index, data_buf_ptr, data_len);
		if (res != FF_SUCCESS) {
			return res;
		}
	} else {
		FF_Device* dev = link_ob->find_dev(device_id);
		if (dev == NULL) {
			return FF_ERR_DEV_NOT_FOUND;
		}
		int rres = dev->fms_write(vfd_type, index, (uint8)sub_index, data_buf_ptr, data_len);
		if (rres != FF_SUCCESS) {
			return (FF_RESULT_CODE)rres;
		}
	}

	return FF_SUCCESS;
}

FF_RESULT_CODE ff_clear_dev_addr(
	FF_MODULE_HANDLE ff_module_handle,
	FF_LINK_INDEX link_index,
	const char* device_id,
	const char* device_pdtag,
	uint8 addr
	)
{
	FF_H1Module* module = FFCommManager::inst().get_module(ff_module_handle);
	if (module == NULL) {
		return FF_ERR_INVALID_MODULE_HANDLE;
	}

	FF_H1Link* link_ob = module->get_link(link_index);
	assert(link_ob != NULL);

	FF_Device* dev = link_ob->find_dev(device_id);
	if (dev == NULL) {
		// TODO: 单独检测此设备, identify...
		return FF_ERR_DEV_NOT_FOUND;
	}
	dev->release_vcr_connection(H1_VFD_ALL);
	return link_ob->SM_ClearAddress(addr, device_id, device_pdtag);
}

FF_RESULT_CODE ff_set_dev_addr(
	FF_MODULE_HANDLE ff_module_handle,
	FF_LINK_INDEX link_index,
	const char* device_pdtag,
	uint8 addr
	)
{
	FF_H1Module* module = FFCommManager::inst().get_module(ff_module_handle);
	if (module == NULL) {
		return FF_ERR_INVALID_MODULE_HANDLE;
	}

	FF_H1Link* link_ob = module->get_link(link_index);
	assert(link_ob != NULL);

	char pdtag_buf[32];
	memset(pdtag_buf, ' ', 32);
    memcpy(pdtag_buf, device_pdtag, strlen(device_pdtag));

	return link_ob->SM_SetAddress(addr, pdtag_buf);
}


FF_RESULT_CODE ff_set_dev_pdtag(
	FF_MODULE_HANDLE ff_module_handle,
	FF_LINK_INDEX link_index,
	const char* device_id,
	const char* device_pdtag,
	uint8 addr,
	bool is_clear
	)
{
	FF_H1Module* module = FFCommManager::inst().get_module(ff_module_handle);
	if (module == NULL) {
		return FF_ERR_INVALID_MODULE_HANDLE;
	}

	FF_H1Link* link_ob = module->get_link(link_index);
	assert(link_ob != NULL);

	return link_ob->SM_SetPdTag(addr, device_id, device_pdtag, is_clear);
}

FF_RESULT_CODE ff_download_to_domain(
	FF_MODULE_HANDLE ff_module_handle,
	FF_LINK_INDEX link_index,
	const char* device_id,
	uint16 domain_index,
	const char* data,
	uint16 data_len
	)
{
	FF_H1Module* module = FFCommManager::inst().get_module(ff_module_handle);
	if (module == NULL) {
		return FF_ERR_INVALID_MODULE_HANDLE;
	}

	FF_H1Link* link_ob = module->get_link(link_index);
	assert(link_ob != NULL);

	FF_Device* dev = NULL;
	if (device_id != NULL) {
		dev = link_ob->find_dev(device_id);
		if (dev == NULL) {
			return FF_ERR_DEV_NOT_FOUND;
		}
	}

	if (dev != NULL) {
		FF_RESULT_CODE res = dev->fms_download_init_sequence(domain_index);
		if (res != FF_SUCCESS) {
			return res;
		}
	}

	uint16 left_bytes = data_len;
	char* data_ptr = (char*)data;
	while (left_bytes > 0) {
        int packet_data_len = min(left_bytes, (uint16)MAX_DATA_FIELD_LEN);
		uint8 more_follow = (left_bytes > packet_data_len);
		// interface device
		if (dev == NULL) {
			FF_RESULT_CODE res = link_ob->IntfDev_DownloadSegment(
				domain_index, more_follow, data_ptr, (uint8)packet_data_len);
			if (res != FF_SUCCESS) {
				return res;
			}
		} else {
			FF_RESULT_CODE res = dev->fms_download_segment(domain_index, more_follow, data_ptr, packet_data_len);
			if (res != FF_SUCCESS) {
				return res;
			}
		}
		left_bytes -= packet_data_len;
		data_ptr += packet_data_len;
	}

	if (dev != NULL) {
		FF_RESULT_CODE res = dev->fms_download_terminate_sequence(domain_index);
		if (res != FF_SUCCESS) {
			return res;
		}
	}

	return FF_SUCCESS;
}

// N4K: CH1Device::RefreshBlockList
FF_RESULT_CODE ff_get_block_list(
	FF_MODULE_HANDLE module_handle,
	FF_LINK_INDEX link_index,
	const char* device_id,
	H1_BLOCK_INFO* block_array,
	int* count,
	GET_BLOCK_LIST_OPTION opt
	)
{
	FF_H1Module* module = FFCommManager::inst().get_module(module_handle);
	if (module == NULL) {
		return FF_ERR_INVALID_MODULE_HANDLE;
	}

	FF_H1Link* link_ob = module->get_link(link_index);
	assert(link_ob != NULL);

	FF_Device* dev = NULL;
	if (device_id != NULL) {
		dev = link_ob->find_dev(device_id);
		if (dev == NULL) {
			return FF_ERR_DEV_NOT_FOUND;
		}
	}

	if (opt == GBLO_REFRESH_ALL) {
		FF_RESULT_CODE res = dev->refresh_func_block_info();
		if (res != FF_SUCCESS) {
			return res;
		}
	}

	return dev->get_block_list(block_array, count);
}

static QUERY_CANCEL_FLAG g_query_cancel_pfn = NULL;
void ff_set_query_cancel_func(QUERY_CANCEL_FLAG pfn)
{
	g_query_cancel_pfn = pfn;
}

bool ff_query_cancel_flag()
{
	if (g_query_cancel_pfn == NULL) {
		return false;
	}
	return (*g_query_cancel_pfn)();
}

static FF_LOG_INFO g_local_log_pfn = NULL;
static void std_log(const char* info)
{
	printf(info);
}

void ff_set_log_func(FF_LOG_INFO pfn)
{
	g_local_log_pfn = pfn;
}

void ff_log_info(LOG_OPTION opt, const char* fmt, ...)
{
	static string sLogFile;
	if ( sLogFile.empty() )
	{
        char szBuf[MAX_PATH];
//		GetModuleFileName(NULL, szBuf, MAX_PATH - 1);
//		sLogFile = szBuf;
//		int iPos = sLogFile.rfind('\\');
//		sLogFile.resize(iPos + 1);
//		sLogFile += "Log\\";
//		if ( !PathFileExists(sLogFile.c_str()) )
//			CreateDirectory(sLogFile.c_str(), NULL);
		
        time_t  st;
        time(&st);
        tm* sst= localtime(&st);
		sprintf(szBuf, "H1Config_LOG_%04d%02d%02d%02d%02d%02d.log", 
            sst->tm_year+1900, sst->tm_mon, sst->tm_mday, sst->tm_hour, sst->tm_min, sst->tm_sec);
		sLogFile += szBuf;
	}

	FILE* fp = fopen(sLogFile.c_str(), "a+");
	if (fp == NULL)
		return;

	fseek(fp, 0, SEEK_END);
	int nFileLen = ftell(fp);
	if ( nFileLen > 0x800000 )
	{
		sLogFile.clear(); // generate new file name next time 
	}
	const int BUF_LEN = 1024;
	char log_buf[BUF_LEN];
	int offset = 0;

	if (opt != LOG_NO_TIMESTAMP) {
        timeval   sys_time;
        gettimeofday(&sys_time,NULL);

		sprintf(log_buf, "%02d:%02d:%02d.%03d ", 
            sys_time.tv_sec/3600, sys_time.tv_sec/60, sys_time.tv_sec, sys_time.tv_usec/1000);
		offset = strlen(log_buf);
	}

	va_list arglist;
	va_start(arglist, fmt);
    int ret = snprintf(log_buf + offset, BUF_LEN - offset, fmt, arglist);
	va_end(arglist);

	strcat(log_buf, "\r\n");

	fprintf(fp, "%s", log_buf);
	fclose(fp);

	if (g_local_log_pfn == NULL) {
		// 用户未设置日志函数，创建控制台用于输出
	//	if (::GetStdHandle(STD_OUTPUT_HANDLE) == INVALID_HANDLE_VALUE) {
            //::AllocConsole();
            ::system("");
			freopen("CONOUT$", "w+t", stdout);
			freopen("CONIN$", "r+t", stdin);
	//	}
		g_local_log_pfn = std_log;
	}

	(*g_local_log_pfn)(log_buf);
}
