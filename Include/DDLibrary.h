#pragma once
#include <list>
#include <map>
#include <vector>
using namespace std;

#include "FFDef.h"

#ifdef FFOB_EXPORTS
	#define FFOB_API extern "C" __declspec(dllexport)
	#define FFOB_CLASS __declspec(dllexport)
#else
	#define FFOB_API extern "C" __declspec(dllimport)
	#define FFOB_CLASS __declspec(dllimport)
	#ifdef _DEBUG
		#pragma comment(lib, "ffobjectd.lib")
	#else
		#pragma comment(lib, "ffobject.lib")
	#endif
#endif	//FFOB_EXPORTS

namespace ffob_dd
{
	enum MIB_S_OD_TYPE
	{
		// ���������
		NM_RESERVED = 1,					// Reserved
		NM_DIR_REVERSION_NUM,				// Directory Revision Number
		NM_NUM_DIR_OBJECT,					// Number of Directory Objects
		NM_TOTAL_NUM_DIR_ENTRY,				// Total Number of Directory Entries
		NM_DIR_IDX_FIRST_COM_LIST_REF,		// Directory Index of First Composite list Reference
		NM_NUM_COM_LIST_REF,				// Number of Composite List Reference

		NM_STACKMGT_ODINDEX,				// StackMgtOdIndex
		NM_NUM_OBJ_IN_STACKMGT,				// NumberOfObjectsInStatckManagement
		NM_VCRLIST_ODINDEX,					// VcrListOdIndex
		NM_NUM_OBJ_IN_VCRLIST,				// NumberOfObjectsInVcrList
		NM_DLMEBASIC_ODINDEX,				// DlmeBasicOdIndex
		NM_NUM_OBJ_IN_DLLBASIC,				// NumberOfObjectsInDllBasic
		NM_DLMELM_ODINDEX,					// DlmeLinkMasterOdIndex
		NM_NUM_OBJ_IN_DLLLME,				// NumberOfObjectsInDllLme
		NM_LINKSCHLIST_ODINDEX,				// LinkScheduleListOdIndex
		NM_NUM_OBJ_IN_DLLLINKSCH,			// NumberOfObjectsInLinkSchedule
		NM_DLMEBRIDGE_ODINDEX,				// DlmeBridegOdIndex
		NM_NUM_OBJ_IN_DLLBRIDGE,			// NumberOfObjectsInDllBridge
		NM_PLMEBASIC_ODINDEX,				// PlmeBasicOdIndex
		NM_NUM_OBJ_IN_PHYLME,				// NumberOfObectsInPhyLme

		// ϵͳ������
		SM_REVERVED,						// Reserved
		SM_DIR_REVERSION_NUM,				// Directory Revision Number
		SM_NUM_DIR_OBJECT,					// Number of Directory Objects
		SM_TOTAL_NUM_DIR_ENTRY,				// Total Number of Directory Entries
		SM_DIR_IDX_FIRST_COM_LIST_REF,		// Directory Index of First Composite list Reference
		SM_NUM_COM_LIST_REF,				// Number of Composite List Reference

		SM_SMAGENTSTARTIN_ODINDEX,			// SmAgentStartingOdIndex
		SM_NUM_OF_SMAGENTOBJ,				// NumberOfSmAgentStartingObjects
		SM_SYNCANDSCHSTARTING_ODINDEX,		// SyncAndScheduleStartingOdIndex
		SM_NUM_OF_SYNCANDSCHSTARTINGOBJ,	// NumberOfSyncAndScheduleStartingObjects
		SM_ADDRASSIGNSTARTING_ODINDEX,		// AddressAssignmentStartingOdIndex
		SM_NUM_OF_ADDRASSIGNSTARTINGOBJ,	// NumberOfAddressAssignmentStartingObjects
		SM_VFDLISTSTARTING_ODINDEX,			// VfdListStartingOdIndex
		SM_NUM_OFVFDLISTSTARTINGOBJ,		// NumberOfVfdListStartingObjects
		SM_FBSCHEDULESTART_ODINDEX,			// FbScheduleStartingOdIndex
		SM_NUM_OF_FBSCHEDULESTARTOBJ,		// NumberOfFbStartingObjects
	};

	enum SCHEDULE_CAP_TYPE
	{
		SCHEDULE_NUMBER,				// �豸�е��ȶ���ĸ���
		SUBSCH_NUM_IN_PRESCH,			// ÿ�����ȶ����п���֧�ֵ��ӵ��ȵĸ���
		SEQUENCE_NUM_IN_SUBSCH,			// ÿ���ӵ��ȶ����п���֧�ֵ�CD�������
		ELEMENT_NUM_IN_SEQUENCE,		// ÿ��CD�������֧�ֵ�PUBLISHER VCR�ĸ���
		MAXSIZE_SCHEDULE,				// ���������󳤶ȣ����ֽڼ���
	};
	
	class FFOB_CLASS DDParseUtil
	{
	public:
		static int StringToValue(const CString& sKeyValue);
		static int SectionNumber(const CString& sSection, const CString& sFilePath);
		static CString ExtractUpperCaseString(LPCTSTR pszStr);
	};

	class VendorInfo;
	class DeviceType;
	class DeviceVersion;
	class DDVersion;
	class CFFVersion;
	class DDBlock;
	class VFDCapability;

	class FFOB_CLASS DDLibrary
	{
	public:
		DDLibrary(void);
		~DDLibrary(void);

	public:
		static DDLibrary* get_local_lib();
		static void release();

		VendorInfo* get_vendor(unsigned int vendor_id, bool create_if_not_found = true);
		const map<unsigned int, VendorInfo*>& get_vendor_map() { return m_map_vendors; };

		static bool cff_profile_key_value(const CString& sSection, const CString& sKey,
			const CString& sFilePath, CString& sKeyValue,
			bool bLog = true, bool bEnableNull = false);

		CFFVersion* find_cff(uint32 vendor_id, uint16 dev_type, 
			uint8 dev_ver, uint8 dd_ver, uint8 cff_ver);

		DDBlock* find_block(uint32 vendor_id, uint16 dev_type, 
			uint8 dev_ver, uint8 dd_ver, uint8 cff_ver,
			uint16 block_index);

	private:
		static DDLibrary* load(const string dd_path = "");
		static bool load_vender(const string path, DDLibrary* ptr_lib);
		static bool load_dev(const string dev_path, DDLibrary* ptr_lib);
		static bool load_cff(const string filename, DDLibrary* ptr_lib);

		static string cff_parse_vendor_name(const string cff_fn);
		static string cff_parse_device_name(const string cff_fn);

		static string validate_cff_version(const CString& sFilePath, DDLibrary* ptr_lib);

		static DDLibrary* m_local_lib;

		// key: vendor id
		map<unsigned int, VendorInfo*> m_map_vendors;

		static const CString FILE_HEADER_SECTION;
		static const CString FILE_TYPE;
		static const CString FILE_VERSION;

		static bool m_no_cff; // �жϳ���Ŀ¼���豸Ŀ¼���Ƿ����*.cff����������ڣ���������Ŀ¼����

	public:
		static DDLibrary* reload(); // �ͷ�DD��ָ��������½���

		static bool m_parse_capability_ret; // ����DD�ļ����
		static bool m_ret; // ��̬����Ϊ����DD�ļ�����������߼��ж�

		vector<string> m_err_dev_id_vtr; // ��������豸ID�б�
		vector<string> m_err_manu_id_vtr; // ������ĳ���ID�б�
	};

	class FFOB_CLASS DDLibraryObjectBase
	{
	public:
		enum DDLIB_OB_TYPE
		{
			DDLO_UNKNOWN = 0,
			DDLO_VENDOR,
			DDLO_DEVICE,
			DDLO_DEVICE_VERSION,
			DDLO_DD_VERSION,
			DDLO_CFF_VERSION,
			DDLO_BLOCK,
		};

		DDLIB_OB_TYPE get_ob_type() { return m_ob_type; }

	protected:
		void set_ob_type(DDLIB_OB_TYPE t) { m_ob_type = t; }
		DDLIB_OB_TYPE m_ob_type;
	};

	class FFOB_CLASS VendorInfo : public DDLibraryObjectBase
	{
	public:
		VendorInfo(unsigned int id);
		~VendorInfo();
		
		bool set_name(string name)
		{
			bool same = (m_vendor_name.length() == 0 || m_vendor_name.compare(name) == 0);
			m_vendor_name = name;
			return same;
		}

		const string get_name() { return m_vendor_name; }
		unsigned int get_id() { return m_vendor_id; }

		DeviceType* get_device(unsigned short dev_id, bool create_if_not_found = true);
		const map<unsigned short, DeviceType*>& get_device_map() { return m_map_device; }

	private:
		string m_vendor_name;
		unsigned int m_vendor_id;

		// key: device id
		map<unsigned short, DeviceType*> m_map_device;
	};

	class FFOB_CLASS DeviceType : public DDLibraryObjectBase
	{
	public:
		DeviceType(unsigned short id);
		~DeviceType();
		void set_name(string name) { m_dev_name = name; }
		const string& get_name() { return m_dev_name; }
		unsigned int get_type_id() { return m_dev_type_id; }
		DeviceVersion* get_version(unsigned char vid, bool create_if_not_found = true);
		const map<unsigned char, DeviceVersion*>& get_dev_ver_map() { return m_map_dev_version; }

	private:
		string m_dev_name;
		unsigned int m_dev_type_id;
		map<unsigned char, DeviceVersion*> m_map_dev_version;
	};

	class FFOB_CLASS DeviceVersion : public DDLibraryObjectBase
	{
	public:
		DeviceVersion(unsigned char vid);
		~DeviceVersion();
		DDVersion* get_dd_version(unsigned char vid, bool create_if_not_found = true);
		unsigned char get_version() { return m_version; }
		const map<unsigned char, DDVersion*>& get_dd_ver_map() { return m_map_dd_version; }

	private:
		unsigned char m_version;
		map<unsigned char, DDVersion*> m_map_dd_version;
	};

	class FFOB_CLASS DDVersion : public DDLibraryObjectBase
	{
	public:
		DDVersion(unsigned char vid);
		~DDVersion();
		CFFVersion* get_cff_version(unsigned char vid, bool create_if_not_found = true);
		unsigned char get_version() { return m_version; }
		const map<unsigned char, CFFVersion*>& get_cff_ver_map() { return m_map_cff_version; }

	private:
		unsigned char m_version;
		map<unsigned char, CFFVersion*> m_map_cff_version;
	};

	class FFOB_CLASS VFDCapabilityLevel
	{
	public:
		VFDCapabilityLevel(const CString& sLevelLabel, WORD wLevelValue, BYTE ucDynInsBlkNum)
			: m_sLevelLabel(sLevelLabel), m_wLevelValue(wLevelValue), m_ucDynInsBlkNum(ucDynInsBlkNum)
		{
		}
		~VFDCapabilityLevel();

		void set_parent(VFDCapability* p) { m_parent = p; }
		bool parse_function_block(int iIndex, BOOL* pbNoSection, const string cff_fn);
		bool parse_transducer_block(int iIndex, int& iTBIndex, const string cff_fn);
		DDBlock* parse_capa_level_block(const CString& sSection, const string cff_fn);
		list<DDBlock*>& get_block_list() { return m_list_blocks; }
		uint8 get_dyn_inst_block_num() { return m_ucDynInsBlkNum; }
		DDBlock* find_block(uint32 dd_item);

	private:
		CString m_sLevelLabel;
		WORD m_wLevelValue;	
		BYTE m_ucDynInsBlkNum; // �����ǰ��VFD��֧�ֶ�̬�������ܿ飬��ֵΪ0

		VFDCapability* m_parent;

		list<DDBlock*> m_list_blocks;
	};

	class FFOB_CLASS VFDCapability
	{
	public:
		VFDCapability(int iVFDID, BYTE ucDynInsBlkTypeNum, BOOL bIsExclusiveLevel = TRUE)
		{
			m_dwVFDRef = 0;
			m_iVFDID = iVFDID;
			m_ucDynInsBlkTypeNum = ucDynInsBlkTypeNum;
			m_bIsExclusiveLevel = bIsExclusiveLevel;
		};
		~VFDCapability();
		void set_parent(CFFVersion* p) { m_parent_cff = p; }
		int get_vfd_id() { return m_iVFDID; }
		bool parse(const string cff_fn);
		bool parse_permanent(const string cff_fn);
		bool collect_default_value(const string cff_fn);
		bool is_permanent_block(const CString& sSection, const string cff_fn);
		const map<WORD, WORD>& get_resource_limit_map() { return m_map_resource_limit; }
		const list<DDBlock*>& get_block_list() { return m_list_blocks; }
		DDBlock* find_block(uint16 block_index);
		uint8 find_inst_blk_num();
		VFDCapabilityLevel* get_level_ob(uint8 level_id);

		enum VFD_OD_INF_TYPE
		{
			ACTIONOBJ_STARTING_ODINDEX,		// ʵ�����������ʼ����
			ACTIONOBJ_NUMBER,				// ʵ��������ĸ���
			LINKOBJ_STARTING_ODINDEX,		// ���Ӷ������ʼ����
			LINKOBJ_NUMBER,					// ���Ӷ���ĸ���
			ALERTOBJ_STARTING_ODINDEX,		// �����������ʼ����
			ALERTOBJ_NUMBER,				// ��������ĸ���
			TRENDOBJ_STARTING_ODINDEX,		// ���ƶ������ʼ����
			TRENDOBJ_NUMBER,				// ���ƶ���ĸ���
			DOMAINOBJ_STARTING_ODINDEX,		// ��������ʼ����
			DOMAINOBJ_NUMBER,				// �����ĸ���
		};

		uint16 get_vfd_od_val(VFD_OD_INF_TYPE type);

	private:
		bool parse_od(const string cff_fn);
		bool parse_capability(const string cff_fn);
		bool parse_capability_level(int level, const string cff_fn);
		void add_resource_limit_info(WORD wResID, WORD wResLimited);
		void collect_permanent_inst_block_list();

		CFFVersion* m_parent_cff;

		map<VFD_OD_INF_TYPE, WORD> m_map_od;
		map<WORD, WORD> m_map_resource_limit;
		map<WORD, VFDCapabilityLevel*> m_map_level;
		list<DDBlock*> m_list_blocks;

		// �����̻����ܿ�Ĭ��ֵ
		map<WORD, string> m_map_start_index_section;

		// ��ֵ������ʱ��Ҫ
		DWORD m_dwVFDRef;

		// VFD��ID��ʶ����2��ʼ��1��MIB��Ŀǰ����֧��һ�����ܿ�VFD
		int  m_iVFDID;
		
		// ֵΪ0��ʾ��ǰ��VFD��֧�ֶ�̬�������ܿ�ʵ������
		// ����֧�ֲ�ͬ����������(��ͬ���ܿ�����)
		BYTE m_ucDynInsBlkTypeNum;
		
		// ��ʾ��ǰ��VFD���������Ƿ��ǻ����ų�
		BOOL m_bIsExclusiveLevel;

		WORD m_wDirectoryIndexForTheResourceBlock;
		WORD m_wNumberOfResourceBlocksInTheVfd;
		WORD m_wDirIndexForTheFirstTransducerBlockPointer;
		WORD m_wNumberOfTransducerBlocksInTheVfd;
		WORD m_wDirIndexForTheFirstFunctionBlockPointer;
		WORD m_wNumberOfFunctionBlocksInTheVfd;

	};

	class FFOB_CLASS CFFVersion : public DDLibraryObjectBase
	{
	public:
		CFFVersion(unsigned char vid);
		~CFFVersion();
		bool parse_capability(const string cff_fn);
		void set_vendor_id(unsigned int id) { m_vendor_id = id; }
		void set_dev_type_id(unsigned short id) { m_dev_type_id = id; }
		void set_dev_version(unsigned char id) { m_dev_version = id; }
		void set_dev_dd_version(unsigned char id) { m_dev_dd_version = id; }
		void set_cff_version_str(CString vs) { m_cff_ver_str = vs; }
		const CString get_cff_version_str() { return m_cff_ver_str; }
		unsigned char get_cff_version() { return m_cff_version; }
		bool load_block_table();
		string get_block_name(uint32 dd_item);
	//	void release_dd_table();

		VFDCapability* get_vfd_capa_ob();

		uint32 get_vfd_ref_id() { return m_vfd_ref_id; }

		enum FF_DEV_CLASS
		{
			FF_BASIC_DEV = 1,		// FF�����豸��������������LAS
			FF_LINKMASTER_DEV = 2,	// FF��·���豸(���л����豸��һ������)������������LAS
			FF_BRIDGE_DEV = 3,		// FF���豸(������·���豸��һ������)��������LAS��
									// �����ǵ�ϵͳ�У���ʱ�����豸��Ϊ�������豸ʹ�ã�
									// ��ʹ���ŵĹ���
			FF_UNKNOWN_DEV = 255,	// ��Ч��FF�豸
		};

		uint16 get_mib_od_val(MIB_S_OD_TYPE type);
		uint16 get_schd_capa_val(SCHEDULE_CAP_TYPE capa_type);

	private:
		bool parse_header(const string cff_fn);
		bool parse_nm(const string cff_fn);
		bool parse_nm_strict(const string cff_fn);
		bool parse_sm(const string cff_fn);
		bool parse_vfd(const string cff_fn);

		VFDCapability* create_vfd_capa_ob(int iVFDID, BYTE ucDynInsBlkTypeNum, BOOL bIsExclusiveLevel = TRUE);

		unsigned int m_vendor_id;
		unsigned short m_dev_type_id;
		unsigned char m_dev_version;
		unsigned char m_dev_dd_version;
		unsigned char m_cff_version;
		CString m_cff_ver_str;
		FF_DEV_CLASS m_dev_class;
		
		uint32 m_vfd_ref_id;

		map<MIB_S_OD_TYPE, WORD> m_map_mib_od;
		map<SCHEDULE_CAP_TYPE, WORD> m_map_schedule_capa_info;
		map<int, VFDCapability*> m_map_vfd_capa;
	//	map<unsigned int, FunctionBlock*> m_map_blocks;
		map<uint32, string> m_map_block_name;
	};

	class FFOB_CLASS DDBlock : public DDLibraryObjectBase
	{
	public:
		DDBlock(BYTE ucDynInsBlkNum, BYTE ucPermanentBlkNum, DWORD dwTempBlkObjID,
			WORD wProfile, WORD wProfileRev);

		DDBlock(int iVFDID, WORD wStartIdx, WORD wViewIndex,
			WORD wProfile, WORD wProfileRev, DWORD dwExecuteTime);

		~DDBlock();

		static DDBlock* parse(const CString& sSection, BOOL bIsFuncBlk, BOOL* pbNoSection,
			int vfd_id, const string cff_fn);
		static bool parse_block_cfg(const CString& sSection, const CString& sKey,
			const CString& sPath, DWORD& dwValue, BOOL& bRead, BOOL& bReadAll, 
			BOOL bMustExists, BOOL bEnableZero = FALSE);
		
		// for VFD capability level parsing
		bool parse_inst(const CString& sSection, BOOL bIsFuncBlk, BOOL* pbNoSection, const string cff_fn);
		bool parse_inst_type(int vfd_id, const string cff_fn);
		void add_resource_limit(WORD wResID, WORD wResLimited);
		bool match_block_type_section(const CString& sSection, WORD wProfile, WORD wProfileRev, const string cff_fn);
	//	bool load_dd_info();

		int get_type() { return m_iBlockType; }
		unsigned char get_permanent_block_number() { return m_ucPermanentBlkNum; }
		uint8 get_instantiable_number() { return m_ucDynInsBlkNum; }
		WORD get_profile() { return m_wProfile; }
		WORD get_profile_revision() { return m_wProfileRev; }
		DWORD get_dd_item() { return m_dwBlkDDItem; }
		DWORD get_exec_time() { return m_dwExecuTime; }
		void set_type(FFBLOCK_TYPE t) { m_iBlockType = t; }
		void set_block_id(DWORD id) { m_dwBlockID = id; }
		const string& get_block_name() { return m_block_name; }
		void set_block_name(string name) { m_block_name = name; }
		void set_dd_item(DWORD v) { m_dwBlkDDItem = v; }
		WORD get_start_index() { return m_wStartIndex; }
		void set_start_index(WORD idx) { m_wStartIndex = idx; }
	private:

 		string m_block_name;
// 		string m_block_type;
// 		unsigned int m_block_index;
// 		unsigned int m_exec_time; // 1/32 ms
// 		unsigned int block_type; // enum ?

		int m_iBlockType;			// ��Դ�顢���ܿ�/ת����
		DWORD m_dwTempBlkObjID;		// ���в����ṹ��Ϣ�Ĺ��ܿ��OID��Ϣ
		DWORD m_dwExecuTime;		// ���ܿ��ִ��ʱ��
		DWORD m_dwBlkDDItem;		// ���ܿ��DDITEM��Ϣ
		WORD m_wProfile;			// ���ܿ��Profile��Ϣ
		WORD m_wProfileRev;			// ���ܿ��Profle_Revision��Ϣ
		BYTE m_ucDynInsBlkNum;		// �ڸü����е�ǰ���͹��ܿ����ʵ�����ĸ���
		BYTE m_ucPermanentBlkNum;	// �ڸü����е�ǰ���͹��ܿ���Թ̻��ĸ���

		//////////////////////////////////////////////////////////////////////////
		// from class CFFTempBlockOb

		DWORD m_dwBlockID;

		// ���ܿ�����VFD����������������ǰ�Ĺ��ܿ��Ǵ��в�����Ϣ�Ĺ��ܿ�
		// ��ֵΪ0��VFD��������2��ʼ��1��ʾMIB��Ϣ
		int m_iVFDIdx;

		// ���ܿ����ʼ�����������ǰ�Ĺ��ܿ��Ǵ��в�����Ϣ�Ĺ��ܿ飬��ֵΪ0
		WORD m_wStartIndex;

		// ����VIEW�������ʼ������Ϣ�������ǰ�Ĺ��ܿ��Ǵ��в�����Ϣ�Ĺ��ܿ�ֵΪ0
		WORD m_wViewStartIndex;
		//////////////////////////////////////////////////////////////////////////

		// ֻ���ڵ�ǰ�Ĺ��ܿ�֧�ֶ�̬ʵ��������£��б�Ϊ��
		map<WORD, WORD> m_map_resource_limit;
	};

}
