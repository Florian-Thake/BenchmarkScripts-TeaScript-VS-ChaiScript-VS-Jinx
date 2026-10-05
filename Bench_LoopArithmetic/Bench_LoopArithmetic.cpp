/*
 * SPDX-FileCopyrightText: Copyright (C) 2026 Florian Thake, <contact |at| tea-age.solutions>.
 * SPDX-License-Identifier: MIT
 */


// Benchmarking a counting loop and floating point arithmetic (Mandelbrot) in TeaScript.
// Each benchmark runs in all execution modes of the TeaScript Host Application:
// compiled for the TeaStackVM with -D, -O0, -O1, -O2 and evaluated as AST with --eval and -D --eval.


#define BENCH_LOOP_N            1000000     // loop count of the Loop benchmark.
#define BENCH_ARITH_N           100         // grid size (n x n) of the Arithmetic (Mandelbrot) benchmark.

#define BENCH_ITERATIONS        10          // runs per benchmark and mode.

#define BENCH_ENABLE_LOOP       1
#define BENCH_ENABLE_ARITH      1


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


#include <teascript/Parser.hpp>
#include <teascript/CoreLibrary.hpp>
#include <teascript/StackVMCompiler.hpp>
#include <teascript/StackMachine.hpp>

#if TEASCRIPT_VERSION < TEASCRIPT_BUILD_VERSION_NUMBER(0,18,0)
# error Use TeaScript 0.18.0 or newer
#endif


#include <cstdlib> // EXIT_SUCCESS
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>


// sums up (i bit_and 7) for i in [0, n). Result for n == 1,000,000 is 3,500,000.
constexpr char tea_loop_code[] = R"_SCRIPT_(
func bench( n ) {
    def sum := 0
    forall( i in _seq( 0, n - 1, 1 ) ) {
        sum := sum + (i bit_and 7)
    }
    sum
}

bench( bench_n )
)_SCRIPT_";

// Mandelbrot on an n x n grid with max 50 iterations per point. Returns the count of all iterations.
constexpr char tea_arith_code[] = R"_SCRIPT_(
func bench( n ) {
    const max_iter := 50
    def count := 0
    forall( y in _seq( 0, n - 1, 1 ) ) {
        const ci := -1.25 + 2.5 * y / n
        forall( x in _seq( 0, n - 1, 1 ) ) {
            const cr := -2.0 + 2.5 * x / n
            def zr := 0.0
            def zi := 0.0
            def k  := 0
            repeat {
                if( k >= max_iter or zr * zr + zi * zi > 4.0 ) { stop }
                const t := zr * zr - zi * zi + cr
                zi := 2.0 * zr * zi + ci
                zr := t
                k  := k + 1
            }
            count := count + k
        }
    }
    count
}

bench( bench_n )
)_SCRIPT_";


// the execution modes, named like the corresponding options of the TeaScript Host Application.
struct Mode
{
    char const          *mpName;
    bool                 mCompile;   // true: compile and run in the TeaStackVM, false: evaluate the AST.
    teascript::eOptimize mOptLevel;  // eOptimize::Debug also enables the debug mode of the Parser and the Context.
};

constexpr Mode g_modes[] = {
    { "-D",         true,  teascript::eOptimize::Debug },
    { "-O0",        true,  teascript::eOptimize::O0 },
    { "-O1",        true,  teascript::eOptimize::O1 },
    { "-O2",        true,  teascript::eOptimize::O2 },
    { "--eval",     false, teascript::eOptimize::O0 },
    { "-D --eval",  false, teascript::eOptimize::Debug },
};


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


// we measure only the execution of the script. bootstrapping, parsing and compiling are excluded.
template< typename T, size_t N>
double exec_tea( Mode const &mode, T const (&code)[N], long long const n )
{
    // setup like the TeaScript Host Application does it (but without tsb store).
    teascript::Context c( teascript::Settings( teascript::config::full(), mode.mOptLevel ) );
    teascript::CoreLibrary().Bootstrap( c, teascript::config::full(), not mode.mCompile );
    c.AddValueObject( "bench_n", teascript::ValueObject( static_cast<teascript::Integer>(n), teascript::ValueConfig( true ) ) );
    teascript::Parser  p;
    p.SetDebug( c.GetSettings().IsDebug() );
    try {
        auto ast = p.Parse( code );
        teascript::ValueObject  teares;
        double secs = 0.0;
        if( mode.mCompile ) {
            teascript::StackVM::Compiler  compiler;
            auto prog = compiler.Compile( ast, c.GetSettings().GetOptimizationLevel() );
            auto machine = std::make_shared<teascript::StackVM::Machine<false>>();

            auto start = Now();
            machine->Exec( prog, c );
            machine->ThrowPossibleErrorException();
            teares = machine->MoveResult();
            auto end = Now();
            secs = CalcTimeInSecs( start, end );
        } else {
            auto start = Now();
            teares = ast->Eval( c );
            auto end = Now();
            secs = CalcTimeInSecs( start, end );
        }

        std::cout << "value: " << teares.GetAsInteger() << std::endl;

        return secs;

    } catch( teascript::exception::runtime_error const &ex ) {
        teascript::util::pretty_print( ex );
    } catch( std::exception const &ex ) {
        puts( ex.what() );
    }

    return -1.0;
}


struct BenchResult
{
    std::string          mName;
    std::vector<double>  mSecs;
};

std::vector<BenchResult>  g_results;

template< typename T, size_t N>
void run_bench( char const *pName, T const (&code)[N], long long const n )
{
    for( auto const &mode : g_modes ) {
        std::string const name = std::string( pName ) + " " + mode.mpName;
        std::cout << "\nStart Test " << name << std::endl;
        BenchResult  result{name, {}};
        for( int i = BENCH_ITERATIONS; i != 0; --i ) {
            auto secs = exec_tea( mode, code, n );
            std::cout << "Calculation took: " << secs << " seconds." << std::endl;
            result.mSecs.push_back( secs );
        }
        g_results.push_back( std::move( result ) );
    }
}

void print_summary()
{
    printf( "\n\nSummary (%d iterations each, times in milliseconds):\n", BENCH_ITERATIONS );
    printf( "%-24s %10s %10s %10s\n", "Test", "min", "median", "mean" );
    for( auto &r : g_results ) {
        auto secs = r.mSecs;
        std::sort( secs.begin(), secs.end() );
        auto const n = secs.size();
        double const median = n % 2 ? secs[n / 2] : (secs[n / 2 - 1] + secs[n / 2]) / 2.0;
        double const mean   = std::accumulate( secs.begin(), secs.end(), 0.0 ) / static_cast<double>(n);
        printf( "%-24s %10.2f %10.2f %10.2f\n", r.mName.c_str(), secs.front() * 1e3, median * 1e3, mean * 1e3 );
    }
}


int main()
{
    std::cout << std::fixed;
    std::cout << std::setprecision( 8 );

    std::cout << "Benchmarking TeaScript " << teascript::version::as_str() << " Loop and Arithmetic in all execution modes.\n";

#if BENCH_ENABLE_LOOP
    run_bench( "Loop", tea_loop_code, BENCH_LOOP_N );
#endif

#if BENCH_ENABLE_ARITH
    run_bench( "Arithmetic", tea_arith_code, BENCH_ARITH_N );
#endif

    print_summary();

    puts( "\n\nTest end." );

    return EXIT_SUCCESS;
}
