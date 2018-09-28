#ifndef SEGYIO_MATCHERS_HPP
#define SEGYIO_MATCHERS_HPP

#include <catch/catch.hpp>

#include <string>
#include <vector>

namespace {

class ApproxRange : public Catch::MatcherBase< std::vector< float > > {
    public:
        explicit ApproxRange( const std::vector< float >& xs ) :
            lhs( xs )
        {}

        virtual bool match( const std::vector< float >& xs ) const override {
            if( xs.size() != lhs.size() ) return false;

            for( std::size_t i = 0; i < xs.size(); ++i )
                if( xs[ i ] != Approx(this->lhs[ i ]) ) return false;

            return true;
        }

        virtual std::string describe() const override {
            using str = Catch::StringMaker< std::vector< float > >;
            return "~= " + str::convert( this->lhs );
        }

    private:
        std::vector< float > lhs;
};

}

#endif // SEGYIO_MATCHERS_HPP
