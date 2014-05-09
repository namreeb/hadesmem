// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>
#include <winnt.h>
#include <winternl.h>

// Structures/enums/macros/etc. shamelessly taken from
// http://bit.ly/1cxEVDJ, http://bit.ly/1cm5xnC, http://bit.ly/1bXTstU,
// http://bit.ly/1nuwpd6, etc.

#define HADESMEM_DETAIL_STATUS_NO_SUCH_FILE (static_cast<NTSTATUS>(0xC000000FL))
#define HADESMEM_DETAIL_STATUS_NO_MORE_FILES                                   \
  (static_cast<NTSTATUS>(0x80000006L))
#define HADESMEM_DETAIL_STATUS_INFO_LENGTH_MISMATCH                            \
  (static_cast<NTSTATUS>(0xC0000004L)
#define HADESMEM_DETAIL_RTL_USER_PROC_PARAMS_NORMALIZED 0x00000001

namespace hadesmem
{

namespace detail
{

namespace winternl
{

enum SYSTEM_INFORMATION_CLASS
{
  SystemBasicInformation = 0x0000,
  SystemProcessorInformation = 0x0001,
  SystemPerformanceInformation = 0x0002,
  SystemTimeOfDayInformation = 0x0003,
  SystemPathInformation = 0x0004,
  SystemProcessInformation = 0x0005,
  SystemCallCountInformation = 0x0006,
  SystemDeviceInformation = 0x0007,
  SystemProcessorPerformanceInformation = 0x0008,
  SystemFlagsInformation = 0x0009,
  SystemCallTimeInformation = 0x000A,
  SystemModuleInformation = 0x000B,
  SystemLocksInformation = 0x000C,
  SystemStackTraceInformation = 0x000D,
  SystemPagedPoolInformation = 0x000E,
  SystemNonPagedPoolInformation = 0x000F,
  SystemHandleInformation = 0x0010,
  SystemObjectInformation = 0x0011,
  SystemPageFileInformation = 0x0012,
  SystemVdmInstemulInformation = 0x0013,
  SystemVdmBopInformation = 0x0014,
  SystemFileCacheInformation = 0x0015,
  SystemPoolTagInformation = 0x0016,
  SystemInterruptInformation = 0x0017,
  SystemDpcBehaviorInformation = 0x0018,
  SystemFullMemoryInformation = 0x0019,
  SystemLoadGdiDriverInformation = 0x001A,
  SystemUnloadGdiDriverInformation = 0x001B,
  SystemTimeAdjustmentInformation = 0x001C,
  SystemSummaryMemoryInformation = 0x001D,
  SystemMirrorMemoryInformation = 0x001E,
  SystemPerformanceTraceInformation = 0x001F,
  SystemCrashDumpInformation = 0x0020,
  SystemExceptionInformation = 0x0021,
  SystemCrashDumpStateInformation = 0x0022,
  SystemKernelDebuggerInformation = 0x0023,
  SystemContextSwitchInformation = 0x0024,
  SystemRegistryQuotaInformation = 0x0025,
  SystemExtendServiceTableInformation = 0x0026,
  SystemPrioritySeperation = 0x0027,
  SystemVerifierAddDriverInformation = 0x0028,
  SystemVerifierRemoveDriverInformation = 0x0029,
  SystemProcessorIdleInformation = 0x002A,
  SystemLegacyDriverInformation = 0x002B,
  SystemCurrentTimeZoneInformation = 0x002C,
  SystemLookasideInformation = 0x002D,
  SystemTimeSlipNotification = 0x002E,
  SystemSessionCreate = 0x002F,
  SystemSessionDetach = 0x0030,
  SystemSessionInformation = 0x0031,
  SystemRangeStartInformation = 0x0032,
  SystemVerifierInformation = 0x0033,
  SystemVerifierThunkExtend = 0x0034,
  SystemSessionProcessInformation = 0x0035,
  SystemLoadGdiDriverInSystemSpace = 0x0036,
  SystemNumaProcessorMap = 0x0037,
  SystemPrefetcherInformation = 0x0038,
  SystemExtendedProcessInformation = 0x0039,
  SystemRecommendedSharedDataAlignment = 0x003A,
  SystemComPlusPackage = 0x003B,
  SystemNumaAvailableMemory = 0x003C,
  SystemProcessorPowerInformation = 0x003D,
  SystemEmulationBasicInformation = 0x003E,
  SystemEmulationProcessorInformation = 0x003F,
  SystemExtendedHandleInformation = 0x0040,
  SystemLostDelayedWriteInformation = 0x0041,
  SystemBigPoolInformation = 0x0042,
  SystemSessionPoolTagInformation = 0x0043,
  SystemSessionMappedViewInformation = 0x0044,
  SystemHotpatchInformation = 0x0045,
  SystemObjectSecurityMode = 0x0046,
  SystemWatchdogTimerHandler = 0x0047,
  SystemWatchdogTimerInformation = 0x0048,
  SystemLogicalProcessorInformation = 0x0049,
  SystemWow64SharedInformationObsolete = 0x004A,
  SystemRegisterFirmwareTableInformationHandler = 0x004B,
  SystemFirmwareTableInformation = 0x004C,
  SystemModuleInformationEx = 0x004D,
  SystemVerifierTriageInformation = 0x004E,
  SystemSuperfetchInformation = 0x004F,
  SystemMemoryListInformation = 0x0050,
  SystemFileCacheInformationEx = 0x0051,
  SystemThreadPriorityClientIdInformation = 0x0052,
  SystemProcessorIdleCycleTimeInformation = 0x0053,
  SystemVerifierCancellationInformation = 0x0054,
  SystemProcessorPowerInformationEx = 0x0055,
  SystemRefTraceInformation = 0x0056,
  SystemSpecialPoolInformation = 0x0057,
  SystemProcessIdInformation = 0x0058,
  SystemErrorPortInformation = 0x0059,
  SystemBootEnvironmentInformation = 0x005A,
  SystemHypervisorInformation = 0x005B,
  SystemVerifierInformationEx = 0x005C,
  SystemTimeZoneInformation = 0x005D,
  SystemImageFileExecutionOptionsInformation = 0x005E,
  SystemCoverageInformation = 0x005F,
  SystemPrefetchPatchInformation = 0x0060,
  SystemVerifierFaultsInformation = 0x0061,
  SystemSystemPartitionInformation = 0x0062,
  SystemSystemDiskInformation = 0x0063,
  SystemProcessorPerformanceDistribution = 0x0064,
  SystemNumaProximityNodeInformation = 0x0065,
  SystemDynamicTimeZoneInformation = 0x0066,
  SystemCodeIntegrityInformation = 0x0067,
  SystemProcessorMicrocodeUpdateInformation = 0x0068,
  SystemProcessorBrandString = 0x0069,
  SystemVirtualAddressInformation = 0x006A,
  SystemLogicalProcessorAndGroupInformation = 0x006B,
  SystemProcessorCycleTimeInformation = 0x006C,
  SystemStoreInformation = 0x006D,
  SystemRegistryAppendString = 0x006E,
  SystemAitSamplingValue = 0x006F,
  SystemVhdBootInformation = 0x0070,
  SystemCpuQuotaInformation = 0x0071,
  SystemNativeBasicInformation = 0x0072,
  SystemErrorPortTimeouts = 0x0073,
  SystemLowPriorityIoInformation = 0x0074,
  SystemBootEntropyInformation = 0x0075,
  SystemVerifierCountersInformation = 0x0076,
  SystemPagedPoolInformationEx = 0x0077,
  SystemSystemPtesInformationEx = 0x0078,
  SystemNodeDistanceInformation = 0x0079,
  SystemAcpiAuditInformation = 0x007A,
  SystemBasicPerformanceInformation = 0x007B,
  SystemQueryPerformanceCounterInformation = 0x007C,
  SystemSessionBigPoolInformation = 0x007D,
  SystemBootGraphicsInformation = 0x007E,
  SystemScrubPhysicalMemoryInformation = 0x007F,
  SystemBadPageInformation = 0x0080,
  SystemProcessorProfileControlArea = 0x0081,
  SystemCombinePhysicalMemoryInformation = 0x0082,
  SystemEntropyInterruptTimingInformation = 0x0083,
  SystemConsoleInformation = 0x0084,
  SystemPlatformBinaryInformation = 0x0085,
  SystemThrottleNotificationInformation = 0x0086,
  SystemHypervisorProcessorCountInformation = 0x0087,
  SystemDeviceDataInformation = 0x0088,
  SystemDeviceDataEnumerationInformation = 0x0089,
  SystemMemoryTopologyInformation = 0x008A,
  SystemMemoryChannelInformation = 0x008B,
  SystemBootLogoInformation = 0x008C,
  SystemProcessorPerformanceInformationEx = 0x008D,
  SystemSpare0 = 0x008E,
  SystemSecureBootPolicyInformation = 0x008F,
  SystemPageFileInformationEx = 0x0090,
  SystemSecureBootInformation = 0x0091,
  SystemEntropyInterruptTimingRawInformation = 0x0092,
  SystemPortableWorkspaceEfiLauncherInformation = 0x0093,
  SystemFullProcessInformation = 0x0094,
  MaxSystemInfoClass = 0x0095
};

struct CLIENT_ID
{
  PVOID UniqueProcess;
  PVOID UniqueThread;
};

struct SYSTEM_THREAD_INFORMATION
{
  LARGE_INTEGER KernelTime;
  LARGE_INTEGER UserTime;
  LARGE_INTEGER CreateTime;
  ULONG WaitTime;
  PVOID StartAddress;
  CLIENT_ID ClientId;
  LONG Priority;
  LONG BasePriority;
  ULONG ContextSwitches;
  ULONG ThreadState;
  ULONG WaitReason;
};

struct SYSTEM_EXTENDED_THREAD_INFORMATION
{
  SYSTEM_THREAD_INFORMATION ThreadInfo;
  PVOID StackBase;
  PVOID StackLimit;
  PVOID Win32StartAddress;
  PVOID TebBase;
  ULONG Reserved2;
  ULONG Reserved3;
  ULONG Reserved4;
};

struct SYSTEM_PROCESS_INFORMATION
{
  ULONG NextEntryOffset;
  ULONG NumberOfThreads;
  LARGE_INTEGER WorkingSetPrivateSize;
  ULONG HardFaultCount;
  ULONG NumberOfThreadsHighWatermark;
  ULONGLONG CycleTime;
  LARGE_INTEGER CreateTime;
  LARGE_INTEGER UserTime;
  LARGE_INTEGER KernelTime;
  UNICODE_STRING ImageName;
  LONG BasePriority;
  PVOID UniqueProcessId;
  PVOID InheritedFromUniqueProcessId;
  ULONG HandleCount;
  ULONG SessionId;
  ULONG UniqueProcessKey;
  ULONG PeakVirtualSize;
  ULONG VirtualSize;
  ULONG PageFaultCount;
  ULONG PeakWorkingSetSize;
  ULONG WorkingSetSize;
  ULONG QuotaPeakPagedPoolUsage;
  ULONG QuotaPagedPoolUsage;
  ULONG QuotaPeakNonPagedPoolUsage;
  ULONG QuotaNonPagedPoolUsage;
  ULONG PagefileUsage;
  ULONG PeakPagefileUsage;
  ULONG PrivatePageCount;
  LARGE_INTEGER ReadOperationCount;
  LARGE_INTEGER WriteOperationCount;
  LARGE_INTEGER OtherOperationCount;
  LARGE_INTEGER ReadTransferCount;
  LARGE_INTEGER WriteTransferCount;
  LARGE_INTEGER OtherTransferCount;
  // SystemProcessInformation
  // SYSTEM_THREAD_INFORMATION Threads[0];
  // SystemExtendedProcessinformation
  // SYSTEM_EXTENDED_THREAD_INFORMATION Threads[0];
  // SystemFullProcessInformation
  // SYSTEM_EXTENDED_THREAD_INFORMATION Threads[0];
  // SYSTEM_PROCESS_INFORMATION_EXTENSION Extension;
};

struct PROCESS_DISK_COUNTERS
{
  ULONG64 BytesRead;
  ULONG64 BytesWritten;
  ULONG64 ReadOperationCount;
  ULONG64 WriteOperationCount;
  ULONG64 FlushOperationCount;
};

struct SYSTEM_PROCESS_INFORMATION_EXTENSION
{
  PROCESS_DISK_COUNTERS DiskCounters;
  ULONG64 ContextSwitches;
  union
  {
    ULONG Flags;
    struct
    {
      ULONG HasStrongId : 1;
      ULONG Spare : 31;
    } s;
  } u;
  ULONG UserSidOffset;
};

struct SYSTEM_SESSION_PROCESS_INFORMATION
{
  ULONG SessionId;
  ULONG SizeOfBuf;
  PVOID Buffer; // SYSTEM_PROCESS_INFORMATION*
};

struct SYSTEM_PROCESS_ID_INFORMATION
{
  PVOID ProcessId;
  UNICODE_STRING ImageName;
};

enum FILE_INFORMATION_CLASS
{
  FileDirectoryInformation = 1,
  FileFullDirectoryInformation,            // 2
  FileBothDirectoryInformation,            // 3
  FileBasicInformation,                    // 4
  FileStandardInformation,                 // 5
  FileInternalInformation,                 // 6
  FileEaInformation,                       // 7
  FileAccessInformation,                   // 8
  FileNameInformation,                     // 9
  FileRenameInformation,                   // 10
  FileLinkInformation,                     // 11
  FileNamesInformation,                    // 12
  FileDispositionInformation,              // 13
  FilePositionInformation,                 // 14
  FileFullEaInformation,                   // 15
  FileModeInformation,                     // 16
  FileAlignmentInformation,                // 17
  FileAllInformation,                      // 18
  FileAllocationInformation,               // 19
  FileEndOfFileInformation,                // 20
  FileAlternateNameInformation,            // 21
  FileStreamInformation,                   // 22
  FilePipeInformation,                     // 23
  FilePipeLocalInformation,                // 24
  FilePipeRemoteInformation,               // 25
  FileMailslotQueryInformation,            // 26
  FileMailslotSetInformation,              // 27
  FileCompressionInformation,              // 28
  FileObjectIdInformation,                 // 29
  FileCompletionInformation,               // 30
  FileMoveClusterInformation,              // 31
  FileQuotaInformation,                    // 32
  FileReparsePointInformation,             // 33
  FileNetworkOpenInformation,              // 34
  FileAttributeTagInformation,             // 35
  FileTrackingInformation,                 // 36
  FileIdBothDirectoryInformation,          // 37
  FileIdFullDirectoryInformation,          // 38
  FileValidDataLengthInformation,          // 39
  FileShortNameInformation,                // 40
  FileIoCompletionNotificationInformation, // 41
  FileIoStatusBlockRangeInformation,       // 42
  FileIoPriorityHintInformation,           // 43
  FileSfioReserveInformation,              // 44
  FileSfioVolumeInformation,               // 45
  FileHardLinkInformation,                 // 46
  FileProcessIdsUsingFileInformation,      // 47
  FileNormalizedNameInformation,           // 48
  FileNetworkPhysicalNameInformation,      // 49
  FileIdGlobalTxDirectoryInformation,      // 50
  FileIsRemoteDeviceInformation,           // 51
  FileUnusedInformation,                   // 52
  FileNumaNodeInformation,                 // 53
  FileStandardLinkInformation,             // 54
  FileRemoteProtocolInformation,           // 55
  FileRenameInformationBypassAccessCheck,  // 56
  FileLinkInformationBypassAccessCheck,    // 57
  FileVolumeNameInformation,               // 58
  FileIdInformation,                       // 59
  FileIdExtdDirectoryInformation,          // 60
  FileReplaceCompletionInformation,        // 61
  FileHardLinkFullIdInformation,           // 62
  FileMaximumInformation
};

struct FILE_DIRECTORY_INFORMATION
{
  ULONG NextEntryOffset;
  ULONG FileIndex;
  LARGE_INTEGER CreationTime;
  LARGE_INTEGER LastAccessTime;
  LARGE_INTEGER LastWriteTime;
  LARGE_INTEGER ChangeTime;
  LARGE_INTEGER EndOfFile;
  LARGE_INTEGER AllocationSize;
  ULONG FileAttributes;
  ULONG FileNameLength;
  WCHAR FileName[1];
};

struct FILE_FULL_DIR_INFORMATION
{
  ULONG NextEntryOffset;
  ULONG FileIndex;
  LARGE_INTEGER CreationTime;
  LARGE_INTEGER LastAccessTime;
  LARGE_INTEGER LastWriteTime;
  LARGE_INTEGER ChangeTime;
  LARGE_INTEGER EndOfFile;
  LARGE_INTEGER AllocationSize;
  ULONG FileAttributes;
  ULONG FileNameLength;
  ULONG EaSize;
  WCHAR FileName[1];
};

struct FILE_ID_FULL_DIR_INFORMATION
{
  ULONG NextEntryOffset;
  ULONG FileIndex;
  LARGE_INTEGER CreationTime;
  LARGE_INTEGER LastAccessTime;
  LARGE_INTEGER LastWriteTime;
  LARGE_INTEGER ChangeTime;
  LARGE_INTEGER EndOfFile;
  LARGE_INTEGER AllocationSize;
  ULONG FileAttributes;
  ULONG FileNameLength;
  ULONG EaSize;
  LARGE_INTEGER FileId;
  WCHAR FileName[1];
};

struct FILE_BOTH_DIR_INFORMATION
{
  ULONG NextEntryOffset;
  ULONG FileIndex;
  LARGE_INTEGER CreationTime;
  LARGE_INTEGER LastAccessTime;
  LARGE_INTEGER LastWriteTime;
  LARGE_INTEGER ChangeTime;
  LARGE_INTEGER EndOfFile;
  LARGE_INTEGER AllocationSize;
  ULONG FileAttributes;
  ULONG FileNameLength;
  ULONG EaSize;
  CCHAR ShortNameLength;
  WCHAR ShortName[12];
  WCHAR FileName[1];
};

struct FILE_ID_BOTH_DIR_INFORMATION
{
  ULONG NextEntryOffset;
  ULONG FileIndex;
  LARGE_INTEGER CreationTime;
  LARGE_INTEGER LastAccessTime;
  LARGE_INTEGER LastWriteTime;
  LARGE_INTEGER ChangeTime;
  LARGE_INTEGER EndOfFile;
  LARGE_INTEGER AllocationSize;
  ULONG FileAttributes;
  ULONG FileNameLength;
  ULONG EaSize;
  CCHAR ShortNameLength;
  WCHAR ShortName[12];
  LARGE_INTEGER FileId;
  WCHAR FileName[1];
};

struct FILE_NAMES_INFORMATION
{
  ULONG NextEntryOffset;
  ULONG FileIndex;
  ULONG FileNameLength;
  WCHAR FileName[1];
};

struct FILE_ID_GLOBAL_TX_DIR_INFORMATION
{
  ULONG NextEntryOffset;
  ULONG FileIndex;
  LARGE_INTEGER CreationTime;
  LARGE_INTEGER LastAccessTime;
  LARGE_INTEGER LastWriteTime;
  LARGE_INTEGER ChangeTime;
  LARGE_INTEGER EndOfFile;
  LARGE_INTEGER AllocationSize;
  ULONG FileAttributes;
  ULONG FileNameLength;
  LARGE_INTEGER FileId;
  GUID LockingTransactionId;
  ULONG TxInfoFlags;
  WCHAR FileName[1];
};
struct FILE_ID_EXTD_DIR_INFORMATION
{
  ULONG NextEntryOffset;
  ULONG FileIndex;
  LARGE_INTEGER CreationTime;
  LARGE_INTEGER LastAccessTime;
  LARGE_INTEGER LastWriteTime;
  LARGE_INTEGER ChangeTime;
  LARGE_INTEGER EndOfFile;
  LARGE_INTEGER AllocationSize;
  ULONG FileAttributes;
  ULONG FileNameLength;
  ULONG EaSize;
  ULONG ReparsePointTag;
  FILE_ID_128 FileId;
  WCHAR FileName[1];
};

struct FILE_OBJECTID_INFORMATION
{
  LONGLONG FileReference;
  UCHAR ObjectId[16];
  union
  {
    struct
    {
      UCHAR BirthVolumeId[16];
      UCHAR BirthObjectId[16];
      UCHAR DomainId[16];
    } s;
    UCHAR ExtendedInfo[48];
  } u;
};

enum SECTION_INHERIT
{
  ViewShare = 1,
  ViewUnmap = 2
};

typedef struct RTL_ACTIVATION_CONTEXT_STACK_FRAME*
  PRTL_ACTIVATION_CONTEXT_STACK_FRAME;

struct RTL_ACTIVATION_CONTEXT_STACK_FRAME
{
  PRTL_ACTIVATION_CONTEXT_STACK_FRAME Previous;
  _ACTIVATION_CONTEXT* ActivationContext;
  ULONG Flags;
};

struct ACTIVATION_CONTEXT_STACK
{
  PRTL_ACTIVATION_CONTEXT_STACK_FRAME ActiveFrame;
  LIST_ENTRY FrameListCache;
  ULONG Flags;
  ULONG NextCookieSequenceNumber;
  ULONG StackId;
};

typedef ACTIVATION_CONTEXT_STACK* PACTIVATION_CONTEXT_STACK;

struct GDI_TEB_BATCH
{
  ULONG Offset;
  ULONG HDC;
  ULONG Buffer[310];
};

struct TEB_ACTIVE_FRAME_CONTEXT
{
  ULONG Flags;
  CHAR* FrameName;
};

typedef TEB_ACTIVE_FRAME_CONTEXT* PTEB_ACTIVE_FRAME_CONTEXT;

typedef struct TEB_ACTIVE_FRAME* PTEB_ACTIVE_FRAME;

struct TEB_ACTIVE_FRAME
{
  ULONG Flags;
  PTEB_ACTIVE_FRAME Previous;
  PTEB_ACTIVE_FRAME_CONTEXT Context;
};

struct TEB
{
  NT_TIB NtTib;
  PVOID EnvironmentPointer;
  CLIENT_ID ClientId;
  PVOID ActiveRpcHandle;
  PVOID ThreadLocalStoragePointer;
  PPEB ProcessEnvironmentBlock;
  ULONG LastErrorValue;
  ULONG CountOfOwnedCriticalSections;
  PVOID CsrClientThread;
  PVOID Win32ThreadInfo;
  ULONG User32Reserved[26];
  ULONG UserReserved[5];
  PVOID WOW32Reserved;
  ULONG CurrentLocale;
  ULONG FpSoftwareStatusRegister;
  VOID* SystemReserved1[54];
  LONG ExceptionCode;
  PACTIVATION_CONTEXT_STACK ActivationContextStackPointer;
  UCHAR SpareBytes1[36];
  ULONG TxFsContext;
  GDI_TEB_BATCH GdiTebBatch;
  CLIENT_ID RealClientId;
  PVOID GdiCachedProcessHandle;
  ULONG GdiClientPID;
  ULONG GdiClientTID;
  PVOID GdiThreadLocalInfo;
  ULONG Win32ClientInfo[62];
  VOID* glDispatchTable[233];
  ULONG glReserved1[29];
  PVOID glReserved2;
  PVOID glSectionInfo;
  PVOID glSection;
  PVOID glTable;
  PVOID glCurrentRC;
  PVOID glContext;
  ULONG LastStatusValue;
  UNICODE_STRING StaticUnicodeString;
  WCHAR StaticUnicodeBuffer[261];
  PVOID DeallocationStack;
  VOID* TlsSlots[64];
  LIST_ENTRY TlsLinks;
  PVOID Vdm;
  PVOID ReservedForNtRpc;
  VOID* DbgSsReserved[2];
  ULONG HardErrorMode;
  VOID* Instrumentation[9];
  GUID ActivityId;
  PVOID SubProcessTag;
  PVOID EtwLocalData;
  PVOID EtwTraceData;
  PVOID WinSockData;
  ULONG GdiBatchCount;
  UCHAR SpareBool0;
  UCHAR SpareBool1;
  UCHAR SpareBool2;
  UCHAR IdealProcessor;
  ULONG GuaranteedStackBytes;
  PVOID ReservedForPerf;
  PVOID ReservedForOle;
  ULONG WaitingOnLoaderLock;
  PVOID SavedPriorityState;
  ULONG SoftPatchPtr1;
  PVOID ThreadPoolData;
  VOID** TlsExpansionSlots;
  ULONG ImpersonationLocale;
  ULONG IsImpersonating;
  PVOID NlsCache;
  PVOID pShimData;
  ULONG HeapVirtualAffinity;
  PVOID CurrentTransactionHandle;
  PTEB_ACTIVE_FRAME ActiveFrame;
  PVOID FlsData;
  PVOID PreferredLanguages;
  PVOID UserPrefLanguages;
  PVOID MergedPrefLanguages;
  ULONG MuiImpersonation;
  WORD CrossTebFlags;
  ULONG SpareCrossTebBits : 16;
  WORD SameTebFlags;
  ULONG DbgSafeThunkCall : 1;
  ULONG DbgInDebugPrint : 1;
  ULONG DbgHasFiberData : 1;
  ULONG DbgSkipThreadAttach : 1;
  ULONG DbgWerInShipAssertCode : 1;
  ULONG DbgRanProcessInit : 1;
  ULONG DbgClonedThread : 1;
  ULONG DbgSuppressDebugMsg : 1;
  ULONG SpareSameTebBits : 8;
  PVOID TxnScopeEnterCallback;
  PVOID TxnScopeExitCallback;
  PVOID TxnScopeContext;
  ULONG LockCount;
  ULONG ProcessRundown;
  UINT64 LastSwitchTime;
  UINT64 TotalSwitchOutTime;
  LARGE_INTEGER WaitReasonBitMap;
};

#if defined(HADESMEM_DETAIL_ARCH_X64)
inline TEB* GetCurrentTeb()
{
  return reinterpret_cast<TEB*>(__readgsqword(offsetof(NT_TIB, Self)));
}
#elif defined(HADESMEM_DETAIL_ARCH_X86)
inline TEB* GetCurrentTeb()
{
  return reinterpret_cast<TEB*>(__readfsdword(offsetof(NT_TIB, Self)));
}
#else
#error "[HadesMem] Unsupported architecture."
#endif

struct CURDIR
{
  UNICODE_STRING DosPath;
  VOID* Handle;
};

typedef CURDIR* PCURDIR;

struct RTL_DRIVE_LETTER_CURDIR
{
  UINT16 Flags;
  UINT16 Length;
  ULONG32 TimeStamp;
  STRING DosPath;
};

typedef RTL_DRIVE_LETTER_CURDIR* PRTL_DRIVE_LETTER_CURDIR;

struct RTL_USER_PROCESS_PARAMETERS
{
  ULONG32 MaximumLength;
  ULONG32 Length;
  ULONG32 Flags;
  ULONG32 DebugFlags;
  VOID* ConsoleHandle;
  ULONG32 ConsoleFlags;
  UINT8 _PADDING0_[0x4];
  VOID* StandardInput;
  VOID* StandardOutput;
  VOID* StandardError;
  CURDIR CurrentDirectory;
  UNICODE_STRING DllPath;
  UNICODE_STRING ImagePathName;
  UNICODE_STRING CommandLine;
  VOID* Environment;
  ULONG32 StartingX;
  ULONG32 StartingY;
  ULONG32 CountX;
  ULONG32 CountY;
  ULONG32 CountCharsX;
  ULONG32 CountCharsY;
  ULONG32 FillAttribute;
  ULONG32 WindowFlags;
  ULONG32 ShowWindowFlags;
  UINT8 _PADDING1_[0x4];
  UNICODE_STRING WindowTitle;
  UNICODE_STRING DesktopInfo;
  UNICODE_STRING ShellInfo;
  UNICODE_STRING RuntimeData;
  RTL_DRIVE_LETTER_CURDIR CurrentDirectores[32];
  UINT64 EnvironmentSize;
  UINT64 EnvironmentVersion;
};

typedef RTL_USER_PROCESS_PARAMETERS* PRTL_USER_PROCESS_PARAMETERS;
}
}
}
