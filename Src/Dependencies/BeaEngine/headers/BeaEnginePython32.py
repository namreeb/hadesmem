# =====================================
#
#	BeaEngine 4.x header for Python (32 bits)
#	using ctypes
#
# =====================================

from ctypes import *

class REX_Struct(Structure):
   _pack_= 1
   _fields_= [("W_",c_char),
              ("R_",c_char),
              ("X_",c_char),
              ("B_",c_char),
              ("state",c_char)]

class PREFIXINFO(Structure):
   _pack_= 1
   _fields_= [("Number",c_long),
              ("NbUndefined",c_long),
              ("LockPrefix",c_char),
              ("OperandSize",c_char),
              ("AddressSize",c_char),
              ("RepnePrefix",c_char),
              ("RepPrefix",c_char),
              ("FSPrefix",c_char),
              ("SSPrefix",c_char),
              ("GSPrefix",c_char),
              ("ESPrefix",c_char),
              ("CSPrefix",c_char),
              ("DSPrefix",c_char),
              ("BranchTaken",c_char),
              ("BranchNotTaken",c_char),
              ("REX",REX_Struct)]

class EFLStruct(Structure):
   _pack_= 1
   _fields_= [("OF_",c_char),
              ("SF_",c_char),
              ("ZF_",c_char),
              ("AF_",c_char),
              ("PF_",c_char),
              ("CF_",c_char),
              ("TF_",c_char),
              ("IF_",c_char),
              ("DF_",c_char),
              ("NT_",c_char),
              ("RF_",c_char),
              ("alignment",c_char)]

class MEMORYTYPE(Structure):
   _pack_= 4
   _fields_= [("BaseRegister", c_long),
              ("IndexRegister",c_long),
              ("Scale",c_long),
              ("Displacement",c_longlong)]


class INSTRTYPE(Structure):
   _pack_= 1
   _fields_= [("Category", c_long),
              ("Opcode", c_long),
              ("Mnemonic", c_char * 16),
              ("BranchType", c_long),
              ("Flags", EFLStruct),
              ("AddrValue", c_longlong),
              ("Immediat", c_longlong),
              ("ImplicitModifiedRegs", c_long)]

class ARGTYPE(Structure):
   _pack_= 4
   _fields_= [("ArgMnemonic", c_char * 32),
              ("ArgType", c_long),
              ("ArgSize", c_long),
			  ("ArgPosition", c_long),
              ("AccessMode", c_long),
              ("Memory", MEMORYTYPE),
              ("SegmentReg", c_long)]

class DISASM(Structure):
    _pack_= 1
    _fields_= [("EIP", c_long),
               ("VirtualAddr", c_longlong),
               ("SecurityBlock", c_long),
               ("CompleteInstr", c_char * 64),
               ("Archi", c_long),
               ("Options", c_longlong),
               ("Instruction", INSTRTYPE),
               ("Argument1", ARGTYPE),
               ("Argument2", ARGTYPE),
               ("Argument3", ARGTYPE),
               ("Prefix", PREFIXINFO), 
               ("Reserved_", c_long * 40)]

# ======================= PREFIXES

NotUsedPrefix = 0
InUsePrefix = 1
SuperfluousPrefix = 2
InvalidPrefix = 4
MandatoryPrefix = 8

UNKNOWN_OPCODE = -1
OUT_OF_BLOCK = 0

# ======================= OPTIONS
NoTabulation = 0
Tabulation = 1
MasmSyntax = 0
GoAsmSyntax = 0x100
NasmSyntax = 0x200
PrefixedNumeral = 0x10000
SuffixedNumeral = 0x20000
ShowSegmentRegs = 0x01000000

LowPosition = 0
HighPosition = 1

# ======================= EFLAGS states

TE_ = 1
MO_ = 2
RE_ = 4
SE_ = 8
UN_ = 0x10
PR_ = 0x20


GENERAL_PURPOSE_INSTRUCTION   =    0x10000
FPU_INSTRUCTION               =    0x20000
MMX_INSTRUCTION               =    0x40000
SSE_INSTRUCTION               =    0x80000
SSE2_INSTRUCTION              =   0x100000
SSE3_INSTRUCTION              =   0x200000
SSSE3_INSTRUCTION             =   0x400000
SSE41_INSTRUCTION             =   0x800000
SSE42_INSTRUCTION             =  0x1000000
SYSTEM_INSTRUCTION            =  0x2000000
VM_INSTRUCTION                =  0x4000000
UNDOCUMENTED_INSTRUCTION      =  0x8000000
AMD_INSTRUCTION               = 0x10000000
ILLEGAL_INSTRUCTION           = 0x20000000
AES_INSTRUCTION               = 0x40000000
CLMUL_INSTRUCTION             = 0x80000000

DATA_TRANSFER = 0x1
ARITHMETIC_INSTRUCTION = 2
LOGICAL_INSTRUCTION = 3
SHIFT_ROTATE = 4
BIT_BYTE = 5
CONTROL_TRANSFER = 6
STRING_INSTRUCTION = 7
InOutINSTRUCTION = 8
ENTER_LEAVE_INSTRUCTION = 9
FLAG_CONTROL_INSTRUCTION = 10
SEGMENT_REGISTER = 11
MISCELLANEOUS_INSTRUCTION = 12
COMPARISON_INSTRUCTION = 13
LOGARITHMIC_INSTRUCTION = 14
TRIGONOMETRIC_INSTRUCTION = 15
UNSUPPORTED_INSTRUCTION = 16
LOAD_CONSTANTS = 17
FPUCONTROL = 18
STATE_MANAGEMENT = 19
CONVERSION_INSTRUCTION = 20
SHUFFLE_UNPACK = 21
PACKED_SINGLE_PRECISION = 22
SIMD128bits = 23
SIMD64bits = 24
CACHEABILITY_CONTROL = 25
FP_INTEGER_CONVERSION = 26
SPECIALIZED_128bits = 27
SIMD_FP_PACKED = 28
SIMD_FP_HORIZONTAL = 29
AGENT_SYNCHRONISATION = 30
PACKED_ALIGN_RIGHT = 31 
PACKED_SIGN = 32
PACKED_BLENDING_INSTRUCTION = 33
PACKED_TEST = 34
PACKED_MINMAX = 35
HORIZONTAL_SEARCH = 36
PACKED_EQUALITY = 37
STREAMING_LOAD = 38
INSERTION_EXTRACTION = 39
DOT_PRODUCT = 40
SAD_INSTRUCTION = 41
ACCELERATOR_INSTRUCTION = 42  
ROUND_INSTRUCTION = 43

JO = 1
JC = 2
JE = 3
JA = 4
JS = 5
JP = 6
JL = 7
JG = 8
JB = 9
JECXZ = 10
JmpType = 11
CallType = 12
RetType = 13
JNO = -1
JNC = -2
JNE = -3
JNA = -4
JNS = -5
JNP = -6
JNL = -7
JNG = -8
JNB = -9

NO_ARGUMENT = 0x10000000
REGISTER_TYPE = 0x20000000
MEMORY_TYPE = 0x40000000
CONSTANT_TYPE = 0x80000000

MMX_REG = 0x10000
GENERAL_REG = 0x20000
FPU_REG = 0x40000
SSE_REG = 0x80000
CR_REG = 0x100000
DR_REG = 0x200000
SPECIAL_REG = 0x400000
MEMORY_MANAGEMENT_REG = 0x800000
SEGMENT_REG = 0x1000000

RELATIVE_ = 0x4000000
ABSOLUTE_ = 0x8000000

READ = 0x1
WRITE = 0x2

REG0 = 0x1
REG1 = 0x2
REG2 = 0x4
REG3 = 0x8
REG4 = 0x10
REG5 = 0x20
REG6 = 0x40
REG7 = 0x80
REG8 = 0x100
REG9 = 0x200
REG10 = 0x400
REG11 = 0x800
REG12 = 0x1000
REG13 = 0x2000
REG14 = 0x4000
REG15 = 0x8000


UNKNOWN_OPCODE = -1
OUT_OF_BLOCK = 0

NoTabulation      = 0x00000000
Tabulation        = 0x00000001

MasmSyntax        = 0x00000000
GoAsmSyntax       = 0x00000100
NasmSyntax        = 0x00000200
ATSyntax          = 0x00000400

PrefixedNumeral   = 0x00010000
SuffixedNumeral   = 0x00000000

ShowSegmentRegs   = 0x01000000


# ====================================== Import Disasm function
Disasm = windll.BeaEngine.Disasm
BeaEngineVersion = windll.BeaEngine.BeaEngineVersion
BeaEngineRevision = windll.BeaEngine.BeaEngineRevision