#pragma once
#include <fc/array.hpp>
#include <fc/io/varint.hpp>
#include <fc/network/ip.hpp>
#include <fc/io/raw.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/reflect/variant.hpp>

namespace taiyi { namespace net {

    /**
     *  Defines an 8 byte header that is always present because the minimum encrypted packet
     *  size is 8 bytes (blowfish).  The maximum message size is defined in config.hpp. The channel,
     *  and message type is also included because almost every channel will have a message type
     *  field and we might as well include it in the 8 byte header to save space.
     */
    struct message_header
    {
     	uint32_t  size = 0;   // number of bytes in message, capped at MAX_MESSAGE_SIZE
     	uint32_t  msg_type = 0;  // every channel gets a 16 bit message type specifier
    };

    typedef fc::uint160_t message_hash_type;

    /**
     *  Abstracts the process of packing/unpacking a message for a
     *  particular channel.
     */
    struct message : public message_header
    {
        std::vector<char> data;

        message(){}

        message(message&& m)
            :message_header(m), data(std::move(m.data)){}

        message(const message& m)
            :message_header(m), data(m.data){}

        /**
         *  Assumes that T::type specifies the message type
         */
        template<typename T>
        message(const T& m) {
            msg_type = T::type;
            data = fc::raw::pack_to_vector(m);
            size = (uint32_t)data.size();
        }

        fc::uint160_t id()const {
            return fc::ripemd160::hash(data.data(), (uint32_t)data.size());
        }

        /**
         *  Automatically checks the type and deserializes T in the
         *  opposite process from the constructor.
         */
        template<typename T>
        T as()const
        {
            try {
                FC_ASSERT(msg_type == T::type);
                T tmp;
                if (data.size()) {
                    fc::datastream<const char*> ds(data.data(), data.size());
                    fc::raw::unpack(ds, tmp);
                }
                else {
                    // just to make sure that tmp shouldn't have any data
                    fc::datastream<const char*> ds(nullptr, 0);
                    fc::raw::unpack(ds, tmp);
                }
                return tmp;
            } FC_RETHROW_EXCEPTIONS(warn,
                "error unpacking network message as a '${type}'  ${x} !=? ${msg_type}",
                ("type", fc::get_typename<T>::name())
                ("x", T::type)
                ("msg_type", msg_type)
                );
        }
    };

} } // taiyi::net

FC_REFLECT( taiyi::net::message_header, (size)(msg_type) )
FC_REFLECT_DERIVED( taiyi::net::message, (taiyi::net::message_header), (data) )
