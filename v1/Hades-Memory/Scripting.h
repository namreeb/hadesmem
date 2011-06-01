/*
This file is part of HadesMem.
Copyright © 2010 Cypherjb (aka Chazwazza, aka Cypher). 
<http://www.cypherjb.com/> <cypher.jb@gmail.com>

HadesMem is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

HadesMem is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with HadesMem.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

// Windows API
#include <Windows.h>

// C++ Standard Library
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <iostream>

// Boost
#pragma warning(push, 1)
#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/make_shared.hpp>
#pragma warning(pop)

// Lua
extern "C"
{
#include "lua.h"
}

// LuaBind
#pragma warning(push, 1)
#include "LuaBind/luabind.hpp"
#include "luabind/iterator_policy.hpp"
#include "luabind/exception_handler.hpp"
#pragma warning(pop)

// Hades
#include "Types.h"
#include "Memory.h"
#include "Module.h"
#include "Region.h"
#include "Scanner.h"
#include "Injector.h"
#include "ManualMap.h"
#include "Disassembler.h"
#include "Hades-Common/I18n.h"

// LuaBind extensions for 64-bit number support
namespace luabind
{
  template <>
  struct default_converter<unsigned long long>
    : native_converter_base<unsigned long long>
  {
    int compute_score(lua_State* L, int index)
    {
      return lua_type(L, index) == LUA_TNUMBER ? 0 : -1;
    };

    unsigned long long from(lua_State* L, int index)
    {
      return static_cast<unsigned long long>(BOOST_PP_CAT(lua_to, number)
        (L, index));
    }

    void to(lua_State* L, unsigned long long const& value)
    {
      BOOST_PP_CAT(lua_push, number)(L, BOOST_PP_CAT(as_lua_, number)(value));
    }
  };

  template <>
  struct default_converter<unsigned long long const>
    : default_converter<unsigned long long>
  {};

  template <>
  struct default_converter<unsigned long long const&>
    : default_converter<unsigned long long>
  {};

  template <>
  struct default_converter<signed long long>
    : native_converter_base<signed long long>
  {
    int compute_score(lua_State* L, int index)
    {
      return lua_type(L, index) == LUA_TNUMBER ? 0 : -1;
    };

    signed long long from(lua_State* L, int index)
    {
      return static_cast<signed long long>(BOOST_PP_CAT(lua_to, number)
        (L, index));
    }

    void to(lua_State* L, signed long long const& value)
    {
      BOOST_PP_CAT(lua_push, number)(L, BOOST_PP_CAT(as_lua_, number)(value));
    }
  };

  template <>
  struct default_converter<signed long long const>
    : default_converter<signed long long>
  {};

  template <>
  struct default_converter<signed long long const&>
    : default_converter<signed long long>
  {};
}

// Wrapper function generator for MemoryMgr::Read
#define HADESMEM_SCRIPTING_GEN_READ(x) \
inline Types::x Memory_Read##x(MemoryMgr const& MyMemory, DWORD_PTR Address)\
{\
  return MyMemory.Read<Types::x>(reinterpret_cast<PVOID>(Address));\
}

// Wrapper function generator for MemoryMgr::Write
#define HADESMEM_SCRIPTING_GEN_WRITE(x) \
inline void Memory_Write##x(MemoryMgr const& MyMemory, DWORD_PTR Address, \
  Types::x Data)\
{\
  MyMemory.Write<Types::x>(reinterpret_cast<PVOID>(Address), Data);\
}

// Wrapper function generator for MyScanner::Find
#define HADESMEM_SCRIPTING_GEN_FIND(x) \
inline DWORD_PTR Scanner_Find##x(Scanner const& MyScanner, Types::x Data)\
{\
  return reinterpret_cast<DWORD_PTR>(MyScanner.Find(Data));\
}

// Wrapper function generator for MyScanner::Find
#define HADESMEM_SCRIPTING_GEN_FIND_ALL(x) \
inline DwordPtrList Scanner_FindAll##x(Scanner const& MyScanner, Types::x Data)\
{\
  auto AddrList = MyScanner.FindAll(Data);\
  DwordPtrList NewList;\
  NewList.List.reserve(AddrList.size());\
  std::transform(AddrList.begin(), AddrList.end(), std::back_inserter(\
  NewList.List), \
  [] (PVOID Current)\
{\
  return reinterpret_cast<DWORD_PTR>(Current);\
});\
  return NewList;\
}

namespace Hades
{
  namespace Memory
  {
    namespace Wrappers
    {
      // Console output wrapper
      inline void WriteLn(std::string const& Data)
      {
        std::cout << Data << std::endl;
      }

      // Convert number to hex string
      inline std::string ToHexStr(DWORD_PTR Num)
      {
        std::stringstream MyStream;
        MyStream << "0x" << std::hex << std::setfill('0') << 
          std::setw(sizeof(PVOID) * 2) << std::uppercase << Num;
        return MyStream.str();
      }

      // Whether this is a x64 build of HadesMem
      inline bool IsAMD64()
      {
#if defined(_M_AMD64) 
        return true;
#elif defined(_M_IX86) 
        return false;
#else 
#error "Unsupported architecture."
#endif
      }

      // MemoryMgr::MemoryMgr wrappers
      inline boost::shared_ptr<MemoryMgr> Memory_CreateMemoryMgr(DWORD ProcId)
      {
        return boost::make_shared<MemoryMgr>(ProcId);
      }
      inline boost::shared_ptr<MemoryMgr> Memory_CreateMemoryMgr(
        std::string const& ProcName)
      {
        return boost::make_shared<MemoryMgr>(boost::lexical_cast<std::wstring>(
          ProcName));
      }
      inline boost::shared_ptr<MemoryMgr> Memory_CreateMemoryMgr(
        std::string const& WindowName, std::string const& ClassName)
      {
        std::wstring ClassNameW(!ClassName.empty() ? 
          boost::lexical_cast<std::wstring>(ClassName) : L"");
        return boost::make_shared<MemoryMgr>(boost::lexical_cast<std::wstring>(
          WindowName), !ClassName.empty() ? &ClassNameW : nullptr);
      }

      // MemoryMgr::CanRead wrapper
      inline bool Memory_CanRead(MemoryMgr const& MyMemory, DWORD_PTR Address)
      {
        return MyMemory.CanRead(reinterpret_cast<PVOID>(Address));
      }

      // MemoryMgr::CanWrite wrapper
      inline bool Memory_CanWrite(MemoryMgr const& MyMemory, DWORD_PTR Address)
      {
        return MyMemory.CanWrite(reinterpret_cast<PVOID>(Address));
      }

      // MemoryMgr::Call wrapper
      inline DWORD Memory_Call(MemoryMgr const& MyMemory, DWORD_PTR Address, 
        std::vector<DWORD_PTR> const& Args, MemoryMgr::CallConv CallConv)
      {
        std::vector<PVOID> ArgsNew;
        std::transform(Args.begin(), Args.end(), std::back_inserter(ArgsNew), 
          [] (DWORD_PTR Arg)
        {
          return reinterpret_cast<PVOID>(Arg);
        });

        return MyMemory.Call(reinterpret_cast<PVOID>(Address), ArgsNew, 
          CallConv);
      }

      // MemoryMgr::Alloc wrapper
      inline DWORD_PTR Memory_Alloc(MemoryMgr const& MyMem, SIZE_T Size)
      {
        return reinterpret_cast<DWORD_PTR>(MyMem.Alloc(Size));
      }

      // MemoryMgr::Free wrapper
      inline void Memory_Free(MemoryMgr const& MyMem, DWORD_PTR Address)
      {
        return MyMem.Free(reinterpret_cast<PVOID>(Address));
      }

      // MemoryMgr::GetProcessID wrapper
      inline DWORD Memory_GetProcessID(MemoryMgr const& MyMem)
      {
        return MyMem.GetProcessID();
      }

      // MemoryMgr::GetProcessHandle wrapper
      inline DWORD_PTR Memory_GetProcessHandle(MemoryMgr const& MyMem)
      {
        return reinterpret_cast<DWORD_PTR>(MyMem.GetProcessHandle());
      }

      // MemoryMgr::GetRemoteProcAddress wrapper
      inline DWORD_PTR Memory_GetRemoteProcAddress(MemoryMgr const& MyMem, 
        DWORD_PTR RemoteMod, std::string const& Module, 
        std::string const& Function)
      {
        return reinterpret_cast<DWORD_PTR>(MyMem.GetRemoteProcAddress(
          reinterpret_cast<HMODULE>(RemoteMod), 
          boost::lexical_cast<std::wstring>(Module), 
          Function.c_str()));
      }

      // MemoryMgr::GetRemoteProcAddress wrapper
      inline DWORD_PTR Memory_GetRemoteProcAddress(MemoryMgr const& MyMem, 
        DWORD_PTR RemoteMod, std::string const& Module, 
        WORD Ordinal)
      {
        return reinterpret_cast<DWORD_PTR>(MyMem.GetRemoteProcAddress(
          reinterpret_cast<HMODULE>(RemoteMod), 
          boost::lexical_cast<std::wstring>(Module), 
          reinterpret_cast<LPCSTR>(MAKELONG(Ordinal, 0))));
      }

      // MemoryMgr::FlushCache wrapper
      inline void Memory_FlushCache(MemoryMgr const& MyMem, DWORD_PTR Address, 
        SIZE_T Size)
      {
        return MyMem.FlushCache(reinterpret_cast<LPCVOID>(Address), Size);
      }

      // MemoryMgr::Read<T> wrappers
      HADESMEM_SCRIPTING_GEN_READ(Int8)
      HADESMEM_SCRIPTING_GEN_READ(UInt8)
      HADESMEM_SCRIPTING_GEN_READ(Int16)
      HADESMEM_SCRIPTING_GEN_READ(UInt16)
      HADESMEM_SCRIPTING_GEN_READ(Int32)
      HADESMEM_SCRIPTING_GEN_READ(UInt32)
      HADESMEM_SCRIPTING_GEN_READ(Int64)
      HADESMEM_SCRIPTING_GEN_READ(UInt64)
      HADESMEM_SCRIPTING_GEN_READ(Float)
      HADESMEM_SCRIPTING_GEN_READ(Double)
      HADESMEM_SCRIPTING_GEN_READ(StrNarrow)

        // Wrapper function generator for MemoryMgr::ReadCharNarrow
      inline std::string Memory_ReadCharNarrow(MemoryMgr const& MyMemory, 
        DWORD_PTR Address)
      {
        std::string MyStr;
        MyStr += MyMemory.Read<Types::CharNarrow>(reinterpret_cast<PVOID>(
          Address));
        return MyStr;
      }

      // Wrapper function generator for MemoryMgr::ReadCharWide
      inline std::string Memory_ReadCharWide(MemoryMgr const& MyMemory, 
        DWORD_PTR Address)
      {
        std::wstring MyStr;
        MyStr += MyMemory.Read<Types::CharWide>(reinterpret_cast<PVOID>(
          Address));
        return boost::lexical_cast<std::string>(MyStr);
      }

      // Wrapper function generator for MemoryMgr::ReadStrWide
      inline std::string Memory_ReadStrWide(MemoryMgr const& MyMemory, 
        DWORD_PTR Address)
      {
        return boost::lexical_cast<std::string>(MyMemory.Read<Types::StrWide>(
          reinterpret_cast<PVOID>(Address)));
      }

      // Wrapper function generator for MemoryMgr::ReadPointer
      inline DWORD_PTR Memory_ReadPointer(MemoryMgr const& MyMemory, 
        DWORD_PTR Address)
      {
        return reinterpret_cast<DWORD_PTR>(MyMemory.Read<Types::Pointer>(
          reinterpret_cast<PVOID>(Address)));
      }

      // MemoryMgr::Write<T> wrappers
      HADESMEM_SCRIPTING_GEN_WRITE(Int8)
      HADESMEM_SCRIPTING_GEN_WRITE(UInt8)
      HADESMEM_SCRIPTING_GEN_WRITE(Int16)
      HADESMEM_SCRIPTING_GEN_WRITE(UInt16)
      HADESMEM_SCRIPTING_GEN_WRITE(Int32)
      HADESMEM_SCRIPTING_GEN_WRITE(UInt32)
      HADESMEM_SCRIPTING_GEN_WRITE(Int64)
      HADESMEM_SCRIPTING_GEN_WRITE(UInt64)
      HADESMEM_SCRIPTING_GEN_WRITE(Float)
      HADESMEM_SCRIPTING_GEN_WRITE(Double)
      HADESMEM_SCRIPTING_GEN_WRITE(StrNarrow)

        // Wrapper function generator for MemoryMgr::WriteCharNarrow
      inline void Memory_WriteCharNarrow(MemoryMgr const& MyMemory, 
        DWORD_PTR Address, std::string const& Value)
      {
        if (Value.size() != 1)
        {
          BOOST_THROW_EXCEPTION(HadesMemError() << 
            ErrorFunction("Memory_WriteCharNarrow") << 
            ErrorString("Value invalid (must be a single character)."));
        }
        MyMemory.Write(reinterpret_cast<PVOID>(Address), Value[0]);
      }

      // Wrapper function generator for MemoryMgr::WriteCharWide
      inline void Memory_WriteCharWide(MemoryMgr const& MyMemory, 
        DWORD_PTR Address, std::string const& Value)
      {
        if (Value.size() != 1)
        {
          BOOST_THROW_EXCEPTION(HadesMemError() << 
            ErrorFunction("Memory_WriteCharWide") << 
            ErrorString("Value invalid (must be a single character)."));
        }
        MyMemory.Write(reinterpret_cast<PVOID>(Address), 
          boost::lexical_cast<std::wstring>(Value)[0]);
      }

      // Wrapper function generator for MemoryMgr::ReadStrWide
      inline void Memory_WriteStrWide(MemoryMgr const& MyMemory, 
        DWORD_PTR Address, std::string const& Value)
      {
        MyMemory.Write(reinterpret_cast<PVOID>(Address), 
          boost::lexical_cast<std::wstring>(Value));
      }

      // Wrapper function generator for MemoryMgr::ReadPointer
      inline void Memory_WritePointer(MemoryMgr const& MyMemory, 
        DWORD_PTR Address, DWORD_PTR Value)
      {
        MyMemory.Write(reinterpret_cast<PVOID>(Address), 
          reinterpret_cast<Types::Pointer>(Value));
      }

      // Module::Module wrappers
      inline Module Module_CreateModule(
        MemoryMgr const& MyMemory, DWORD_PTR Handle)
      {
        return Module(MyMemory, reinterpret_cast<HMODULE>(Handle));
      }
      inline Module Module_CreateModule(MemoryMgr const& MyMemory, 
        std::string const& ModuleName)
      {
        return Module(MyMemory, boost::lexical_cast<std::wstring>(
          ModuleName));
      }

      // Module::GetBase wrapper
      inline DWORD_PTR Module_GetBase(Module const& MyModule)
      {
        return reinterpret_cast<DWORD_PTR>(MyModule.GetBase());
      }

      // Module::GetSize wrapper
      inline DWORD Module_GetSize(Module const& MyModule)
      {
        return MyModule.GetSize();
      }

      // Module::GetName wrapper
      inline std::string Module_GetName(Module const& MyModule)
      {
        return boost::lexical_cast<std::string>(MyModule.GetName());
      }

      // Module::GetPath wrapper
      inline std::string Module_GetPath(Module const& MyModule)
      {
        return boost::lexical_cast<std::string>(MyModule.GetPath());
      }

      // Module::Found wrapper
      inline bool Module_Found(Module const& MyModule)
      {
        return MyModule.Found();
      }

      // Module list wrapper
      struct ModuleList
      {
        std::vector<boost::shared_ptr<Module>> List;
      };

      // GetModuleList wrapper
      inline ModuleList Module_GetModuleList(MemoryMgr const& MyMemory)
      {
        ModuleList MyModuleList;
        MyModuleList.List = GetModuleList(MyMemory);
        return MyModuleList;
      }

      // Region::Region wrappers
      inline Region Region_CreateRegion(MemoryMgr const& MyMemory, 
        DWORD_PTR Address)
      {
        return Region(MyMemory, reinterpret_cast<PVOID>(Address));
      }

      // Region::GetBase wrapper
      inline DWORD_PTR Region_GetBaseAddress(Region const& MyRegion)
      {
        return reinterpret_cast<DWORD_PTR>(MyRegion.GetBase());
      }

      // Region::GetAllocBase wrapper
      inline DWORD_PTR Region_GetAllocationBase(Region const& MyRegion)
      {
        return reinterpret_cast<DWORD_PTR>(MyRegion.GetAllocBase());
      }

      // Region::GetAllocProtect wrapper
      inline DWORD Region_GetAllocationProtect(Region const& MyRegion)
      {
        return MyRegion.GetAllocProtect();
      }

      // Region::GetSize wrapper
      inline SIZE_T Region_GetRegionSize(Region const& MyRegion)
      {
        return MyRegion.GetSize();
      }

      // Region::GetState wrapper
      inline DWORD Region_GetState(Region const& MyRegion)
      {
        return MyRegion.GetState();
      }

      // Region::GetProtect wrapper
      inline DWORD Region_GetProtect(Region const& MyRegion)
      {
        return MyRegion.GetProtect();
      }

      // Region::GetType wrapper
      inline DWORD Region_GetType(Region const& MyRegion)
      {
        return MyRegion.GetType();
      }

      // Module list wrapper
      struct RegionList
      {
        std::vector<boost::shared_ptr<Region>> List;
      };

      // GetRegionList wrapper
      inline RegionList Region_GetRegionList(MemoryMgr const& MyMemory)
      {
        RegionList MyRegionList;
        MyRegionList.List = GetMemoryRegionList(MyMemory);
        return MyRegionList;
      }

      // Injector::Injector wrappers
      inline Injector Injector_CreateInjector(MemoryMgr const& MyMemory)
      {
        return Injector(MyMemory);
      }

      // Injector::InjectDll wrapper
      inline DWORD_PTR Injector_InjectDll(Injector const& MyInjector, 
        std::string const& Path, bool PathResolution)
      {
        return reinterpret_cast<DWORD_PTR>(MyInjector.InjectDll(
          boost::lexical_cast<std::wstring>(Path), PathResolution));
      }

      // Injector::CallExport wrapper
      inline DWORD Injector_CallExport(Injector const& MyInjector, 
        std::string const& ModulePath, DWORD_PTR ModuleRemote, 
        std::string const& Export)
      {
        return MyInjector.CallExport(boost::lexical_cast<std::wstring>(
          ModulePath), reinterpret_cast<HMODULE>(ModuleRemote), 
          Export);
      }

      struct CreateAndInjectInfo
      {
        boost::shared_ptr<MemoryMgr> Memory;
        DWORD_PTR ModBase;
        DWORD ExportRet;
      };

      // CreateAndInject wrapper
      inline CreateAndInjectInfo Injector_CreateAndInject(
        std::string const& Path, std::string const& Args, 
        std::string const& Module, std::string const& Export)
      {
        HMODULE ModBase = nullptr;
        DWORD ExportRet = 0;
        auto MyMemory = CreateAndInject(
          boost::lexical_cast<std::wstring>(Path), 
          boost::lexical_cast<std::wstring>(Args), 
          boost::lexical_cast<std::wstring>(Module), 
          Export, 
          &ModBase, 
          &ExportRet);

        CreateAndInjectInfo MyInfo;
        MyInfo.Memory = MyMemory;
        MyInfo.ModBase = reinterpret_cast<DWORD_PTR>(ModBase);
        MyInfo.ExportRet = ExportRet;

        return MyInfo;
      }

      // Disassembler::Disassembler wrappers
      inline Disassembler Disassembler_CreateDisassembler(
        MemoryMgr const& MyMemory)
      {
        return Disassembler(MyMemory);
      }

      // String list wrapper
      struct StringList
      {
        std::vector<std::string> List;
      };

      // Disassembler::DisassembleToStr wrapper
      inline StringList Disassembler_DisassembleToStr(
        Disassembler const& MyDisassembler, DWORD_PTR Address, 
        DWORD_PTR NumInstructions)
      {
        StringList MyStringList;
        MyStringList.List =  MyDisassembler.DisassembleToStr(
          reinterpret_cast<PVOID>(Address), NumInstructions);
        return MyStringList;
      }

      // Scanner::Scanner wrappers
      inline Scanner Scanner_CreateScanner(MemoryMgr const& MyMemory)
      {
        return Scanner(MyMemory);
      }
      inline Scanner Scanner_CreateScanner(MemoryMgr const& MyMemory, 
        DWORD_PTR Module)
      {
        return Scanner(MyMemory, reinterpret_cast<HMODULE>(Module));
      }
      inline Scanner Scanner_CreateScanner(MemoryMgr const& MyMemory, 
        DWORD_PTR Start, DWORD_PTR End)
      {
        return Scanner(MyMemory, reinterpret_cast<PVOID>(Start), 
          reinterpret_cast<PVOID>(End));
      }

      // Scanner::LoadFromXML wrapper
      inline void Scanner_LoadFromXML(Scanner& MyScanner, 
        std::string const& Path)
      {
        MyScanner.LoadFromXML(boost::lexical_cast<std::wstring>(Path));
      }

      // Scanner::operator[] wrapper
      inline DWORD_PTR Scanner_GetAddress(Scanner const& MyScanner, 
        std::string const& Name)
      {
        return reinterpret_cast<DWORD_PTR>(MyScanner[
          boost::lexical_cast<std::wstring>(Name)]);
      }

      // Scanner::Find<T> wrappers
      HADESMEM_SCRIPTING_GEN_FIND(Int8)
      HADESMEM_SCRIPTING_GEN_FIND(UInt8)
      HADESMEM_SCRIPTING_GEN_FIND(Int16)
      HADESMEM_SCRIPTING_GEN_FIND(UInt16)
      HADESMEM_SCRIPTING_GEN_FIND(Int32)
      HADESMEM_SCRIPTING_GEN_FIND(UInt32)
      HADESMEM_SCRIPTING_GEN_FIND(Int64)
      HADESMEM_SCRIPTING_GEN_FIND(UInt64)
      HADESMEM_SCRIPTING_GEN_FIND(Float)
      HADESMEM_SCRIPTING_GEN_FIND(Double)
      HADESMEM_SCRIPTING_GEN_FIND(StrNarrow)

        // Wrapper function for Scanner::FindCharNarrow
      inline DWORD_PTR Scanner_FindCharNarrow(Scanner const& MyScanner, 
        std::string const& Data)
      {
        if (Data.size() != 1)
        {
          BOOST_THROW_EXCEPTION(HadesMemError() << 
            ErrorFunction("Scanner_FindCharNarrow") << 
            ErrorString("Value invalid (must be a single character)."));
        }
        return reinterpret_cast<DWORD_PTR>(MyScanner.Find(Data[0]));
      }

      // Wrapper function for Scanner::FindCharWide
      inline DWORD_PTR Scanner_FindCharWide(Scanner const& MyScanner, 
        std::string const& Data)
      {
        if (Data.size() != 1)
        {
          BOOST_THROW_EXCEPTION(HadesMemError() << 
            ErrorFunction("Scanner_FindCharNarrow") << 
            ErrorString("Value invalid (must be a single character)."));
        }
        return reinterpret_cast<DWORD_PTR>(MyScanner.Find(
          boost::lexical_cast<Types::StrWide>(Data[0])));
      }

      // Wrapper function for Scanner::FindStrWide
      inline DWORD_PTR Scanner_FindStrWide(Scanner const& MyScanner, 
        std::string const& Data)
      {
        return reinterpret_cast<DWORD_PTR>(MyScanner.Find(
          boost::lexical_cast<Types::StrWide>(Data)));
      }

      // Wrapper function for MemoryMgr::ReadPointer
      inline DWORD_PTR Scanner_FindPointer(Scanner const& MyScanner, 
        DWORD_PTR Data)
      {
        return reinterpret_cast<DWORD_PTR>(MyScanner.Find(
          reinterpret_cast<Types::Pointer>(Data)));
      }

      // Dword pointer list wrapper
      struct DwordPtrList
      {
        std::vector<DWORD_PTR> List;
      };

      // Scanner::FindAll<T> wrappers
      HADESMEM_SCRIPTING_GEN_FIND_ALL(Int8)
      HADESMEM_SCRIPTING_GEN_FIND_ALL(UInt8)
      HADESMEM_SCRIPTING_GEN_FIND_ALL(Int16)
      HADESMEM_SCRIPTING_GEN_FIND_ALL(UInt16)
      HADESMEM_SCRIPTING_GEN_FIND_ALL(Int32)
      HADESMEM_SCRIPTING_GEN_FIND_ALL(UInt32)
      HADESMEM_SCRIPTING_GEN_FIND_ALL(Int64)
      HADESMEM_SCRIPTING_GEN_FIND_ALL(UInt64)
      HADESMEM_SCRIPTING_GEN_FIND_ALL(Float)
      HADESMEM_SCRIPTING_GEN_FIND_ALL(Double)
      HADESMEM_SCRIPTING_GEN_FIND_ALL(StrNarrow)


        // Wrapper function for Scanner::FindAllCharNarrow
      inline DwordPtrList Scanner_FindAllCharNarrow(Scanner const& MyScanner, 
        std::string const& Data)
      {
        if (Data.size() != 1)
        {
          BOOST_THROW_EXCEPTION(HadesMemError() << 
            ErrorFunction("Scanner_FindAllCharNarrow") << 
            ErrorString("Value invalid (must be a single character)."));
        }
        auto AddrList = MyScanner.FindAll(Data[0]);
        DwordPtrList NewList;
        NewList.List.reserve(AddrList.size());
        std::transform(AddrList.begin(), AddrList.end(), std::back_inserter(
          NewList.List), 
          [] (PVOID Current)
        {
          return reinterpret_cast<DWORD_PTR>(Current);
        });
        return NewList;
      }

      // Wrapper function for Scanner::FindAllCharWide
      inline DwordPtrList Scanner_FindAllCharWide(Scanner const& MyScanner, 
        std::string const& Data)
      {
        if (Data.size() != 1)
        {
          BOOST_THROW_EXCEPTION(HadesMemError() << 
            ErrorFunction("Scanner_FindAllCharWide") << 
            ErrorString("Value invalid (must be a single character)."));
        }
        auto AddrList = MyScanner.FindAll(boost::lexical_cast<Types::StrWide>(
          Data[0]));
        DwordPtrList NewList;
        NewList.List.reserve(AddrList.size());
        std::transform(AddrList.begin(), AddrList.end(), std::back_inserter(
          NewList.List), 
          [] (PVOID Current)
        {
          return reinterpret_cast<DWORD_PTR>(Current);
        });
        return NewList;
      }

      // Wrapper function for Scanner::FindAllStrWide
      inline DwordPtrList Scanner_FindAllStrWide(Scanner const& MyScanner, 
        std::string const& Data)
      {
        auto AddrList = MyScanner.FindAll(boost::lexical_cast<Types::StrWide>(
          Data));
        DwordPtrList NewList;
        NewList.List.reserve(AddrList.size());
        std::transform(AddrList.begin(), AddrList.end(), std::back_inserter(
          NewList.List), 
          [] (PVOID Current)
        {
          return reinterpret_cast<DWORD_PTR>(Current);
        });
        return NewList;
      }

      // Wrapper function for Scanner::FindAllPointer
      inline DwordPtrList Scanner_FindAllPointer(Scanner const& MyScanner, 
        DWORD_PTR Data)
      {
        auto AddrList = MyScanner.FindAll(reinterpret_cast<Types::Pointer>(
          Data));
        DwordPtrList NewList;
        NewList.List.reserve(AddrList.size());
        std::transform(AddrList.begin(), AddrList.end(), std::back_inserter(
          NewList.List), 
          [] (PVOID Current)
        {
          return reinterpret_cast<DWORD_PTR>(Current);
        });
        return NewList;
      }

      // Wrapper function for ManualMap::ManualMap
      inline ManualMap ManualMap_CreateManualMap(MemoryMgr const& MyMemory)
      {
        return ManualMap(MyMemory);
      }

      // Wrapper function for ManualMap::Map
      inline DWORD_PTR ManualMap_Map(ManualMap const& MyManualMapper, 
        std::string const& Path, std::string const& Export, bool InjectHelper)
      {
        return reinterpret_cast<DWORD_PTR>(MyManualMapper.Map(
          boost::lexical_cast<std::wstring>(Path), Export, InjectHelper));
      }
    }

    // Lua exception type
    class LuaError : public virtual HadesMemError 
    { };

    // LuaState wrapper class for RAII
    class LuaState : private boost::noncopyable
    {
      // Only LuaMgr can create states
      friend class LuaMgr;

      // Underlying lua state
      lua_State* m_State;

    public:
      // Destructor
      ~LuaState()
      {
        // Close LUA
        lua_close(m_State);
      }

      // Implicitly act as a lua_State pointer
      operator lua_State*() const 
      {
        // Return underlying lua state
        return m_State;
      }

      // Implicitly act as a lua_State pointer
      operator lua_State*() 
      {
        // Return underlying lua state
        return m_State;
      }

    protected:
      // Constructor
      LuaState() 
        : m_State(lua_open()) // Open LUA
      { }
    };

    // Lua managing class
    class LuaMgr
    {
    public:
      // Constructor
      LuaMgr() 
        : m_State()
      {
        // Open LuaBind with Lua state
        luabind::open(m_State);
      }

      // Get LUA state
      const LuaState& GetState() 
      {
        return m_State;
      }

      // Run a LUA script on disk
      void RunFile(std::string const& Path)
      {
        // Load and execute file
        int Status = luaL_dofile(m_State, Path.c_str());
        // Clean up if an error occurred
        if (Status != 0) 
        {
          lua_gc(m_State, LUA_GCCOLLECT, 0);
        }
        // Report any errors
        ReportError(Status);
      }

      // Run a LUA script from a string
      void RunString(std::string const& Script)
      {
        // Load and execute string
        int Status = luaL_dostring(m_State, Script.c_str());
        // Clean up if an error occurred
        if (Status != 0) 
        {
          lua_gc(m_State, LUA_GCCOLLECT, 0);
        }
        // Report any errors
        ReportError(Status);
      }

      // Reports an error to the console
      void ReportError(int Status)
      {
        // Check if an error occurred
        if (Status && !lua_isnil(m_State, -1)) 
        {
          // Get error message as string
          const char* Message = lua_tostring(m_State, -1);
          // If a conversion to string is not possible set that as the message
          if (Message == NULL) 
          {
            Message = "Error object is not a string";
          }
          // Pop error message off stack
          lua_pop(m_State, 1);
          // Throw exception for error
          BOOST_THROW_EXCEPTION(LuaError() << 
            ErrorFunction("LuaMgr::ReportError") << 
            ErrorString(Message));
        }
      }

    private:
      // Lua state
      LuaState m_State;
    };

    namespace Wrappers
    {
      inline void VecPushDwordPtr(std::vector<DWORD_PTR>& Vec, 
        DWORD_PTR const& Val)
      {
        Vec.push_back(Val);
      }
    }

    class ScriptMgr : public LuaMgr
    {
    public:
      ScriptMgr() 
        : LuaMgr() 
      {
        // Register HadesMem exception handler
        luabind::register_exception_handler<HadesError>(std::bind(
          &ScriptMgr::TranslateHadesMemException, this, std::placeholders::_1, 
          std::placeholders::_2));

        luabind::module(GetState(), "std")
        [
          luabind::class_<std::vector<DWORD_PTR>>("vector_dwordptr")
          .def("push_back", &Wrappers::VecPushDwordPtr)
          .def(luabind::constructor<>())
          .def(luabind::constructor<std::size_t>())
        ];

        // Register HadesMem API
        luabind::module(GetState(), "HadesMem")
        [
          // Bind console output wrapper
          luabind::def("WriteLn", &Wrappers::WriteLn)

          // Number to hex string converter
          ,luabind::def("ToHexStr", &Wrappers::ToHexStr)

          // AMD64 build checker
          ,luabind::def("IsAMD64", &Wrappers::IsAMD64)

          // Bind MemoryMgr::MemoryMgr wrappers
          ,luabind::def("CreateMemoryMgr", static_cast<boost::shared_ptr<
            MemoryMgr> (*)(DWORD)>(&Wrappers::Memory_CreateMemoryMgr)) 
          ,luabind::def("CreateMemoryMgr", static_cast<boost::shared_ptr<
            MemoryMgr> (*)(std::string const&)>(&Wrappers::
            Memory_CreateMemoryMgr)) 
          ,luabind::def("CreateMemoryMgr", static_cast<boost::shared_ptr<
            MemoryMgr> (*)(std::string const& WindowName, 
            std::string const& ClassName)>(&Wrappers::Memory_CreateMemoryMgr)) 

          // Bind MemoryMgr class
          ,luabind::class_<MemoryMgr>("MemoryMgr")

          // Bind calling conventions
          .enum_("CallConv")
          [
            luabind::value("CallConv_CDECL", MemoryMgr::CallConv_CDECL),
            luabind::value("CallConv_STDCALL", MemoryMgr::CallConv_STDCALL),
            luabind::value("CallConv_THISCALL", MemoryMgr::CallConv_THISCALL),
            luabind::value("CallConv_FASTCALL", MemoryMgr::CallConv_FASTCALL),
            luabind::value("CallConv_X64", MemoryMgr::CallConv_X64),
            luabind::value("CallConv_Default", MemoryMgr::CallConv_Default)
          ]

          // Bind MemoryMgr::Call wrapper
          .def("Call", &Wrappers::Memory_Call) 

          // Bind MemoryMgr::Read<T> wrappers
          .def("ReadInt8", &Wrappers::Memory_ReadInt8) 
          .def("ReadUInt8", &Wrappers::Memory_ReadUInt8) 
          .def("ReadInt16", &Wrappers::Memory_ReadInt16) 
          .def("ReadUInt16", &Wrappers::Memory_ReadUInt16)
          .def("ReadInt32", &Wrappers::Memory_ReadInt32)
          .def("ReadUInt32", &Wrappers::Memory_ReadUInt32)
          .def("ReadInt64", &Wrappers::Memory_ReadInt64)
          .def("ReadUInt64", &Wrappers::Memory_ReadUInt64)
          .def("ReadFloat", &Wrappers::Memory_ReadFloat)
          .def("ReadDouble", &Wrappers::Memory_ReadDouble)
          .def("ReadCharNarrow", &Wrappers::Memory_ReadCharNarrow)
          .def("ReadCharWide", &Wrappers::Memory_ReadCharWide)
          .def("ReadStrNarrow", &Wrappers::Memory_ReadStrNarrow)
          .def("ReadStrWide", &Wrappers::Memory_ReadStrWide)
          .def("ReadPointer", &Wrappers::Memory_ReadPointer)

          // Bind MemoryMgr::Write<T> wrappers
          .def("WriteInt8", &Wrappers::Memory_WriteInt8) 
          .def("WriteUInt8", &Wrappers::Memory_WriteUInt8) 
          .def("WriteInt16", &Wrappers::Memory_WriteInt16) 
          .def("WriteUInt16", &Wrappers::Memory_WriteUInt16)
          .def("WriteInt32", &Wrappers::Memory_WriteInt32)
          .def("WriteUInt32", &Wrappers::Memory_WriteUInt32)
          .def("WriteInt64", &Wrappers::Memory_WriteInt64)
          .def("WriteUInt64", &Wrappers::Memory_WriteUInt64)
          .def("WriteFloat", &Wrappers::Memory_WriteFloat)
          .def("WriteDouble", &Wrappers::Memory_WriteDouble)
          .def("WriteCharNarrow", &Wrappers::Memory_WriteCharNarrow)
          .def("WriteCharWide", &Wrappers::Memory_WriteCharWide)
          .def("WriteStrNarrow", &Wrappers::Memory_WriteStrNarrow)
          .def("WriteStrWide", &Wrappers::Memory_WriteStrWide)
          .def("WritePointer", &Wrappers::Memory_WritePointer)

          // Bind MemoryMgr::CanRead wrapper
          .def("CanRead", &Wrappers::Memory_CanRead)

          // Bind MemoryMgr::CanWrite wrapper
          .def("CanWrite", &Wrappers::Memory_CanWrite)

          // Bind MemoryMgr::Alloc wrapper
          .def("Alloc", &Wrappers::Memory_Alloc)

          // Bind MemoryMgr::Free wrapper
          .def("Free", &Wrappers::Memory_Free)

          // Bind MemoryMgr::GetProcessID wrapper
          .def("GetProcessID", &Wrappers::Memory_GetProcessID)

          // Bind MemoryMgr::GetProcessHandle wrapper
          .def("GetProcessHandle", &Wrappers::Memory_GetProcessHandle)

          // Bind MemoryMgr::GetRemoteProcAddress wrappers
          .def("GetRemoteProcAddress", static_cast<DWORD_PTR (*) (
            MemoryMgr const&, DWORD_PTR, std::string const&, 
            std::string const&)>(&Wrappers::Memory_GetRemoteProcAddress))
          .def("GetRemoteProcAddress", static_cast<DWORD_PTR (*) (
            MemoryMgr const&, DWORD_PTR, std::string const&, WORD)>(
            &Wrappers::Memory_GetRemoteProcAddress))

          // Bind MemoryMgr::FlushCache wrapper
          .def("FlushCache", &Wrappers::Memory_FlushCache)

          // Bind Module::Module wrappers
          ,luabind::def("CreateModule", static_cast<Module (*)(
            MemoryMgr const&, DWORD_PTR)>(&Wrappers::Module_CreateModule)) 
          ,luabind::def("CreateModule", static_cast<Module (*)(
            MemoryMgr const&, std::string const&)>(&Wrappers::
            Module_CreateModule)) 

          // Bind Module class
          ,luabind::class_<Module>("Module")

          // Bind Module::GetBase wrapper
          .def("GetBase", &Wrappers::Module_GetBase)

          // Bind Module::GetSize wrapper
          .def("GetSize", &Wrappers::Module_GetSize)

          // Bind Module::GetName wrapper
          .def("GetName", &Wrappers::Module_GetName)

          // Bind Module::GetPath wrapper
          .def("GetPath", &Wrappers::Module_GetPath)

          // Bind Module::Found wrapper
          .def("Found", &Wrappers::Module_Found)

          // Bind ModuleList class
          ,luabind::class_<Wrappers::ModuleList>("ModuleList")
          .def(luabind::constructor<>())
          .def_readonly("List", &Wrappers::ModuleList::List, 
            luabind::return_stl_iterator)

          // Bind GetModuleList function
          ,luabind::def("GetModuleList", &Wrappers::Module_GetModuleList)

          // Bind Region::Region wrappers
          ,luabind::def("CreateRegion", &Wrappers::Region_CreateRegion) 

          // Bind Region class
          ,luabind::class_<Region>("Region")

          // Bind Region::GetBase wrapper
          .def("GetBase", &Wrappers::Region_GetBaseAddress)

          // Bind Region::GetSize wrapper
          .def("GetAllocBase", &Wrappers::Region_GetAllocationBase)

          // Bind Region::GetName wrapper
          .def("GetAllocProtect", &Wrappers::Region_GetAllocationProtect)

          // Bind Region::GetPath wrapper
          .def("GetSize", &Wrappers::Region_GetRegionSize)

          // Bind Region::Found wrapper
          .def("GetState", &Wrappers::Region_GetState)

          // Bind Region::Found wrapper
          .def("GetProtect", &Wrappers::Region_GetProtect)

          // Bind Region::Found wrapper
          .def("GetType", &Wrappers::Region_GetType)

          // Bind RegionList class
          ,luabind::class_<Wrappers::RegionList>("RegionList")
          .def(luabind::constructor<>())
          .def_readonly("List", &Wrappers::RegionList::List, 
          luabind::return_stl_iterator)

          // Bind GetRegionList function
          ,luabind::def("GetRegionList", &Wrappers::Region_GetRegionList)

          // Bind Injector::Injector wrappers
          ,luabind::def("CreateInjector", &Wrappers::Injector_CreateInjector) 

          // Bind Injector class
          ,luabind::class_<Injector>("Injector")

          // Bind Injector::InjectDll wrapper
          .def("InjectDll", &Wrappers::Injector_InjectDll)

          // Bind Injector::GetBase wrapper
          .def("CallExport", &Wrappers::Injector_CallExport)

          // Bind CreateAndInject wrapper
          ,luabind::def("CreateAndInject", &Wrappers::Injector_CreateAndInject)

          // Bind CreateAndInjectInfo class
          ,luabind::class_<Wrappers::CreateAndInjectInfo>(
          "CreateAndInjectInfo")
          .def(luabind::constructor<>())
          .def_readonly("Memory", &Wrappers::CreateAndInjectInfo::Memory)
          .def_readonly("ModBase", &Wrappers::CreateAndInjectInfo::ModBase)
          .def_readonly("ExportRet", &Wrappers::CreateAndInjectInfo::
            ExportRet)

          // Bind StringList class
          ,luabind::class_<Wrappers::StringList>("StringList")
          .def(luabind::constructor<>())
          .def_readonly("List", &Wrappers::StringList::List, 
            luabind::return_stl_iterator)

          // Bind Disassembler::Disassembler wrappers
          ,luabind::def("CreateDisassembler", &Wrappers::
            Disassembler_CreateDisassembler) 

          // Bind Disassembler class
          ,luabind::class_<Disassembler>("Disassembler")

          // Bind Injector::InjectDll wrapper
          .def("DisassembleToStr", &Wrappers::Disassembler_DisassembleToStr)

          // Bind Scanner::Scanner wrappers
          ,luabind::def("CreateScanner", static_cast<Scanner (*) 
            (MemoryMgr const&)>(&Wrappers::Scanner_CreateScanner)) 
          ,luabind::def("CreateScanner", static_cast<Scanner (*) (
            MemoryMgr const&, DWORD_PTR)>(&Wrappers::Scanner_CreateScanner)) 
          ,luabind::def("CreateScanner", static_cast<Scanner (*) 
            (MemoryMgr const&, DWORD_PTR, DWORD_PTR)>(&Wrappers::
            Scanner_CreateScanner)) 

          // Bind Scanner class
          ,luabind::class_<Scanner>("Scanner")

          // Bind Scanner::Find<T> wrappers
          .def("FindInt8", &Wrappers::Scanner_FindInt8) 
          .def("FindUInt8", &Wrappers::Scanner_FindUInt8) 
          .def("FindInt16", &Wrappers::Scanner_FindInt16) 
          .def("FindUInt16", &Wrappers::Scanner_FindUInt16)
          .def("FindInt32", &Wrappers::Scanner_FindInt32)
          .def("FindUInt32", &Wrappers::Scanner_FindUInt32)
          .def("FindInt64", &Wrappers::Scanner_FindInt64)
          .def("FindUInt64", &Wrappers::Scanner_FindUInt64)
          .def("FindFloat", &Wrappers::Scanner_FindFloat)
          .def("FindDouble", &Wrappers::Scanner_FindDouble)
          .def("FindCharNarrow", &Wrappers::Scanner_FindCharNarrow)
          .def("FindCharWide", &Wrappers::Scanner_FindCharWide)
          .def("FindStrNarrow", &Wrappers::Scanner_FindStrNarrow)
          .def("FindStrWide", &Wrappers::Scanner_FindStrWide)
          .def("FindPointer", &Wrappers::Scanner_FindPointer)

          // Bind Scanner::FindAll<T> wrappers
          .def("FindAllInt8", &Wrappers::Scanner_FindAllInt8) 
          .def("FindAllUInt8", &Wrappers::Scanner_FindAllUInt8) 
          .def("FindAllInt16", &Wrappers::Scanner_FindAllInt16) 
          .def("FindAllUInt16", &Wrappers::Scanner_FindAllUInt16)
          .def("FindAllInt32", &Wrappers::Scanner_FindAllInt32)
          .def("FindAllUInt32", &Wrappers::Scanner_FindAllUInt32)
          .def("FindAllInt64", &Wrappers::Scanner_FindAllInt64)
          .def("FindAllUInt64", &Wrappers::Scanner_FindAllUInt64)
          .def("FindAllFloat", &Wrappers::Scanner_FindAllFloat)
          .def("FindAllDouble", &Wrappers::Scanner_FindAllDouble)
          .def("FindAllCharNarrow", &Wrappers::Scanner_FindAllCharNarrow)
          .def("FindAllCharWide", &Wrappers::Scanner_FindAllCharWide)
          .def("FindAllStrNarrow", &Wrappers::Scanner_FindAllStrNarrow)
          .def("FindAllStrWide", &Wrappers::Scanner_FindAllStrWide)
          .def("FindAllPointer", &Wrappers::Scanner_FindAllPointer)

          // Bind Scanner::LoadFromXML wrapper
          .def("LoadFromXML", &Wrappers::Scanner_LoadFromXML)

          // Bind Scanner::operator[] wrapper
          .def("GetAddress", &Wrappers::Scanner_GetAddress)

          // Bind DwordPtrList class
          ,luabind::class_<Wrappers::DwordPtrList>("DwordPtrList")
          .def(luabind::constructor<>())
          .def_readonly("List", &Wrappers::DwordPtrList::List, 
            luabind::return_stl_iterator)

          // Bind ManualMap::ManualMap wrappers
          ,luabind::def("CreateManualMap", &Wrappers::
            ManualMap_CreateManualMap) 

          // Bind ManualMap class
          ,luabind::class_<ManualMap>("ManualMap")

          // Bind ManualMap::Map wrapper
          .def("Map", &Wrappers::ManualMap_Map)
        ];
      }

      void TranslateHadesMemException(lua_State* L, HadesError const& Error)
      {
        lua_pushstring(L, boost::diagnostic_information(Error).c_str());
      }
    };
  }
}
