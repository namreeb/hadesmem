// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include <windows.h>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <tclap/cmdline.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/call.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/debug_privilege.hpp>
#include <hadesmem/detail/to_upper_ordinal.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/find_pattern.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/process_helpers.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/write.hpp>

#include "character.hpp"
#include "character_manager.hpp"
#include "game_object_manager.hpp"
#include "hash_table.hpp"
#include "item_manager.hpp"
#include "std_string.hpp"
#include "translated_string_repository.hpp"
#include "vec3f.hpp"

struct Offsets
{
  enum : std::uint32_t
  {
    g_character_manager = 0x00E4C524 - 0x00400000,
    g_translated_string_repository = 0x00DEC4C4 - 0x00400000,
  };
};

void DumpHashTableEntry(hadesmem::Process const& process,
                        TranslatedStringData const& data)
{
  std::cout << "Value::Unknown0000: " << data.unknown_0000_ << '\n';
  std::cout << "Value::UUID (Address): " << static_cast<void*>(data.uuid_)
            << '\n';
  std::cout << "Value::UUID: "
            << hadesmem::ReadString<char>(process, data.uuid_) << '\n';
}

void DumpHashTableEntry(hadesmem::Process const& /*process*/,
                        TranslatedNumberData const& data)
{
  std::cout << "Value::Unknown0000: " << data.unknown_0000_ << '\n';
  std::cout << "Value::Unknown0004: " << data.unknown_0004_ << '\n';
  std::cout << "Value::Unknown0008: " << data.unknown_0008_ << '\n';
  std::cout << "Value::Unknown000C: " << data.unknown_000C_ << '\n';
}

template <typename HashTableT>
void DumpStringManagerHashTable(hadesmem::Process const& process,
                                HashTableT const& hash_table)
{
  for (std::uint32_t i = 0; i < hash_table.size_; ++i)
  {
    auto const entry_ptr = hadesmem::Read<typename HashTableT::Entry*>(
      process, hash_table.table_ + i);
    std::cout << "\nHead: " << entry_ptr << '\n';

    for (auto entry =
           hadesmem::Read<typename HashTableT::Entry>(process, entry_ptr);
         ;
         entry =
           hadesmem::Read<typename HashTableT::Entry>(process, entry.next_))
    {
      std::cout << '\n';

      std::cout << "Next: " << entry.next_ << '\n';
      std::cout << "String: " << hadesmem::ReadString<char>(process, entry.key_)
                << '\n';
      std::cout << "String (Address): " << static_cast<void const*>(entry.key_)
                << '\n';
      DumpHashTableEntry(process, entry.value_);

      if (entry.next_ == nullptr)
      {
        break;
      }
    }

    std::cout << '\n';
  }
}

void DumpStringManager(hadesmem::Process const& process, std::uint8_t* base)
{
  std::vector<Character*> characters;

  auto const translated_string_repository_ptr = hadesmem::Read<void*>(
    process, base + Offsets::g_translated_string_repository);
  auto const translated_string_repository =
    hadesmem::Read<TranslatedStringRepository>(
      process, translated_string_repository_ptr);

  std::cout << "TranslatedStringRepository: "
            << translated_string_repository_ptr << '\n';
  std::cout << "TranslatedStringRepository::VTable: "
            << translated_string_repository.vtable_ << '\n';
  std::cout << '\n';

  std::cout << "Dumping translated strings.\n\n";
  DumpStringManagerHashTable(process, translated_string_repository.strings_);

  std::cout << "Dumping translated numbers.\n\n";
  DumpStringManagerHashTable(process, translated_string_repository.numbers_);
}

void DumpCharacter(hadesmem::Process const& process, Character* character_ptr)
{
  auto const character =
    hadesmem::ReadUnsafe<Character>(process, character_ptr);
  std::cout << "Position::X: " << character.position_.x_ << '\n';
  std::cout << "Position::Y: " << character.position_.y_ << '\n';
  std::cout << "Position::Z: " << character.position_.z_ << '\n';
  std::cout << "Handle: " << std::hex << character.handle_ << std::dec << '\n';
  std::cout << "Flags: " << std::hex << character.flags_ << std::dec << '\n';
  std::cout << "Flags::IsPlayer: " << std::boolalpha
            << !!(character.flags_ & CharacterFlags::CharacterFlags_IsPlayer)
            << std::noboolalpha << '\n';
  std::cout << "Flags::IsPartyMember: " << std::boolalpha
            << !!(character.flags_ &
                  CharacterFlags::CharacterFlags_IsPartyMember)
            << std::noboolalpha << '\n';
  std::cout << "Flags::IsDead: " << std::boolalpha
            << !!(character.flags_ & CharacterFlags::CharacterFlags_IsDead)
            << std::noboolalpha << '\n';
  std::cout << "Flags::IsSummon: " << std::boolalpha
            << !!(character.flags_ & CharacterFlags::CharacterFlags_IsSummon)
            << std::noboolalpha << '\n';
  std::cout << "Flags::IsInvulnerable: " << std::boolalpha
            << !!(character.flags_ &
                  CharacterFlags::CharacterFlags_IsInvulnerable)
            << std::noboolalpha << '\n';
  std::cout << "Flags::IsImmortal: " << std::boolalpha
            << !!(character.flags_ & CharacterFlags::CharacterFlags_IsImmortal)
            << std::noboolalpha << '\n';
  std::cout << "Flags::IsStoryNpc: " << std::boolalpha
            << !!(character.flags_ & CharacterFlags::CharacterFlags_IsStoryNpc)
            << std::noboolalpha << '\n';
  std::cout << "Flags::CanFight: " << std::boolalpha
            << !!(character.flags_ & CharacterFlags::CharacterFlags_CanFight)
            << std::noboolalpha << '\n';
  std::cout << "Flags::IsInvisible: " << std::boolalpha
            << !!(character.flags_ & CharacterFlags::CharacterFlags_IsVisible)
            << std::noboolalpha << '\n';
  std::cout << "Flags::IsFloating: " << std::boolalpha
            << !!(character.flags_ & CharacterFlags::CharacterFlags_IsFloating)
            << std::noboolalpha << '\n';
  std::cout << "Flags::IsInFightMode: " << std::boolalpha
            << !!(character.flags_ &
                  CharacterFlags::CharacterFlags_IsInFightMode)
            << std::noboolalpha << '\n';
  std::cout << "Flags::CanSpotSneakers: " << std::boolalpha
            << !!(character.flags_ &
                  CharacterFlags::CharacterFlags_CanSpotSneakers)
            << std::noboolalpha << '\n';
  std::cout << "Region (Address): " << static_cast<void*>(character.region_)
            << '\n';
  std::cout << "Region: "
            << hadesmem::ReadString<char>(process, character.region_) << '\n';
  std::cout << "UUID (Address): " << static_cast<void*>(character.uuid_)
            << '\n';
  std::cout << "UUID: " << hadesmem::ReadString<char>(process, character.uuid_)
            << '\n';
  std::cout << "CurrentTemplate: " << character.current_template_ << '\n';
  auto const current_character_template =
    hadesmem::Read<CharacterTemplate>(process, character.current_template_);
  std::cout << "CurrentTemplate::Name: "
            << GetString(process, current_character_template.name_) << '\n';
  std::cout << "OriginalTemplate: " << character.original_template_ << '\n';
  auto const original_character_template =
    hadesmem::Read<CharacterTemplate>(process, character.current_template_);
  std::cout << "OriginalTemplate::Name: "
            << GetString(process, original_character_template.name_) << '\n';
  std::cout << "Stats: " << character.stats_ << '\n';
  auto const character_stats =
    hadesmem::Read<CharacterStats>(process, character.stats_);
  std::cout << "Stats::Level: " << character_stats.level_ << '\n';
  std::cout << "Stats::HP: " << character_stats.vitality_ << '\n';
  std::cout << "Stats::AP: " << character_stats.action_points_ << '\n';
  std::cout << "Stats::XP: " << character_stats.experience_ << '\n';
  std::cout << "Stats::Reputation: " << character_stats.reputation_ << '\n';
  std::cout << "IsInCombat: " << static_cast<int>(character.is_in_combat_)
            << '\n';
  std::cout << "ReservedPeerId: " << std::hex << character.reserved_peer_id_
            << std::dec << '\n';
  std::cout << "PlayerData: " << character.player_data_ << '\n';
  if (character.player_data_)
  {
    auto const character_player_data =
      hadesmem::Read<CharacterPlayerData>(process, character.player_data_);
    std::cout << "PlayerData::Attribute: "
              << character_player_data.attribute_points_ << '\n';
    std::cout << "PlayerData::Ability: "
              << character_player_data.ability_points_ << '\n';
    std::cout << "PlayerData::Talent: " << character_player_data.talent_points_
              << '\n';
  }
  std::cout << "Gender: " << std::hex << character.gender_ << std::dec << '\n';
}

std::vector<Character*> DumpCharacterArray(hadesmem::Process const& process,
                                           Character** ptr_array,
                                           WORD* type_id_array,
                                           std::uint32_t array_len)
{
  std::vector<Character*> characters;
  characters.reserve(array_len);

  auto character_ptr_array =
    hadesmem::ReadVector<Character*>(process, ptr_array, array_len);
  auto const character_type_id_array =
    type_id_array
      ? hadesmem::ReadVector<std::uint16_t>(process, type_id_array, array_len)
      : std::vector<std::uint16_t>();
  for (std::size_t i = 0; i < character_ptr_array.size(); ++i)
  {
    auto const character_ptr = character_ptr_array[i];
    if (type_id_array)
    {
      std::cout << "Character " << std::hex << i << std::dec << ": "
                << character_ptr << " (Type: " << std::hex
                << character_type_id_array[i] << std::dec << ")\n";
    }
    else
    {
      std::cout << "Character " << std::hex << i << std::dec << ": "
                << character_ptr << "\n";
    }

    if (character_ptr != 0)
    {
      DumpCharacter(process, character_ptr);
      characters.push_back(character_ptr);
    }
    else
    {
      std::cout << "  Empty character slot.\n";
    }

    std::cout << '\n';
  }

  return characters;
}

std::vector<Character*> DumpCharacterManager(hadesmem::Process const& process,
                                             std::uint8_t* base)
{
  auto const character_manager_ptr = hadesmem::Read<CharacterManager*>(
    process, base + Offsets::g_character_manager);
  auto const character_manager =
    hadesmem::Read<CharacterManager>(process, character_manager_ptr);
  std::cout << "CharacterManager::CharacterPtrArray: "
            << character_manager.character_ptr_array_ << '\n';
  std::cout << "CharacterManager::CharacterPtrArrayLen: " << std::hex
            << character_manager.character_ptr_array_len_ << std::dec << '\n';
  std::cout << "CharacterManager::CharacterTypeIdArray: "
            << static_cast<void*>(character_manager.character_type_id_array_)
            << '\n';
  std::cout << "CharacterManager::PartyMemberPtrArray: "
            << character_manager.party_manager_.party_member_ptr_array_ << '\n';
  std::cout << "CharacterManager::PartyMemberPtrArrayLen: " << std::hex
            << character_manager.party_manager_.party_member_ptr_array_len_
            << std::dec << '\n';
  std::cout << '\n';

  std::cout << "Dumping character array.\n\n";
  auto character_array =
    DumpCharacterArray(process,
                       character_manager.character_ptr_array_,
                       character_manager.character_type_id_array_,
                       character_manager.character_ptr_array_len_);

  std::cout << "Dumping party array.\n\n";
  DumpCharacterArray(
    process,
    character_manager.party_manager_.party_member_ptr_array_,
    nullptr,
    character_manager.party_manager_.party_member_ptr_array_len_);

  return character_array;
}

void DumpCharacterContainer(hadesmem::Process const& process,
                            std::vector<Character*> const& characters)
{
  for (auto const& character : characters)
  {
    DumpCharacter(process, character);
    std::cout << '\n';
  }
}

int main(int argc, char* argv[])
{
  try
  {
    std::cout << "HadesMem \"Divinity: Original Sin\" Trainer ["
              << HADESMEM_VERSION_STRING << "]\n";

    TCLAP::CmdLine cmd{
      "Divinity: Original Sin Trainer", ' ', HADESMEM_VERSION_STRING};
    TCLAP::ValueArg<DWORD> pid_arg{"",
                                   "pid",
                                   "Process ID (for multiple ESO instances)",
                                   false,
                                   0,
                                   "DWORD",
                                   cmd};
    cmd.parse(argc, argv);

    try
    {
      hadesmem::GetSeDebugPrivilege();

      std::wcout << "\nAcquired SeDebugPrivilege.\n";
    }
    catch (std::exception const& /*e*/)
    {
      std::wcout << "\nFailed to acquire SeDebugPrivilege.\n";
    }

    std::unique_ptr<hadesmem::Process> process;

    if (pid_arg.isSet())
    {
      DWORD const pid = pid_arg.getValue();
      process = std::make_unique<hadesmem::Process>(pid);
    }
    else
    {
      std::wstring const kProcName = L"EoCApp.exe";
      process = std::make_unique<hadesmem::Process>(
        hadesmem::GetProcessByName(kProcName, false));
    }

    auto const base = reinterpret_cast<std::uint8_t*>(
      hadesmem::Module(*process, nullptr).GetHandle());

    DumpCharacterManager(*process, base);

    DumpStringManager(*process, base);

    std::cout << "\nFinished.\n";

    return 0;
  }
  catch (...)
  {
    std::cerr << "\nError!\n";
    std::cerr << boost::current_exception_diagnostic_information() << '\n';

    return 1;
  }
}
