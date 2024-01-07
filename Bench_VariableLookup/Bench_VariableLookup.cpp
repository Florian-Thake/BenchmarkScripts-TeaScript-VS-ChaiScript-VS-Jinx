/*
 * SPDX-FileCopyrightText: Copyright (C) 2024 Florian Thake, <contact |at| tea-age.solutions>.
 * SPDX-License-Identifier: MIT
 *
 * NOTE: The license above does - of course - NOT apply for any '#include' file below. Different licenses may apply with each included file!
 */

// Benchmarking variable lookup / change in TeaScript's Context.


#define BENCH_SCOPES            10
#define BENCH_VARS_PER_SCOPE    1000
#define BENCH_OPERATIONS        ((BENCH_VARS_PER_SCOPE) / 2)

#define BENCH_ITERATIONS        10


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


# define TEASCRIPT_USE_COLLECTION_VARIABLE_STORAGE     1

#include "teascript/Context.hpp"


#include <cstdlib> // EXIT_SUCCESS
#include <cstdio>
#include <iostream>
#include <chrono>


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



// FIXME: better use 'random' names and store them in table??
std::string make_name( int s, int v )
{
    return "var_" + std::to_string( s ) + "_" + std::to_string( v );
}

void setup( teascript::Context &c )
{
    // reset everything
    c = teascript::Context( teascript::TypeSystem() );
    
    for( int scope = 0; scope < BENCH_SCOPES; ++scope ) {

        for( int var_idx = 0; var_idx < BENCH_VARS_PER_SCOPE; ++var_idx ) {
           
            c.AddValueObject( make_name( scope, var_idx ), teascript::ValueObject( static_cast<long long>(scope) * var_idx, true ) );
        }

        c.EnterScope();
    }
    c.ExitScope(); // one too much.
}

double exec_lookup( teascript::Context &c )
{
    teascript::ValueObject val_res;
    unsigned long long res = 0;
    auto start = Now();
    // first current scope
    for( int i = 0; i < BENCH_OPERATIONS; ++i ) {
        val_res = c.FindValueObject( make_name( BENCH_SCOPES - 1, i ) );
        res += static_cast<unsigned long long>(val_res.GetValue<teascript::Integer>());
    }
#if 1
    // then global scope
    for( int i = 0; i < BENCH_OPERATIONS; ++i ) {
        val_res = c.FindValueObject( make_name( 0, i ) );
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
        val_res = c.RemoveValueObject( make_name( BENCH_SCOPES - 1, i ) );
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
        val_res = c.AddValueObject( make_name( BENCH_SCOPES - 1, BENCH_VARS_PER_SCOPE + i ), to_add );
        res += static_cast<unsigned long long>(val_res.GetValue<teascript::Integer>());
    }
    auto end = Now();

    std::cout << "value: " << res << std::endl;

    return CalcTimeInSecs( start, end );
}


double exec_set_copy( teascript::Context &c )
{
    teascript::ValueObject  copy_from( 1LL, true );
    teascript::ValueObject val_res;
    unsigned long long res = 0;
    auto start = Now();
    // only current scope for now
    for( int i = 0; i < BENCH_OPERATIONS; ++i ) {
        val_res = c.SetValue( make_name( BENCH_SCOPES - 1, i ), copy_from, false );
        res += static_cast<unsigned long long>(val_res.GetValue<teascript::Integer>());
    }
    auto end = Now();

    std::cout << "value: " << res << std::endl;

    return CalcTimeInSecs( start, end );
}


double exec_set_shared( teascript::Context &c )
{
    teascript::ValueObject  shared_with( 1LL, true );
    teascript::ValueObject val_res;
    unsigned long long res = 0;
    auto start = Now();
    // only current scope for now
    for( int i = 0; i < BENCH_OPERATIONS; ++i ) {
        val_res = c.SetValue( make_name( BENCH_SCOPES - 1, i ), shared_with, true );
        res += static_cast<unsigned long long>(val_res.GetValue<teascript::Integer>());
    }
    auto end = Now();

    std::cout << "value: " << res << std::endl;

    return CalcTimeInSecs( start, end );
}


int main()
{
    std::cout << std::fixed;
    std::cout << std::setprecision( 8 );

    std::cout << "Benchmarking TeaScript Variable Lookup, Remove and Set by directly use the Context class.\n";

    teascript::Context c;

    std::cout << "\nStart Test Lookup" << std::endl;
    for( int i = BENCH_ITERATIONS; i != 0; --i ) {
        setup( c );
        auto secs = exec_lookup( c );
        std::cout << "Calculation took: " << secs << " seconds." << std::endl;
    }

    std::cout << "\nStart Test Add" << std::endl;
    for( int i = BENCH_ITERATIONS; i != 0; --i ) {
        setup( c );
        auto secs = exec_add( c );
        std::cout << "Calculation took: " << secs << " seconds." << std::endl;
    }

    std::cout << "\nStart Test Set Assign" << std::endl;
    for( int i = BENCH_ITERATIONS; i != 0; --i ) {
        setup( c );
        auto secs = exec_set_copy( c );
        std::cout << "Calculation took: " << secs << " seconds." << std::endl;
    }

    std::cout << "\nStart Test Set SharedAssign" << std::endl;
    for( int i = BENCH_ITERATIONS; i != 0; --i ) {
        setup( c );
        auto secs = exec_set_shared( c );
        std::cout << "Calculation took: " << secs << " seconds." << std::endl;
    }

    std::cout << "\nStart Test Remove" << std::endl;
    for( int i = BENCH_ITERATIONS; i != 0; --i ) {
        setup( c );
        auto secs = exec_remove( c );
        std::cout << "Calculation took: " << secs << " seconds." << std::endl;
    }

    puts( "\n\nTest end." );

    return EXIT_SUCCESS;
}