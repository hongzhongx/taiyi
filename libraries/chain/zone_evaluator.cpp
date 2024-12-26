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

#include <chain/util/manabar.hpp>

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
    //=============================================================================
    operation_result create_zone_evaluator::do_apply( const create_zone_operation& o )
    { try {
        const auto& creator = _db.get_account( o.creator ); // prove it exists
        auto now = _db.head_block_time();
                    
        //check zone existence
        auto check_zone = _db.find< zone_object, by_name >( o.name );
        FC_ASSERT( check_zone == nullptr, "There is already exist zone named \"${a}\".", ("a", o.name) );

        const auto& props = _db.get_dynamic_global_properties();
        const siming_schedule_object& wso = _db.get_siming_schedule_object();

        FC_ASSERT( o.fee <= asset( TAIYI_MAX_ZONE_CREATION_FEE, QI_SYMBOL ), "Zone creation fee cannot be too large" );
        FC_ASSERT( o.fee == (wso.median_props.account_creation_fee * TAIYI_QI_SHARE_PRICE), "Must pay the exact zone creation fee. paid: ${p} fee: ${f}", ("p", o.fee) ("f", wso.median_props.account_creation_fee * TAIYI_QI_SHARE_PRICE) );

        _db.adjust_balance( creator, -o.fee );

        contract_result result;

        //先创建NFA
        string nfa_symbol_name = "nfa.zone.default";
        const auto* nfa_symbol = _db.find<nfa_symbol_object, by_symbol>(nfa_symbol_name);
        FC_ASSERT(nfa_symbol != nullptr, "NFA symbol named \"${n}\" is not exist.", ("n", nfa_symbol_name));
        
        const auto* current_trx = _db.get_current_trx_ptr();
        FC_ASSERT(current_trx);
        const flat_set<public_key_type>& sigkeys = current_trx->get_signature_keys(_db.get_chain_id(), fc::ecc::fc_canonical);
        
        LuaContext context;
        _db.initialize_VM_baseENV(context);
        
        const auto& nfa = _db.create_nfa(creator, *nfa_symbol, sigkeys, true, context);
        
        protocol::nfa_affected affected;
        affected.affected_account = creator.name;
        affected.affected_item = nfa.id;
        affected.action = nfa_affected_type::create_for;
        result.contract_affecteds.push_back(std::move(affected));
        
        affected.affected_account = creator.name;
        affected.action = nfa_affected_type::create_by;
        result.contract_affecteds.push_back(std::move(affected));
        
        //创建一个虚空区域
        const auto& new_zone = _db.create< zone_object >( [&]( zone_object& zone ) {
            _db.initialize_zone_object( zone, o.name, nfa, XUKONG);
        });
        
        if( o.fee.amount > 0 ) {
            _db.modify(nfa, [&](nfa_object& obj) {
                obj.qi += o.fee;
            });
        }

        int64_t used_qi = fc::raw::pack_size(new_zone) * TAIYI_USEMANA_STATE_BYTES_SCALE + 2000 * TAIYI_USEMANA_EXECUTION_SCALE;
        FC_ASSERT( creator.qi.amount.value >= used_qi, "Creator account does not have enough qi to create zone." );

        //reward to treasury
        _db.reward_contract_owner_from_account(_db.get<account_object, by_name>(TAIYI_TREASURY_ACCOUNT), creator, asset(used_qi, QI_SYMBOL));

        affected.affected_account = creator.name;
        affected.affected_item = nfa.id;
        affected.action = nfa_affected_type::deposit_qi;
        result.contract_affecteds.push_back(std::move(affected));
                                
        //_db.push_virtual_operation( zone_creation_approved_operation( new_comment.author, new_comment.permlink, new_zone.name ) );
        
        return result;
    } FC_CAPTURE_AND_RETHROW( (o) ) }

} } // taiyi::chain

