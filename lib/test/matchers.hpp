#include <vector>

#include <catch/catch.hpp>

namespace {

template< typename T >
class ApproxRangeMatcher : public Catch::MatcherBase< std::vector< T > > {
    public:
        explicit ApproxRangeMatcher( const std::vector< T >& xs ) :
            lhs( xs )
        {}

        virtual bool match( const std::vector< T >& xs ) const override {
            if( xs.size() != lhs.size() ) return false;

            for( size_t i = 0; i < xs.size(); ++i )
                if( xs[ i ] != Approx(this->lhs[ i ]) ) return false;

            return true;
        }

        virtual std::string describe() const override {
            using str = Catch::StringMaker< std::vector< T > >;
            return "~= " + str::convert( this->lhs );
        }

    private:
        std::vector< T > lhs;
};

template< typename T >
ApproxRangeMatcher< T > ApproxRange( const std::vector< T >& xs ) {
    return ApproxRangeMatcher< T >( xs );
}

}
