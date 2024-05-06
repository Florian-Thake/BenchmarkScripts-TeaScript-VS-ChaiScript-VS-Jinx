/*
 * SPDX-FileCopyrightText: Copyright (C) 2024 Florian Thake, <contact |at| tea-age.solutions>.
 * SPDX-License-Identifier: MIT
 */


// Benchmarking overhead of Buffer manipulating TeaScript code VS ChaiScript VS TeaScript CoreLibrary VS C++
// 
// doing this by filling a rgb(a) buffer in FUll HD resolution (1920 x 1080) or in UHD (3840 x 2160)


// === BENCH CONFIG ===

#define BENCH_IMAGE_FHD         1                   // option to use Full HD resolution (1920 x 1080)
#define BENCH_IMAGE_UHD         2                   // option to use UHD resolution (3840 x 2160)
#define BENCH_IMAGE_RESOLUTION  BENCH_IMAGE_FHD     // decide which resolution shall be used for the benchmark.

#if BENCH_IMAGE_RESOLUTION ==  BENCH_IMAGE_FHD      // Full HD
#define BENCH_IMAGE_WIDTH   1920 
#define BENCH_IMAGE_HEIGHT  1080
#elif BENCH_IMAGE_RESOLUTION ==  BENCH_IMAGE_UHD    // UHD
#define BENCH_IMAGE_WIDTH   3840 
#define BENCH_IMAGE_HEIGHT  2160
#else
#error Wrong resolution configuration!
#endif

#define BENCH_ITERATIONS   3                    // loop count for each test.

#define BENCH_ENABLE_TEACODE         1
#define BENCH_ENABLE_TEA_COMPILE     1          // only possible with version >= 0.14
#define BENCH_ENABLE_CHAI            1
#define BENCH_ENABLE_CORE_LIB        1
#define BENCH_ENABLE_CORE_LIB_FUNC   1
#define BENCH_ENABLE_CPP             1


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


#include <teascript/Engine.hpp>
// check version if new enough (Buffer exists at all...)
#if TEASCRIPT_VERSION < TEASCRIPT_BUILD_VERSION_NUMBER(0,13,0)
# error Use TeaScript 0.13.0 or newer
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
# include <chaiscript/chaiscript.hpp>
# if defined( _MSC_VER )
#  pragma warning( pop )
# endif
#endif


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


constexpr char tea_code_prepare[] = R"_SCRIPT_(
is_defined make_rgb or (func make_rgb( r, g, b ) { r bit_lsh 16 bit_or g bit_lsh 8 bit_or b })

const green  := make_rgb( 0, 255, 0 ) as u64

const size   := width * height * 4

def buf := _buf( size )
_buf_fill( buf, 0, -1, 0u8 )
)_SCRIPT_";

constexpr char tea_code_test[] = R"_SCRIPT_(
forall( pixel in _seq( 0, width*height - 1, 1) ) {
    _buf_set_u32( buf, pixel * 4, green )
}
_buf_size( buf ) // return sth...
)_SCRIPT_";


// we use our own engine for get access to the low level parts.
class MyEngine : public teascript::Engine
{
public:
    MyEngine( ) : teascript::Engine( teascript::config::util() ) {}
#if TEASCRIPT_VERSION >= TEASCRIPT_BUILD_VERSION_NUMBER(0,14,0)
    inline teascript::Parser &GetParser() noexcept { return mBuildTools->mParser; }
    inline teascript::Parser const &GetParser() const noexcept { return mBuildTools->mParser; }
#else
    inline teascript::Parser &GetParser() noexcept { return mParser; }
    inline teascript::Parser const &GetParser() const noexcept { return mParser; }
#endif
    inline teascript::Context &GetContext() noexcept { return mContext; }
    inline teascript::Context const &GetContext() const noexcept { return mContext; }
};

double exec_tea()
{
    MyEngine  engine;

    engine.AddConst( "width", BENCH_IMAGE_WIDTH );
    engine.AddConst( "height", BENCH_IMAGE_HEIGHT );
    engine.ExecuteCode( tea_code_prepare );
    auto ast = engine.GetParser().Parse( tea_code_test );
    try {
        auto start  = Now();
        auto teares = ast->Eval( engine.GetContext() );
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

#if TEASCRIPT_VERSION >= TEASCRIPT_BUILD_VERSION_NUMBER(0,14,0) && BENCH_ENABLE_TEA_COMPILE
double exec_tea_compile()
{
    teascript::Engine  engine;

    engine.AddConst( "width", BENCH_IMAGE_WIDTH );
    engine.AddConst( "height", BENCH_IMAGE_HEIGHT );
    engine.ExecuteCode( tea_code_prepare );
    auto prog = engine.CompileCode( tea_code_test, teascript::eOptimize::O2 );
    try {
        auto start  = Now();
        auto teares = engine.ExecuteProgram( prog );
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
#endif

#if BENCH_ENABLE_CHAI
constexpr char chai_code[] = R"_SCRIPT_(
var size = width * height - 1;
for( var pixel = 0; pixel < size; ++pixel ) {
    _buf_set_u32( buf, pixel * 4, green );
}
buf.size(); // return sth ...
)_SCRIPT_";

bool BufSetU32_Cpp( std::vector<unsigned char> &rBuffer, size_t const pos, unsigned long long const val );

double exec_chai()
{
    auto make_rgb = []( unsigned char r, unsigned char g, unsigned char b ) { return static_cast<unsigned int>(r) * 256 * 256 + static_cast<unsigned int>(g) * 256 + b; };
    auto const green  = static_cast<unsigned long long>( make_rgb( 0, 255, 0 ) );
    auto const width  = BENCH_IMAGE_WIDTH;
    auto const height = BENCH_IMAGE_HEIGHT;
    auto const size   = width * height * 4;

    chaiscript::ChaiScript chai;
    std::vector<unsigned char>  buf( size );
    chai.add( chaiscript::bootstrap::standard_library::vector_type<std::vector<unsigned char>>( "Buffer" ) );
    chai.add( chaiscript::fun( BufSetU32_Cpp ), "_buf_set_u32" );
    chai.add( chaiscript::var( buf ), "buf" );
    chai.add( chaiscript::const_var( width ), "width" );
    chai.add( chaiscript::const_var( height ), "height" );
    chai.add( chaiscript::const_var( green ), "green" );
    auto ast = chai.parse( chai_code );
    try {
        auto start = Now();
        auto chres = chai.eval( *ast );
        auto end = Now();

        std::cout << "value: " << chaiscript::boxed_cast<size_t>(chres) << std::endl;

        return CalcTimeInSecs( start, end );

    } catch( chaiscript::Boxed_Value const &bv ) {
        puts( chaiscript::boxed_cast<chaiscript::exception::eval_error const &>(bv).what() );
    } catch( std::exception const &ex ) {
        puts( ex.what() );
    }

    return -1.0;
}
#endif

double exec_core()
{
    auto make_rgb = []( unsigned char r, unsigned char g, unsigned char b ) { return static_cast<unsigned int>(r) * 256 * 256 + static_cast<unsigned int>(g) * 256 + b; };
    auto const green  = make_rgb( 0, 255, 0 );
    auto const width  = BENCH_IMAGE_WIDTH;
    auto const height = BENCH_IMAGE_HEIGHT;
    auto const size   = width * height * 4;
    
    auto buf = teascript::CoreLibrary::MakeBuffer( teascript::ValueObject( static_cast<teascript::U64>(size) ) );
    teascript::CoreLibrary::BufFill( buf, teascript::ValueObject( 0LL ), teascript::ValueObject( -1LL ), 0 );
    try {
        auto start  = Now();

        for( size_t pixel = 0; pixel < width * height - 1; ++pixel ) {
            teascript::CoreLibrary::BufSetU32( buf, teascript::ValueObject( static_cast<teascript::U64>(pixel * 4) ), static_cast<teascript::U64>(green) );
        }
        
        auto teares = teascript::ValueObject( teascript::CoreLibrary::BufSize( buf ) );
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



#define EXEC_CORE_FUNCS_ALWAYS_NEW_VECTOR    0


double exec_core_funcs()
{
    teascript::Context c;
    teascript::CoreLibrary().Bootstrap( c, teascript::config::util() );
    auto make_rgb = []( unsigned char r, unsigned char g, unsigned char b ) { return static_cast<unsigned int>(r) * 256 * 256 + static_cast<unsigned int>(g) * 256 + b; };
    auto const green  = make_rgb( 0, 255, 0 );
    auto const width  = BENCH_IMAGE_WIDTH;
    auto const height = BENCH_IMAGE_HEIGHT;
    auto const size   = width * height * 4;

    auto val_buf = teascript::ValueObject( teascript::CoreLibrary::MakeBuffer( teascript::ValueObject( static_cast<teascript::U64>(size) ) ), teascript::ValueConfig( true ) );
    auto &buf = val_buf.GetValue<teascript::Buffer>();
    teascript::CoreLibrary::BufFill( buf, teascript::ValueObject( 0LL ), teascript::ValueObject( -1LL ), 0 );

    
    auto val_green = teascript::ValueObject( static_cast<teascript::U64>(green), teascript::ValueConfig( true ) );

    // get function object
    auto f_buf_set_u32 = c.FindValueObject( "_buf_set_u32" );

    try {
        auto start = Now();

#if !EXEC_CORE_FUNCS_ALWAYS_NEW_VECTOR
        std::vector< teascript::ValueObject> params;
        params.reserve( 3 );
        params.push_back( val_buf );
        params.push_back( teascript::ValueObject( teascript::U64{}, teascript::ValueConfig( true ) ) );
        params.push_back( val_green );
#endif

        for( size_t pixel = 0; pixel < width * height - 1; ++pixel ) {
#if EXEC_CORE_FUNCS_ALWAYS_NEW_VECTOR
            std::vector< teascript::ValueObject> params;
            params.reserve( 3 );
            params.push_back( val_buf );
            params.push_back( teascript::ValueObject( static_cast<teascript::U64>(pixel * 4), teascript::ValueConfig( true ) ) );
            params.push_back( val_green );
#else
            params[1].AssignValue( static_cast<teascript::U64>(pixel * 4) );
#endif
            f_buf_set_u32.GetValue<teascript::FunctionPtr>()->Call( c, params, {} );
        }

        auto teares = teascript::ValueObject( teascript::CoreLibrary::BufSize( buf ) );
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


// writing unsigned 32 bit data in host byte order into the buffer. mimic the CoreLibrary behavior but without TeaScript types.
bool BufSetU32_Cpp( std::vector<unsigned char> &rBuffer, size_t const pos, unsigned long long const val )
{
    if( val > std::numeric_limits<std::uint32_t>::max() ) {
        return false;
    }
    auto const valu32 = static_cast<std::uint32_t>(val);

    if( pos > rBuffer.size() ) {
        return false;
    }
    auto const wanted = sizeof( valu32 );
    if( std::numeric_limits<size_t>::max() - wanted < pos ) { // overflow protection
        return false;
    }
    if( pos + wanted > rBuffer.capacity() ) {
        return false;
    }
    // grow?
    if( pos + wanted > rBuffer.size() ) {
        rBuffer.resize( pos + wanted );
    }

    ::memcpy( rBuffer.data() + pos, &valu32, sizeof( valu32 ) );

    return true;
}


#define EXEC_CPP_NO_CHECKS_AND_INLINE    0

double exec_cpp()
{
    auto make_rgb = []( unsigned char r, unsigned char g, unsigned char b ) { return static_cast<unsigned int>(r) * 256 * 256 + static_cast<unsigned int>(g) * 256 + b; };
    auto const green  = make_rgb( 0, 255, 0 );
    auto const width  = BENCH_IMAGE_WIDTH;
    auto const height = BENCH_IMAGE_HEIGHT;
    auto const size   = width * height * 4;

    std::vector<unsigned char>  buffer( size );

    try {
        auto start = Now();
        for( size_t pixel = 0; pixel < width * height - 1; ++pixel ) {
#if EXEC_CPP_NO_CHECKS_AND_INLINE
            ::memcpy( buffer.data() + pixel * 4, &green, sizeof( green ) );
#else
            BufSetU32_Cpp( buffer, pixel * 4, static_cast<unsigned long long>(green) );
#endif
        }

        auto res = buffer.size();
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

    std::cout << "Benchmarking TeaScript Buffer Overhead.\n";
    std::cout << "using image resolution: " << BENCH_IMAGE_WIDTH << " x " << BENCH_IMAGE_HEIGHT << std::endl;

#if BENCH_ENABLE_TEACODE
    std::cout << "\nStart Test TeaScript" << std::endl;
    for( int i = BENCH_ITERATIONS; i != 0; --i ) {
        auto secs = exec_tea();
        std::cout << "Calculation took: " << secs << " seconds." << std::endl;
    }
#endif

#if BENCH_ENABLE_TEA_COMPILE
#if TEASCRIPT_VERSION >= TEASCRIPT_BUILD_VERSION_NUMBER( 0, 14, 0 )
    std::cout << "\nStart Test TeaScript in TeaStackVM" << std::endl;
    for( int i = BENCH_ITERATIONS; i != 0; --i ) {
        auto secs = exec_tea_compile();
        std::cout << "Calculation took: " << secs << " seconds." << std::endl;
    }
#else
    std::cout << "TeaScript version is too old for test in TeaStackVM. Test skipped. " << std::endl;
#endif
#endif

#if BENCH_ENABLE_CHAI
    std::cout << "\nStart Test ChaiScript" << std::endl;
    for( int i = BENCH_ITERATIONS; i != 0; --i ) {
        auto secs = exec_chai();
        std::cout << "Calculation took: " << secs << " seconds." << std::endl;
    }
#endif

#if BENCH_ENABLE_CORE_LIB
    std::cout << "\nStart Test CoreLibrary" << std::endl;
    for( int i = BENCH_ITERATIONS; i != 0; --i ) {
        auto secs = exec_core();
        std::cout << "Calculation took: " << secs << " seconds." << std::endl;
    }
#endif

#if BENCH_ENABLE_CORE_LIB_FUNC
    std::cout << "\nStart Test CoreLibrary w. FuncObj" << std::endl;
    for( int i = BENCH_ITERATIONS; i != 0; --i ) {
        auto secs = exec_core_funcs();
        std::cout << "Calculation took: " << secs << " seconds." << std::endl;
    }
#endif

#if BENCH_ENABLE_CPP
    std::cout << "\nStart Test pure C++" << std::endl;
    for( int i = BENCH_ITERATIONS; i != 0; --i ) {
        auto secs = exec_cpp();
        std::cout << "Calculation took: " << secs << " seconds." << std::endl;
    }
#endif

    puts( "\n\nTest end." );

    return EXIT_SUCCESS;
}