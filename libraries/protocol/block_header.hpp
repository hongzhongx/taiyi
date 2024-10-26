#pragma once
#include <protocol/base.hpp>

namespace taiyi { namespace protocol
{
    typedef static_variant<
        void_t,
        version,                // Normal siming version reporting, for diagnostics and voting
        hardfork_version_vote   // Voting for the next hardfork to trigger
    > block_header_extensions;

    typedef flat_set<block_header_extensions> block_header_extensions_type;

    struct block_header
    {
        digest_type                   digest()const;
        block_id_type                 previous;
        uint32_t                      block_num()const { return num_from_id(previous) + 1; }
        fc::time_point_sec            timestamp;
        string                        siming;
        checksum_type                 transaction_merkle_root;
        block_header_extensions_type  extensions;

        static uint32_t num_from_id(const block_id_type& id);
    };

    struct signed_block_header : public block_header
    {
        block_id_type       id()const;
        fc::ecc::public_key signee( fc::ecc::canonical_signature_type canon_type = fc::ecc::bip_0062 )const;
        void                sign( const fc::ecc::private_key& signer, fc::ecc::canonical_signature_type canon_type = fc::ecc::bip_0062 );
        bool                validate_signee( const fc::ecc::public_key& expected_signee, fc::ecc::canonical_signature_type canon_type = fc::ecc::bip_0062 )const;

        signature_type      siming_signature;
    };

} } // taiyi::protocol

FC_REFLECT_TYPENAME( taiyi::protocol::block_header_extensions )

FC_REFLECT( taiyi::protocol::block_header, (previous)(timestamp)(siming)(transaction_merkle_root)(extensions) )
FC_REFLECT_DERIVED( taiyi::protocol::signed_block_header, (taiyi::protocol::block_header), (siming_signature) )
