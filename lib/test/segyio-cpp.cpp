#include <catch/catch.hpp>
#include "matchers.hpp"

#include <segyio/segyio.hpp>

using namespace segyio;
using namespace segyio::literals;

TEST_CASE( "strong typedef string same has noexcept as string", "[c++]" ) {
    auto lhs = "lhs"_path;
    auto rhs = "rhs"_path;
    std::string strlhs = "lhs";
    std::string strrhs = "rhs";

    using std::swap;

    auto str_noexcept  = noexcept( swap( strlhs, strrhs ) );
    auto path_noexcept = noexcept( swap( lhs, rhs ) );
    CHECK( str_noexcept == path_noexcept );
}

TEST_CASE( "strong typedef int swap is noexcept", "[c++]" ) {
    segyio::ilbyte lhs;
    segyio::ilbyte rhs;
    auto is_noexcept = noexcept( std::swap( lhs, rhs ) );
    CHECK( is_noexcept );
}

TEST_CASE( "unstructured throws on non-existing paths", "[c++]" ) {
    CHECK_THROWS( unstructured( "garbage"_path ) );
}

struct Unstructured {
    Unstructured() : f( "test-data/small.sgy"_path ) {}
    unstructured f;
};

TEST_CASE_METHOD( Unstructured,
                  "unstructured file is copyable",
                  "[c++]" ) {
    const auto copyable = std::is_copy_constructible< decltype(f) >::value;
    CHECK( copyable );
}

TEST_CASE_METHOD( Unstructured,
                  "unstructured file is movable",
                  "[c++]" ) {
    const auto movable = std::is_move_constructible< decltype(f) >::value;
    CHECK( movable );
}

TEST_CASE_METHOD( Unstructured,
                  "unstructured reads correct trace metadata",
                  "[c++]" ) {
    const auto samplecount = 50;
    const auto format = fmt::ibm();
    const auto trace0 = SEGY_TEXT_HEADER_SIZE + SEGY_BINARY_HEADER_SIZE;
    const auto tracesize = samplecount * 4;
    const auto tracecount = 25;

    CHECK( f.samplecount() == samplecount );
    CHECK( f.format()      == format );
    CHECK( f.trace0()      == trace0 );
    CHECK( f.tracesize()   == tracesize );
    CHECK( f.tracecount()  == tracecount );
}

TEST_CASE_METHOD( Unstructured,
                  "unstructured can read an arbitrary trace",
                  "[c++]" ) {
    const auto samplecount = 50;

    std::vector< float > out;
    f.get( 0, std::back_inserter( out ) );

    CHECK( out.size()  == samplecount );
    CHECK( out.at( 0 ) == Approx( 1.20 ) );
}

TEST_CASE_METHOD( Unstructured,
                  "unstructured can get trace headers",
                  "[c++]" ) {
    auto x = f.get_th( 0 );
    auto y = f.get_th( 1 );
    auto z = f.get_th( 5 );

    CHECK( x.iline == 1 );
    CHECK( y.iline == 1 );
    CHECK( z.iline == 2 );

    CHECK( x.xline == 20 );
    CHECK( y.xline == 21 );
    CHECK( z.xline == 20 );
}

TEST_CASE_METHOD( Unstructured,
                  "unstructured can get binary header",
                  "[c++]" ) {
    auto x = f.get_bin();

    CHECK( x.format == 1 );
    CHECK( x.samples == 50 );
    CHECK( x.interval == 4000 );
    CHECK( x.traces == 25);
}

struct move_only {
    using filetype = basic_unstructured< disable_copy, open_status >;
    filetype f;
    move_only() : f( "test-data/small.sgy"_path ) {}
};

TEST_CASE_METHOD( move_only,
                  "basic move-only file is non-copyable",
                  "[c++]" ) {
    auto copyable = std::is_copy_constructible< decltype(f) >::value;
    auto movable  = std::is_move_constructible< decltype(f) >::value;
    CHECK_FALSE( copyable );
    CHECK( movable );
}

TEST_CASE_METHOD( move_only,
                  "a moved-assigned-from file is closed",
                  "[c++]" ) {
    decltype(f) dst = std::move( f );
    CHECK(  !f.is_open() );
    CHECK( dst.is_open() );
}

TEST_CASE_METHOD( move_only,
                  "a moved-constructed-from file is closed",
                  "[c++]" ) {
    decltype(f) dst( std::move( f ) );
    CHECK(  !f.is_open() );
    CHECK( dst.is_open() );
}

struct Closable {
    using filetype = basic_unstructured< open_status, closable >;
    filetype f;
    Closable() : f( "test-data/small.sgy"_path ) {}
};

TEST_CASE_METHOD( Closable,
                  "closable file can bve closed",
                  "[c++]" ) {
    REQUIRE( f.is_open() );

    f.close();
    CHECK( !f.is_open() );
}

TEST_CASE_METHOD( Closable,
                  "copying-and-closing leaves other intact",
                  "[c++]" ) {
    auto g = f;
    REQUIRE( f.is_open() );
    REQUIRE( g.is_open() );

    SECTION( "closing copy leaves original intact" ) {
        g.close();
        CHECK(  f.is_open() );
        CHECK( !g.is_open() );
    }

    SECTION( "closing original leaves copy intact" ) {
        f.close();
        CHECK(  g.is_open() );
        CHECK( !f.is_open() );
    }
}

struct Openable {
    using filetype = basic_unstructured< open_status, openable >;
    filetype f;
};

TEST_CASE_METHOD( Openable,
                  "open can be deferred",
                  "[c++]" ) {
    REQUIRE( !f.is_open() );

    f.open( "test-data/small.sgy"_path );
    CHECK( f.is_open() );
}

TEST_CASE( "file can be opened as write_always", "[c++]" ) {
    using F = basic_unstructured< write_always >;
    F f( "test-data/small.sgy"_path );
}

TEST_CASE( "default constructibility can be disabled", "[c++]" ) {
    using F = basic_unstructured< disable_default >;
    constexpr auto default_ctor = std::is_default_constructible< F >::value;
    CHECK_FALSE( default_ctor );
}

TEST_CASE( "bounds-checked throws on out-of-range trace access", "[c++]" ) {
    using F = basic_unstructured< trace_bounds_check >;
    F f( "test-data/small.sgy"_path );

    std::vector< float > out;
    CHECK_THROWS_AS( f.get( 25, std::back_inserter( out ) ),
                     std::out_of_range );

    CHECK_THROWS_AS( f.get( 1000, std::back_inserter( out ) ),
                     std::out_of_range );

    CHECK_THROWS_AS( f.get( -1, std::back_inserter( out ) ),
                     std::out_of_range );
}

struct Writable {
    unstructured_writable f;
    Writable() : f( "test-data/small-w.sgy"_path,
                    config{}.with(mode::readwrite()) )
    {}
};

TEST_CASE_METHOD( Writable,
                  "unstructured_writable can put trace",
                  "[c++]" ) {

    std::vector< float > in( 50 );
    for( std::size_t i = 0; i < in.size(); ++i ) in[i] = i;
    f.put( 0, in.begin() );

    std::vector< float > out;
    f.get( 0, std::back_inserter( out ) );

    CHECK_THAT( out, ApproxRange( in ) );
}

struct Volume {
    basic_volume<> f;
    Volume() : f( "test-data/small.sgy"_path ) {}
};

TEST_CASE_METHOD( Volume,
                  "volume reads correct metadata from file",
                  "[c++]" ) {
    const auto inlines = 5;
    const auto crosslines = 5;
    const auto offsets = 1;
    const auto sorting = segyio::sorting::iline();

    CHECK( f.inlinecount()      == inlines );
    CHECK( f.crosslinecount()   == crosslines );
    CHECK( f.offsetcount()      == offsets );
    CHECK( f.sorting()          == sorting );
}
