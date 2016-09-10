/*
dependencies:
    pvt.cppan.demo.intel.tbb: 4
*/

/*
    Copyright 2005-2016 Intel Corporation.  All Rights Reserved.

    This file is part of Threading Building Blocks. Threading Building Blocks is free software;
    you can redistribute it and/or modify it under the terms of the GNU General Public License
    version 2  as  published  by  the  Free Software Foundation.  Threading Building Blocks is
    distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See  the GNU General Public License for more details.   You should have received a copy of
    the  GNU General Public License along with Threading Building Blocks; if not, write to the
    Free Software Foundation, Inc.,  51 Franklin St,  Fifth Floor,  Boston,  MA 02110-1301 USA

    As a special exception,  you may use this file  as part of a free software library without
    restriction.  Specifically,  if other files instantiate templates  or use macros or inline
    functions from this file, or you compile this file and link it with other files to produce
    an executable,  this file does not by itself cause the resulting executable to be covered
    by the GNU General Public License. This exception does not however invalidate any other
    reasons why the executable file might be covered by the GNU General Public License.
*/

// Workaround for ICC 11.0 not finding __sync_fetch_and_add_4 on some of the Linux platforms.
#if __linux__ && defined(__INTEL_COMPILER)
#define __sync_fetch_and_add(ptr,addend) _InterlockedExchangeAdd(const_cast<void*>(reinterpret_cast<volatile void*>(ptr)), addend)
#endif
#include <string>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include "tbb/concurrent_hash_map.h"
#include "tbb/blocked_range.h"
#include "tbb/parallel_for.h"
#include "tbb/tick_count.h"
#include "tbb/task_scheduler_init.h"
#include "tbb/tbb_allocator.h"


/*
    Copyright 2005-2016 Intel Corporation.  All Rights Reserved.

    This file is part of Threading Building Blocks. Threading Building Blocks is free software;
    you can redistribute it and/or modify it under the terms of the GNU General Public License
    version 2  as  published  by  the  Free Software Foundation.  Threading Building Blocks is
    distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See  the GNU General Public License for more details.   You should have received a copy of
    the  GNU General Public License along with Threading Building Blocks; if not, write to the
    Free Software Foundation, Inc.,  51 Franklin St,  Fifth Floor,  Boston,  MA 02110-1301 USA

    As a special exception,  you may use this file  as part of a free software library without
    restriction.  Specifically,  if other files instantiate templates  or use macros or inline
    functions from this file, or you compile this file and link it with other files to produce
    an executable,  this file does not by itself cause the resulting executable to be covered
    by the GNU General Public License. This exception does not however invalidate any other
    reasons why the executable file might be covered by the GNU General Public License.
*/

#ifndef UTILITY_H_
#define UTILITY_H_

#if __TBB_MIC_OFFLOAD
#pragma offload_attribute (push,target(mic))
#include <exception>
#include <cstdio>
#pragma offload_attribute (pop)
#endif // __TBB_MIC_OFFLOAD

#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>
#include <numeric>
#include <stdexcept>
#include <memory>
#include <cassert>
#include <iostream>
#include <cstdlib>
// TBB headers should not be used, as some examples may need to be built without TBB.

namespace utility{
    namespace internal{

#if ((__GNUC__*100+__GNUC_MINOR__>=404 && __GXX_EXPERIMENTAL_CXX0X__) || _MSC_VER >= 1600) && (!__INTEL_COMPILER || __INTEL_COMPILER >= 1200 )
    // std::unique_ptr is available, and compiler can use it
    #define smart_ptr std::unique_ptr
    using std::swap;
#else
    #if __INTEL_COMPILER && __GXX_EXPERIMENTAL_CXX0X__
    // std::unique_ptr is unavailable, so suppress std::auto_prt<> deprecation warning
    #pragma warning(disable: 1478)
    #endif
    #define smart_ptr std::auto_ptr
    // in some C++ libraries, std::swap does not work with std::auto_ptr
    template<typename T>
    void swap( std::auto_ptr<T>& ptr1, std::auto_ptr<T>& ptr2 ) {
        std::auto_ptr<T> tmp; tmp = ptr2; ptr2 = ptr1; ptr1 = tmp;
    }
#endif

        //TODO: add tcs
        template<class dest_type>
        dest_type& string_to(std::string const& s, dest_type& result){
            std::stringstream stream(s);
            stream>>result;
            if ((!stream)||(stream.fail())){
                throw std::invalid_argument("error converting string '"+std::string(s)+"'");
            }
            return result;
        }

        template<class dest_type>
        dest_type string_to(std::string const& s){
            dest_type result;
            return string_to(s,result);
        }

        template<typename>
        struct is_bool          { static bool value(){return false;}};
        template<>
        struct is_bool<bool>    { static bool value(){return true;}};

        class type_base {
            type_base& operator=(const type_base&);
            public:
            const std::string name;
            const std::string description;

            type_base (std::string a_name, std::string a_description) : name(a_name), description(a_description) {}
            virtual void parse_and_store (const std::string & s)=0;
            virtual std::string value() const =0;
            virtual smart_ptr<type_base> clone()const =0;
            virtual ~type_base(){}
        };
        template <typename type>
        class type_impl : public type_base {
        private:
            type_impl& operator=(const type_impl&);
            typedef bool(*validating_function_type)(const type&);
        private:
            type & target;
            validating_function_type validating_function;
        public:
            type_impl(std::string a_name, std::string a_description, type & a_target, validating_function_type a_validating_function = NULL)
                : type_base (a_name,a_description), target(a_target),validating_function(a_validating_function)
            {};
            void parse_and_store (const std::string & s){
                try{
                    const bool is_bool = internal::is_bool<type>::value();
                    if (is_bool && s.empty()){
                        //to avoid directly assigning true
                        //(as it will impose additional layer of indirection)
                        //so, simply pass it as string
                        internal::string_to("1",target);
                    }else {
                        internal::string_to(s,target);
                    }
                }catch(std::invalid_argument& e){
                    std::stringstream str;
                    str <<"'"<<s<<"' is incorrect input for argument '"<<name<<"'"
                        <<" ("<<e.what()<<")";
                    throw std::invalid_argument(str.str());
                }
                if (validating_function){
                    if (!((validating_function)(target))){
                        std::stringstream str;
                        str <<"'"<<target<<"' is invalid value for argument '"<<name<<"'";
                        throw std::invalid_argument(str.str());
                    }
                }
            }
            template <typename t>
            static bool is_null_c_str(t&){return false;}
            static bool is_null_c_str(char* s){return s==NULL;}
            virtual std::string value()const{
                std::stringstream str;
                if (!is_null_c_str(target))
                    str<<target;
                return str.str();
            }
            virtual smart_ptr<type_base> clone() const {
                return smart_ptr<type_base>(new type_impl(*this));
            }
        };

        class argument{
        private:
            smart_ptr<type_base> p_type;
            bool matched_;
        public:
            argument(argument const& other)
                : p_type(other.p_type.get() ? (other.p_type->clone()).release() : NULL)
                 ,matched_(other.matched_)
            {}
            argument& operator=(argument a){
                this->swap(a);
                return *this;
            }
            void swap(argument& other){
                internal::swap(p_type, other.p_type);
                std::swap(matched_,other.matched_);
            }
            template<class type>
            argument(std::string a_name, std::string a_description, type& dest, bool(*a_validating_function)(const type&)= NULL)
                :p_type(new type_impl<type>(a_name,a_description,dest,a_validating_function))
                 ,matched_(false)
            {}
            std::string value()const{
                return p_type->value();
            }
            std::string name()const{
                return p_type->name;
            }
            std::string description() const{
                return p_type->description;
            }
            void parse_and_store(const std::string & s){
                p_type->parse_and_store(s);
                matched_=true;
            }
            bool is_matched() const{return matched_;}
        };
    } // namespace internal

    class cli_argument_pack{
        typedef std::map<std::string,internal::argument> args_map_type;
        typedef std::vector<std::string> args_display_order_type;
        typedef std::vector<std::string> positional_arg_names_type;
    private:
        args_map_type args_map;
        args_display_order_type args_display_order;
        positional_arg_names_type positional_arg_names;
        std::set<std::string> bool_args_names;
    private:
        void add_arg(internal::argument const& a){
            std::pair<args_map_type::iterator, bool> result = args_map.insert(std::make_pair(a.name(),a));
            if (!result.second){
                throw std::invalid_argument("argument with name: '"+a.name()+"' already registered");
            }
            args_display_order.push_back(a.name());
        }
    public:
        template<typename type>
        cli_argument_pack& arg(type& dest,std::string const& name, std::string const& description, bool(*validate)(const type &)= NULL){
            internal::argument a(name,description,dest,validate);
            add_arg(a);
            if (internal::is_bool<type>::value()){
                bool_args_names.insert(name);
            }
            return *this;
        }

        //Positional means that argument name can be omitted in actual CL
        //only key to match values for parameters with
        template<typename type>
        cli_argument_pack& positional_arg(type& dest,std::string const& name, std::string const& description, bool(*validate)(const type &)= NULL){
            internal::argument a(name,description,dest,validate);
            add_arg(a);
            if (internal::is_bool<type>::value()){
                bool_args_names.insert(name);
            }
            positional_arg_names.push_back(name);
            return *this;
        }

        void parse(std::size_t argc, char const* argv[]){
            {
                std::size_t current_positional_index=0;
                for (std::size_t j=1;j<argc;j++){
                    internal::argument* pa = NULL;
                    std::string argument_value;

                    const char * const begin=argv[j];
                    const char * const end=begin+std::strlen(argv[j]);

                    const char * const assign_sign = std::find(begin,end,'=');

                    struct throw_unknown_parameter{ static void _(std::string const& location){
                        throw std::invalid_argument(std::string("unknown parameter starting at:'")+location+"'");
                    }};
                    //first try to interpret it like parameter=value string
                    if (assign_sign!=end){
                        std::string name_found = std::string(begin,assign_sign);
                        args_map_type::iterator it = args_map.find(name_found );

                        if(it!=args_map.end()){
                            pa= &((*it).second);
                            argument_value = std::string(assign_sign+1,end);
                        }else {
                            throw_unknown_parameter::_(argv[j]);
                        }
                    }
                    //then see is it a named flag
                    else{
                        args_map_type::iterator it = args_map.find(argv[j] );
                        if(it!=args_map.end()){
                            pa= &((*it).second);
                            argument_value = "";
                        }
                        //then try it as positional argument without name specified
                        else if (current_positional_index < positional_arg_names.size()){
                            std::stringstream str(argv[j]);
                            args_map_type::iterator found_positional_arg = args_map.find(positional_arg_names.at(current_positional_index));
                            //TODO: probably use of smarter assert would help here
                            assert(found_positional_arg!=args_map.end()/*&&"positional_arg_names and args_map are out of sync"*/);
                            if (found_positional_arg==args_map.end()){
                                throw std::logic_error("positional_arg_names and args_map are out of sync");
                            }
                            pa= &((*found_positional_arg).second);
                            argument_value = argv[j];

                            current_positional_index++;
                        }else {
                            //TODO: add tc to check
                            throw_unknown_parameter::_(argv[j]);
                        }
                    }
                    assert(pa);
                    if (pa->is_matched()){
                        throw std::invalid_argument(std::string("several values specified for: '")+pa->name()+"' argument");
                    }
                    pa->parse_and_store(argument_value);
                }
            }
        }
        std::string usage_string(const std::string& binary_name)const{
            std::string command_line_params;
            std::string summary_description;

            for (args_display_order_type::const_iterator it = args_display_order.begin();it!=args_display_order.end();++it){
                const bool is_bool = (0!=bool_args_names.count((*it)));
                args_map_type::const_iterator argument_it = args_map.find(*it);
                //TODO: probably use of smarter assert would help here
                assert(argument_it!=args_map.end()/*&&"args_display_order and args_map are out of sync"*/);
                if (argument_it==args_map.end()){
                    throw std::logic_error("args_display_order and args_map are out of sync");
                }
                const internal::argument & a = (*argument_it).second;
                command_line_params +=" [" + a.name() + (is_bool ?"":"=value")+ "]";
                summary_description +=" " + a.name() + " - " + a.description() +" ("+a.value() +")" + "\n";
            }

            std::string positional_arg_cl;
            for (positional_arg_names_type::const_iterator it = positional_arg_names.begin();it!=positional_arg_names.end();++it){
                positional_arg_cl +=" ["+(*it);
            }
            for (std::size_t i=0;i<positional_arg_names.size();++i){
                positional_arg_cl+="]";
            }
            command_line_params+=positional_arg_cl;
            std::stringstream str;
            using std::endl;
            str << " Program usage is:" << endl
                 << " " << binary_name << command_line_params
                 << endl << endl
                 << " where:" << endl
                 << summary_description
            ;
            return str.str();
        }
    }; // class cli_argument_pack

    namespace internal {
        template<typename T>
        bool is_power_of_2( T val ) {
            size_t intval = size_t(val);
            return (intval&(intval-1)) == size_t(0);
        }
        int step_function_plus(int previous, double step){
            return static_cast<int>(previous+step);
        }
        int step_function_multiply(int previous, double multiply){
            return static_cast<int>(previous*multiply);
        }
        // "Power-of-2 ladder": nsteps is the desired number of steps between any subsequent powers of 2.
        // The actual step is the quotient of the nearest smaller power of 2 divided by that number (but at least 1).
        // E.g., '1:32:#4' means 1,2,3,4,5,6,7,8,10,12,14,16,20,24,28,32
        int step_function_power2_ladder(int previous, double nsteps){
            int steps = int(nsteps);
            assert( is_power_of_2(steps) );  // must be a power of 2
            // The actual step is 1 until the value is twice as big as nsteps
            if( previous < 2*steps )
                return previous+1;
            // calculate the previous power of 2
            int prev_power2 = previous/2;                 // start with half the given value
            int rshift = 1;                               // and with the shift of 1;
            while( int shifted = prev_power2>>rshift ) {  // shift the value right; while the result is non-zero,
                prev_power2 |= shifted;                   //   add the bits set in 'shifted';
                rshift <<= 1;                             //   double the shift, as twice as many top bits are set;
            }                                             // repeat.
            ++prev_power2; // all low bits set; now it's just one less than the desired power of 2
            assert( is_power_of_2(prev_power2) );
            assert( (prev_power2<=previous)&&(2*prev_power2>previous) );
            // The actual step value is the previous power of 2 divided by steps
            return previous + (prev_power2/steps);
        }
        typedef int (* step_function_ptr_type)(int,double);

        struct step_function_descriptor  {
            char mnemonic;
            step_function_ptr_type function;
        public:
            step_function_descriptor(char a_mnemonic, step_function_ptr_type a_function) : mnemonic(a_mnemonic), function(a_function) {}
        private:
            void operator=(step_function_descriptor  const&);
        };
        step_function_descriptor step_function_descriptors[] = {
                step_function_descriptor('*',step_function_multiply),
                step_function_descriptor('+',step_function_plus),
                step_function_descriptor('#',step_function_power2_ladder)
        };

        template<typename T, size_t N>
        inline size_t array_length(const T(&)[N])
        {
           return N;
        }

        struct thread_range_step {
            step_function_ptr_type step_function;
            double step_function_argument;

            thread_range_step ( step_function_ptr_type step_function_, double step_function_argument_)
                :step_function(step_function_),step_function_argument(step_function_argument_)
            {
                if (!step_function_)
                    throw std::invalid_argument("step_function for thread range step should not be NULL");
            }
            int operator()(int previous)const {
                assert(0<=previous); // test 0<=first and loop discipline
                const int ret = step_function(previous,step_function_argument);
                assert(previous<ret);
                return ret;
            }
            friend std::istream& operator>>(std::istream& input_stream, thread_range_step& step){
                char function_char;
                double function_argument;
                input_stream >> function_char >> function_argument;
                size_t i = 0;
                while ((i<array_length(step_function_descriptors)) && (step_function_descriptors[i].mnemonic!=function_char)) ++i;
                if (i >= array_length(step_function_descriptors)){
                    throw std::invalid_argument("unknown step function mnemonic: "+std::string(1,function_char));
                } else if ((function_char=='#') && !is_power_of_2(function_argument)) {
                    throw std::invalid_argument("the argument of # should be a power of 2");
                }
                step.step_function = step_function_descriptors[i].function;
                step.step_function_argument = function_argument;
                return input_stream;
            }
        };
    } // namespace internal

    struct thread_number_range{
        int (*auto_number_of_threads)();
        int first; // 0<=first (0 can be used as a special value)
        int last;  // first<=last

        internal::thread_range_step step;

        thread_number_range( int (*auto_number_of_threads_)(),int low_=1, int high_=-1
                , internal::thread_range_step step_ =  internal::thread_range_step(internal::step_function_power2_ladder,4)
        )
            : auto_number_of_threads(auto_number_of_threads_), first(low_), last((high_>-1) ? high_ : auto_number_of_threads_())
              ,step(step_)
        {
            if (first<0) {
                throw std::invalid_argument("negative value not allowed");
            }
            if (first>last) {
                throw std::invalid_argument("decreasing sequence not allowed");
            }
        }
        friend std::istream& operator>>(std::istream& i, thread_number_range& range){
            try{
                std::string s;
                i>>s;
                struct string_to_number_of_threads{
                    int auto_value;
                    string_to_number_of_threads(int auto_value_):auto_value(auto_value_){}
                    int operator()(const std::string & value)const{
                        return (value=="auto")? auto_value : internal::string_to<int>(value);
                    }
                };
                string_to_number_of_threads string_to_number_of_threads(range.auto_number_of_threads());
                int low, high;
                std::size_t colon = s.find(':');
                if ( colon == std::string::npos ){
                    low = high = string_to_number_of_threads(s);
                } else {
                    //it is a range
                    std::size_t second_colon = s.find(':',colon+1);

                    low  = string_to_number_of_threads(std::string(s, 0, colon)); //not copying the colon
                    high = string_to_number_of_threads(std::string(s, colon+1, second_colon - (colon+1))); //not copying the colons
                    if (second_colon != std::string::npos){
                        internal::string_to(std::string(s,second_colon + 1),range.step);
                    }
                }
                range = thread_number_range(range.auto_number_of_threads,low,high,range.step);
            }catch(std::invalid_argument&){
                i.setstate(std::ios::failbit);
                throw;
            }
            return i;
        }
        friend std::ostream& operator<<(std::ostream& o, thread_number_range const& range){
            using namespace internal;
            size_t i = 0;
            for (; i < array_length(step_function_descriptors) && step_function_descriptors[i].function != range.step.step_function; ++i ) {}
            if (i >= array_length(step_function_descriptors)){
                throw std::invalid_argument("unknown step function for thread range");
            }
            o<<range.first<<":"<<range.last<<":"<<step_function_descriptors[i].mnemonic<<range.step.step_function_argument;
            return o;
        }
    }; // struct thread_number_range
    //TODO: fix unused warning here
    //TODO: update the thread range description in the .html files
    static const char* thread_number_range_desc="number of threads to use; a range of the form low[:high[:(+|*|#)step]],"
                                                "\n\twhere low and optional high are non-negative integers or 'auto' for the default choice,"
                                                "\n\tand optional step expression specifies how thread numbers are chosen within the range."
                                                "\n\tSee examples/common/index.html for detailed description."
   ;

    inline void report_elapsed_time(double seconds){
        std::cout<<"elapsed time : "<<seconds<<" seconds"<<std::endl;
    }

    inline void report_skipped(){
        std::cout<<"skip"<<std::endl;
    }

    inline void parse_cli_arguments(int argc, const char* argv[], utility::cli_argument_pack cli_pack){
        bool show_help = false;
        cli_pack.arg(show_help,"-h","show this message");

        bool invalid_input=false;
        try {
            cli_pack.parse(argc,argv);
        }catch(std::exception& e){
            std::cerr
                    <<"error occurred while parsing command line."<<std::endl
                    <<"error text: "<<e.what()<<std::endl
                    <<std::flush;
            invalid_input =true;
        }
        if (show_help || invalid_input){
            std::cout<<cli_pack.usage_string(argv[0])<<std::flush;
            std::exit(0);
        }

    }
    inline void parse_cli_arguments(int argc, char* argv[], utility::cli_argument_pack cli_pack){
         parse_cli_arguments(argc, const_cast<const char**>(argv), cli_pack);
    }
}

#endif /* UTILITY_H_ */


//! String type with scalable allocator.
/** On platforms with non-scalable default memory allocators, the example scales
    better if the string allocator is changed to tbb::tbb_allocator<char>. */
typedef std::basic_string<char,std::char_traits<char>,tbb::tbb_allocator<char> > MyString;

using namespace tbb;
using namespace std;

//! Set to true to counts.
static bool verbose = false;
static bool silent = false;
//! Problem size
long N = 1000000;
const int size_factor = 2;

//! A concurrent hash table that maps strings to ints.
typedef concurrent_hash_map<MyString,int> StringTable;

//! Function object for counting occurrences of strings.
struct Tally {
    StringTable& table;
    Tally( StringTable& table_ ) : table(table_) {}
    void operator()( const blocked_range<MyString*> range ) const {
        for( MyString* p=range.begin(); p!=range.end(); ++p ) {
            StringTable::accessor a;
            table.insert( a, *p );
            a->second += 1;
        }
    }
};

static MyString* Data;

static void CountOccurrences(int nthreads) {
    StringTable table;

    tick_count t0 = tick_count::now();
    parallel_for( blocked_range<MyString*>( Data, Data+N, 1000 ), Tally(table) );
    tick_count t1 = tick_count::now();

    int n = 0;
    for( StringTable::iterator i=table.begin(); i!=table.end(); ++i ) {
        if( verbose && nthreads )
            printf("%s %d\n",i->first.c_str(),i->second);
        n += i->second;
    }

    if ( !silent ) printf("total = %d  unique = %u  time = %g\n", n, unsigned(table.size()), (t1-t0).seconds());
}

/// Generator of random words

struct Sound {
    const char *chars;
    int rates[3];// beginning, middle, ending
};
Sound Vowels[] = {
    {"e", {445,6220,1762}}, {"a", {704,5262,514}}, {"i", {402,5224,162}}, {"o", {248,3726,191}},
    {"u", {155,1669,23}}, {"y", {4,400,989}}, {"io", {5,512,18}}, {"ia", {1,329,111}},
    {"ea", {21,370,16}}, {"ou", {32,298,4}}, {"ie", {0,177,140}}, {"ee", {2,183,57}},
    {"ai", {17,206,7}}, {"oo", {1,215,7}}, {"au", {40,111,2}}, {"ua", {0,102,4}},
    {"ui", {0,104,1}}, {"ei", {6,94,3}}, {"ue", {0,67,28}}, {"ay", {1,42,52}},
    {"ey", {1,14,80}}, {"oa", {5,84,3}}, {"oi", {2,81,1}}, {"eo", {1,71,5}},
    {"iou", {0,61,0}}, {"oe", {2,46,9}}, {"eu", {12,43,0}}, {"iu", {0,45,0}},
    {"ya", {12,19,5}}, {"ae", {7,18,10}}, {"oy", {0,10,13}}, {"ye", {8,7,7}},
    {"ion", {0,0,20}}, {"ing", {0,0,20}}, {"ium", {0,0,10}}, {"er", {0,0,20}}
};
Sound Consonants[] = {
    {"r", {483,1414,1110}}, {"n", {312,1548,1114}}, {"t", {363,1653,251}}, {"l", {424,1341,489}},
    {"c", {734,735,260}}, {"m", {732,785,161}}, {"d", {558,612,389}}, {"s", {574,570,405}},
    {"p", {519,361,98}}, {"b", {528,356,30}}, {"v", {197,598,16}}, {"ss", {3,191,567}},
    {"g", {285,430,42}}, {"st", {142,323,180}}, {"h", {470,89,30}}, {"nt", {0,350,231}},
    {"ng", {0,117,442}}, {"f", {319,194,19}}, {"ll", {1,414,83}}, {"w", {249,131,64}},
    {"k", {154,179,47}}, {"nd", {0,279,92}}, {"bl", {62,235,0}}, {"z", {35,223,16}},
    {"sh", {112,69,79}}, {"ch", {139,95,25}}, {"th", {70,143,39}}, {"tt", {0,219,19}},
    {"tr", {131,104,0}}, {"pr", {186,41,0}}, {"nc", {0,223,2}}, {"j", {184,32,1}},
    {"nn", {0,188,20}}, {"rt", {0,148,51}}, {"ct", {0,160,29}}, {"rr", {0,182,3}},
    {"gr", {98,87,0}}, {"ck", {0,92,86}}, {"rd", {0,81,88}}, {"x", {8,102,48}},
    {"ph", {47,101,10}}, {"br", {115,43,0}}, {"cr", {92,60,0}}, {"rm", {0,131,18}},
    {"ns", {0,124,18}}, {"sp", {81,55,4}}, {"sm", {25,29,85}}, {"sc", {53,83,1}},
    {"rn", {0,100,30}}, {"cl", {78,42,0}}, {"mm", {0,116,0}}, {"pp", {0,114,2}},
    {"mp", {0,99,14}}, {"rs", {0,96,16}}, /*{"q", {52,57,1}},*/ {"rl", {0,97,7}},
    {"rg", {0,81,15}}, {"pl", {56,39,0}}, {"sn", {32,62,1}}, {"str", {38,56,0}},
    {"dr", {47,44,0}}, {"fl", {77,13,1}}, {"fr", {77,11,0}}, {"ld", {0,47,38}},
    {"ff", {0,62,20}}, {"lt", {0,61,19}}, {"rb", {0,75,4}}, {"mb", {0,72,7}},
    {"rc", {0,76,1}}, {"gg", {0,74,1}}, {"pt", {1,56,10}}, {"bb", {0,64,1}},
    {"sl", {48,17,0}}, {"dd", {0,59,2}}, {"gn", {3,50,4}}, {"rk", {0,30,28}},
    {"nk", {0,35,20}}, {"gl", {40,14,0}}, {"wh", {45,6,0}}, {"ntr", {0,50,0}},
    {"rv", {0,47,1}}, {"ght", {0,19,29}}, {"sk", {23,17,5}}, {"nf", {0,46,0}},
    {"cc", {0,45,0}}, {"ln", {0,41,0}}, {"sw", {36,4,0}}, {"rp", {0,36,4}},
    {"dn", {0,38,0}}, {"ps", {14,19,5}}, {"nv", {0,38,0}}, {"tch", {0,21,16}},
    {"nch", {0,26,11}}, {"lv", {0,35,0}}, {"wn", {0,14,21}}, {"rf", {0,32,3}},
    {"lm", {0,30,5}}, {"dg", {0,34,0}}, {"ft", {0,18,15}}, {"scr", {23,10,0}},
    {"rch", {0,24,6}}, {"rth", {0,23,7}}, {"rh", {13,15,0}}, {"mpl", {0,29,0}},
    {"cs", {0,1,27}}, {"gh", {4,10,13}}, {"ls", {0,23,3}}, {"ndr", {0,25,0}},
    {"tl", {0,23,1}}, {"ngl", {0,25,0}}, {"lk", {0,15,9}}, {"rw", {0,23,0}},
    {"lb", {0,23,1}}, {"tw", {15,8,0}}, /*{"sq", {15,8,0}},*/ {"chr", {18,4,0}},
    {"dl", {0,23,0}}, {"ctr", {0,22,0}}, {"nst", {0,21,0}}, {"lc", {0,22,0}},
    {"sch", {16,4,0}}, {"ths", {0,1,20}}, {"nl", {0,21,0}}, {"lf", {0,15,6}},
    {"ssn", {0,20,0}}, {"xt", {0,18,1}}, {"xp", {0,20,0}}, {"rst", {0,15,5}},
    {"nh", {0,19,0}}, {"wr", {14,5,0}}
};
const int VowelsNumber = sizeof(Vowels)/sizeof(Sound);
const int ConsonantsNumber = sizeof(Consonants)/sizeof(Sound);
int VowelsRatesSum[3] = {0,0,0}, ConsonantsRatesSum[3] = {0,0,0};

int CountRateSum(Sound sounds[], const int num, const int part)
{
    int sum = 0;
    for(int i = 0; i < num; i++)
        sum += sounds[i].rates[part];
    return sum;
}

const char *GetLetters(int type, const int part)
{
    Sound *sounds; int rate, i = 0;
    if(type & 1)
        sounds = Vowels, rate = rand() % VowelsRatesSum[part];
    else
        sounds = Consonants, rate = rand() % ConsonantsRatesSum[part];
    do {
        rate -= sounds[i++].rates[part];
    } while(rate > 0);
    return sounds[--i].chars;
}

static void CreateData() {
    for(int i = 0; i < 3; i++) {
        ConsonantsRatesSum[i] = CountRateSum(Consonants, ConsonantsNumber, i);
        VowelsRatesSum[i] = CountRateSum(Vowels, VowelsNumber, i);
    }
    for( int i=0; i<N; ++i ) {
        int type = rand();
        Data[i] = GetLetters(type++, 0);
        for( int j = 0; j < type%size_factor; ++j )
            Data[i] += GetLetters(type++, 1);
        Data[i] += GetLetters(type, 2);
    }
    MyString planet = Data[12]; planet[0] = toupper(planet[0]);
    MyString helloworld = Data[0]; helloworld[0] = toupper(helloworld[0]);
    helloworld += ", "+Data[1]+" "+Data[2]+" "+Data[3]+" "+Data[4]+" "+Data[5];
    if ( !silent ) printf("Message from planet '%s': %s!\nAnalyzing whole text...\n", planet.c_str(), helloworld.c_str());
}

int main( int argc, char* argv[] ) {
    try {
        tbb::tick_count mainStartTime = tbb::tick_count::now();
        srand(2);

        //! Working threads count
        // The 1st argument is the function to obtain 'auto' value; the 2nd is the default value
        // The example interprets 0 threads as "run serially, then fully subscribed"
        utility::thread_number_range threads(tbb::task_scheduler_init::default_num_threads,0);

        utility::parse_cli_arguments(argc,argv,
            utility::cli_argument_pack()
            //"-h" option for displaying help is present implicitly
            .positional_arg(threads,"n-of-threads",utility::thread_number_range_desc)
            .positional_arg(N,"n-of-strings","number of strings")
            .arg(verbose,"verbose","verbose mode")
            .arg(silent,"silent","no output except elapsed time")
            );

        if ( silent ) verbose = false;

        Data = new MyString[N];
        CreateData();

        if ( threads.first ) {
            for(int p = threads.first;  p <= threads.last; p = threads.step(p)) {
                if ( !silent ) printf("threads = %d  ", p );
                task_scheduler_init init( p );
                CountOccurrences( p );
            }
        } else { // Number of threads wasn't set explicitly. Run serial and parallel version
            { // serial run
                if ( !silent ) printf("serial run   ");
                task_scheduler_init init_serial(1);
                CountOccurrences(1);
            }
            { // parallel run (number of threads is selected automatically)
                if ( !silent ) printf("parallel run ");
                task_scheduler_init init_parallel;
                CountOccurrences(0);
            }
        }

        delete[] Data;

        utility::report_elapsed_time((tbb::tick_count::now() - mainStartTime).seconds());

        return 0;
    } catch(std::exception& e) {
        std::cerr<<"error occurred. error text is :\"" <<e.what()<<"\"\n";
    }
}