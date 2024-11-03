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
    get_account_contract_data=get_account_contract_data, assert=assert, next=next, pairs=pairs, ipairs=ipairs, pcall=pcall, \
    print=print, select=select, tonumber=tonumber, tostring=tostring, type=type, format_vector_with_table=format_vector_with_table, \
    unpack=unpack, _VERSION=_VERSION, xpcall=xpcall, string={ byte=string.byte, char=string.char, find=string.find, \
    format=string.format, gmatch=string.gmatch, gsub=string.gsub, len=string.len, lower=string.lower, match=string.match, \
    rep=string.rep, reverse=string.reverse, sub=string.sub, upper=string.upper }, table={ insert=table.insert, maxn=table.maxn, \
    remove=table.remove, sort=table.sort, concat=table.concat }, math={ abs=math.abs, acos=math.acos, asin=math.asin, \
    atan=math.atan, atan2=math.atan2, ceil=math.ceil, cos=math.cos, cosh=math.cosh, deg=math.deg, exp=math.exp, floor=math.floor, \
    fmod=math.fmod, frexp=math.frexp, huge=math.huge,  ldexp=math.ldexp, log=math.log, log10=math.log10, max=math.max, min=math.min, \
    modf=math.modf, pi=math.pi, pow=math.pow, rad=math.rad, sin=math.sin, sinh=math.sinh, sqrt=math.sqrt, tan=math.tan, \
    tanh=math.tanh } } return baseENV"

namespace taiyi { namespace chain {

    void database::create_VM()
    {
        if(_luaVM)
            delete _luaVM;
        _luaVM = new LuaContext(true);
    }
    //=============================================================================
    void database::release_VM()
    {
        if(_luaVM) {
            delete _luaVM;
            _luaVM = 0;
        }
    }
    //=============================================================================
    void database::initialize_VM_baseENV()
    {
        assert(_luaVM);
        
        const auto &contract_base = get<contract_object, by_id>(contract_id_type());
        const auto &contract_base_code = get<contract_bin_code_object, by_id>(contract_base.lua_code_b_id);
        lua_getglobal(_luaVM->mState, "baseENV");
        if (lua_isnil(_luaVM->mState, -1))
        {
            lua_pop(_luaVM->mState, 1);
            luaL_loadbuffer(_luaVM->mState, contract_base_code.lua_code_b.data(), contract_base_code.lua_code_b.size(), contract_base.name.data());
            lua_setglobal(_luaVM->mState, "baseENV");
        }
    }
    //=============================================================================
    size_t database::create_contract_objects(const account_name_type& owner, const string& contract_name, const string& contract_data, const public_key_type& contract_authority, long long& vm_drops)
    {
        const auto& owner_account = get_account( owner );
        
        const contract_object& contract = create<contract_object>([&](contract_object &c) {
            c.owner = owner_account.id;
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
        lua_table aco = worker.do_contract(contract.id, contract.name, contract_data, lua_code_b, vm_drops, get_luaVM().mState, *this);
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
        long long vm_drops = 1000000;
        create_contract_objects(TAIYI_YEMING_ACCOUNT, "contract.baseENV", CONTRACT_BASE_ENV, public_key_type(), vm_drops);
    }
    //=========================================================================
    void database::reward_contract_owner(const account_name_type& account_name, const asset& qi )
    {
        FC_ASSERT(qi.symbol == QI_SYMBOL);
        FC_ASSERT(qi.amount.value > 0);
        
        const reward_fund_object& rf = get< reward_fund_object, by_name >( TAIYI_CONTENT_REWARD_FUND_NAME );
        if(rf.reward_qi_balance.amount <= 0)
            return; //no reward fund
        
        const account_object& account = get_account(account_name);
        
        if(qi > rf.reward_qi_balance) {
            modify_reward_balance(account, asset(0, YANG_SYMBOL), rf.reward_qi_balance, true);

            modify(rf, [&](reward_fund_object& rfo) {
                rfo.reward_qi_balance.amount = 0;
            });
        }
        else {
            modify_reward_balance(account, asset(0, YANG_SYMBOL), qi, true);

            modify(rf, [&](reward_fund_object& rfo) {
                rfo.reward_qi_balance -= qi;
            });
        }
    }

} } //taiyi::chain
