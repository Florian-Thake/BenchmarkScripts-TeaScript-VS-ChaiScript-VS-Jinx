/*
 * SPDX-FileCopyrightText: Copyright (C) 2024 Florian Thake, <contact |at| tea-age.solutions>.
 * SPDX-License-Identifier: MIT
 */


// Benchmarking variable lookup / change in TeaScript's Context.


#define BENCH_SCOPES            10
#define BENCH_VARS_PER_SCOPE    1000
#define BENCH_OPERATIONS        ((BENCH_VARS_PER_SCOPE) / 2)

// for Lookup Deep: count of local scopes between the lookup and the global scope (like in a deep recursion).
#define BENCH_DEEP_SCOPES           50
#define BENCH_DEEP_VARS_PER_SCOPE   4

// for Function Call: count of simulated calls (EnterScope, add params, lookup, ExitScope) and params per call.
#define BENCH_CALLS             5000
#define BENCH_CALL_PARAMS       4

#define BENCH_ITERATIONS        10


#define BENCH_ENABLE_LOOKUP         1
#define BENCH_ENABLE_ADD            1
#define BENCH_ENABLE_SET            1
#define BENCH_ENABLE_SHARED_SET     1
#define BENCH_ENABLE_REMOVE         1
#define BENCH_ENABLE_LOOKUP_DEEP    1
#define BENCH_ENABLE_FUNC_CALL      1



// With this define a switch between the new (== 0) and the old (== 1) Context implementation is possible.
// NOTE: This define only exists for the transition, the old implementation will be removed with the next release.
#ifndef TEASCRIPT_DISABLE_NEW_CONTEXT
# define TEASCRIPT_DISABLE_NEW_CONTEXT     0
#endif


// NOTE: Both implementations use std::unordered_map for the lookup.
//       (The Collection used by the old one has TEASCRIPT_DISABLE_BOOST hard defined in Collection.hpp.)



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



#include "teascript/Context.hpp"

#if TEASCRIPT_USE_NEW_CONTEXT
# define BENCH_CONTEXT_IMPL_NAME   "new"
#else
# define BENCH_CONTEXT_IMPL_NAME   "old"
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



// All variable names are built only once at startup (see prepare_names()),
// so that the measurement does not contain building of std::string instances.
std::vector<std::vector<std::string>>  g_names; // [scope][var_idx]

std::string make_name( int s, int v )
{
    return "var_" + std::to_string( s ) + "_" + std::to_string( v );
}

// ensures that at least the names for scopes [0, scopes) with each [0, vars_per_scope) are available.
void prepare_names( int const scopes, int const vars_per_scope )
{
    if( g_names.size() < static_cast<size_t>(scopes) ) {
        g_names.resize( scopes );
    }
    for( int s = 0; s < scopes; ++s ) {
        auto &names = g_names[s];
        for( int v = static_cast<int>(names.size()); v < vars_per_scope; ++v ) {
            names.push_back( make_name( s, v ) );
        }
    }
}

std::string const &name( int s, int v )
{
    return g_names[s][v];
}


void setup_global_only( teascript::Context &c )
{
    for( int var_idx = 0; var_idx < BENCH_VARS_PER_SCOPE; ++var_idx ) {
        c.AddValueObject( name( 0, var_idx ), teascript::ValueObject( static_cast<long long>(var_idx), true ) );
    }
}

void setup( teascript::Context &c )
{
    for( int scope = 0; scope < BENCH_SCOPES; ++scope ) {

        for( int var_idx = 0; var_idx < BENCH_VARS_PER_SCOPE; ++var_idx ) {

            c.AddValueObject( name( scope, var_idx ), teascript::ValueObject( static_cast<long long>(scope) * var_idx, true ) );
        }

        c.EnterScope();
    }
    c.ExitScope(); // one too much.
}

// global scope with BENCH_VARS_PER_SCOPE vars, then BENCH_DEEP_SCOPES local scopes with only a few vars each.
void setup_deep( teascript::Context &c )
{
    setup_global_only( c );

    for( int scope = 1; scope <= BENCH_DEEP_SCOPES; ++scope ) {
        c.EnterScope();
        for( int var_idx = 0; var_idx < BENCH_DEEP_VARS_PER_SCOPE; ++var_idx ) {
            c.AddValueObject( name( scope, var_idx ), teascript::ValueObject( static_cast<long long>(scope) * var_idx, true ) );
        }
    }
}

double exec_lookup( teascript::Context &c )
{
    teascript::ValueObject val_res;
    unsigned long long res = 0;
    auto start = Now();
    // first current scope
    for( int i = 0; i < BENCH_OPERATIONS; ++i ) {
        val_res = c.FindValueObject( name( BENCH_SCOPES - 1, i ) );
        res += static_cast<unsigned long long>(val_res.GetValue<teascript::Integer>());
    }
#if 1
    // then global scope
    for( int i = 0; i < BENCH_OPERATIONS; ++i ) {
        val_res = c.FindValueObject( name( 0, i ) );
        res += static_cast<unsigned long long>(val_res.GetValue<teascript::Integer>());
    }
#endif
    auto end = Now();

    std::cout << "value: " << res << std::endl;

    return CalcTimeInSecs( start, end );
}


double exec_remove( teascript::Context &c )
{
    teascript::ValueObject val_res;
    unsigned long long res = 0;
    auto start = Now();
    // only current scope possible
    for( int i = 0; i < BENCH_OPERATIONS; ++i ) {
        val_res = c.RemoveValueObject( name( BENCH_SCOPES - 1, i ) );
        res += static_cast<unsigned long long>(val_res.GetValue<teascript::Integer>());
    }
    auto end = Now();

    std::cout << "value: " << res << std::endl;

    return CalcTimeInSecs( start, end );
}

double exec_add( teascript::Context &c )
{
    teascript::ValueObject  to_add( 1LL, true );
    teascript::ValueObject val_res;
    unsigned long long res = 0;
    auto start = Now();
    // only current scope possible
    for( int i = 0; i < BENCH_OPERATIONS; ++i ) {
        val_res = c.AddValueObject( name( BENCH_SCOPES - 1, BENCH_VARS_PER_SCOPE + i ), to_add );
        res += static_cast<unsigned long long>(val_res.GetValue<teascript::Integer>());
    }
    auto end = Now();

    std::cout << "value: " << res << std::endl;

    return CalcTimeInSecs( start, end );
}


// sets either only variables of the current scope or variables spread over all scopes (round robin).
double exec_set( teascript::Context &c, bool const shared, bool const all_scopes )
{
    teascript::ValueObject  new_val( 1LL, true );
    teascript::ValueObject val_res;
    unsigned long long res = 0;
    auto start = Now();
    for( int i = 0; i < BENCH_OPERATIONS; ++i ) {
        int const scope = all_scopes ? i % BENCH_SCOPES : BENCH_SCOPES - 1;
        val_res = c.SetValue( name( scope, i ), new_val, shared );
        res += static_cast<unsigned long long>(val_res.GetValue<teascript::Integer>());
    }
    auto end = Now();

    std::cout << "value: " << res << std::endl;

    return CalcTimeInSecs( start, end );
}

double exec_set_copy( teascript::Context &c )
{
    return exec_set( c, false, false );
}

double exec_set_shared( teascript::Context &c )
{
    return exec_set( c, true, false );
}

double exec_set_copy_all_scopes( teascript::Context &c )
{
    return exec_set( c, false, true );
}

double exec_set_shared_all_scopes( teascript::Context &c )
{
    return exec_set( c, true, true );
}


double exec_lookup_deep( teascript::Context &c )
{
    teascript::ValueObject val_res;
    unsigned long long res = 0;
    auto start = Now();
    // all from the global scope, which is BENCH_DEEP_SCOPES scopes away.
    for( int i = 0; i < BENCH_OPERATIONS; ++i ) {
        val_res = c.FindValueObject( name( 0, i ) );
        res += static_cast<unsigned long long>(val_res.GetValue<teascript::Integer>());
    }
    auto end = Now();

    std::cout << "value: " << res << std::endl;

    return CalcTimeInSecs( start, end );
}


// simulates function calls: new scope, add the parameters, use them and one global var, leave the scope.
double exec_func_call( teascript::Context &c )
{
    teascript::ValueObject  param( 1LL, true );
    teascript::ValueObject val_res;
    unsigned long long res = 0;
    auto start = Now();
    for( int i = 0; i < BENCH_CALLS; ++i ) {
        c.EnterScope();
        for( int p = 0; p < BENCH_CALL_PARAMS; ++p ) {
            c.AddValueObject( name( 1, p ), param );
        }
        for( int p = 0; p < BENCH_CALL_PARAMS; ++p ) {
            val_res = c.FindValueObject( name( 1, p ) );
            res += static_cast<unsigned long long>(val_res.GetValue<teascript::Integer>());
        }
        val_res = c.FindValueObject( name( 0, i % BENCH_VARS_PER_SCOPE ) );
        res += static_cast<unsigned long long>(val_res.GetValue<teascript::Integer>());
        c.ExitScope();
    }
    auto end = Now();

    std::cout << "value: " << res << std::endl;

    return CalcTimeInSecs( start, end );
}


struct BenchResult
{
    char const          *mpName;
    std::vector<double>  mSecs;
};

std::vector<BenchResult>  g_results;

template< typename SetupFunc, typename ExecFunc >
void run_bench( char const *pName, SetupFunc setup_func, ExecFunc exec_func )
{
    std::cout << "\nStart Test " << pName << std::endl;
    BenchResult  result{pName, {}};
    for( int i = BENCH_ITERATIONS; i != 0; --i ) {
        teascript::Context c; // always a fresh one. It will be destructed at loop end (not measured).
        setup_func( c );
        auto secs = exec_func( c );
        std::cout << "Calculation took: " << secs << " seconds." << std::endl;
        result.mSecs.push_back( secs );
    }
    g_results.push_back( std::move( result ) );
}

void print_summary()
{
    printf( "\n\nSummary for %s Context (%d iterations each, times in microseconds):\n", BENCH_CONTEXT_IMPL_NAME, BENCH_ITERATIONS );
    printf( "%-28s %12s %12s %12s\n", "Test", "min", "median", "mean" );
    for( auto &r : g_results ) {
        auto secs = r.mSecs;
        std::sort( secs.begin(), secs.end() );
        auto const n = secs.size();
        double const median = n % 2 ? secs[n / 2] : (secs[n / 2 - 1] + secs[n / 2]) / 2.0;
        double const mean   = std::accumulate( secs.begin(), secs.end(), 0.0 ) / static_cast<double>(n);
        printf( "%-28s %12.2f %12.2f %12.2f\n", r.mpName, secs.front() * 1e6, median * 1e6, mean * 1e6 );
    }
}


int main()
{
    std::cout << std::fixed;
    std::cout << std::setprecision( 8 );

    std::cout << "Benchmarking TeaScript Variable Lookup, Remove and Set by directly use the Context class.\n";
    std::cout << "Using the " BENCH_CONTEXT_IMPL_NAME " Context implementation.\n";

    // build all variable names upfront.
    prepare_names( BENCH_SCOPES, BENCH_VARS_PER_SCOPE + BENCH_OPERATIONS ); // Add test needs BENCH_OPERATIONS more names.
    prepare_names( BENCH_DEEP_SCOPES + 1, BENCH_DEEP_VARS_PER_SCOPE );
    prepare_names( 2, BENCH_CALL_PARAMS );

#if BENCH_ENABLE_LOOKUP
    run_bench( "Lookup", setup, exec_lookup );
#endif

#if BENCH_ENABLE_ADD
    run_bench( "Add", setup, exec_add );
#endif

#if BENCH_ENABLE_SET
    run_bench( "Set Assign", setup, exec_set_copy );
    run_bench( "Set Assign All Scopes", setup, exec_set_copy_all_scopes );
#endif

#if BENCH_ENABLE_SHARED_SET
    run_bench( "Set SharedAssign", setup, exec_set_shared );
    run_bench( "Set SharedAssign All Scopes", setup, exec_set_shared_all_scopes );
#endif

#if BENCH_ENABLE_REMOVE
    run_bench( "Remove", setup, exec_remove );
#endif

#if BENCH_ENABLE_LOOKUP_DEEP
    run_bench( "Lookup Deep", setup_deep, exec_lookup_deep );
#endif

#if BENCH_ENABLE_FUNC_CALL
    run_bench( "Function Call", setup_global_only, exec_func_call );
#endif

    print_summary();

    puts( "\n\nTest end." );

    return EXIT_SUCCESS;
}
