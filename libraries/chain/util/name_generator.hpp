#pragma once

#include <chain/taiyi_fwd.hpp>

namespace taiyi { namespace chain {

#define RARITY_VALUE_COMMON     1000
#define RARITY_VALUE_UNCOMMON   350
#define RARITY_VALUE_RARE       150
#define RARITY_VALUE_EPIC       75
#define RARITY_VALUE_LEGENDARY  30
#define RARITY_VALUE_MYTHIC     12
#define RARITY_VALUE_EXOTIC     5

/** FNV-1a implementation https://en.wikipedia.org/wiki/Fowler-Noll-Vo_hash_function#FNV-1a_hash */
struct hasher {
   static int32_t hash( uint8_t byte, uint32_t _hash )
   {
      return ( byte ^ _hash ) * prime;
   }
      
   static uint32_t hash( uint32_t four_bytes, uint32_t _hash = offset_basis )
   {
      uint8_t* ptr = (uint8_t*) &four_bytes;
      _hash = hash( *ptr++, _hash );
      _hash = hash( *ptr++, _hash );
      _hash = hash( *ptr++, _hash );
      return  hash( *ptr,   _hash );
   }
      
private:
   // Recommended by http://isthe.com/chongo/tech/comp/fnv/
   static const uint32_t prime         = 0x01000193; //   16777619
   static const uint32_t offset_basis  = 0x811C9DC5; // 2166136261
};

std::string g_generate_location_name(uint32_t rarity, uint32_t random_seed, const std::string& kind = "");
std::string g_generate_actor_name(uint32_t random_seed, std::string& familyName, std::string& middleCharacter, std::string& lastCharacter, int& gender, int& style);

} } // namespace taiyi::chain
