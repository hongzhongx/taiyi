#include <boost/test/unit_test.hpp>

#include <chain/taiyi_fwd.hpp>

#include <chain/database.hpp>
#include <protocol/protocol.hpp>

#include <protocol/taiyi_operations.hpp>
#include <chain/account_object.hpp>
#include <chain/contract_objects.hpp>

#include <fc/crypto/digest.hpp>
#include <fc/crypto/hex.hpp>
#include "../db_fixture/database_fixture.hpp"

extern "C" {
#include "lundump.h"
#include "lstate.h"
}

#include <algorithm>
#include <random>

#include <chain/lua_context.hpp>
#include <chain/contract_handles.hpp>

using namespace taiyi;
using namespace taiyi::chain;
using namespace taiyi::protocol;

void get_data(lua_State *L, int index)
{
    int type = lua_type(L, index);
    switch(type)
    {
        case LUA_TNIL:
            printf("\r\nLUA_TNIL");
            break;
        case LUA_TSTRING:
            printf("\r\nLUA_TSTRING:%s", lua_tostring(L, index));
            break;
        case LUA_TBOOLEAN:
            printf("\r\nLUA_TBOOLEAN:%s", lua_toboolean(L, index)?"true":"false");
            break;
        case LUA_TNUMBER:
            printf("\r\nLUA_TNUMBER:%g",lua_tonumber(L, index));
            break;
        case LUA_TTABLE:
        {
            printf("\r\nthis is a table:");
            lua_pushnil( L );  // nil 入栈作为初始 key
            while( 0 != lua_next( L, index ) )
            {
                // 现在栈顶(-1)是 value，-2 位置是对应的 key
                // 这里可以判断 key 是什么并且对 value 进行各种处理
                get_data(L, index+1);
                get_data(L, index+2);
                lua_pop( L, 1 );  // 弹出 value，让 key 留在栈顶
            }
            break;
        }
        default:
            printf("\r\n%s", lua_typename(L, type));
            break;
    }
}

void stackDump(lua_State *L, int top)
{
    int i;
    //printf("the size of stack is:%d\n",top);
    //for(i=1; i<=top; i++)
    {
        get_data(L,top);
    }
    printf("\n");
}

BOOST_FIXTURE_TEST_SUITE( contract_tests, clean_database_fixture )

namespace contract_test_case_lua_vm
{
    int compiling_contract_writer_test(lua_State *L, const void *p, size_t size, void *u)
    {
        vector<char> *s = (vector<char> *)u;
        s->insert(s->end(), (const char *)p, (const char *)p + size);
        return 0;
    }
}

BOOST_AUTO_TEST_CASE( lua_vm )
{ try {
    using namespace contract_test_case_lua_vm;
    
    LuaContext context;
    
    lua_enabledrops(context.mState, 1, true);
    lua_setdrops(context.mState, 2000);
    
    string lua_code =   "function hello_world() \n \
                            -- very import \n \
                            print('hello world') \n \
                        end";
    string name = "test";
    
    lua_getglobal(context.mState, name.c_str());
    BOOST_CHECK_EQUAL(lua_isnil(context.mState, -1), true);
    
    lua_newtable(context.mState);
    lua_setglobal(context.mState, name.c_str());
    
    int err = luaL_loadbuffer(context.mState, lua_code.data(), lua_code.size(), name.c_str());
    if(err != 0)
        wlog("Try the contract resolution compile failure, ${message}", ("message", string(lua_tostring(context.mState, -1))));
    
    lua_getglobal(context.mState, name.c_str());
    lua_setupvalue(context.mState, -2, 1);
    
    Proto *f = getproto(context.mState->top - 1);
    lua_lock(context.mState);
    vector<char> lua_code_b;
    luaU_dump(context.mState, f, compiling_contract_writer_test, &lua_code_b, 0);
    lua_unlock(context.mState);
    
    err |= lua_pcall(context.mState, 0, 0, 0);
    if(err != 0)
        wlog("Try the contract resolution compile failure, ${message}", ("message", string(lua_tostring(context.mState, -1))));
    
    lua_getglobal(context.mState, name.c_str());
    
    auto index = lua_gettop(context.mState);
    FC_ASSERT(context.mState);
    FC_ASSERT(lua_type(context.mState, index) == LUA_TTABLE);
    lua_table v_table;
    lua_pushnil(context.mState); // nil 入栈作为初始 key
    while (0 != lua_next(context.mState, index))
    {
        try
        {
            if (LUA_TFUNCTION == lua_type(context.mState, index + 2))
            {
                auto op_key = lua_string(lua_tostring(context.mState, index + 1));
                wlog("${k}", ("k", op_key));
                lua_key key = lua_types(op_key);
                auto sfunc = LuaContext::Reader<FunctionSummary>::read(context.mState, index + 2);
                if (sfunc) {
                    v_table.v[key] = *sfunc;
                    wlog("${f}", ("f", *sfunc));
                }
            }
        }
        FC_CAPTURE_AND_RETHROW()
        lua_pop(context.mState, 1);
    }
    
    lua_pushnil(context.mState);
    lua_setglobal(context.mState, name.c_str());
    
    ilog("consume ${d} drops.", ("d", 2000 - lua_getdrops(context.mState)));
    
} FC_LOG_AND_RETHROW() }
//=============================================================================
BOOST_AUTO_TEST_CASE( native_functions )
{ try {
    struct Foo {
        static int increment(int x)
        {
            return x + 1;
        }
    };
    
    LuaContext context;
    
    context.writeVariable("f", &Foo::increment);
    context.writeFunction<int (int)>("g", &Foo::increment);
    context.writeFunction("h", &Foo::increment);
    
    BOOST_CHECK_EQUAL(3, context.executeCode<int>("return f(2)"));
    BOOST_CHECK_EQUAL(13, context.executeCode<int>("return g(12)"));
    BOOST_CHECK_EQUAL(9, context.executeCode<int>("return h(8)"));
    BOOST_CHECK_THROW(context.executeCode<int>("return f(true)"), LuaContext::ExecutionErrorException);
} FC_LOG_AND_RETHROW() }
//=============================================================================
BOOST_AUTO_TEST_CASE( callback_functions )
{ try {
    
    ACTORS( (alice)(bob)(carol) )
    
#if 0 //need actor db fixture
    
    struct Foo {
        static int increment(int x)
        {
            return x + 1;
        }
    };
    
    string envcode = "local baseENV={ \
                        f = f \
                    } return baseENV";
    
    string code = "function test_back() \
                    local a = import_contract('contract.actor.base') \
                    return a \
                   end";
    
    LuaContext context;
    string space_name = "test_space";
    
    const contract_object& contract = db->get<contract_object, by_id>(0);
    const account_object& caller = alice;
    contract_base_info cbi(*db, context, "alice", contract.name, caller.name, string(contract.creation_date), string(contract.contract_authority), contract.name);
    contract_result result;
    lua_map account_data;
    flat_set<public_key_type> sigkeys;
    contract_handler ch(*db, caller, contract, result, context, sigkeys, result, account_data);
    
    context.new_sandbox(space_name, envcode.c_str(), envcode.size()); //sandbox
    context.writeVariable(space_name, "chainhelper", &ch);
    context.writeVariable(space_name, "contract_base_info", &cbi);
    context.writeVariable(space_name, "actorhelper", &ca);
    context.writeVariable(space_name, "zonehelper", &cz);
    
    lua_getglobal(context.mState, space_name.c_str());
    stackDump(context.mState, lua_gettop(context.mState));
    
    int t = lua_getfield(context.mState, -1, "import_contract");
    BOOST_CHECK_EQUAL(LUA_TFUNCTION, t);
    lua_pop(context.mState, -1);
    
    bool bOK = context.load_script_to_sandbox(space_name, code.c_str(), code.size());
    FC_ASSERT(bOK);
    context.writeVariable("current_contract", space_name);
    
    bOK = context.get_function(space_name, "test_back");
    FC_ASSERT(bOK);
    int err = lua_pcall(context.mState, 0, 1, 0);
    if(err)
        wlog("${message}", ("message", string(lua_tostring(context.mState, -1))));
#endif
    
} FC_LOG_AND_RETHROW() }
//=============================================================================
namespace contract_test_case_var
{
    typedef struct student
    {
        string name = "dasd";
        vector<char> info;
    } student;
    
    //待Lua调用的C注册函数示例——加法
    int add2(lua_State* L)
    {
        //检查栈中的参数是否合法，1表示Lua调用时的第一个参数(从左到右)，依此类推。
        //如果Lua代码在调用时传递的参数不为number，该函数将报错并终止程序的执行。
        double op1 = luaL_checknumber(L,1);
        double op2 = luaL_checknumber(L,2);
        
        //将函数的结果压入栈中。如果有多个返回值，可以在这里多次压入栈中。
        lua_pushnumber(L, op1 + op2);
        
        //返回值用于提示该C函数的返回值数量，即压入栈中的返回值数量。
        return 1;
    }
    
    //待Lua调用的C注册函数示例——减法
    int sub2(lua_State* L)
    {
        double op1 = luaL_checknumber(L,1);
        double op2 = luaL_checknumber(L,2);
        lua_pushnumber(L,op1 - op2);
        return 1;
    }
    
    typedef int(*function_ptr)(lua_State*);
    
    int new_database_to_lua(lua_State* L, student &u, string name)
    {
        student *p = (student*)lua_newuserdata(L, sizeof(student));
        memcpy(p, &u, sizeof(student));
        luaL_getmetatable(L, name.data());
        lua_setmetatable(L, -2);
        return 1;
    }
}

BOOST_AUTO_TEST_CASE( var )
{ try {
    
    using namespace contract_test_case_var;
    
    LuaContext context;
    
    context.writeVariable("a", 5);
    context.writeVariable("tab_test",
                          std::vector< std::pair< std::string, std::string >>
                          {
        { "test", "dasd" },
        { "hello","dasd" },
        { "world", "asd" },
    }
                          );
    student val = student();
    context.writeVariable("val_lua",val);
    context.writeFunction("f", [&val](student& val_lua) {
        val.name = "wobianle";
        val_lua.name = "woyebianle";
    });
    
    context.writeVariable("b", -397);
    
    const string  code = "f(val_lua)";
    context.executeCode(code);
    
    printf("b:%d\r\n", context.readVariable<int>("b"));
    printf("val:%s\r\n", val.name.data());
    printf("val:%s\r\n", context.readVariable<student>("val_lua").name.data());
    
    //context.executeCode("print(cjson.encode(tab_test))");
    
    struct Foo { int value = 0; };
    
    context.writeVariable("foo", Foo{});
    context.writeFunction("foo", LuaContext::Metatable, "__call", [](Foo& foo) { foo.value++; });
    context.writeFunction("foo", LuaContext::Metatable, "__index", [](Foo& foo, std::string index) -> int { foo.value += index.length(); return 12; });
    context.writeFunction("foo", LuaContext::Metatable, "daxia", [](Foo& foo) -> int { foo.value--; return foo.value; });
    
} FC_LOG_AND_RETHROW() }
//=============================================================================
namespace contract_test_case_code
{
    template<typename Vector, std::size_t... I>
    auto writer_lua_value(LuaContext& context, const Vector& a, std::index_sequence<I...>)
    {
        return context.writeVariable(a[I]...);
    }
    
    template<typename T, std::size_t N >
    struct writer_helper{
        static auto v2t(LuaContext& context, std::vector<T>& a)
        {
            if(a.size() == N)
                return writer_lua_value(context, a, std::make_index_sequence<N>());
            else
                return writer_helper<T, N-1>::v2t(context, a);
        }
    };
    
    template<typename T>
    struct writer_helper<T, 2>
    {
        static auto v2t(LuaContext& context, std::vector<T>& a)
        {
            throw -1;
        }
    };
}

BOOST_AUTO_TEST_CASE( code )
{ try {
    using namespace contract_test_case_code;
    
    const char* szLua_code = "  contract_global_variable = {'ret', abc = 1} \
                                a = 0 b = 1 \
                                contract_global_variable['1'] = 10 \
                                contract_global_variable[1] = 10 \
                                contract_global_variable['sum'] = 0 \
                                contract_global_variable['nidaye'] = 'dayenihao' \
                                function sum() \
                                    contract_global_variable['sum'] = a + b + contract_global_variable['sum'] \
                                    a = a+1 b = b+2 \
                                end";
    
    const char* main_code = "   function whiletrue() i=1  while(i) do  i=i+1 end end ";
    
    //Lua的字符串模式
    const char* szMode = "(%w+)%s*=%s*(%w+)";
    //要处理的字符串
    const char* szStr = "key1 = value1 key2 = value2";
    //目标字符串模式
    const char* szTag = "<%1>%2</%1>";
    
    LuaContext context;
    lua_State*L = context.mState;
    int err = luaL_loadbuffer(context.mState, szLua_code, strlen(szLua_code), "demo");
    err |= lua_pcall(L, 0, 0, 0);
    if(err)
    {
        //如果错误，显示
        printf("err::%s\r\n", lua_tostring(L, -1));
        //弹出栈顶的这个错误信息
        lua_pop(L, 1);
    }
    
    std::vector<boost::variant<int, std::string>> values;
    values.push_back(boost::variant<int, std::string>("contract_global_variable"));
    values.push_back(boost::variant<int, std::string>("sum"));
    values.push_back(boost::variant<int, std::string>(30));
    
    writer_helper<boost::variant<int, std::string>, 100>::v2t(context, values);
    
    lua_getglobal(L, "sum");
    err |= lua_pcall(L, 0, 0, 0);
    
    if(err)
    {
        printf("err::%s\r\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    
    context.writeVariable("t", LuaContext::EmptyArray);
    
    //for(size_t i = 0; i < count; i++)
    {
        context.writeVariable("t", 1, "woshi nibaba");
        context.writeVariable("t", "Mode", szMode);
        context.writeVariable("t", "Tag", szTag);
        context.writeVariable("t", "Str", szStr);
    }
    
    if(err==0)
    {
        printf("type:%s\r\n", context.executeCode<std::string>("return type(t)").data());
        
        // 现在栈顶是 table
        
        //Lua执行后取得全局变量的值
        lua_getglobal(L, "contract_global_variable");
        
        // 进行下面步骤前先将 table 压入栈顶
        stackDump(L, lua_gettop(L));
        //这个x应该是个table
        
        //弹出栈顶的x
        lua_pop(L, 1);
    }
    
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( create_contract_apply )
{ try {
    string lua_code =   "function hello_world() \n \
                            -- very important \n \
                            print('hello world') \n \
                        end";

    BOOST_TEST_MESSAGE( "Testing: create_contract_apply" );

    signed_transaction tx;
    ACTORS( (alice)(bob)(charlie) )
    generate_block();
        
    BOOST_TEST_MESSAGE( "--- Test failure name invalid" );
    
    create_contract_operation op;
    op.owner = "alice";
    op.name = "contract.tt"; //less than 3 letters
    op.data = lua_code;

    tx.operations.push_back( op );
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    sign( tx, alice_private_key );
    BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

    BOOST_TEST_MESSAGE( "--- Test failure not enough mana" );

    db_plugin->debug_update( [=]( database& db ) {
        db.modify( db.get_account( "alice" ), [&]( account_object& a ) {
            a.manabar.current_mana = 100;
            a.manabar.last_update_time = db.head_block_time().sec_since_epoch();
        });
    });

    tx.operations.clear();
    tx.signatures.clear();
    op.name = "contract.test";
    tx.operations.push_back( op );
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    sign( tx, alice_private_key );
    BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

    BOOST_TEST_MESSAGE( "--- Test use mana" );

    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    generate_block();

    db_plugin->debug_update( [=]( database& db ) {
        db.modify( db.get_account( "alice" ), [&]( account_object& a ) {
            a.manabar.current_mana = util::get_effective_qi_shares(a);
            a.manabar.last_update_time = db.head_block_time().sec_since_epoch();
        });
    });

    util::manabar old_manabar = db->get_account( "alice" ).manabar;
        
    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( op );
    sign( tx, alice_private_key );
    db->push_transaction( tx, 0 );
    
    const account_object& alice_acc = db->get_account( "alice" );
    int64_t used_mana = old_manabar.current_mana - alice_acc.manabar.current_mana;
    //idump( (used_mana) );
    BOOST_REQUIRE( used_mana == 876000 );
    
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( revise_contract_apply )
{ try {
    string lua_code1 =  "function hello_world() \n \
                            -- very important \n \
                            print('hello world') \n \
                        end";

    string lua_code2 =  "function hello_world() \n \
                            -- very important \n \
                            print('better world') \n \
                        end";

    BOOST_TEST_MESSAGE( "Testing: revise_contract_apply" );

    ACTORS( (alice)(bob)(charlie) )
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    generate_block();
        
    signed_transaction tx;

    create_contract_operation op;
    op.owner = "alice";
    op.name = "contract.test";
    op.data = lua_code1;

    tx.operations.push_back( op );
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    sign( tx, alice_private_key );
    db->push_transaction( tx, 0 );
    generate_block();

    BOOST_TEST_MESSAGE( "--- Test failure not enough mana" );

    db_plugin->debug_update( [=]( database& db ) {
        db.modify( db.get_account( "alice" ), [&]( account_object& a ) {
            a.manabar.current_mana = 100;
            a.manabar.last_update_time = db.head_block_time().sec_since_epoch();
        });
    });

    revise_contract_operation rop;
    rop.reviser = "alice";
    rop.contract_name = "contract.test";
    rop.data = lua_code2;

    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( rop );
    sign( tx, alice_private_key );
    BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

    BOOST_TEST_MESSAGE( "--- Test use mana" );

    db_plugin->debug_update( [=]( database& db ) {
        db.modify( db.get_account( "alice" ), [&]( account_object& a ) {
            a.manabar.current_mana = util::get_effective_qi_shares(a);
            a.manabar.last_update_time = db.head_block_time().sec_since_epoch();
        });
    });

    util::manabar old_manabar = db->get_account( "alice" ).manabar;
    
    db->push_transaction( tx, 0 );
    
    const account_object& alice_acc = db->get_account( "alice" );
    int64_t used_mana = old_manabar.current_mana - alice_acc.manabar.current_mana;
    //idump( (used_mana) );
    BOOST_REQUIRE( used_mana == 476000 );
    
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( call_contract_function_apply )
{ try {
    string lua_code1 =  "function hello_world() \n \
                            -- very important \n \
                            print('hello world') \n \
                        end";

    string lua_code2 =  "function hello_world() \n \
                            -- very important \n \
                            print('better world') \n \
                        end";

    BOOST_TEST_MESSAGE( "Testing: call_contract_function_apply" );

    ACTORS( (alice)(bob)(charlie) )
    vest( TAIYI_INIT_SIMING_NAME, "alice", ASSET( "1000.000 YANG" ) );
    vest( TAIYI_INIT_SIMING_NAME, "bob", ASSET( "1000.000 YANG" ) );
    generate_block();
        
    signed_transaction tx;

    create_contract_operation op;
    op.owner = "bob";
    op.name = "contract.test";
    op.data = lua_code1;

    tx.operations.push_back( op );
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    sign( tx, bob_private_key );
    db->push_transaction( tx, 0 );
    validate_database();

    generate_block();

    BOOST_TEST_MESSAGE( "--- Test failure not enough mana" );

    db_plugin->debug_update( [=]( database& db ) {
        db.modify( db.get_account( "alice" ), [&]( account_object& a ) {
            a.manabar.current_mana = 100;
            a.manabar.last_update_time = db.head_block_time().sec_since_epoch();
        });
    });

    call_contract_function_operation cop;
    cop.caller = "alice";
    cop.contract_name = "contract.test";
    cop.function_name = "hello_world";

    tx.operations.clear();
    tx.signatures.clear();
    tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
    tx.operations.push_back( cop );
    sign( tx, alice_private_key );
    BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

    BOOST_TEST_MESSAGE( "--- Test use mana" );

    db_plugin->debug_update( [=]( database& db ) {
        db.modify( db.get_account( "alice" ), [&]( account_object& a ) {
            a.manabar.current_mana = util::get_effective_qi_shares(a);
            a.manabar.last_update_time = db.head_block_time().sec_since_epoch();
        });
    });

    util::manabar old_manabar = db->get_account( "alice" ).manabar;
    asset old_reward_qi = db->get_account("bob").reward_qi_balance;
    
    db->push_transaction( tx, 0 );
    validate_database();

    int64_t used_mana = old_manabar.current_mana - db->get_account( "alice" ).manabar.current_mana;
    //idump( (used_mana) );
    BOOST_REQUIRE( used_mana == 431000 );
    
    asset reward_qi = db->get_account("bob").reward_qi_balance - old_reward_qi;
    //idump( (reward_qi) );
    BOOST_REQUIRE( reward_qi == asset(431000, QI_SYMBOL) );

    BOOST_TEST_MESSAGE( "--- Test again use same mana" );
    
    for(int i = 0; i<3; i++) {

        generate_block();

        db_plugin->debug_update( [=]( database& db ) {
            db.modify( db.get_account( "alice" ), [&]( account_object& a ) {
                a.manabar.current_mana = util::get_effective_qi_shares(a);
                a.manabar.last_update_time = db.head_block_time().sec_since_epoch();
            });
        });
        
        old_manabar = db->get_account( "alice" ).manabar;
        old_reward_qi = db->get_account("bob").reward_qi_balance;
        
        tx.signatures.clear();
        tx.set_expiration( db->head_block_time() + TAIYI_MAX_TIME_UNTIL_EXPIRATION );
        sign( tx, alice_private_key );
        db->push_transaction( tx, 0 );
        validate_database();
        
        used_mana = old_manabar.current_mana - db->get_account( "alice" ).manabar.current_mana;
        //idump( (used_mana) );
        BOOST_REQUIRE( used_mana == 431000 );
        
        reward_qi = db->get_account("bob").reward_qi_balance - old_reward_qi;
        //idump( (reward_qi) );
        if(i == 0)
            BOOST_REQUIRE( reward_qi == asset(319000, QI_SYMBOL) ); //通胀后给到的奖励基金中的气只有这么多
        else if(i == 1)
            BOOST_REQUIRE( reward_qi == asset(150000, QI_SYMBOL) ); //通胀后给到的奖励基金中的气只有这么多
        else //i == 2
            BOOST_REQUIRE( reward_qi == asset(150000, QI_SYMBOL) ); //通胀后给到的奖励基金中的气只有这么多
    }
    
} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_SUITE_END()
