#pragma once
#include <chain/taiyi_fwd.hpp>

#include <protocol/authority.hpp>
#include <protocol/taiyi_operations.hpp>

#include <chain/taiyi_object_types.hpp>

namespace taiyi { namespace chain {
    
    using chainbase::t_flat_map;

    class cultivation_object : public object < cultivation_object_type, cultivation_object >
    {
        TAIYI_STD_ALLOCATOR_CONSTRUCTOR(cultivation_object)

    public:
        template< typename Constructor, typename Allocator >
        cultivation_object(Constructor&& c, allocator< Allocator > a)
            : beneficiary_map(a), participants(a)
        {
            c(*this);
        }

        id_type                         id;

        nfa_id_type                     manager_nfa_id = nfa_id_type::max(); ///修真的启动者和管理者
        t_flat_map<nfa_id_type, uint>   beneficiary_map; ///修真受益人以及对应的分配比例，比例总合必须等于 TAIYI_100_PERCENT
        std::set<nfa_id_type>           participants; /// 修真参与者

        time_point_sec                  create_time = time_point_sec::maximum();
        time_point_sec                  start_deadline = time_point_sec::maximum(); //如果在截止时间之前还未开始，则自动释放修真对象
        time_point_sec                  start_time = time_point_sec::maximum();
    };

    struct by_start_deadline;
    struct by_start_time;
    struct by_manager_nfa;
    typedef multi_index_container<
        cultivation_object,
        indexed_by<
            ordered_unique< tag< by_id >, member< cultivation_object, cultivation_id_type, &cultivation_object::id > >,
            ordered_unique< tag< by_manager_nfa >,
                composite_key< cultivation_object,
                    member< cultivation_object, nfa_id_type, &cultivation_object::manager_nfa_id >,
                    member< cultivation_object, cultivation_id_type, &cultivation_object::id >
                >
            >,
            ordered_unique< tag< by_start_deadline >,
                composite_key< cultivation_object,
                    member< cultivation_object, time_point_sec, &cultivation_object::start_deadline >,
                    member< cultivation_object, cultivation_id_type, &cultivation_object::id >
                >
            >,
            ordered_unique< tag< by_start_time >,
                composite_key< cultivation_object,
                    member< cultivation_object, time_point_sec, &cultivation_object::start_time >,
                    member< cultivation_object, cultivation_id_type, &cultivation_object::id >
                >
            >
        >,
        allocator< cultivation_object >
    > cultivation_index;

} } // taiyi::chain

namespace mira {
} // mira

FC_REFLECT( taiyi::chain::cultivation_object, (id)(manager_nfa_id)(beneficiary_map)(participants)(create_time)(start_deadline)(start_time) )
CHAINBASE_SET_INDEX_TYPE(taiyi::chain::cultivation_object, taiyi::chain::cultivation_index)
