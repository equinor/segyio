#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>
#include <vector>

#include <catch/catch.hpp>

#include <segyio/segy.h>
#include <segyio/segyio.hpp>

#include "matchers.hpp"

SCENARIO( "constructing a filehandle", "[c++]" ) {
    GIVEN( "a closed filehandle" ) {

        sio::simple_file f;
        REQUIRE( !f.is_open() );

        WHEN( "closing the file" ) {
            f.close();

            THEN( "the file is closed" ) {
                CHECK( !f.is_open() );
            }
        }

        WHEN( "the filehandle is unchanged" ) {

                CHECK( !f.is_open() );
            THEN( "the size should be zero" ) {
                CHECK( f.size() == 0 );
            }

            THEN( "it can be copied" ) {
                auto g = f;
                CHECK( g.size() == f.size() );
            }

            THEN( "it can be moved-assigned" ) {
                auto g = std::move( f );
                CHECK( g.size() == 0 );

            }

            THEN( "it can be move-constructed" ) {
                sio::simple_file g( std::move( f ) );
                CHECK( g.size() == 0 );
            }

            THEN( "it can be return-val moved" ) {
                auto x = [] { 
                    return sio::simple_file{};
                } ();

                CHECK( x.size() == 0 );
            }
        }

        WHEN( "opening a non-existing wrong path" ) {
            sio::config c;
            THEN( "opening read-only fails" ) {
                CHECK_THROWS_AS( f.open( "garbage", c.readonly()),
                                 std::runtime_error );

                AND_THEN( "the file remains closed" )
                    CHECK( !f.is_open() );
            }

            THEN( "opening writable fails" ) {
                CHECK_THROWS_AS( f.open( "garbage", c.readwrite() ),
                                 std::runtime_error );

                AND_THEN( "the file remains closed" )
                    CHECK( !f.is_open() );
            }

            THEN( "opening truncating fails (on parse)" ) {
                // TODO: move-to tmpdir
                CHECK_THROWS_AS( f.open( "garbage", c.truncate() ),
                                 std::runtime_error );

                AND_THEN( "the file remains closed" )
                    CHECK( !f.is_open() );
            }
        }
    }
}

namespace {

struct tracegen {
    tracegen( double seed ) : n( seed - 0.00001 ) {}
    double operator()() { return this->n += 0.00001; };
    double n;
};

template< typename T >
std::vector< T > genrange( int len, T seed ) {
    std::vector< T > x( len );
    std::generate( x.begin(), x.end(), tracegen( seed ) );
    return x;
}

}

SCENARIO( "reading a single trace", "[c++]" ) {
    GIVEN( "an open file" ) {
        sio::simple_file f( "test-data/small.sgy" );

        WHEN( "the filehandle is unchanged" ) {
            THEN( "the file is open" ) {
                CHECK( f.is_open() );
            }

            THEN( "size matches tracecount" ) {
                CHECK( f.size() == 25 );
            }
        }

        WHEN( "closing the file" ) {
            f.close();

            THEN( "the filehandle should be closed" )
                CHECK( !f.is_open() );

            THEN( "the size should be zero" )
                CHECK( f.size() == 0 );
        }

        WHEN( "fetching a trace as default type" ) {
            const auto x = f.read(0);

            const auto expected = genrange( x.size(), 1.2 );
            THEN( "the data should be correct" ) {
                CHECK_THAT( x, ApproxRange( expected ) );
            }
        }

        WHEN( "fetching a trace as float" ) {
            // inline 2, crossline 21
            const auto x = f.read< float >( 6 );

            const auto expected = genrange( x.size(), 2.21f );
            THEN( "the data should be correct" ) {
                CHECK_THAT( x, ApproxRange( expected ) );
            }
        }

        WHEN( "reading into a too small vector" ) {
            std::vector< double > x( 10 );
            f.read( 6, x );

            THEN( "the buffer should be resized" ) {
                CHECK( x.size() == 50 );

                const auto expected = genrange( x.size(), 2.21 );
                AND_THEN( "the data should be correct" ) {
                    CHECK_THAT( x, ApproxRange( expected ) );
                }
            }
        }

        WHEN( "reading into an array" ) {
            std::array< float, 50 > x;
            f.read( 0, x.begin() );

            const auto expected = genrange( x.size(), 1.2f );
            THEN( "the data should be correct" ) {
                std::vector< float > xs( x.begin(), x.end() );
                CHECK_THAT( xs, ApproxRange( expected ) );
            }
        }

        WHEN( "reading into a back-insert'd vector" ) {
            std::vector< double > x;
            REQUIRE( x.empty() );

            f.read( 0, std::back_inserter( x ) );

            THEN( "the resulting vector should be resized" ) {
                CHECK( x.size() == 50 );

                AND_THEN( "the data should be correct" ) {
                    const auto expected = genrange( x.size(), 1.2 );
                    CHECK_THAT( x, ApproxRange( expected ) );
                }
            }
        }

        WHEN( "reading a trace outside the file" ) {
            THEN( "the read fails" ) {
                CHECK_THROWS_AS( f.read( f.size() ), std::out_of_range );

                AND_THEN( "size should be unchanged" )
                    CHECK( f.size() == 25 );

                AND_THEN( "file should still be open" )
                    CHECK( f.is_open() );

                AND_THEN( "a following valid read should be correct" ) {
                    const auto expected = genrange( 50, 1.2 );
                    CHECK_THAT( f.read( 0 ), ApproxRange( expected ) );
                }
            }
        }
    }

    GIVEN( "a closed file" ) {
        sio::simple_file f;

        WHEN( "reading a trace" ) {
            THEN( "the read should fail" ) {
                CHECK_THROWS_AS( f.read( 10 ), std::runtime_error );
            }
        }

        WHEN( "reading a trace out of range" ) {
            THEN( "the read should fail on being closed" ) {
                CHECK_THROWS_AS( f.read( 100 ), std::runtime_error );
            }
        }
    }
}

namespace {

std::string copyfile( const std::string& src, std::string dst ) {
    std::ifstream source( src, std::ios::binary );
    std::ofstream dest( dst, std::ios::binary | std::ios::trunc );
    dest << source.rdbuf();

    return dst;
}

}

SCENARIO( "writing a single trace", "[c++]" ) {
    GIVEN( "an open file" ) {
        auto filename = copyfile( "test-data/small.sgy",
                                  "c++-small-write-single.sgy" );

        sio::config c;
        sio::simple_file f( filename, c.readwrite() );

        WHEN( "filling the the first trace with zero" ) {
            std::vector< double > zeros( 50, 0 );
            f.put( 0, zeros );

            THEN( "getting it should produce zeros" ) {
                CHECK_THAT( f.read( 0 ), ApproxRange( zeros ) );
            }
        }

        WHEN( "writing a short trace" ) {
            auto orig = f.read( 0 );
            std::vector< double > zeros( 5, 0 );

            THEN( "the put should fail" ) {
                CHECK_THROWS_AS( f.put( 0, zeros ), std::length_error );

                AND_THEN( "the file should be unchanged" ) {
                    CHECK_THAT( f.read( 0 ), ApproxRange( orig ) );
                }
            }
        }

        WHEN( "writing a long trace" ) {
            auto orig = f.read( 0 );
            std::vector< double > zeros( 500, 0 );

            THEN( "the put should fail" ) {
                CHECK_THROWS_AS( f.put( 0, zeros ), std::length_error );

                AND_THEN( "the file should be unchanged" ) {
                    CHECK_THAT( f.read( 0 ), ApproxRange( orig ) );
                }
            }
        }

        WHEN( "reading a trace outside the file" ) {

            std::vector< double > zeros( 50, 0 );

            THEN( "the write fails" ) {
                CHECK_THROWS_AS( f.put( f.size(), zeros ), std::out_of_range );

                AND_THEN( "size should be unchanged" )
                    CHECK( f.size() == 25 );

                AND_THEN( "file should still be open" )
                    CHECK( f.is_open() );

                AND_THEN( "a following valid read should be correct" ) {
                    const auto expected = genrange( 50, 1.2 );
                    CHECK_THAT( f.read( 0 ), ApproxRange( expected ) );
                }
            }
        }
    }
}

SCENARIO( "reading a single inline", "[c++]" ) {
    GIVEN( "an open file" ) {
        sio::simple_file f( "test-data/small.sgy" );

        auto reference = [&f] {
            std::vector< double > reference( 50 * 5 );

            auto itr = reference.begin();
            for( int i = 0; i < 5; ++i )
                itr = f.read( i, itr );

            return reference;
        }();

        WHEN( "reading the first inline" ) {
            auto x = f.get_iline( 1 );

            THEN( "the data should be correct" ) {
                CHECK_THAT( x, ApproxRange( reference ) );
            }
        }
    }
}
