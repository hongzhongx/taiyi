#pragma once
#include <fc/io/stdio.hpp>
#include <fc/io/json.hpp>
#include <fc/io/buffered_iostream.hpp>
#include <fc/io/sstream.hpp>
#include <fc/rpc/api_connection.hpp>
#include <fc/thread/thread.hpp>

#include <iostream>

namespace taiyi { namespace danuo {

    using fc::variants;
    using fc::variant;
    using std::string;

    class cli : public fc::api_connection
    {
    public:
        ~cli();
        
        virtual variant send_call( fc::api_id_type api_id, string method_name, variants args = variants() );
        virtual variant send_call( string api_name, string method_name, variants args = variants() );
        virtual variant send_callback( uint64_t callback_id, variants args = variants() );
        virtual void    send_notice( uint64_t callback_id, variants args = variants() );
        
        void start();
        void stop();
        void cancel();
        void wait();
        void format_result( const string& method, std::function<string(variant, const variants&)> formatter);

        virtual void getline( const std::string& prompt, std::string& line );
        
        void set_prompt( const string& prompt );
        void set_regex_secret( const string& expr );
        void set_actor( const string& account, const string& actor) { _account = account; _actor = actor; }
        
    private:
        void run();
        
        string _account = "";
        string _actor = "";
        string _prompt = ">>>";
        std::map<string,std::function<string(variant,const variants&)> > _result_formatters;
        fc::future<void> _run_complete;
        fc::thread* _getline_thread = nullptr; ///< Wait for user input in this thread
    };

} } //taiyi::danuo
