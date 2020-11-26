/////////////////////////////////////////////////////////////////////////////
// MIB.h
/////////////////////////////////////////////////////////////////////////////
#ifndef __MIB_H__
#define __MIB_H__

#include "FFDef.h"

#define MIBP_READ			1
#define MIBP_WRITE			2
#define MIBP_READWRITE		3 // MIBP_READ | MIBP_WRITE

#define PARAM_READ			1
#define PARAM_WRITE			2
#define PARAM_READWRITE		3 // MIBP_READ | MIBP_WRITE

#define MIB_PARAM_COUNT		31

#define NONE		0 // 

typedef struct _MIB_PARAM 
{
	unsigned int	nID;
	unsigned int	nDataType;
	unsigned int	nSize;
	unsigned int	nAccessRight; // read / write...
	char*	sName;
} MIB_PARAM;

/* define basic date type */
/*typedef unsigned char	  BOOL;                 /* index 1 -- basic data type */
// 2009-08-20 9:35:32, KK
#ifdef _VC_6_
typedef char              INT8;                 /* index 2 -- basic data type */
#endif
typedef short             INT16;                /* index 3 -- basic data type */
typedef int           	  INT32;                /* index 4 -- basic data type */
typedef unsigned char     USIGN8;               /* index 5 -- basic data type */
typedef unsigned short    USIGN16;              /* index 6 -- basic data type */
typedef unsigned long     USIGN32;              /* index 7 -- basic data type */
//typedef float             FLOAT;                /* index 8 -- basic data type */
typedef char		      VIS_STRING;			/* index 9 -- basic data type */
typedef unsigned char	  OCT_STRING;			/* index 10-- basic data type */

typedef struct {
	unsigned char      ucDateMs1;
	unsigned char      ucDateMs2;
	unsigned char      ucDateMin;
	unsigned char      ucDateHour;
	unsigned char      ucDateDay;
	unsigned char      ucDateMon;
	unsigned char      ucDateYear;
} FF_DATE;										/* index 11-- basic data type */

typedef struct {
	USIGN32		ulMsec;
	USIGN16		uDay;
} FF_TIME_OFDAY;								/* index 12-- basic data type */

typedef FF_TIME_OFDAY	   FF_TIME_DIFF;		/* index 13-- basic data type */
typedef USIGN8			   BIT_STRING;			/* index 14-- basic data type */

typedef struct {
    INT32		lHighValue;
    USIGN32		ulLowValue;
} FF_TIME_VALUE;								/* index 21-- basic data type */

/* define true/false macro */
#define FF_TRUE			0xFF
#define FF_FALSE		0x00

// #endif

#pragma pack(push)
#pragma pack(1)

typedef unsigned char	uchar;
typedef unsigned short	ushort;
typedef unsigned long	ulong;

typedef unsigned char UINT8;
typedef short INT16;
typedef unsigned short UINT16;
//typedef long INT32;
//typedef unsigned long unsigned int;
typedef unsigned char OctetString;
typedef unsigned char BitString;

/*
  The following MIB parameters are elements of a list (such as the list of
  Function Block Schedule entries or VCR entries).
  You can use the name for the items with a ".X" appended, where X is the
  element in the list you want to access. For example, the first Function
  Block Schedule entry in the MIB is named "FB_START_ENTRY.1", the first VCR
  static entry in the MIB is named "VCR_STATIC_ENTRY.1". If X exceeds the
  number of objects of that type in the MIB, the NI-FBUS Communications
  Manager returns the E_ORDINAL_NUM_OUT_OF_RANGE error code.
*/

typedef struct{ //system manager
	unsigned int			StartTimeOffset;
	UINT16			FbObjectIndex;
	unsigned int			VfdRef;
}FB_START_ENTRY;

typedef struct{ //system manager
	unsigned int			VfdRef;
	unsigned char	VfdTag[32];
}VFD_REF_ENTRY;

typedef struct{
	OctetString		FasArTypeAndRole;  //1 byte
	unsigned int			FasDllLocalAddr;
	unsigned int			FasDllConfigedRemoteAddr;
	OctetString		FasDllSDAP;
	UINT16			FasDllMaxConfirmDelayOnConnect;
	UINT16			FasDllMaxConfirmDelayOnData;
	UINT16			FasDllMaxDlsduSize;
	uint8			FasDllResidualActivitySupported;
	OctetString		FasDllTimeLinessClass;
	UINT16			FasDllPublisherTimeWindowSize;
	unsigned int			FasDllPublisherSynchronizingDlcep;
	UINT16			FasDllSubscriberTimeWindowSize;
	unsigned int			FasDllSubscriberSynchronizingDlcep;
	unsigned int			FmsVfdId;
	UINT8			FmsMaxOutstandingServicesCalling;
	UINT8			FmsMaxOutstandingServicesCalled;
	BitString		FmsFeaturesSupported[8];
} VCR_STATIC_ENTRY;

typedef struct{
	UINT16			Version;
	unsigned int			MacrocycleDuration;
	UINT16			TimeResolution;
}SCHEDULE_DESCRIPTOR;

typedef struct{
	OctetString		MaxTokenHoldTime[8][64];
}MAX_TOKEN_HOLD_TIME;  //LINK MASTER(interface) DEVICE MIB

// end of MIB parameters used by appending with a ".X"

typedef struct{
	UINT8		Version;
	bool		BasicStatisticsSupportedFlag;
	UINT8		DlOperatFunctionalClass;
	OctetString	DlDeviceConformance[4]; // 4 bytes
}DLME_BASIC_charACTERISTICS;

typedef struct{
	UINT16		SlotTime;
	UINT8		PerDlpduPhlOverhead;
	UINT8		MaxResponseDelay;
	UINT8		ThisNode;
	UINT16		ThisLink;
	UINT8		MinInterPduDelay;
	UINT8		TimeSyncClass;
	UINT8		PreambleExtersion;
	UINT8		PostTransGapExtension;
	UINT8		MaxInterChanSignalSkew;
}DLME_BASIC_INFO;

typedef struct{
	UINT8		MaxSchedulingOverhead;
	UINT16		DefMinTokenDeleyTime;
	UINT16		DefTokenHoldTime;
	UINT16		TargetTokenRotTime;
	UINT8		FistUnpolledNodeld;
	UINT8		NumConsecUnpolledNodeld;
	UINT16		LinkMaintTokHoldTime;
	unsigned int		TimeDistributionPeriod;
	UINT16		MaximunInactivityToClaimLasDelay;
	UINT16		LasDatabaseStatusSpduDistributionPeriod;
}DLME_LINK_MASTER_INFO;

typedef struct{
	OctetString	FasArTypeAndROleSupported[4]; // 4 bytes
	UINT16		MaxDlsapAddressesSupported;
	UINT16		MaxDlcepAddressesSupported;
	OctetString DlcepDeliveryFeaturesSupported; // 1 bytes
	UINT16		VersionOfNmSpecSupported;
	OctetString AgentFunctionsSupported;  //1 byte
	BitString	FmsFeaturesSupported[8];  //8 bytes
}STACK_CAPABILITIES;

//  FMS/FAS Management Data Structure Defintion
typedef struct{
    uint16		Version;
	uint16		MaxEntries;
	uint16		NumPermanentEntries;
	uint16		NumCurrentlyConfigured;
	uint16		FirstUnconfiguredEntry;
	bool		DynamicSupportedFlag;
	OctetString	StatisticsSupported;   // 1 byte
	uint16		NumOfStatisticsEntries;
} VCR_LIST_CHARACTERISTICS;

typedef struct{
	OctetString	ChannelStatisticsSupported;  // 1 byte
	OctetString	MediumAndDataRatesSupported[8];  // 8 bytes
	UINT16		IecVersion;
	UINT8		NumOfChannels;
	UINT8		PowerMode;
}PLME_BASIC_charACTERISTICS;

typedef struct{
	UINT8	ChannelStatesVariable[8];
}CHANNEL_STATES;

typedef struct{
	UINT8		InterfaceMode;
	UINT8		LoopBackMode;
	UINT8		XmitEbanled;
	UINT8		RcvEnabled;
	UINT8		PreferredReceivedChannel;
	UINT8		MediaTypeSelected;
	UINT8		ReceiveSelect;
}PLME_BASIC_INFO;

typedef struct{
	UINT8		ChannalNumber;
	unsigned int		TotalGoodMsgSent;
	unsigned int		TotalGoodMsgRcvd;
	UINT16		NumOfJabberFaults;
	UINT16		NumOfInternAndJabberFaults;
	UINT16		NumEndActivityBeforeEndData;
}MME_WIRE_INFO;

typedef struct{
		UINT8		NumOfSchedules;
		UINT8		NumOfSubschedulesPerSchedule;
		UINT16		ActiveScheduleVersion;
		UINT16		ActiveScheduleOdIndex;
		nifTime_t	ActiveScheduleStartTime;
}LINK_SCHEDULE_LIST_charACTERISTICS;

typedef struct
{
	UINT16		SlotTime;     //ST
	UINT8		PerDlpduPhlOverhead;   //PhLO
	UINT8		MaxResponseDelay;   //MRD
	UINT8		FirstUnpolledNodeId;   //TN
	UINT16		ThisLink;     //TL
	UINT8		MinInterPduDelay;   //MID
	UINT8		NumConsecUnpolledNodeId;
	UINT8		PreambleExtension;
	UINT8		PostTransGapExtension;
	UINT8		MaxInterChanSignalSkew;
	UINT8		TimeSyscClass;    //TSC
} DLME_LINK_SETTING;

// NOTE: in VC 4.2, bool is int, in VC 5.0 and later, bool is byte,
// so sizeof(bool) = 1 and sizeof(BOOL) = 4 now...
typedef struct 
{
	//The following are System management parameters
	unsigned int		SM_SUPPORT;
	unsigned int		T1;
	unsigned int		T2;
	unsigned int		T3;
	nifTime_t	CURRENT_TIME;
	INT32		LOCAL_TIME_DIFF;
	UINT8		AP_CLOCK_SYNC_INTERVAL;
	nifTime_t	TIME_LAST_RCVD;
	UINT8		PRIMARY_AP_TIME_PUBLISHER;
	UINT8		TIME_PUBLISHER_ADDR;
	nifTime_t	LAST_T0;
	unsigned int		MACROCYCLE_DURATION;
	char		DEV_ID[32];
	char		PD_TAG[32];
	bool		OPERATIONAL_POWERUP;
	UINT16		VERSION_OF_SCHEDULE;
	
	// the following are Network management parameters
	
	DLME_BASIC_charACTERISTICS	DlmeBasicCharacteristics;
	DLME_BASIC_INFO				DlmeBasicInfo;
	DLME_LINK_MASTER_INFO		DlmeLinkMasterInfo;
	STACK_CAPABILITIES			StackCapabilities;
    VCR_LIST_CHARACTERISTICS	VcrListCharacteristics;
	PLME_BASIC_charACTERISTICS	PlmeBasicCharacteristics;
	CHANNEL_STATES				ChannelStates;
	PLME_BASIC_INFO				PlmeBasicInfo;
	MME_WIRE_INFO				MmeWireInfo;
	bool						PRIMARY_LINK_MASTER_FLAG;
	OctetString					LIVE_LIST_STATUS[32]; //32 bytes
	UINT16						LINK_SCHEDULE_ACTIVATION;
	LINK_SCHEDULE_LIST_charACTERISTICS	LinkScheduleListCharacteristics;
	DLME_LINK_SETTING			CurrentLinkSetting; // kking, 2003-11-07
	DLME_LINK_SETTING			ConfigureedLinkSetting;
} DEV_MIB;

typedef struct __h1_linkage
{
	USIGN16		LocalIndex;		// 发送/接受数据的参数索引	
	USIGN16		VcrNumber;		// 相关VCR索引...
	USIGN16		RemoteIndex;	// 发送数据的参数索引（如果当前的LINKAGE用于发送数据，该值为0）
	USIGN8		ServiceOperation;
	USIGN8		StateCountLimit;
} H1_LINKAGE;

//the structs for schedule
typedef enum _PRIORITY
{
	PRIORITY1 = 1,
	PRIORITY2,
	PRIORITY3,
} PRIORITY;

typedef struct _ELEMENT
{
	PRIORITY Priority;
    long Address;
} ELEMENT;

typedef struct _SEQUENCE
{
    unsigned long    StartTimeOffset;
    long     WorstCaseDuration;
	ELEMENT* pElement;
} SEQUENCE;

typedef struct _SUBSCHEDULE
{
    unsigned long     Period;
    long      NumOfSequence;
	SEQUENCE* pSequenceList;
} SUBSCHEDULE;

typedef struct _LM_SCHEDULE
{
    long          VersionNumber;
    long          BuildID;
    unsigned long          MacroCycle;
    unsigned char          NumOfSubSchedule;
//	WORD*         pSubSchRef
	SUBSCHEDULE*  pSubScheduleList;
} LM_SCHEDULE;

typedef char		      VIS_STRING;			/* index 9 -- basic data type */

#pragma pack(pop)

#endif
