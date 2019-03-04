#ifndef SEGYIO_MATCHERS_HPP
#define SEGYIO_MATCHERS_HPP

#include <catch/catch.hpp>

#include <string>
#include <vector>

namespace {

/*
 * Approx-range, but works with arbitrary types, not just float ~= float
 */
template < typename T >
class SimilarRange : public Catch::MatcherBase< std::vector< T > > {
public:
    explicit SimilarRange(const std::vector< T >& xs) :
        lhs(xs)
    {}

    virtual bool match(const std::vector< T >& xs) const override {
        if (xs.size() != lhs.size()) return false;

        for (std::size_t i = 0; i < xs.size(); ++i)
            if (xs[ i ] != Approx(this->lhs[ i ])) return false;

        return true;
    }

    template < typename U >
    static SimilarRange from(const std::vector< U >& xs) {
        std::vector< T > v(xs.begin(), xs.end());
        return SimilarRange(v);
    }

    virtual std::string describe() const override {
        using str = Catch::StringMaker< std::vector< T > >;
        return "~= " + str::convert(this->lhs);
    }

private:
    std::vector< T > lhs;
};

struct ApproxRange : public SimilarRange< float > {
    using SimilarRange< float >::SimilarRange;

    template < typename U >
    static SimilarRange from(const std::vector< U >& ) = delete;
};

}

#endif // SEGYIO_MATCHERS_HPP
