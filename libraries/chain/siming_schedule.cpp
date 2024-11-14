
#include <chain/taiyi_fwd.hpp>
#include <chain/database.hpp>
#include <chain/siming_objects.hpp>
#include <chain/siming_schedule.hpp>

#include <protocol/config.hpp>

namespace taiyi { namespace chain {

    void reset_virtual_schedule_time( database& db )
    {
        const siming_schedule_object& wso = db.get_siming_schedule_object();
        db.modify( wso, [&](siming_schedule_object& o ) {
            o.current_virtual_time = fc::uint128(); // reset it 0
        } );
        
        const auto& idx = db.get_index<siming_index>().indices();
        for( const auto& siming : idx )
        {
            db.modify( siming, [&]( siming_object& wobj ) {
                wobj.virtual_position = fc::uint128();
                wobj.virtual_last_update = wso.current_virtual_time;
                wobj.virtual_scheduled_time = TAIYI_VIRTUAL_SCHEDULE_LAP_LENGTH / (wobj.adores.value+1);
            } );
        }
    }

    void update_median_siming_props( database& db )
    {
        const siming_schedule_object& wso = db.get_siming_schedule_object();
        
        // fetch all siming objects
        vector<const siming_object*> active; active.reserve( wso.num_scheduled_simings );
        for( int i = 0; i < wso.num_scheduled_simings; i++ )
            active.push_back( &db.get_siming( wso.current_shuffled_simings[i] ) );
        
        // sort them by account_creation_fee
        std::sort( active.begin(), active.end(), [&]( const siming_object* a, const siming_object* b ) {
            return a->props.account_creation_fee.amount < b->props.account_creation_fee.amount;
        } );
        asset median_account_creation_fee = active[active.size()/2]->props.account_creation_fee;
        
        // sort them by maximum_block_size
        std::sort( active.begin(), active.end(), [&]( const siming_object* a, const siming_object* b ) {
            return a->props.maximum_block_size < b->props.maximum_block_size;
        } );
        uint32_t median_maximum_block_size = active[active.size()/2]->props.maximum_block_size;
                        
        db.modify( wso, [&]( siming_schedule_object& _wso ) {
            _wso.median_props.account_creation_fee       = median_account_creation_fee;
            _wso.median_props.maximum_block_size         = median_maximum_block_size;
        } );
        
        db.modify( db.get_dynamic_global_properties(), [&]( dynamic_global_property_object& _dgpo ) {
            _dgpo.maximum_block_size = median_maximum_block_size;
        } );
    }

    void update_siming_schedule4( database& db )
    {
        const siming_schedule_object& wso = db.get_siming_schedule_object();
        vector< account_name_type > active_simings;
        active_simings.reserve( TAIYI_MAX_SIMINGS );
        
        // Add the highest adored simings
        flat_set< siming_id_type > selected_adored;
        selected_adored.reserve( wso.max_adored_simings );
        
        const auto& widx = db.get_index<siming_index>().indices().get<by_adore_name>();
        for( auto itr = widx.begin(); itr != widx.end() && selected_adored.size() < wso.max_adored_simings; ++itr )
        {
            if( itr->signing_key == public_key_type() )
                continue;
            selected_adored.insert( itr->id );
            active_simings.push_back( itr->owner) ;
            db.modify( *itr, [&]( siming_object& wo ) { wo.schedule = siming_object::elected; } );
        }
        
        auto num_elected = active_simings.size();
        
        // Add the running simings in the lead
        fc::uint128 new_virtual_time = wso.current_virtual_time;
        const auto& schedule_idx = db.get_index<siming_index>().indices().get<by_schedule_time>();
        auto sitr = schedule_idx.begin();
        vector<decltype(sitr)> processed_simings;
        for( auto siming_count = selected_adored.size(); sitr != schedule_idx.end() && siming_count < TAIYI_MAX_SIMINGS; ++sitr )
        {
            new_virtual_time = sitr->virtual_scheduled_time; // everyone advances to at least this time
            processed_simings.push_back(sitr);
            
            if( sitr->signing_key == public_key_type() )
                continue; // skip simings without a valid block signing key
            
            if( selected_adored.find(sitr->id) == selected_adored.end() )
            {
                active_simings.push_back(sitr->owner);
                db.modify( *sitr, [&]( siming_object& wo ) { wo.schedule = siming_object::timeshare; } );
                ++siming_count;
            }
        }
        
        auto num_timeshare = active_simings.size() - num_elected;
        
        // Update virtual schedule of processed simings
        bool reset_virtual_time = false;
        for( auto itr = processed_simings.begin(); itr != processed_simings.end(); ++itr )
        {
            auto new_virtual_scheduled_time = new_virtual_time + TAIYI_VIRTUAL_SCHEDULE_LAP_LENGTH / ((*itr)->adores.value+1);
            if( new_virtual_scheduled_time < new_virtual_time )
            {
                reset_virtual_time = true; /// overflow
                break;
            }
            db.modify( *(*itr), [&]( siming_object& wo ) {
                wo.virtual_position        = fc::uint128();
                wo.virtual_last_update     = new_virtual_time;
                wo.virtual_scheduled_time  = new_virtual_scheduled_time;
            } );
        }
        
        if( reset_virtual_time )
        {
            new_virtual_time = fc::uint128();
            reset_virtual_schedule_time(db);
        }
        
        size_t expected_active_simings = std::min( size_t(TAIYI_MAX_SIMINGS), widx.size() );
        FC_ASSERT( active_simings.size() == expected_active_simings, "number of active simings does not equal expected_active_simings=${expected_active_simings}", ("active_simings.size()",active_simings.size()) ("TAIYI_MAX_SIMINGS",TAIYI_MAX_SIMINGS) ("expected_active_simings", expected_active_simings) );
        
        auto majority_version = wso.majority_version;
        
        flat_map< version, uint32_t, std::greater< version > > siming_versions;
        flat_map< std::tuple< hardfork_version, time_point_sec >, uint32_t > hardfork_version_votes;
        
        for( uint32_t i = 0; i < wso.num_scheduled_simings; i++ )
        {
            auto siming = db.get_siming( wso.current_shuffled_simings[ i ] );
            if( siming_versions.find( siming.running_version ) == siming_versions.end() )
                siming_versions[ siming.running_version ] = 1;
            else
                siming_versions[ siming.running_version ] += 1;
            
            auto version_vote = std::make_tuple( siming.hardfork_version_vote, siming.hardfork_time_vote );
            if( hardfork_version_votes.find( version_vote ) == hardfork_version_votes.end() )
                hardfork_version_votes[ version_vote ] = 1;
            else
                hardfork_version_votes[ version_vote ] += 1;
        }
        
        int simings_on_version = 0;
        auto ver_itr = siming_versions.begin();
        
        // The map should be sorted highest version to smallest, so we iterate until we hit the majority of simings on at least this version
        while( ver_itr != siming_versions.end() )
        {
            simings_on_version += ver_itr->second;
            if( simings_on_version >= wso.hardfork_required_simings )
            {
                majority_version = ver_itr->first;
                break;
            }
            
            ++ver_itr;
        }
        
        auto hf_itr = hardfork_version_votes.begin();
        while( hf_itr != hardfork_version_votes.end() )
        {
            if( hf_itr->second >= wso.hardfork_required_simings )
            {
                const auto& hfp = db.get_hardfork_property_object();
                if( hfp.next_hardfork != std::get<0>( hf_itr->first ) || hfp.next_hardfork_time != std::get<1>( hf_itr->first ) ) {
                    db.modify( hfp, [&]( hardfork_property_object& hpo ) {
                        hpo.next_hardfork = std::get<0>( hf_itr->first );
                        hpo.next_hardfork_time = std::get<1>( hf_itr->first );
                    } );
                }
                break;
            }
            
            ++hf_itr;
        }
        
        // We no longer have a majority
        if( hf_itr == hardfork_version_votes.end() )
        {
            db.modify( db.get_hardfork_property_object(), [&]( hardfork_property_object& hpo ) {
                hpo.next_hardfork = hpo.current_hardfork_version;
            });
        }
        
        assert( num_elected + num_timeshare == active_simings.size() );
        
        db.modify( wso, [&]( siming_schedule_object& _wso ) {
            for( size_t i = 0; i < active_simings.size(); i++ )
            {
                _wso.current_shuffled_simings[i] = active_simings[i];
            }
            
            for( size_t i = active_simings.size(); i < TAIYI_MAX_SIMINGS; i++ )
            {
                _wso.current_shuffled_simings[i] = account_name_type();
            }
            
            _wso.num_scheduled_simings = std::max< uint8_t >( active_simings.size(), 1 );
            _wso.siming_pay_normalization_factor = _wso.elected_weight * num_elected + _wso.timeshare_weight * num_timeshare;
            
            // shuffle current shuffled simings
            auto now_hi = uint64_t(db.head_block_time().sec_since_epoch()) << 32;
            for( uint32_t i = 0; i < _wso.num_scheduled_simings; ++i )
            {
                // High performance random generator
                // http://xorshift.di.unimi.it/
                uint64_t k = now_hi + uint64_t(i)*2685821657736338717ULL;
                k ^= (k >> 12);
                k ^= (k << 25);
                k ^= (k >> 27);
                k *= 2685821657736338717ULL;
                
                uint32_t jmax = _wso.num_scheduled_simings - i;
                uint32_t j = i + k%jmax;
                std::swap( _wso.current_shuffled_simings[i], _wso.current_shuffled_simings[j] );
            }
            
            _wso.current_virtual_time = new_virtual_time;
            _wso.next_shuffle_block_num = db.head_block_num() + _wso.num_scheduled_simings;
            _wso.majority_version = majority_version;
        } );
        
        update_median_siming_props(db);
    }

    /**
     *
     *  See @ref siming_object::virtual_last_update
     */
    void update_siming_schedule(database& db)
    {
        if( (db.head_block_num() % TAIYI_MAX_SIMINGS) == 0 ) //wso.next_shuffle_block_num )
        {
            update_siming_schedule4(db);
            return;
        }
    }

} } //taiyi::chain
