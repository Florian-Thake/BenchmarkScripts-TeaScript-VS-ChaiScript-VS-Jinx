/*
 * SPDX-FileCopyrightText: Copyright (C) 2024 Florian Thake, <contact |at| tea-age.solutions>.
 * SPDX-License-Identifier: MIT
 */

// Benchmarking calculating Fibonacci in script languages in and for C++ (ChaiScript, TeaScript, Jinx), using C++ as reference.

// === BENCH CONFIG ===

#define BENCH_ENABLE_CPP   1                    // 1 == Enable C++, 0 == Disable
#define BENCH_ENABLE_CHAI  1                    // 1 == Enable ChaiScript, 0 == Disable
#define BENCH_ENABLE_JINX  1                    // 1 == Enable Jinx, 0 == Disable
#define BENCH_ENABLE_TEA   1                    // 1 == Enable TeaScript, 0 == Disable

#define BENCH_RECURSIVE    1                    // option for recursive calculation of Fibonacci 25
#define BENCH_ITERATIVE    2                    // option for iterative calculation of Fibonacci 25
#define BENCH_KIND         BENCH_RECURSIVE      // decide between recursive or iterative calculation benchmark.

#define BENCH_ITERATIONS   3                    // loop count of each tested language.

#define BENCH_FIB_NUM      25                   // the Fibonacci number to calculate.


// handle some annoying compile errors on MSVC
#if defined _MSC_VER  && !defined _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
# define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#endif
#if defined _MSC_VER  && !defined _SILENCE_CXX20_U8PATH_DEPRECATION_WARNING
# define _SILENCE_CXX20_U8PATH_DEPRECATION_WARNING
#endif
#if defined _MSC_VER  && !defined _CRT_SECURE_NO_WARNINGS
# define _CRT_SECURE_NO_WARNINGS
#endif


//for VS use /Zc:__cplusplus
#if __cplusplus < 202002L
# if defined _MSVC_LANG // fallback without /Zc:__cplusplus
#  if !_HAS_CXX20
#   error must use at least C++20
#  endif
# else
#  error must use at least C++20
# endif
#endif


#include <cstdlib> // EXIT_SUCCESS
#include <cstdio>
#include <iostream>
#include <chrono>

#if BENCH_ENABLE_JINX
#include <Jinx.hpp>
#endif
#if BENCH_ENABLE_TEA
#include <teascript/Parser.hpp>
#include <teascript/CoreLibrary.hpp>
// check version if new enough (Bootstrap( w. config) and GetAsInteger() exists)
#if TEASCRIPT_VERSION < TEASCRIPT_BUILD_VERSION_NUMBER(0,9,0)
# error Use TeaScript 0.9.0 or newer
#endif
// check for compile and run in TeaStackVM feature
#if TEASCRIPT_VERSION >= TEASCRIPT_BUILD_VERSION_NUMBER(0,14,0)
#include <teascript/StackVMCompiler.hpp>
#include <teascript/StackMachine.hpp>
#endif
#endif
#if BENCH_ENABLE_CHAI
#if defined(_WIN32)
#  define NOMINMAX
#  define WIN32_LEAN_AND_MEAN
# endif
# if defined( _MSC_VER )
#  pragma warning( push )
#  pragma warning( disable: 4244 )
# endif
#include <chaiscript/chaiscript.hpp>
# if defined( _MSC_VER )
#  pragma warning( pop )
# endif
#endif



// recursive fibonacci function in TeaScript
constexpr char tea_code[] = R"_SCRIPT_(
func fib( x ) {
    if( x == 1 or x == 0 ) {
       x
    } else {
       fib( x - 1 ) + fib( x - 2 )
    }
}

fib( fib_num )
)_SCRIPT_";

// iterative fibonacci function in TeaScript
constexpr char tea_loop_code[] = R"_SCRIPT_(
func fib( x ) {
    if( x > 1 ) {
        def out  := 1
        def prev := 0
        def tmp  := 1
        def i    := 2
        repeat {
            if( i > x ) { stop }
            tmp  := out        
            out  := out + prev
            prev := tmp
            i    := i + 1
        }
        out
    } else {
        x
    }
}

fib( fib_num )
)_SCRIPT_";

#if BENCH_ENABLE_TEA
#if TEASCRIPT_VERSION >= TEASCRIPT_BUILD_VERSION_NUMBER(0,12,0)
// iterative fibonacci function in TeaScript with forall loop (available since 0.12)
constexpr char tea_loop_code_new[] = R"_SCRIPT_(
func fib( x ) {
    if( x > 1 ) {
        def out  := 1
        def prev := 0
        def tmp  := 1
        def i    := 2
        forall( i in _seq(2, x, 1) ) {
            tmp  := out        
            out  := out + prev
            prev := tmp
        }
        out
    } else {
        x
    }
}

fib( fib_num )
)_SCRIPT_";
#endif
#endif

// recursive fibonacci function in ChaiScript
constexpr char chai_code[] = R"_SCRIPT_(
def fib( x )
{
    if( x == 0 || x == 1 ) {
        return x;
    } else {
        return fib( x - 1 ) + fib( x - 2 );
    }
}

fib( fib_num );
)_SCRIPT_";

// iterative fibonacci function in ChaiScript
constexpr char chai_loop_code[] = R"_SCRIPT_(
def fib( x )
{
    if( x > 1 ) {
        var out  = 1;
        var prev = 0;
        var tmp  = 1;
        for( var i = 2; i <= x; ++i ) {
            tmp  = out;
            out  = out + prev;
            prev = tmp;
        }
        out;
    } else {
        x;
    }
}

fib( fib_num );
)_SCRIPT_";

// recursive fibonacci function in Jinx
constexpr char jinx_code[] = R"_SCRIPT_(
import core

function fib {x}
    if x < 2 
        return x
    end
    return fib (x - 1) + fib (x - 2)
end

set res to fib fib_num
)_SCRIPT_";

// iterative fibonacci function in Jinx
constexpr char jinx_loop_code[] = R"_SCRIPT_(
import core

function fib {x}
    if x > 1 
        set out  to 1
        set prev to 0
        set tmp  to 1
        loop var from 2 to x
            set tmp  to out
            increment out by prev
            set prev to tmp
        end
        return out
    else
        return x
    end
end

set res to fib fib_num
)_SCRIPT_";


// for time measurement...

auto Now()
{
    return std::chrono::steady_clock::now();
}

double CalcTimeInSecs( auto s, auto e )
{   
    std::chrono::duration<double> const  timesecs = e - s;
    return timesecs.count();
}


// now the execution functions. we meausre only the execution times of the scripts. parsing and bootstrapping are excluded.

#if BENCH_ENABLE_TEA
double exec_tea()
{
    teascript::Context c;
    teascript::CoreLibrary().Bootstrap( c, teascript::config::core() );
    c.AddValueObject( "fib_num", teascript::ValueObject( static_cast<teascript::Integer>(BENCH_FIB_NUM), teascript::ValueConfig( true ) ) );
    teascript::Parser  p;
    auto ast = p.Parse( tea_code );
    try {
        auto start  = Now();
        auto teares = ast->Eval( c );
        auto end    = Now();

        std::cout << "value: " << teares.GetAsInteger() << std::endl;

        return CalcTimeInSecs( start, end );

    } catch( teascript::exception::runtime_error const &ex ) {
        teascript::util::pretty_print( ex );
    } catch( std::exception const &ex ) {
        puts( ex.what() );
    }

    return -1.0;
}

#if TEASCRIPT_VERSION >= TEASCRIPT_BUILD_VERSION_NUMBER(0,14,0)
double exec_tea_compiled()
{
    teascript::Context c;
    teascript::CoreLibrary().Bootstrap( c, teascript::config::core() );
    c.AddValueObject( "fib_num", teascript::ValueObject( static_cast<teascript::Integer>(BENCH_FIB_NUM), teascript::ValueConfig( true ) ) );
    auto machine = std::make_shared<teascript::StackVM::Machine<false>>();
    teascript::Parser  p;
    teascript::StackVM::Compiler  compiler;
    try {
        auto prog = compiler.Compile( p.Parse( tea_code ), teascript::eOptimize::O2 );

        auto start = Now();
        machine->Exec( prog, c );
        machine->ThrowPossibleErrorException();
        auto teares = machine->MoveResult();
        auto end = Now();

        std::cout << "value: " << teares.GetAsInteger() << std::endl;

        return CalcTimeInSecs( start, end );

    } catch( teascript::exception::runtime_error const &ex ) {
        teascript::util::pretty_print( ex );
    } catch( std::exception const &ex ) {
        puts( ex.what() );
    }

    return -1.0;
}
#endif

template< typename T, size_t N>
double exec_tea_loop( T const (&code)[N] )
{
    teascript::Context c;
    teascript::CoreLibrary().Bootstrap( c, teascript::config::core() );
    c.AddValueObject( "fib_num", teascript::ValueObject( static_cast<teascript::Integer>(BENCH_FIB_NUM), teascript::ValueConfig( true ) ) );
    teascript::Parser  p;
    auto ast = p.Parse( code );
    try {
        auto start = Now();
        auto teares = ast->Eval( c );
        auto end = Now();

        std::cout << "value: " << teares.GetAsInteger() << std::endl;

        return CalcTimeInSecs( start, end );

    } catch( teascript::exception::runtime_error const &ex ) {
        teascript::util::pretty_print( ex );
    } catch( std::exception const &ex ) {
        puts( ex.what() );
    }

    return -1.0;
}

#if TEASCRIPT_VERSION >= TEASCRIPT_BUILD_VERSION_NUMBER(0,14,0)
template< typename T, size_t N>
double exec_tea_loop_compiled( T const (&code)[N] )
{
    teascript::Context c;
    teascript::CoreLibrary().Bootstrap( c, teascript::config::core() );
    c.AddValueObject( "fib_num", teascript::ValueObject( static_cast<teascript::Integer>(BENCH_FIB_NUM), teascript::ValueConfig( true ) ) );
    auto machine = std::make_shared<teascript::StackVM::Machine<false>>();
    teascript::Parser  p;
    teascript::StackVM::Compiler  compiler;
    try {
        auto prog = compiler.Compile( p.Parse( code ), teascript::eOptimize::O2 );

        auto start = Now();
        machine->Exec( prog, c );
        machine->ThrowPossibleErrorException();
        auto teares = machine->MoveResult();
        auto end = Now();

        std::cout << "value: " << teares.GetAsInteger() << std::endl;

        return CalcTimeInSecs( start, end );

    } catch( teascript::exception::runtime_error const &ex ) {
        teascript::util::pretty_print( ex );
    } catch( std::exception const &ex ) {
        puts( ex.what() );
    }

    return -1.0;
}
#endif
#endif

#if BENCH_ENABLE_CHAI
double exec_chai()
{
    chaiscript::ChaiScript chai;
    chai.add( chaiscript::const_var( BENCH_FIB_NUM ), "fib_num" );
    auto ast = chai.parse( chai_code );
    try {
        auto start = Now();
        auto chres = chai.eval( *ast );
        auto end   = Now();

        std::cout << "value: " << chaiscript::boxed_cast<int>(chres) << std::endl;

        return CalcTimeInSecs( start, end );
        
    } catch( chaiscript::Boxed_Value const &bv ) {
        puts( chaiscript::boxed_cast<chaiscript::exception::eval_error const &>(bv).what() );
    } catch( std::exception const &ex ) {
        puts( ex.what() );
    }

    return -1.0;
}

double exec_chai_loop()
{
    chaiscript::ChaiScript chai;
    chai.add( chaiscript::const_var( BENCH_FIB_NUM ), "fib_num" );
    auto ast = chai.parse( chai_loop_code );
    try {
        auto start = Now();
        auto chres = chai.eval( *ast );
        auto end = Now();

        std::cout << "value: " << chaiscript::boxed_cast<int>(chres) << std::endl;

        return CalcTimeInSecs( start, end );

    } catch( chaiscript::Boxed_Value const &bv ) {
        puts( chaiscript::boxed_cast<chaiscript::exception::eval_error const &>(bv).what() );
    } catch( std::exception const &ex ) {
        puts( ex.what() );
    }

    return -1.0;
}
#endif


#if BENCH_ENABLE_JINX
double exec_jinx()
{
    Jinx::GlobalParams  params;
    params.errorOnMaxInstrunctions = false;
    //params.logSymbols = true;
    //params.logBytecode = true;
    Jinx::Initialize( params );
    auto jinx = Jinx::CreateRuntime();
    jinx->GetLibrary( "core" )->RegisterProperty( Jinx::Visibility::Public, Jinx::Access::ReadOnly, "fib_num", Jinx::Variant( BENCH_FIB_NUM ) );
    auto script = jinx->CreateScript( jinx_code );
    try {
        auto start = Now();
        do {
            bool const res = script->Execute();
            if( !res ) {
                throw std::runtime_error( "Jinx Error!" );
            }
        } while( !script->IsFinished() );
        auto end = Now();

        std::cout << "value: " << script->GetVariable( "res" ).GetInteger() << std::endl;

        return CalcTimeInSecs( start, end );

    } catch( std::exception const &ex ) {
        puts( ex.what() );
    }
    return -1.0;
}

double exec_jinx_loop()
{
    Jinx::GlobalParams  params;
    params.errorOnMaxInstrunctions = false;
    //params.logSymbols = true;
    //params.logBytecode = true;
    Jinx::Initialize( params );
    auto jinx = Jinx::CreateRuntime();
    jinx->GetLibrary( "core" )->RegisterProperty( Jinx::Visibility::Public, Jinx::Access::ReadOnly, "fib_num", Jinx::Variant( BENCH_FIB_NUM ) );
    auto script = jinx->CreateScript( jinx_loop_code );
    try {
        auto start = Now();
        do {
            bool const res = script->Execute();
            if( !res ) {
                throw std::runtime_error( "Jinx Error!" );
            }
        } while( !script->IsFinished() );
        auto end = Now();

        std::cout << "value: " << script->GetVariable( "res" ).GetInteger() << std::endl;

        return CalcTimeInSecs( start, end );

    } catch( std::exception const &ex ) {
        puts( ex.what() );
    }
    return -1.0;
}
#endif

// recursive fibonacci function in C++
long long fib( long long x )
{
    if( x == 0 || x == 1 ) {
        return x;
    } else {
        return fib( x - 1 ) + fib( x - 2 );
    }
}

double exec_cpp()
{
    try {
        auto start = Now();
        auto res   = fib( BENCH_FIB_NUM );
        auto end   = Now();

        std::cout << "value: " << res << std::endl;

        return CalcTimeInSecs( start, end );

    } catch( std::exception const &ex ) {
        puts( ex.what() );
    }

    return -1.0;
}

// iterative fibonacci function in C++
long long fib_loop( long long x ) noexcept
{
    long long  out  = 1;
    long long  prev = 0;

    for( long long i = 2; i <= x; ++i ) {
        auto tmp = out;
        out += prev;
        prev = tmp;
    }

    return out;
}

double exec_cpp_loop()
{
    try {
        auto start = Now();
        auto res = fib_loop( BENCH_FIB_NUM );
        auto end = Now();

        std::cout << "value: " << res << std::endl;

        return CalcTimeInSecs( start, end );

    } catch( std::exception const &ex ) {
        puts( ex.what() );
    }

    return -1.0;
}

int main()
{
    std::cout << std::fixed;
    std::cout << std::setprecision( 8 );

    std::cout << "Benchmarking TeaScript, ChaiScript and Jinx in calculating Fibonacci of " << BENCH_FIB_NUM << " ...\n";
    std::cout << "... and C++ as a reference ... \n";

    // --- recursive ---

#if BENCH_ENABLE_CPP && (BENCH_KIND == BENCH_RECURSIVE)
    std::cout << "\nStart Test C++" << std::endl;
    for( int i = BENCH_ITERATIONS; i != 0; --i ) {
        auto secs = exec_cpp();
        std::cout << "Calculation took: " << secs << " seconds." << std::endl;
    }
#endif

#if BENCH_ENABLE_JINX && (BENCH_KIND == BENCH_RECURSIVE)
    std::cout << "\nStart Test Jinx" << std::endl;
    for( int i = BENCH_ITERATIONS; i != 0; --i ) {
        auto secs = exec_jinx();
        std::cout << "Calculation took: " << secs << " seconds." << std::endl;
    }
#endif

#if BENCH_ENABLE_TEA && (BENCH_KIND == BENCH_RECURSIVE)
    std::cout << "\nStart Test TeaScript" << std::endl;
    for( int i = BENCH_ITERATIONS; i != 0; --i ) {
        auto secs = exec_tea();
        std::cout << "Calculation took: " << secs << " seconds." << std::endl;
    }
#endif

#if BENCH_ENABLE_TEA && (BENCH_KIND == BENCH_RECURSIVE)
#if TEASCRIPT_VERSION >= TEASCRIPT_BUILD_VERSION_NUMBER(0,14,0)
    std::cout << "\nStart Test TeaScript in TeaStackVM" << std::endl;
    for( int i = BENCH_ITERATIONS; i != 0; --i ) {
        auto secs = exec_tea_compiled();
        std::cout << "Calculation took: " << secs << " seconds." << std::endl;
    }
#endif
#endif

#if BENCH_ENABLE_CHAI && (BENCH_KIND == BENCH_RECURSIVE)
    std::cout << "\nStart Test ChaiScript" << std::endl;
    for( int i = BENCH_ITERATIONS; i != 0; --i ) {
        auto secs = exec_chai();
        std::cout << "Calculation took: " << secs << " seconds." << std::endl;
    }
#endif

    // --- iterative ---

#if BENCH_ENABLE_CPP && (BENCH_KIND == BENCH_ITERATIVE)
    std::cout << "\nStart Test C++ LOOP" << std::endl;
    for( int i = BENCH_ITERATIONS; i != 0; --i ) {
        auto secs = exec_cpp_loop();
        std::cout << "Calculation took: " << secs << " seconds." << std::endl;
    }
#endif

#if BENCH_ENABLE_JINX && (BENCH_KIND == BENCH_ITERATIVE)
    std::cout << "\nStart Test Jinx LOOP" << std::endl;
    for( int i = BENCH_ITERATIONS; i != 0; --i ) {
        auto secs = exec_jinx_loop();
        std::cout << "Calculation took: " << secs << " seconds." << std::endl;
    }
#endif

#if BENCH_ENABLE_TEA && (BENCH_KIND == BENCH_ITERATIVE)
    std::cout << "\nStart Test TeaScript LOOP" << std::endl;
    for( int i = BENCH_ITERATIONS; i != 0; --i ) {
        auto secs = exec_tea_loop( tea_loop_code );
        std::cout << "Calculation took: " << secs << " seconds." << std::endl;
    }
#endif

#if BENCH_ENABLE_TEA && (BENCH_KIND == BENCH_ITERATIVE)
#if TEASCRIPT_VERSION >= TEASCRIPT_BUILD_VERSION_NUMBER(0,12,0)
    std::cout << "\nStart Test TeaScript LOOP (NEW forall)" << std::endl;
    for( int i = BENCH_ITERATIONS; i != 0; --i ) {
        auto secs = exec_tea_loop( tea_loop_code_new );
        std::cout << "Calculation took: " << secs << " seconds." << std::endl;
    }
#endif
#endif

#if BENCH_ENABLE_TEA && (BENCH_KIND == BENCH_ITERATIVE)
#if TEASCRIPT_VERSION >= TEASCRIPT_BUILD_VERSION_NUMBER(0,14,0)
    std::cout << "\nStart Test TeaScript LOOP in TeaStackVM" << std::endl;
    for( int i = BENCH_ITERATIONS; i != 0; --i ) {
        auto secs = exec_tea_loop_compiled( tea_loop_code );
        std::cout << "Calculation took: " << secs << " seconds." << std::endl;
    }

    std::cout << "\nStart Test TeaScript LOOP (NEW forall) in TeaStackVM" << std::endl;
    for( int i = BENCH_ITERATIONS; i != 0; --i ) {
        auto secs = exec_tea_loop_compiled( tea_loop_code_new );
        std::cout << "Calculation took: " << secs << " seconds." << std::endl;
    }
#endif
#endif

#if BENCH_ENABLE_CHAI && (BENCH_KIND == BENCH_ITERATIVE)
    std::cout << "\nStart Test ChaiScript LOOP" << std::endl;
    for( int i = BENCH_ITERATIONS; i != 0; --i ) {
        auto secs = exec_chai_loop();
        std::cout << "Calculation took: " << secs << " seconds." << std::endl;
    }
#endif


    puts( "\n\nTest end." );

    return EXIT_SUCCESS;
}
