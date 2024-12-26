#include <protocol/taiyi_operations.hpp>

#include <fc/macros.hpp>
#include <fc/io/json.hpp>
#include <fc/macros.hpp>

#include <locale>

namespace taiyi { namespace protocol {

    void validate_auth_size( const authority& a )
    {
        size_t size = a.account_auths.size() + a.key_auths.size();
        FC_ASSERT( size <= TAIYI_MAX_AUTHORITY_MEMBERSHIP,
                  "Authority membership exceeded. Max: ${max} Current: ${n}", ("max", TAIYI_MAX_AUTHORITY_MEMBERSHIP)("n", size) );
    }
    
    void account_create_operation::validate() const
    {
        validate_account_name( new_account_name );
        FC_ASSERT( is_asset_type( fee, YANG_SYMBOL ), "Account creation fee must be YANG" );
        owner.validate();
        active.validate();
        
        if ( json_metadata.size() > 0 )
        {
            FC_ASSERT( fc::is_utf8(json_metadata), "JSON Metadata not formatted in UTF8" );
            FC_ASSERT( fc::json::is_valid(json_metadata), "JSON Metadata not valid JSON" );
        }
        FC_ASSERT( fee >= asset( 0, YANG_SYMBOL ), "Account creation fee cannot be negative" );
    }
        
    void account_update_operation::validate() const
    {
        validate_account_name( account );
        /*if( owner )
         owner->validate();
         if( active )
         active->validate();
         if( posting )
         posting->validate();*/
        
        if ( json_metadata.size() > 0 )
        {
            FC_ASSERT( fc::is_utf8(json_metadata), "JSON Metadata not formatted in UTF8" );
            FC_ASSERT( fc::json::is_valid(json_metadata), "JSON Metadata not valid JSON" );
        }
    }
        
    void transfer_operation::validate() const
    { try {
        validate_account_name( from );
        validate_account_name( to );
        FC_ASSERT( amount.symbol.is_qi() == false, "Transfer of qi is not allowed." );
        FC_ASSERT( amount.amount > 0, "Cannot transfer a negative amount (aka: stealing)" );
        FC_ASSERT( memo.size() < TAIYI_MAX_MEMO_SIZE, "Memo is too large" );
        FC_ASSERT( fc::is_utf8( memo ), "Memo is not UTF8" );
    } FC_CAPTURE_AND_RETHROW( (*this) ) }
    
    void transfer_to_qi_operation::validate() const
    {
        validate_account_name( from );
        FC_ASSERT( amount.symbol == YANG_SYMBOL,
                  "Amount must be YANG" );
        if ( to != account_name_type() ) validate_account_name( to );
        FC_ASSERT( amount.amount > 0, "Must transfer a nonzero amount" );
    }
    
    void withdraw_qi_operation::validate() const
    {
        validate_account_name( account );
        FC_ASSERT( is_asset_type( qi, QI_SYMBOL), "Amount must be QI"  );
    }
    
    void set_withdraw_qi_route_operation::validate() const
    {
        validate_account_name( from_account );
        validate_account_name( to_account );
        FC_ASSERT( 0 <= percent && percent <= TAIYI_100_PERCENT, "Percent must be valid taiyi percent" );
    }
    
    void siming_update_operation::validate() const
    {
        validate_account_name( owner );
        
        FC_ASSERT( url.size() <= TAIYI_MAX_SIMING_URL_LENGTH, "URL is too long" );
        
        FC_ASSERT( url.size() > 0, "URL size must be greater than 0" );
        FC_ASSERT( fc::is_utf8( url ), "URL is not valid UTF8" );
        FC_ASSERT( fee >= asset( 0, YANG_SYMBOL ), "Fee cannot be negative" );
        props.validate< false >();
    }
    
    void siming_set_properties_operation::validate() const
    {
        validate_account_name( owner );
        
        // current signing key must be present
        FC_ASSERT( props.find( "key" ) != props.end(), "No signing key provided" );
        
        auto itr = props.find( "account_creation_fee" );
        if( itr != props.end() )
        {
            asset account_creation_fee;
            fc::raw::unpack_from_vector( itr->second, account_creation_fee );
            FC_ASSERT( account_creation_fee.symbol == YANG_SYMBOL, "account_creation_fee must be in YANG" );
            FC_ASSERT( account_creation_fee.amount >= TAIYI_MIN_ACCOUNT_CREATION_FEE, "account_creation_fee smaller than minimum account creation fee" );
        }
        
        itr = props.find( "maximum_block_size" );
        if( itr != props.end() )
        {
            uint32_t maximum_block_size;
            fc::raw::unpack_from_vector( itr->second, maximum_block_size );
            FC_ASSERT( maximum_block_size >= TAIYI_MIN_BLOCK_SIZE_LIMIT, "maximum_block_size smaller than minimum max block size" );
        }
        
        itr = props.find( "new_signing_key" );
        if( itr != props.end() )
        {
            public_key_type signing_key;
            fc::raw::unpack_from_vector( itr->second, signing_key );
            FC_UNUSED( signing_key ); // This tests the deserialization of the key
        }
        
        itr = props.find( "url" );
        if( itr != props.end() )
        {
            std::string url;
            fc::raw::unpack_from_vector< std::string >( itr->second, url );
            
            FC_ASSERT( url.size() <= TAIYI_MAX_SIMING_URL_LENGTH, "URL is too long" );
            FC_ASSERT( url.size() > 0, "URL size must be greater than 0" );
            FC_ASSERT( fc::is_utf8( url ), "URL is not valid UTF8" );
        }
    }
    
    void account_siming_adore_operation::validate() const
    {
        validate_account_name( account );
        validate_account_name( siming );
    }
    
    void account_siming_proxy_operation::validate() const
    {
        validate_account_name( account );
        if( proxy.size() )
            validate_account_name( proxy );
        FC_ASSERT( proxy != account, "Cannot proxy to self" );
    }
    
    void custom_operation::validate() const {
        /// required auth accounts are the ones whose bandwidth is consumed
        FC_ASSERT( required_auths.size() > 0, "at least one account must be specified" );
    }

    void custom_json_operation::validate() const {
        /// required auth accounts are the ones whose bandwidth is consumed
        FC_ASSERT( (required_auths.size() + required_posting_auths.size()) > 0, "at least one account must be specified" );
        FC_ASSERT( id.size() <= TAIYI_CUSTOM_OP_ID_MAX_LENGTH,
                  "Operation ID length exceeded. Max: ${max} Current: ${n}", ("max", TAIYI_CUSTOM_OP_ID_MAX_LENGTH)("n", id.size()) );
        FC_ASSERT( fc::is_utf8(json), "JSON Metadata not formatted in UTF8" );
        FC_ASSERT( fc::json::is_valid(json), "JSON Metadata not valid JSON" );
    }
            
    void request_account_recovery_operation::validate()const
    {
        validate_account_name( recovery_account );
        validate_account_name( account_to_recover );
        new_owner_authority.validate();
    }
    
    void recover_account_operation::validate()const
    {
        validate_account_name( account_to_recover );
        FC_ASSERT( !( new_owner_authority == recent_owner_authority ), "Cannot set new owner authority to the recent owner authority" );
        FC_ASSERT( !new_owner_authority.is_impossible(), "new owner authority cannot be impossible" );
        FC_ASSERT( !recent_owner_authority.is_impossible(), "recent owner authority cannot be impossible" );
        FC_ASSERT( new_owner_authority.weight_threshold, "new owner authority cannot be trivial" );
        new_owner_authority.validate();
        recent_owner_authority.validate();
    }
    
    void change_recovery_account_operation::validate()const
    {
        validate_account_name( account_to_recover );
        validate_account_name( new_recovery_account );
    }
    
    void decline_adoring_rights_operation::validate()const
    {
        validate_account_name( account );
    }
        
    void claim_reward_balance_operation::validate()const
    {
        validate_account_name( account );
        FC_ASSERT( is_asset_type( reward_yang, YANG_SYMBOL ), "Reward yang must be YANG" );
        FC_ASSERT( is_asset_type( reward_qi, QI_SYMBOL ), "Reward qi must be QI" );
        FC_ASSERT( is_asset_type( reward_feigang, QI_SYMBOL ), "Reward feigang must be QI" );
        
        FC_ASSERT( reward_yang.amount >= 0, "Cannot claim a negative yang amount" );
        FC_ASSERT( reward_qi.amount >= 0, "Cannot claim a negative qi amount" );
        FC_ASSERT( reward_feigang.amount >= 0, "Cannot claim a negative feigang amount" );
        
        FC_ASSERT( reward_yang.amount > 0 || reward_qi.amount > 0 || reward_feigang.amount > 0, "Must claim something." );
    }
        
    void delegate_qi_operation::validate()const
    {
        validate_account_name( delegator );
        validate_account_name( delegatee );
        FC_ASSERT( delegator != delegatee, "You cannot delegate QI to yourself" );
        FC_ASSERT( is_asset_type( qi, QI_SYMBOL ), "Delegation must be QI" );
        FC_ASSERT( qi >= asset( 0, QI_SYMBOL ), "Delegation cannot be negative" );
    }
    
    void create_contract_operation::validate() const
    {
        validate_account_name( owner );
        
        FC_ASSERT(memcmp(name.data(), "contract.", 9) == 0);
        FC_ASSERT( is_valid_contract_name( name ), "contract name ${n} is invalid", ("n", name) );
    }
    
    void call_contract_function_operation::validate() const
    {
        validate_account_name( caller );
        
        FC_ASSERT(memcmp(contract_name.data(), "contract.", 9) == 0);
        FC_ASSERT( is_valid_contract_name( contract_name ), "contract name ${n} is invalid", ("n", contract_name) );
    }
    
    void revise_contract_operation::validate() const
    {
        validate_account_name( reviser );
        
        FC_ASSERT(memcmp(contract_name.data(), "contract.", 9) == 0);
        FC_ASSERT( is_valid_contract_name( contract_name ), "contract name ${n} is invalid", ("n", contract_name) );
    }    
    
    void create_nfa_symbol_operation::validate() const
    {
        validate_account_name( creator );
                
        FC_ASSERT(memcmp(symbol.data(), "nfa.", 4) == 0);
        FC_ASSERT( is_valid_nfa_symbol( symbol ), "symbol ${q} is invalid", ("n", symbol) );
        
        FC_ASSERT( describe.size() > 0, "describe is empty" );
        FC_ASSERT( fc::is_utf8( describe ), "describe not formatted in UTF8" );

        FC_ASSERT(memcmp(default_contract.data(), "contract.", 9) == 0);
        FC_ASSERT( is_valid_contract_name( default_contract ), "contract name ${n} is invalid", ("n", default_contract) );
    }
    
    void create_nfa_operation::validate() const
    {
        validate_account_name( creator );
                
        FC_ASSERT(memcmp(symbol.data(), "nfa.", 4) == 0);
        FC_ASSERT( is_valid_nfa_symbol( symbol ), "symbol ${q} is invalid", ("n", symbol) );
    }
    
    void transfer_nfa_operation::validate() const
    {
        validate_account_name( from );
        validate_account_name( to );
        
        FC_ASSERT( from != to, "no need transfer NFA to self." );
    }

    void approve_nfa_active_operation::validate() const
    {
        validate_account_name( owner );
        validate_account_name( active_account );        
    }

    void action_nfa_operation::validate() const
    {
        validate_account_name( caller );
    }

    void create_actor_talent_rule_operation::validate() const
    {
        validate_account_name( creator );
        
        FC_ASSERT(memcmp(contract.data(), "contract.", 9) == 0);
        FC_ASSERT( is_valid_contract_name( contract ), "contract name ${n} is invalid", ("n", contract) );
    }

    void create_actor_operation::validate() const
    {
        validate_account_name( creator );

        FC_ASSERT( is_asset_type( fee, QI_SYMBOL ), "Actor creation fee must be QI" );
        FC_ASSERT( fee >= asset( 0, QI_SYMBOL ), "Actor creation fee cannot be negative" );
        
        FC_ASSERT( family_name.size() > 0, "Family name is empty" );
        FC_ASSERT( family_name.size() < TAIYI_ACTOR_NAME_LIMIT,
                  "Family name size limit exceeded. Max: ${max} Current: ${n}", ("max", TAIYI_ACTOR_NAME_LIMIT - 1)("n", family_name.size()) );
        FC_ASSERT( fc::is_utf8( family_name ), "Family name not formatted in UTF8" );
        
        FC_ASSERT( last_name.size() > 0, "Last name is empty" );
        FC_ASSERT( last_name.size() < TAIYI_ACTOR_NAME_LIMIT,
                  "Last name size limit exceeded. Max: ${max} Current: ${n}", ("max", TAIYI_ACTOR_NAME_LIMIT - 1)("n", last_name.size()) );
        FC_ASSERT( fc::is_utf8( last_name ), "Last name not formatted in UTF8" );
    }

    void create_zone_operation::validate() const
    {
        validate_account_name( creator );
        
        FC_ASSERT( is_asset_type( fee, QI_SYMBOL ), "Zone creation fee must be QI" );
        FC_ASSERT( fee >= asset( 0, QI_SYMBOL ), "Zone creation fee cannot be negative" );

        FC_ASSERT( name.size() > 0, "Name is empty" );
        FC_ASSERT( name.size() < TAIYI_ZONE_NAME_LIMIT,
                  "Name size limit exceeded. Max: ${max} Current: ${n}", ("max", TAIYI_ZONE_NAME_LIMIT - 1)("n", name.size()) );
        FC_ASSERT( fc::is_utf8( name ), "Name not formatted in UTF8" );        
    }    

} } // taiyi::protocol
