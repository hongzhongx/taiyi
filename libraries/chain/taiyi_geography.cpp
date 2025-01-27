#include <chain/taiyi_fwd.hpp>

#include <chain/taiyi_evaluator.hpp>
#include <chain/database.hpp>

#include <chain/taiyi_objects.hpp>
#include <chain/block_summary_object.hpp>
#include <chain/account_object.hpp>
#include <chain/tiandao_property_object.hpp>
#include <chain/contract_objects.hpp>
#include <chain/nfa_objects.hpp>
#include <chain/zone_objects.hpp>
#include <chain/actor_objects.hpp>
#include <chain/cultivation_objects.hpp>

#include <chain/taiyi_geography.hpp>

namespace taiyi { namespace chain {
    
    static std::vector<nature_place_def> s_nature_places = {
        {"山", {SHANYUE, LINDI, DONGXUE, SHILIN, QIULIN, TAOYUAN, SANGYUAN, YAOYUAN}},
        {"峰", {SHANYUE, LINDI, DONGXUE, SHILIN, TAOYUAN, SANGYUAN, YAOYUAN}},
        {"岭", {SHANYUE, LINDI, DONGXUE, SHILIN, QIULIN, TAOYUAN, SANGYUAN, YAOYUAN}},
        {"坪", {YUANYE, NONGTIAN, LINDI, MILIN, YUANLIN, QIULIN, TAOYUAN, SANGYUAN, YAOYUAN}},
        {"崖", {SHANYUE, DONGXUE, SHILIN, XIAGU, YAOYUAN}},
        {"谷", {SHANYUE, LINDI, MILIN, YUANLIN, DONGXUE, SHILIN, TAOYUAN, SANGYUAN, XIAGU, ZAOZE, YAOYUAN}},
        {"岗", {YUANYE, NONGTIAN, SHANYUE, QIULIN}},
        {"洞", {MILIN, SHANYUE, DONGXUE, SHILIN, XIAGU, ZAOZE, YAOYUAN, ANYUAN}},
        {"川", {YUANYE, HUPO, NONGTIAN, XIAGU, ZAOZE, HAIYANG, ANYUAN}},
        {"坡", {NONGTIAN, SHANYUE, QIULIN, SHAMO, HUANGYE}},
        {"原", {YUANYE, NONGTIAN, QIULIN, SHAMO, HUANGYE}},
        {"岛", {HUPO, ZAOZE, HAIYANG, SHAMO}},
        {"屿", {HUPO, HAIYANG}},
        {"渚", {NONGTIAN, ZAOZE, ANYUAN}},
        {"汀", {NONGTIAN, ZAOZE, ANYUAN}},
        {"湾", {NONGTIAN, HAIYANG}},
        {"港", {HUPO, NONGTIAN, HAIYANG}},
        {"渡", {HUPO, NONGTIAN, HAIYANG}},
        {"津", {HUPO, NONGTIAN, HAIYANG}},
        {"林", {NONGTIAN, LINDI, MILIN, YUANLIN, SHILIN, TAOYUAN, SANGYUAN, YAOYUAN}},
        {"湖", {HUPO, ANYUAN}},
        {"潭", {HUPO, ANYUAN}},
        {"池", {HUPO, ZAOZE, ANYUAN}},
        {"泊", {HUPO, ZAOZE, ANYUAN}},
        {"泽", {HUPO, ZAOZE, ANYUAN}},
        {"河", {XIAGU, HUPO, ZAOZE, ANYUAN}},
        {"海", {XIAGU, HUPO, ZAOZE, ANYUAN, HAIYANG}},
        {"江", {XIAGU, ANYUAN}},
        {"水", {XIAGU, HUPO, ZAOZE, ANYUAN, HAIYANG}}
    };

    const std::vector<nature_place_def>& g_get_nature_places() {
        return s_nature_places;
    }


} } // namespace taiyi::chain
