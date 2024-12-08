#include "cli.hpp"
#include <fc/thread/thread.hpp>

#include <iostream>

#ifndef WIN32
#include <unistd.h>
#endif

#ifdef HAVE_EDITLINE
# include "editline.h"
# include <signal.h>
# ifdef WIN32
#  include <io.h>
# endif
#endif

#include <boost/regex.hpp>

namespace taiyi { namespace danuo {

    using fc::variants;
    using fc::variant;

    static boost::regex& cli_regex_secret()
    {
        static boost::regex regex_expr;
        return regex_expr;
    }
    //=============================================================================
    static std::vector<std::string>& cli_commands()
    {
        static std::vector<std::string>* cmds = new std::vector<std::string>();
        return *cmds;
    }
    //=============================================================================
    cli::~cli()
    {
        if( _run_complete.valid() )
            stop();
    }
    //=============================================================================
    variant cli::send_call( fc::api_id_type api_id, string method_name, variants args /* = variants() */ )
    {
        FC_ASSERT(false);
    }
    //=============================================================================
    variant cli::send_call( string api_name, string method_name, variants args /* = variants() */ )
    {
        FC_ASSERT(false);
    }
    //=============================================================================
    variant cli::send_callback( uint64_t callback_id, variants args /* = variants() */ )
    {
        FC_ASSERT(false);
    }
    //=============================================================================
    void cli::send_notice( uint64_t callback_id, variants args /* = variants() */ )
    {
        FC_ASSERT(false);
    }
    //=============================================================================
    void cli::format_result( const string& method, std::function<string(variant,const variants&)> formatter)
    {
        _result_formatters[method] = formatter;
    }
    //=============================================================================
    void cli::set_prompt( const string& prompt )
    {
        _prompt = prompt;
    }
    //=============================================================================
    void cli::set_regex_secret( const string& expr )
    {
        cli_regex_secret() = expr;
    }
    //=============================================================================
    void cli::run()
    {
        while( !_run_complete.canceled() )
        {
            try
            {
                std::string line;
                try
                {
                    getline( _prompt.c_str(), line );
                }
                catch ( const fc::eof_exception& e )
                {
                    _getline_thread = nullptr;
                    break;
                }
                catch ( const fc::canceled_exception& e )
                {
                    _getline_thread = nullptr;
                    break;
                }
                if(_account != "" && _actor != "") {
                    //in game
                    const std::string cmd_prefix = string("action_actor ") + _account + " \"" + _actor + "\" ";
                    line = cmd_prefix + line;
                    
                    fc::variants args = fc::json::variants_from_string(line);;
                    if( args.size() == 0 )
                        continue;
                    
                    const string& method = args[0].get_string();
                    auto result = receive_call(0, method, variants(args.begin()+1, args.end()));
                    FC_ASSERT(result.is_string(), "MUD result must be string.");
                    
                    std::cout << result.get_string();
                }
                else {
                    //out game
                    line += char(EOF);
                    fc::variants args = fc::json::variants_from_string(line);;
                    if( args.size() == 0 )
                        continue;
                    
                    const string& method = args[0].get_string();
                    
                    auto result = receive_call( 0, method, variants( args.begin()+1,args.end() ) );
                    auto itr = _result_formatters.find( method );
                    if( itr == _result_formatters.end() )
                    {
                        std::cout << fc::json::to_pretty_string( result ) << "\n";
                    }
                    else
                        std::cout << itr->second( result, args ) << "\n";
                }
                
            }
            catch ( const fc::exception& e )
            {
                if (e.code() == fc::canceled_exception_code)
                {
                    _getline_thread = nullptr;
                    break;
                }
                std::cout << e.to_detail_string() << "\n";
            }
        }
    }
    //=============================================================================
#ifdef HAVE_EDITLINE
    
    /****
     * @brief loop through list of commands, attempting to find a match
     * @param token what the user typed
     * @param match sets to 1 if only 1 match was found
     * @returns the remaining letters of the name of the command or NULL if 1 match not found
     */
    static char *my_rl_complete(char *token, int *match)
    {
        const auto& cmds = cli_commands();
        const size_t partlen = strlen (token); /* Part of token */
        
        std::vector<std::reference_wrapper<const std::string>> matched_cmds;
        for( const std::string& it : cmds )
        {
            if( it.compare(0, partlen, token) == 0 )
            {
                matched_cmds.push_back( it );
            }
        }
        
        if( matched_cmds.size() == 0 )
            return NULL;
        
        const std::string& first_matched_cmd = matched_cmds[0];
        if( matched_cmds.size() == 1 )
        {
            *match = 1;
            std::string matched_cmd = first_matched_cmd + " ";
            return strdup( matched_cmd.c_str() + partlen );
        }
        
        size_t first_cmd_len = first_matched_cmd.size();
        size_t matched_len = partlen;
        for( ; matched_len < first_cmd_len; ++matched_len )
        {
            char next_char = first_matched_cmd[matched_len];
            bool end = false;
            for( const std::string& s : matched_cmds )
            {
                if( s.size() <= matched_len || s[matched_len] != next_char )
                {
                    end = true;
                    break;
                }
            }
            if( end )
                break;
        }
        
        if( matched_len == partlen )
            return NULL;
        
        std::string matched_cmd_part = first_matched_cmd.substr( partlen, matched_len - partlen );
        return strdup( matched_cmd_part.c_str() );
    }

    /***
     * @brief return an array of matching commands
     * @param token the incoming text
     * @param array the resultant array of possible matches
     * @returns the number of matches
     */
    static int cli_completion(char *token, char ***array)
    {
        auto& cmd = cli_commands();
        int num_commands = cmd.size();
        
        char **copy = (char **) malloc (num_commands * sizeof(char *));
        if (copy == NULL)
        {
            // possible out of memory
            return 0;
        }
        int total_matches = 0;
        
        const size_t partlen = strlen(token);
        
        for (const std::string& it : cmd)
        {
            if ( it.compare(0, partlen, token) == 0)
            {
                copy[total_matches] = strdup ( it.c_str() );
                ++total_matches;
            }
        }
        *array = copy;
        
        return total_matches;
    }

    /***
     * @brief regex match for secret information
     * @param source the incoming text source
     * @returns integer 1 in event of regex match for secret information, otherwise 0
     */
    static int cli_check_secret(const char *source)
    {
        if (!cli_regex_secret().empty() && boost::regex_match(source, cli_regex_secret()))
            return 1;
        
        return 0;
    }
    
    /***
     * Indicates whether CLI is quitting after got a SIGINT signal.
     * In order to be used by editline which is C-style, this is a global variable.
     */
    static int cli_quitting = false;
    
    /**
     * Get next character from stdin, or EOF if got a SIGINT signal
     */
    static int interruptible_getc(void)
    {
        if( cli_quitting )
            return EOF;
        
        int r;
        char c;
        
        r = read(0, &c, 1); // read from stdin, will return -1 on SIGINT
        
        if( r == -1 && errno == EINTR )
            cli_quitting = true;
        
        return r == 1 && !cli_quitting ? c : EOF;
    }
    
#endif //HAVE_EDITLINE
    //=============================================================================
    void cli::start()
    {
        
#ifdef HAVE_EDITLINE
        el_hist_size = 256;
        
        rl_set_complete_func(my_rl_complete);
        rl_set_list_possib_func(cli_completion);
        //rl_set_check_secret_func(cli_check_secret);
        rl_set_getc_func(interruptible_getc);
        
        static fc::thread getline_thread("getline");
        _getline_thread = &getline_thread;
        
        cli_quitting = false;
        
        cli_commands() = get_method_names(0);
#endif
        
        _run_complete = fc::async( [this](){ run(); } );
    }
    //=============================================================================
    void cli::cancel()
    {
        _run_complete.cancel();
#ifdef HAVE_EDITLINE
        cli_quitting = true;
        if( _getline_thread )
        {
            _getline_thread->signal(SIGINT);
            _getline_thread = nullptr;
        }
#endif
    }
    
    void cli::stop()
    {
        cancel();
        _run_complete.wait();
    }
    //=============================================================================
    void cli::wait()
    {
        _run_complete.wait();
    }
    //=============================================================================
    /***
     * @brief Read input from the user
     * @param prompt the prompt to display
     * @param line what the user typed
     */
    void cli::getline( const std::string& prompt, std::string& line)
    {
        // getting file descriptor for C++ streams is near impossible
        // so we just assume it's the same as the C stream...
#ifdef HAVE_EDITLINE
#ifndef WIN32
        if( isatty( fileno( stdin ) ) )
#else
        // it's implied by
        // https://msdn.microsoft.com/en-us/library/f4s0ddew.aspx
        // that this is the proper way to do this on Windows, but I have
        // no access to a Windows compiler and thus,
        // no idea if this actually works
        if( _isatty( _fileno( stdin ) ) )
#endif
        {
            if( _getline_thread )
            {
                _getline_thread->async( [&prompt,&line](){
                    char* line_read = nullptr;
                    std::cout.flush(); //readline doesn't use cin, so we must manually flush _out
                    line_read = readline(prompt.c_str());
                    if( line_read == nullptr )
                        FC_THROW_EXCEPTION( fc::eof_exception, "" );
                    line = line_read;
                    // we don't need here to add line in editline's history, cause it will be doubled
                    if (cli_check_secret(line_read)) {
                        free(line_read);
                        el_no_echo = 1;
                        line_read = readline("Enter password: ");
                        el_no_echo = 0;
                        if( line_read == nullptr )
                            FC_THROW_EXCEPTION( fc::eof_exception, "" );
                        line = line + ' ' + line_read;
                    }
                    free(line_read);
                }).wait();
            }
        }
        else
#endif
        {
            std::cout << prompt;
            // sync_call( cin_thread, [&](){ std::getline( *input_stream, line ); }, "getline");
            fc::getline( fc::cin, line );
        }
    }

} } // namespace taiyi::danuo
