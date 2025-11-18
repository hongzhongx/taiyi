#include <chain/taiyi_fwd.hpp>

#include <chain/taiyi_evaluator.hpp>
#include <chain/database.hpp>
#include <chain/taiyi_objects.hpp>
#include <chain/block_summary_object.hpp>
#include <chain/account_object.hpp>
#include <chain/tiandao_property_object.hpp>
#include <chain/nfa_objects.hpp>
#include <chain/actor_objects.hpp>
#include <chain/zone_objects.hpp>

#include <chain/lua_context.hpp>
#include <chain/contract_worker.hpp>

#include <fc/macros.hpp>

#include <limits>

extern std::wstring utf8_to_wstring(const std::string& str);
extern std::string wstring_to_utf8(const std::wstring& str);

namespace taiyi { namespace chain {

    extern std::string s_debug_actor;

    std::string g_zone_type_strings[_ZONE_TYPE_NUM] = {
        "XUKONG",         //虚空
        
        "YUANYE",         //原野
        "HUPO",           //湖泊
        "NONGTIAN",       //农田

        "LINDI",          //林地
        "MILIN",          //密林
        "YUANLIN",        //园林
        
        "SHANYUE",        //山岳
        "DONGXUE",        //洞穴
        "SHILIN",         //石林
        
        "QIULIN",         //丘陵
        "TAOYUAN",        //桃源
        "SANGYUAN",       //桑园
        
        "XIAGU",          //峡谷
        "ZAOZE",          //沼泽
        "YAOYUAN",        //药园
        
        "HAIYANG",        //海洋
        "SHAMO",          //沙漠
        "HUANGYE",        //荒野
        "ANYUAN",         //暗渊

        "DUHUI",          //都会
        "MENPAI",         //门派
        "SHIZHEN",        //市镇
        "GUANSAI",        //关寨
        "CUNZHUANG"      //村庄
    };

    E_ZONE_TYPE get_zone_type_from_string(const std::string& sType) {
        for(unsigned int i=0; i<_ZONE_TYPE_NUM; i++) {
            if(sType == g_zone_type_strings[i])
                return (E_ZONE_TYPE)i;
        }
        
        return _ZONE_INVALID_TYPE;
    }
    
    string get_zone_type_string(E_ZONE_TYPE type) {
        if(type >= _ZONE_TYPE_NUM)
            return "";
        else
            return g_zone_type_strings[type];
    }

} } // taiyi::chain

