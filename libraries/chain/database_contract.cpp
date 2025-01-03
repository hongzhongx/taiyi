#include <chain/taiyi_fwd.hpp>

#include <protocol/taiyi_operations.hpp>

#include <chain/database.hpp>
#include <chain/database_exceptions.hpp>
#include <chain/global_property_object.hpp>

#include <chain/taiyi_objects.hpp>
#include <chain/transaction_object.hpp>
#include <chain/account_object.hpp>
#include <chain/contract_objects.hpp>

#include <chain/lua_context.hpp>
#include <chain/contract_worker.hpp>

#include <chain/util/uint256.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/uint128.hpp>

#include <fc/container/deque.hpp>

#include <fc/io/fstream.hpp>

#include <iostream>

#include <cstdint>
#include <deque>
#include <fstream>
#include <functional>

#define CONTRACT_BASE_ENV "local baseENV={ \
    setmetatable=setmetatable, date=os.date, import_contract=import_contract, \
    assert=assert, next=next, pairs=pairs, ipairs=ipairs, pcall=pcall, \
    print=print, select=select, tonumber=tonumber, tostring=tostring, type=type, format_vector_with_table=format_vector_with_table, \
    unpack=unpack, _VERSION=_VERSION, xpcall=xpcall, string={ byte=string.byte, char=string.char, find=string.find, \
    format=string.format, gmatch=string.gmatch, gsub=string.gsub, len=string.len, lower=string.lower, match=string.match, \
    rep=string.rep, reverse=string.reverse, sub=string.sub, upper=string.upper }, table={ insert=table.insert, maxn=table.maxn, \
    remove=table.remove, sort=table.sort, concat=table.concat }, math={ abs=math.abs, acos=math.acos, asin=math.asin, \
    atan=math.atan, atan2=math.atan2, ceil=math.ceil, cos=math.cos, cosh=math.cosh, deg=math.deg, exp=math.exp, floor=math.floor, \
    fmod=math.fmod, frexp=math.frexp, huge=math.huge,  ldexp=math.ldexp, log=math.log, log10=math.log10, max=math.max, min=math.min, \
    modf=math.modf, pi=math.pi, pow=math.pow, rad=math.rad, sin=math.sin, sinh=math.sinh, sqrt=math.sqrt, tan=math.tan, \
    tanh=math.tanh } } return baseENV"

#define CONTRACT_BASE_ACTOR "               \
    welcome = { consequence = false }       \
    look = { consequence = false }          \
    go = { consequence = true }             \
    function eval_welcome()                   \
        local nfa = nfa_helper:get_info()   \
        contract_helper:log(string.format('- 欢迎&YEL&%s&NOR&进入游戏 -', contract_helper:get_actor_info(nfa.id).name))  \
    end                                     \
    function eval_look(target)              \
        local nfa = nfa_helper:get_info()   \
        contract_helper:log(string.format('&YEL&%s&NOR&看了看四周。', contract_helper:get_actor_info(nfa.id).name)) \
    end                                     \
    function do_go(dir)                     \
        local nfa = nfa_helper:get_info()   \
        contract_helper:log(string.format('&YEL&%s&NOR&不会走路。', contract_helper:get_actor_info(nfa.id).name)) \
    end                                     \
    function init_data()                    \
        return {                            \
            is_actor = true,                \
            unit = '个'                      \
        }                                   \
    end                                     \
"

#define CONTRACT_BASE_ZONE "                \
    short = { consequence = false }         \
    long = { consequence = false }          \
    exits = { consequence = false }         \
    map = { consequence = false }           \
    function eval_short()                   \
        local nfa = nfa_helper:get_info()   \
        local zone_me = contract_helper:get_zone_info(nfa.id)   \
        return { zone_me.name }             \
    end                                     \
    function eval_long()                         \
        local nfa = nfa_helper:get_info()   \
        local zone_me = contract_helper:get_zone_info(nfa.id)   \
        return { string.format('这是一片&CYN&%s&NOR&', zone_me.type) }  \
    end                                     \
    function eval_exits() return {} end     \
    function eval_map() return {""} end     \
    function on_actor_enter(actor_nfa_id)   \
    end                                     \
    function init_data()                    \
        return {                            \
            is_zone = true,                 \
            unit = '处'                     \
        }                                   \
    end                                     \
"

namespace taiyi { namespace chain {

    void database::initialize_VM_baseENV(LuaContext& context)
    {
        const auto &contract_base = get<contract_object, by_id>(contract_id_type());
        const auto &contract_base_code = get<contract_bin_code_object, by_id>(contract_base.lua_code_b_id);
        lua_getglobal(context.mState, "baseENV");
        if (lua_isnil(context.mState, -1))
        {
            lua_pop(context.mState, 1);
            luaL_loadbuffer(context.mState, contract_base_code.lua_code_b.data(), contract_base_code.lua_code_b.size(), contract_base.name.data());
            lua_setglobal(context.mState, "baseENV");
        }
    }
    //=============================================================================
    size_t database::create_contract_objects(const account_object& owner, const string& contract_name, const string& contract_data, const public_key_type& contract_authority, long long& vm_drops)
    {
        const contract_object& contract = create<contract_object>([&](contract_object &c) {
            c.owner = owner.id;
            c.name = contract_name;
            
            if (c.id != contract_id_type())
            {
                c.contract_authority = contract_authority;
                c.current_version = get_current_trx();
            }
            c.creation_date = head_block_time();
        });
        
        vector<char> lua_code_b;
        contract_worker worker;
        lua_table aco = worker.do_contract(contract.id, contract.name, contract_data, lua_code_b, vm_drops, *this);
        const auto& code_bin_object = create<contract_bin_code_object>([&](contract_bin_code_object& cbo) {
            cbo.contract_id = contract.id;
            cbo.lua_code_b = lua_code_b;
        });
        
        modify(contract, [&](contract_object& c) {
            c.lua_code_b_id = code_bin_object.id;
            c.contract_ABI = aco.v;
        });
        
        return fc::raw::pack_size(contract) + fc::raw::pack_size(code_bin_object);
    }
    //=========================================================================
    void database::create_basic_contract_objects()
    {
        const auto& owner = get_account( TAIYI_DANUO_ACCOUNT );
        long long vm_drops = 1000000;
        create_contract_objects(owner, "contract.baseENV", CONTRACT_BASE_ENV, public_key_type(), vm_drops);

        create_contract_objects(owner, "contract.actor.default", CONTRACT_BASE_ACTOR, public_key_type(), vm_drops);
        create_contract_objects(owner, "contract.zone.default", CONTRACT_BASE_ZONE, public_key_type(), vm_drops);
    }
    //=========================================================================
    lua_map database::prepare_account_contract_data(const account_object& account, const contract_object& contract)
    {
        const auto* acd = find<account_contract_data_object, by_account_contract>( boost::make_tuple(account.id, contract.id) );
        if(acd == nullptr) {
            create<account_contract_data_object>([&](account_contract_data_object &obj) {
                obj.owner = account.id;
                obj.contract_id = contract.id;
            } );
            acd = find<account_contract_data_object, by_account_contract>( boost::make_tuple(account.id, contract.id) );
        }
        
        return acd->contract_data;
    }
    //=========================================================================
    void database::reward_feigang(const account_object& to_account, const account_object& from_account, const asset& feigang )
    {
        FC_ASSERT(feigang.symbol == QI_SYMBOL);
        FC_ASSERT(feigang.amount.value > 0);
        
        adjust_balance( from_account, -feigang );
        adjust_proxied_siming_adores( from_account, -feigang.amount );

        reward_feigang_operation vop = reward_feigang_operation( to_account.name, feigang );
        pre_push_virtual_operation( vop );

        modify_reward_balance(to_account, asset(0, YANG_SYMBOL), asset(0, QI_SYMBOL), feigang, false);

        post_push_virtual_operation( vop );
        
        // Update global feigang pool numbers.
        modify( get_dynamic_global_properties(), [&]( dynamic_global_property_object& props ) {
            props.pending_rewarded_feigang += feigang;
            props.total_qi -= feigang;
        } );
    }
    //=========================================================================
    void database::reward_feigang(const account_object& to_account, const nfa_object& from_nfa, const asset& feigang )
    {
        FC_ASSERT(feigang.symbol == QI_SYMBOL);
        FC_ASSERT(feigang.amount.value > 0);
        
        adjust_nfa_balance( from_nfa, -feigang );

        reward_feigang_operation vop = reward_feigang_operation( to_account.name, feigang );
        pre_push_virtual_operation( vop );

        modify_reward_balance(to_account, asset(0, YANG_SYMBOL), asset(0, QI_SYMBOL), feigang, false);

        post_push_virtual_operation( vop );

        // Update global feigang pool numbers.
        modify( get_dynamic_global_properties(), [&]( dynamic_global_property_object& props ) {
            props.pending_rewarded_feigang += feigang;
            props.total_qi -= feigang;
        } );
    }

} } //taiyi::chain
