#pragma once
#include "afx.h"
#include <vector>
using namespace std;

#include "DDSWrap.h"
#include "DDLibrary.h"
#include "hvwwin.h"		// HyperView++
#include "HobBrdr.h"
#include "MIB.H"
// using namespace ffob_dd;
#include "FFComm.h"

// #define DL_FF211_ONLY

#define CHECK_CANCEL 		if (ff_query_cancel_flag()) { goto LABEL_CANCEL; }
#define DECL_CANCEL_LABEL \
	LABEL_CANCEL: \
	FF_LOG(_T("网段下载已取消。 ")); \
	return FF_ERR_USER_CANCEL;

namespace ffob_cfg
{
	#define DECL_POB_TYPE(x) virtual POB_TYPE get_ob_type() { return x; }

	enum POB_TYPE
	{
		POB_NONE = 0,
		POB_BLOCK,
		POB_VIRTUAL_BLOCK,
		POB_PHYSICAL_BLOCK,
		POB_DEVICE,
		POB_VIRTUAL_DEVICE,
		POB_PHYSICAL_DEVICE,
		POB_INTERFACE_DEVICE,
		POB_LINK_APP,
		POB_LINK_SCHEDULE,
		POB_PHYSICAL_NETWORK,
		POB_H1_LINK,
		POB_H1_MODULE,
		POB_BLOCK_PARAMETER,
	};

	class FFDevice;
	class VirtualDevice;
	class LinkApplication;
	class H1LinkOb;
	class CBlockOb;

	class CfgData_Link;

	class FFOB_CLASS CfgObjectBase : public CObject
	{
	public:
		CfgObjectBase(void);
		~CfgObjectBase(void);
		void set_name(CString n) { m_name = n; }
		const CString& get_name() { return m_name; }

		void set_short_name(CString n) { m_short_name = n; }
		const CString& get_short_name() 
		{ 
			if(!m_short_name.IsEmpty())
				return m_short_name; 
			else
			{
				CString name = m_name;
				int index = name.ReverseFind(_T('-'));
				m_short_name = name.Right(name.GetLength() - index - 2);
				return m_short_name;
			}
		}

		virtual void Serialize(CArchive &ar);
		virtual POB_TYPE get_ob_type() { return POB_NONE; };

	protected:
		DECLARE_SERIAL(CfgObjectBase)

	private:
		CString m_name;
		CString m_short_name;
	};

	class FFOB_CLASS FFBlock : public CfgObjectBase
	{
	public:
		FFBlock();
		virtual void Serialize(CArchive &ar);
		FFDevice* get_parent_dev() { return m_parent_dev; }

		uint16 get_block_index() { return m_block_index; }
		void set_block_index(uint16 index) { m_block_index = index; }
		bool is_permanent() { return m_is_permanent; }
		void set_permanent(bool x) { m_is_permanent = x; }
		int get_type() { return m_type; }
		void set_type(int t) { m_type = t; }

		DECL_POB_TYPE(POB_BLOCK);

	protected:
		DECLARE_SERIAL(FFBlock)

		FFDevice* m_parent_dev;
		uint32 m_dd_item;
		uint16 m_block_index; // start index
		bool m_is_permanent;
		int m_type;
	};

	class FFOB_CLASS PhysicalBlock : public FFBlock
	{
	public:
	//	virtual void Serialize(CArchive &ar); // physical device/block needn't save
		DECL_POB_TYPE(POB_PHYSICAL_BLOCK)

	protected:
	//	DECLARE_SERIAL(PhysicalBlock)
	};

	class FFOB_CLASS BlockParameter : public CfgObjectBase
	{
	public:
		BlockParameter();
		BlockParameter(uint16 index, uint16 sub_index, 
			char* val, int val_len);
		~BlockParameter();
		virtual void Serialize(CArchive &ar);
		DECL_POB_TYPE(POB_BLOCK_PARAMETER);

		uint16 get_index() { return m_index; }
		uint16 get_sub_index() { return m_sub_index; }
		void set_value(char* data, int data_len);
		char* get_value() {return m_value; }
		int get_value_len() { return m_val_len; }

	protected:
		DECLARE_SERIAL(BlockParameter);

	private:
		uint16 m_index;
		uint16 m_sub_index;
		char* m_value;
		int m_val_len;
	};

	class FFOB_CLASS VirtualBlock : public FFBlock
	{
	public:
		VirtualBlock();
		~VirtualBlock();
		VirtualBlock(VirtualDevice* dev, uint16 index, uint32 dd_item, int type);
		void set_parent_dev(VirtualDevice* dev);
		ffob_dd::DDBlock* get_dd_block();
		uint32 get_dd_item();
		virtual void Serialize(CArchive &ar);

		void set_app_id(uint32 id) { m_app_id = id; }
		uint32 get_app_id() { return m_app_id; }
		FB_PARA* get_param();
		char* get_param_addr(int index, int sub_index);

		uint16 get_param_index(uint32 anchor_id, uint32 param_type, bool& is_bkcal);

		void set_uid(uint32 id) { m_uid = id; };
		uint32 get_uid() { return m_uid; };

		uint32 get_exec_time();
		uint32 get_start_time() { return m_start_time; }
		void set_start_time(uint32 t) { m_start_time = t; }
		uint32 get_end_time() { return m_start_time + get_exec_time(); }

		bool modify_param_value(uint16 idx, uint16 sub_idx, char* data, int data_len);
		bool restore_saved_value();

		BOOL is_exec_time_conflict_with(VirtualBlock *pBlkToCheck);
		BOOL is_same_virtual_block(VirtualBlock *blk_to_check);

		DECL_POB_TYPE(POB_VIRTUAL_BLOCK);

	protected:
		DECLARE_SERIAL(VirtualBlock)

	private:
		uint32 m_uid;
		// application that using this block
		LinkApplication* m_app;
		uint32 m_app_id;
		FB_PARA* m_param;
		vector<vector<char*>*> m_value_buf_array;

		map<uint32, BlockParameter*> m_modified_value_map;

		uint32 m_start_time;
	};

	class FFOB_CLASS FFDevice : public CfgObjectBase
	{
	public:
		FFDevice() {};
		FFDevice(uint32 vendor_id, uint16 dev_type, uint8 dev_ver, uint8 dd_ver, uint8 cff_ver);
		virtual void Serialize(CArchive &ar);

		uint8 get_addr() { return m_dev_addr; }
		bool set_addr(uint8 addr, bool check_limit = true);

		const char* get_pdtag() { return m_pdtag.GetBuffer(0); }
		bool set_pdtag(const CString& tag);

		char* get_device_id() { return m_device_id.GetBuffer(0); }
		void set_device_id(const CString& dev_id) { m_device_id = dev_id; }

		uint32 get_vendor_id() { return m_vendor_id; }
		uint16 get_dev_type() { return m_dev_type; }
		uint8 get_dev_ver() { return m_dev_ver; }
		uint8 get_dd_ver() { return m_dd_ver; }
		uint8 get_cff_ver() { return m_cff_ver; }
		void set_cff_ver(uint8 cff_ver) { m_cff_ver = cff_ver; }

		ffob_dd::CFFVersion* get_cff_ob();
		uint8 get_parent_link_index() { return m_parent_link_index; }
		void set_parent_link_index(uint8 idx) { m_parent_link_index = idx; }

		H1LinkOb* get_parent_link();

		DECL_POB_TYPE(POB_DEVICE);

	protected:
		DECLARE_SERIAL(FFDevice)

		CString m_device_id;

	private:
		// static device info
		uint32 m_vendor_id;
		uint16 m_dev_type;
		uint8 m_dev_ver;
		uint8 m_dd_ver;
		uint8 m_cff_ver;

		CString m_pdtag;
		CString m_desc;
		uint8 m_dev_addr;

		uint8 m_parent_link_index;
	};

	class FFOB_CLASS PhysicalDevice : public FFDevice
	{
	public:
		PhysicalDevice(){};
		PhysicalDevice(uint32 vendor_id, uint16 dev_type, uint8 dev_ver, uint8 dd_ver, uint8 cff_ver)
			: FFDevice(vendor_id, dev_type, dev_ver, dd_ver, cff_ver)
		{
		}

		DECL_POB_TYPE(POB_PHYSICAL_DEVICE);

	protected:
	//	DECLARE_SERIAL(PhysicalDevice);
	};

	class FFOB_CLASS VirtualDevice : public FFDevice
	{
	public:
		VirtualDevice();
		VirtualDevice(uint32 vendor_id, uint16 dev_type, uint8 dev_ver, uint8 dd_ver, uint8 cff_ver);
		~VirtualDevice();

		DECL_POB_TYPE(POB_VIRTUAL_DEVICE);

		VirtualBlock* add_inst_block(uint32 blk_dd_item);
		void add_block(VirtualBlock* blk) { m_block_list.AddTail(blk); }
		const CTypedPtrList<CObList, VirtualBlock*>& get_block_list() { return m_block_list; }
		void remove_block(VirtualBlock* blk);

		static VirtualDevice* create_new(uint32 vendor_id, uint16 dev_type, uint8 dev_ver, uint8 dd_ver, uint8 cff_ver);

		void set_uid(uint32 id) { m_uid = id; };
		uint32 get_uid() { return m_uid; };

		VirtualBlock* find_block(uint32 uid);
		DEV_MIB& get_mib_data() { return m_mib_data; }

		int get_inst_block_count();

		bool is_mapped();
		bool is_configured(H1LinkOb* link_ob);
		bool match_phy_dev(PhysicalDevice* pdev);

		virtual uint16 get_mib_od_index(ffob_dd::MIB_S_OD_TYPE type);
		virtual uint16 get_schedule_capa_val(ffob_dd::SCHEDULE_CAP_TYPE capa_type);

		virtual void Serialize(CArchive &ar);

		uint32 get_mib_ref_id()
		{
			// TODO:
			return 1;
		}

	protected:
		DECLARE_SERIAL(VirtualDevice)

	private:
		uint32 m_uid;
		DEV_MIB m_mib_data;
		CTypedPtrList<CObList, VirtualBlock*> m_block_list;
	};

	class FFOB_CLASS InterfaceDevice : public VirtualDevice
	{
	public:
		InterfaceDevice();

		static bool load_mib();
		static uint16 get_schedule_cap(ffob_dd::SCHEDULE_CAP_TYPE type);

		virtual uint16 get_mib_od_index(ffob_dd::MIB_S_OD_TYPE type);
		virtual uint16 get_schedule_capa_val(ffob_dd::SCHEDULE_CAP_TYPE capa_type);

		DECL_POB_TYPE(POB_VIRTUAL_DEVICE);

	protected:
		static uint16 get_mib_param_index(ffob_dd::MIB_S_OD_TYPE type);
		static bool parse_od_item(const CString& section, const CString& key, ffob_dd::MIB_S_OD_TYPE od_type);
		static bool parse_sch_char_item(const CString& section, const CString& key, ffob_dd::SCHEDULE_CAP_TYPE type);

		DECLARE_SERIAL(InterfaceDevice);

	private:
		static map<ffob_dd::MIB_S_OD_TYPE, WORD> m_map_mib_od;
		static map<ffob_dd::SCHEDULE_CAP_TYPE, WORD> m_map_schedule_cap_od;
		static CString m_mib_ini_file_name;
	};

	class FFOB_CLASS LinkApplication : public CfgObjectBase
	{
	public:
		LinkApplication();
		LinkApplication(H1LinkOb* parent_link);
		~LinkApplication();

		H1LinkOb* get_link_ob() { return m_parent_link; }
		void set_link_ob(H1LinkOb* p) { m_parent_link = p; }

		CHyperArray& get_view_object_array() { return m_listObjects; }
		void clear_all_view_object() { m_listObjects.DeleteContents(); }
		void add_view_object(CHyperOb* ob) { m_listObjects.Add(ob); }

		uint32 get_looptime() { return m_looptime; }
		BOOL set_looptime(uint32 loop_time) {m_looptime = loop_time; return TRUE;}

		uint32 get_id() { return m_id; }
		void set_id(uint32 id) { m_id = id; }

		CHyperOb* find_draw_ob_by_name(const CString name);
		void init_draw_ob();

		void remove_dev_block(VirtualDevice* dev);
		void remove_block(VirtualBlock* block);
		void remove_assoc_connect_lines(CBlockOb* block_ob);

		static uint32 DEFAULT_LOOPTIME;

		DECL_POB_TYPE(POB_LINK_APP)

	protected:
		DECLARE_SERIAL(LinkApplication);
		virtual void Serialize(CArchive &ar);

	private:
		uint32 m_id; // unique application ID
		H1LinkOb* m_parent_link;
		uint32 m_looptime;

		// application view form info
		CSize m_sizePane;			// work pane area size
		COLORREF m_crBackGround;	// back ground color
		BOOL m_bGridOn;				// show grid or not

		CHyperArray	m_listObjects;	// draw objects list
	};

	class FFOB_CLASS LinkScheduleInfo : public  CfgObjectBase
	{
	public:
		LinkScheduleInfo(){}
		LinkScheduleInfo(H1LinkOb* parent_link) : m_parent_link(parent_link) {}
		DECL_POB_TYPE(POB_LINK_SCHEDULE);
		H1LinkOb* get_link_ob() { return m_parent_link; }

	protected:
		DECLARE_SERIAL(LinkScheduleInfo);

	private:
		H1LinkOb* m_parent_link;
	};

	class FFOB_CLASS PhysicalNetwork : public CfgObjectBase
	{
	public:
		PhysicalNetwork();
		~PhysicalNetwork();

		H1LinkOb* get_link() { return m_parent_link; }
		void set_link(H1LinkOb* ob) { m_parent_link = ob; }
		DECL_POB_TYPE(POB_PHYSICAL_NETWORK);

		vector<PhysicalDevice*>& get_online_devices() { return m_online_dev_array; }
	//	FF_RESULT_CODE refresh_online_devices(FF_MODULE_HANDLE ff_module_handle);

		void load_dev_list(H1_DEV_INFO* dev_array, int count);

	protected:
	//	DECLARE_SERIAL(PhysicalNetwork)

	private:
		H1LinkOb* m_parent_link;
		vector<PhysicalDevice*> m_online_dev_array;
	};

	class FFOB_CLASS H1LinkOb : public CfgObjectBase
	{
	public:
		H1LinkOb();
		~H1LinkOb();
		DECL_POB_TYPE(POB_H1_LINK);
		void init();
		virtual void Serialize(CArchive &ar);
		
		LinkApplication* add_new_app();
		uint32 get_free_app_id();
		CString get_unique_app_name();
		CString get_short_app_name();
		bool add_virtual_device(VirtualDevice* dev);
		void delete_virtual_device(VirtualDevice* dev);
		void delete_app(LinkApplication* app);
		void remove_dev_from_all_app(VirtualDevice* dev);
		const CTypedPtrList<CObList, LinkApplication*>& get_app_list() { return m_app_list; }
		LinkApplication* get_app_by_id(uint32 id);

		InterfaceDevice* get_interface_dev() { return &m_interface_dev; }

		const CTypedPtrList<CObList, VirtualDevice*>& get_virtual_dev_list() { return m_virtual_device_list; }
		LinkScheduleInfo* get_schedule_info() { return m_schedule_info; }
		PhysicalNetwork* get_physical_network() { return m_phy_network; }
		uint8 get_link_index() { return m_link_index; }
		void set_link_index(uint8 idx)
		{
			m_link_index = idx;
			m_interface_dev.set_parent_link_index(idx);
		}

		VirtualDevice* find_mapped_dev(const char* dev_id);
		VirtualDevice* find_mapped_dev2(const char* dev_id);
		VirtualDevice* find_device(uint32 dev_uid);

		uint32 get_macro_cycle();
		void set_macro_cycle(uint32 mc);

		static unsigned char min_dev_addr() { return 0x11; }
		static unsigned char max_dev_addr() { return 0x2b; }

		static int MAX_APP_COUNT;
		static int MAX_DEVICE_COUNT;

	protected:
		DECLARE_SERIAL(H1LinkOb)

	private:
		CTypedPtrList<CObList, LinkApplication*> m_app_list;
		CTypedPtrList<CObList, VirtualDevice*> m_virtual_device_list;
		InterfaceDevice m_interface_dev;
		LinkScheduleInfo* m_schedule_info;
		PhysicalNetwork* m_phy_network;
		
		uint8 m_link_index;

		uint32 m_macro_cycle; // in millisecond
	};

	class FFOB_CLASS H1ModuleOb : public CfgObjectBase
	{
	public:
		H1ModuleOb();
		~H1ModuleOb();
		DECL_POB_TYPE(POB_H1_MODULE)
		
		const vector<H1LinkOb*>& get_link_array() { return m_link_array; }
		uint32 generate_unique_id();

		virtual void Serialize(CArchive &ar);

	protected:
		DECLARE_SERIAL(H1ModuleOb)

	private:
		vector<H1LinkOb*> m_link_array;
		static const int m_fixed_link_count;
		uint32 m_max_unique_id;
	};

/////////////////////////////////////////////////////////////////////////////
// derived from yourCAnchorAsst to do custom checking ...
class FFOB_CLASS CFBAnchorAsst : public yourCAnchorAsst
{
public:
	CFBAnchorAsst() {};
//	virtual ~CFBAnchorAsst();
	virtual BOOL RejectConnect(CHyperOb* pSnapToOb,
		yourCAnchorOb* pAnchorOb, int nSide);
};

/////////////////////////////////////////////////////////////////////////////
// CBlockOb (function block only)
// Block draw object in application form.
#ifndef baseCBlockOb
#define baseCBlockOb CBorderOb
#endif
class FFOB_CLASS CBlockOb : public baseCBlockOb
{
public:
	CBlockOb();
	virtual ~CBlockOb();

	/* yourCAnchorAsst */
	CFBAnchorAsst m_snapAnchors;
	// ANCHOR1.) override HasAnchors() and supply GetAnchors()
	virtual yourCAnchorAsst* HasAnchors(BOOL bNoRemap = FALSE)
		{ return BuildAnchors(bNoRemap, &m_snapAnchors); }
	//  equal to { return &m_snapAnchors; } ...

private:
	DECLARE_SERIAL(CBlockOb)

protected:
	void CalcStaticSize();
 	virtual void GetDefault();

// attributes
public:
	void ResetAnchorIn();
	void SetAnchorIn(int nID);

#ifdef CNCS_OEM
	VirtualBlock* get_block() { return m_virtual_block; }
	BOOL set_block_info(VirtualBlock* blk, BOOL bFillOb = TRUE);
	BOOL init_parent_ob_info(LinkApplication* app);
#endif

	BOOL InitFromFBPara();
	virtual void SetRect(CRect& r);
	void DrawAlarmParam(CDC *pDC, int nParamHeight);
	void DrawParamIcon(CDC *pDC, int x, int y, int nSize, UINT nType);
	void DrawParams(CDC *pDC);
	void DrawBlockName(CDC *pDC);
	virtual void Paint(CDC *pDC);
	virtual void Serialize(CArchive &ar);
	virtual const void operator=(const CHyperOb& e);
	virtual void Init(CHyperRunView* pParentView,
		LPCRECT prInit = NULL, BOOL bGetDefault = TRUE);	
//	virtual BOOL OnSelectDblClick(); // { return FALSE; }

// variables
public:
	// associate device ID
	inline const CString& GetAssDevDeviceID();
	
	// associate block parameter start index
	inline WORD GetAssBlkStartIdx(); 
	
	// associate block object dditem
	inline uint32 GetDdItem() { return m_nDdItem; }
	
	// flag of current draw object whether is validity
	inline void SetValid(BOOL bValid);
	inline BOOL IsValid() const;

	void SetDispName(const CString& sName);

	// in/out parameters name list
	CStringArray m_listParamsIn;
	CStringArray m_listParamsOut;

	CStringArray m_listValIn;
	CStringArray m_listValOut;

protected:

	VirtualBlock* m_virtual_block;
	VirtualDevice* m_virtual_device;

#ifdef CNCS_OEM
	uint32 m_dev_uid;
	uint32 m_block_uid;
#endif

	// for drawing
	int m_nCharHeight;
	int m_nMinWidth;
	int m_nMinHeight;

private:
	// associate block DD item
	uint32 m_nDdItem;
	// associate block object parameter start index
	WORD m_nStartIndex;
	// associate device ID
	CString m_strDevID;
	// current draw block object name
	CString m_sBlockName;
	// if the function block can be found or uncertain, this block object is invalid
	BOOL m_bValid;

	BOOL m_bOnline;

public:
	void SetOnline(BOOL b) { m_bOnline = b; }
	BOOL GetOnline() { return m_bOnline; }
};
////////////////////////////////////////////////////////////////////////////////
//inline function declaration class CBlockOb
////////////////////////////////////////////////////////////////////////////////
// associate block object parameter start index
inline WORD CBlockOb::GetAssBlkStartIdx()
{
	return m_nStartIndex;
}
// associate device ID
inline const CString& CBlockOb::GetAssDevDeviceID()
{
	return m_strDevID;
}

/*
// current draw block object name
inline const CString& CBlockOb::GetDispName() const
{
	return m_sBlockName;
}*/

// flag of current draw block object whether is validity
inline BOOL CBlockOb::IsValid() const
{
	return m_bValid;
}

inline void CBlockOb::SetValid(BOOL bValid)
{
	m_bValid = bValid;
}

	class FFOB_CLASS ProjectManager
	{
	public:
		static H1ModuleOb* get_h1module();
		static H1ModuleOb* create_new_h1module();
		static bool release_current_h1module();
		static void set_modified(bool x = true) { m_is_modified = x; }
		static bool is_modified() { return m_is_modified; }
		static string get_proj_filename() { return m_proj_filename; }
		static void set_proj_filename(string x) { m_proj_filename = x; }
		static bool load_project(const string filename);
		static H1ModuleOb* load_module_ob(const string filename);
		static bool save_project(const string filename = "");
		static void close_project();
		static uint32 generate_unique_id();
		static CfgData_Link* generate_download_info(uint8 link_index);
		static string get_root_dir() { return m_root_dir; }
		static void set_root_dir(string dir) { m_root_dir = dir; }
		static FF_COMM_TYPE get_comm_type() { return m_comm_type; }
		static void set_comm_type(FF_COMM_TYPE t) { m_comm_type = t; }

	private:
		static H1ModuleOb* m_unique_h1module_obj;
		static bool m_is_modified;
		static string m_proj_filename;

		static string m_root_dir;
		static FF_COMM_TYPE m_comm_type;
	};

	typedef struct __linkpub
	{
		uint32	dwCDTime;		// 1/32 millisecond
		BOOL	bIsCDTimeValid; // 2003-7-14 17:17, kking
		uint16	nLocalIndex;	// publisher parameter index
		uint8	ucSelectorPub;	// publisher selector it will be set zero when publisher device is HSE device
		uint32	unHSESelectorPub; // publisher selector it will be set zero when 
		// publisher device is H1 device
		uint8	szSubDevID[DEV_ID_SIZE + 1]; // subscriber device ID, if current publisher data
											// need republisher, this parameter store the republisher 
											// device ID(LAS or LD device) ???
	} LINK_PUB;

	typedef struct __linksub
	{
		BOOL	bIsRepub;			// TRUE means receive data from LAS device or LD device 
		uint16	nLocalIndex;		// subscriber parameter index(H1/HSE)
		uint16	nRemoteIndex;		// publisher parameter index(H1/HSE)
		uint8	ucRemoteNodeAddr;	// publisher device Node Addr if bIsRepub = TRUE,
									// this address is the LAS device address
		uint8	ucSelectorSubfrom;  // publisher selector, and set by caller
		uint32 unHSESelectorSubfrom; // publisher selector, it will be set zero when
									// subscriber device is H1 device, and set by caller
		uint8 szPubDevID[DEV_ID_SIZE + 1]; // data publisher device ID 

		// if bIsRepub = TREU this member store the republisher device ID
		// if current subscriber is in HSE link, store the LD device ID
		// if current subscriber is in H1 link, store the LAS device ID
		BYTE szRepubDevID[DEV_ID_SIZE + 1];
	} LINK_SUB;

	enum H1_LINKAGE_TYPE
	{
		UNKNOWN_LINKAGE = 0,
		H1_LOCAL_LINK = 1,				// 用于本地连接
		H1_PUBLISHER_LINK = 2,			// 用于发送数据
		H1_SUBSCRIBER_LINK = 3,		    // 用于接受数据
	};

	enum H1_STATIC_VCR_TYPE
	{
		H1_SERVICE_VCR = 0x32,
		H1_PUBLISHER_VCR = 0x66,
		H1_SUBSCRIBER_VCR = 0x76,
	};

	typedef struct __external_io
	{
		string id;
		uint8 link_index;
		char device_id[FFH1_DEVID_LEN];
		char device_pdtag[FFH1_PDTAG_LEN];
		uint8 dev_addr;
		string block_tag;
		uint32 block_dd_item;
		uint16 block_start_index;
		string io_name;
		uint32 io_type;
		uint8 io_data_type;
		uint8 io_data_size;
		uint16 io_index;
		uint16 io_sub_index;
		// generated by CNCS
		uint8 virtual_slave_addr;
		uint8 virtual_slave_offset;
	} FF_EXT_IO;
	
	//////////////////////////////////////////////////////////////////////////
	// 
	// 用于CNCS控制器混合编程的 FF211 Linkage 对象结构
	// 索引从2000开始
	//
#define CNCS_FF211_LINK_OD_INDEX      2000
#define CNCS_FF211_LINK_START_INDEX    257
#define CNCS_FF211_VCR_START_INDEX     403
#define CNCS_FF211_VCR_START_OFFSET     35
	enum H1_CTLR_LINK_TYPE
	{
		LINK_DISABLE = 0,
		LINK_LOCAL = 1,
		LINK_PUBLISHER = 2,
		LINK_SUBSCRIBER = 3,
	};

	typedef struct _H1_CTLR_LINK
	{
		USIGN16 uLinkIndex; // 连接对象索引, 与仪表设备中Linkage的远程索引匹配，需大于 256 
		USIGN16 uVcrIndex;  // 接收数据VCR的绝对索引（OD）
		USIGN8 ucLinkType;	/*
							链接对象的使用类型，H1_CTLR_LINK_TYPE
							0：表示清除链接对象，<====
							1：表示本地使用，
							2：表示远程使用，角色为Publisher，   <====       
							3：表示远程使用，角色为Subscriber，<====
							4、5：保留，
							6：表示报警使用，       
							7：表示趋势使用，
							8：表示本地的现场总线通讯模块使用，角色为Publisher                       
							9：表示本地的现场总线通讯模块使用，角色为Subscriber
							*/  
		USIGN8 ucSlaveNum; /*虚拟从站地址*/
		USIGN8 ucOffset;   /*数据偏移*/    
		USIGN8 ucIODataType; /*数据类型, 01表示离散量，02表示模拟量*/
		USIGN16 uRemoteBlockID;  //远程的功能块编号 ，此处不使用 
		USIGN16 uRemoteIndex; // 远程的数据索引
	} H1_CTLR_LINK;

	class CfgData_Block;
	class VCRObject;
	class LinkageObject
	{
	public:
		LinkageObject()
		{
			cdtime = 0;
		}

		VCRObject* get_pub_vcr();
		void set_pub_vcr(VCRObject* vcr) { m_pub_vcr = vcr; }

		static uint32 transmission_time() { return 50 * 32; }

		USIGN16		LocalIndex;		// 发送/接受数据的参数索引
		USIGN16		VcrNumber;		// 相关VCR索引...
		USIGN16		RemoteIndex;	// 发送数据的参数索引（如果当前的LINKAGE用于发送数据，该值为0）
		USIGN8		ServiceOperation;
	//	USIGN8		StateCountLimit; // useless??

		uint32		cdtime;
		vector<CfgData_Block*> sub_block_array;

	private:
		VCRObject* m_pub_vcr; // 仅对PUB类型的Linkage有意义
	};

	class CfgData_Device;

	class VCRObject
	{
	public:
		VCRObject(H1_STATIC_VCR_TYPE type, CfgData_Device* dev_cfg);

		H1_STATIC_VCR_TYPE get_type()
		{
			return (H1_STATIC_VCR_TYPE)m_vcr_data.FasArTypeAndRole; 
		}

		LinkageObject* get_pub_linkage() { return m_pub_linkage; }
		
		void set_pub_linkage(LinkageObject* lnkg) { m_pub_linkage = lnkg; }
		
		void add_sub_vcr(VCRObject* sub_vcr)
		{
			ASSERT(m_vcr_data.FasArTypeAndRole == H1_PUBLISHER_VCR);
			m_sub_vcr_array.push_back(sub_vcr);
		}

		void add_sub_linkage(LinkageObject* lnkg)
		{
			ASSERT(m_vcr_data.FasArTypeAndRole == H1_SUBSCRIBER_VCR);
			m_sub_linkage_array.push_back(lnkg);
		}

		void set_entry_index(uint16 index);
		uint16 get_entry_index() { return m_vcr_entry_index; }

		CfgData_Device* get_dev_cfg() { return m_dev_cfg; }

		static void init_vcr_data(VCR_STATIC_ENTRY* vcr_entry, H1_STATIC_VCR_TYPE type);

		VCR_STATIC_ENTRY& get_vcr_data() { return m_vcr_data; }

	public:
		LinkageObject* m_pub_linkage;
		vector<VCRObject*> m_sub_vcr_array;

	private:
		vector<LinkageObject*> m_sub_linkage_array;
		// 实际下载到的VCR索引
		uint16 m_vcr_entry_index;
		CfgData_Device* m_dev_cfg;
		VCR_STATIC_ENTRY m_vcr_data;
	};

	class FFOB_CLASS CfgData_Block
	{
	public:
		CfgData_Block(VirtualBlock* vblk, CfgData_Device* cfgdev) :
		  m_bblk(vblk), m_parent_dev(cfgdev), m_scheduled(false)
		{};

		VirtualBlock* get_block_ob() { return m_bblk; }
		void add_pub_linkage(LinkageObject* linkage);
		vector<LinkageObject*>& get_pub_linkage_array() { return m_pub_linkage_array; }

		void add_sub_linkage(LinkageObject* linkage) { m_sub_linkage_array.push_back(linkage); }
		const vector<LinkageObject*>& get_sub_linkage_array() { return m_sub_linkage_array; }

		CfgData_Device* get_parent_dev_cfg() { return m_parent_dev; }

		bool is_scheduled() { return m_scheduled; }
		void set_scheduled(bool b = true) { m_scheduled = b; }

	private:
		VirtualBlock* m_bblk;
		vector<LinkageObject*> m_pub_linkage_array;
		vector<LinkageObject*> m_sub_linkage_array;
		CfgData_Device* m_parent_dev;

		bool m_scheduled;
	};

	typedef struct __dev_vcr_info
	{
		uint16 od_index;
		bool is_fixed;
		bool reserved;
		bool used;
		bool reusable;
		VCR_STATIC_ENTRY data;
		vector<LinkageObject*> linkage_array;
	} DEV_VCR_INFO;

	class FFOB_CLASS CfgData_Device
	{
	public:
		CfgData_Device(VirtualDevice* vd, CfgData_Link* link_cfg)
			: m_vdev(vd)
			, m_link_cfg(link_cfg)
			, m_cncs_selector(0xB0)
		{
		}

		~CfgData_Device();

		VirtualDevice* get_virtual_dev() { return m_vdev; }
		int read_physical_dev_info();
		bool read_vcr_list_characteristics(uint16 index, VCR_LIST_CHARACTERISTICS& data);

		LinkageObject* add_linkage(H1_LINKAGE_TYPE type, uint16 pub_index, uint16 sub_index);

		LinkageObject* add_linkage_local(uint16 pub_index, uint16 sub_index);
		LinkageObject* add_linkage_pub(uint16 pub_index); // AddH1PublisherObjInf
		
		LinkageObject* add_linkage_sub(
			uint16 pub_index,
			uint16 sub_index,
			VCRObject* pub_vcr);

		const vector<LinkageObject*>& get_linkage_array() { return m_linkage_array; }

		CfgData_Block* add_schedule_block(VirtualBlock* vblk);
		map<uint32, CfgData_Block*>& get_schedule_block_map() { return m_block_map; }
		void get_ordered_block_list(list<CfgData_Block*>& block_list);

		void calc_block_start_time(CfgData_Block* cfg_blk, uint32 min_time);

		bool compose_download_info();
		bool assign_selector();

		FF_RESULT_CODE download(FF_MODULE_HANDLE h_module);

		DEV_VCR_INFO* get_vcr_by_index(uint16 vcr_index);

		void fill_vcr_common_data(VCRObject* vcr_ob, VCR_STATIC_ENTRY* vcr_entry);

		bool find_sub_linkage(FF_EXT_IO* io_ptr);

		VCRObject* cncs_alloc_sub_vcr_and_linkage(FF_EXT_IO* io_ptr);
		bool cncs_alloc_pub_vcr_and_linkage(FF_EXT_IO* io_ptr);

	protected:
		FF_RESULT_CODE download_fb_start(FF_MODULE_HANDLE h_module);
		FF_RESULT_CODE download_vcr(FF_MODULE_HANDLE h_module);
		FF_RESULT_CODE download_linkage(FF_MODULE_HANDLE h_module);

		FF_RESULT_CODE read_dev_fb_start_info();
		FF_RESULT_CODE read_dev_linkage_info();

	private:
		vector<LinkageObject*> m_linkage_array;

		// 此设备需要下载的VCR信息
		vector<VCRObject*> m_cfg_vcr_array;

		// 设备当前的VCR信息, key为设备中VCR实际的地址
		map<uint16, DEV_VCR_INFO*> m_vcr_entry_map;

		VirtualDevice* m_vdev;

		// 需要调度的功能块
		map<uint32, CfgData_Block*> m_block_map;

		uint16 m_fb_start_index;
		uint16 m_fb_start_count;

		uint16 m_linkage_start_index;
		uint16 m_linkage_count;

		CfgData_Link* m_link_cfg;

		uint8 m_cncs_selector;
	};

#pragma pack(push, 1)
	typedef struct __schedule_header
	{
		uint8 ucValue1;					// 固定值 FF
		uint8 ucValue2;					// 固定制 02
		uint16 wVersionNumber;			// 用户设定
		uint16 wBuildID;				// 用户设定
		uint8 ucNumOfSubSchedule;		// 子调度个数
		uint8 ucMSO;					// 没有使用 3f
		uint16 wSchSize;				// 调度表的大小 
		uint16 wTimeingResolution;		// 没有使用 0000
		uint16 wMRDMutilST;				// 目前取值为 0000
		uint16 wTSL;					// 没有使用 0000
		uint8 T0[6];					// 没有使用 000000000000
		uint32 dwMacroCycle;			// 调度宏周期
	} SCHEDULE_HEADER;
#pragma pack(pop)

	class FFOB_CLASS CfgData_Link
	{
	public:
		CfgData_Link(H1LinkOb* link_ob, FF_MODULE_HANDLE comm_handle)
			: m_link_ob(link_ob)
			, m_comm_handle(comm_handle)
			, m_ext_io_vcr_selector(0xa0)
		{}
		~CfgData_Link();

		FF_MODULE_HANDLE get_comm_handle() { return m_comm_handle; }

		bool calc_schedule_time(vector<CfgData_Block*> ordered_block_array);
		bool calc_block_schedule_time(CfgData_Block* cfg_blk, uint32 time_base);
		bool calc_block_pub_cdtime(CfgData_Block* cfg_blk, uint32 time_base);

		map<uint32, CfgData_Device*>& get_dev_map() { return m_map_dev; }

		bool sort_blocks(const map<VirtualBlock*, CfgData_Block*>& block_map, vector<CfgData_Block*>& block_array);

		bool contain_device(VirtualDevice* vd);

		bool parse();
		int collect_dev_current_cfg();
		int compose_cfg_info();
		bool pre_download();

		// 实时操作
		FF_RESULT_CODE active_las(bool active, uint16 ver_val);
		FF_RESULT_CODE download(H1LinkOb* link_ob,FF_MODULE_HANDLE h_module);
		FF_RESULT_CODE download_dev(H1LinkOb* link_ob,VirtualDevice* dl_virtual_device,FF_MODULE_HANDLE h_module);
		FF_RESULT_CODE download_schedule_info(FF_MODULE_HANDLE h_module);

		FF_RESULT_CODE download_macro_cycle_info(
			FF_MODULE_HANDLE h_module,
			VirtualDevice* vdev = NULL);
		
		FF_RESULT_CODE download_domain_to_dev(
			char* data, uint16 data_len,
			uint16 domain_index, VirtualDevice* vdev);

		FF_RESULT_CODE download_cncs_ext_io(FF_MODULE_HANDLE h_module);

		void init_schd_header(SCHEDULE_HEADER& schd_header);

		CfgData_Device* find_device(uint8 addr);

		// CNCS混合编程相关
		bool cncs_load_external_io();
		DEV_VCR_INFO* cncs_alloc_vcr(H1_STATIC_VCR_TYPE vcr_type, uint32 remote_addr);
		H1_CTLR_LINK* cncs_alloc_intf_link();

	private:
		bool parse_app(LinkApplication* app);
		CfgData_Device* add_dev(VirtualDevice* vd);

	private:
		// uid ==> dev
		map<uint32, CfgData_Device*> m_map_dev;

		H1LinkOb* m_link_ob;
		FF_MODULE_HANDLE m_comm_handle;

		map<uint32, LinkageObject*> m_scheduled_pub_linkage;

		// CNCS混合编程相关
		vector<FF_EXT_IO*> m_ext_io_array;
		vector<DEV_VCR_INFO*> m_ext_io_vcr_array;
		vector<H1_CTLR_LINK*> m_ext_io_link_array;
		uint8 m_ext_io_vcr_selector;
	};

	class FFOB_CLASS ConfigUtil
	{
	public:
		static uint32 gcd(uint32 a, uint32 b); // Greatest Common Divisor
		static uint32 lcm(uint32 a, uint32 b); // Least Common Multiple 

		static CString dev_char_to_string(const char* char_buf, int len);
	};

	typedef struct _mib_param_ex
	{
		UINT	nID;
		UINT	nDataType;
		UINT	nSize;
		UINT	nAccessRight; // read / write...
		char*	sName;
		ffob_dd::MIB_S_OD_TYPE od_type;
		uint16  offset;
	} MIB_PARAM_EX;

	class FFOB_CLASS MIBSubParamOb
	{
	public:
		MIBSubParamOb(const char* name, uint8 data_type, uint8 size, uint8 access_right);

	private:
		string m_name;
		uint8 m_data_type;
		uint8 m_data_size;
		uint8 m_access_type;
	};

	class FFOB_CLASS MIBParamOb
	{
	public:
		MIBParamOb(MIB_PARAM_EX& pi);

		MIBParamOb(
			const char* name,
			uint8 data_type,
			uint8 data_size,
			uint8 access_right,
			ffob_dd::MIB_S_OD_TYPE od_type,
			uint16 offset
			);

		~MIBParamOb();

		const string& get_name() { return m_name; }
		ffob_dd::MIB_S_OD_TYPE get_od_type() { return m_od_type; }
		uint16 get_offset() { return m_od_index_offset; }

		void add_sub_param(const char* name, uint8 data_type, uint8 size, uint8 access_right);

	private:
		string m_name;
		ffob_dd::MIB_S_OD_TYPE m_od_type;
		uint16 m_od_index_offset;
		uint8 m_data_type;
		uint8 m_data_size;
		uint8 m_access_type; // MIBP_READ / MIBP_READWRITE

		vector<MIBSubParamOb*> m_sub_param_array;
	};

	class FFOB_CLASS MIBManager
	{
	public:
		static bool init();
		static void exit();
		static MIBParamOb* get_param(uint32 index);
		static MIBParamOb* get_param(const char* name);
		static ffob_dd::MIB_S_OD_TYPE get_od_type_by_name(const char* name);

	private:
		static void add_param(MIBParamOb* p);

		static vector<MIBParamOb*> m_param_array;
		static map<string, MIBParamOb*> m_param_map;
	};
}

/***将字符串转成int***/
FFOB_API int char2int(const char* str);

/***将int转成字符串***/
FFOB_API void int2char(int v, char* s);

/***将十六进制字符串转成int***/
FFOB_API int hexchar2int(const char* str, int len);
