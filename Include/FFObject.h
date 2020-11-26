//
// FFObject.h
//

#ifndef __FF_OBJECT_H__
#define __FF_OBJECT_H__

#include "FFComm.h"
#include "DDLibrary.h"

FFOB_API FF_RESULT_CODE ff_map_dev(
	FF_MODULE_HANDLE module_handle,
	VirtualDevice* vdev
	);

FFOB_API FF_RESULT_CODE ff_download_link(
	FF_MODULE_HANDLE ff_module_comm_handle,
	int link_index,
	const char* prj_file_name
	);

FFOB_API FF_RESULT_CODE ff_download_dev(
	FF_MODULE_HANDLE ff_module_comm_handle,
	int link_index,
	VirtualDevice* dl_virtual_device,
	const char* prj_file_name);

FFOB_API FF_RESULT_CODE cncs_store_module_cfg_data(
	FF_MODULE_HANDLE ff_module_handle,
	char* cfg_data,
	uint32 data_len
	);

FFOB_API FF_RESULT_CODE cncs_load_module_cfg_data(
	FF_MODULE_HANDLE ff_module_handle,
	char** cfg_data_buf,
	uint32* data_len
	);

FFOB_API FF_RESULT_CODE cncs_release_data_buffer(
	char* cfg_data_buf
	);

#endif // __FF_OBJECT_H__
