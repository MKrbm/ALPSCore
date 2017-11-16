#include <alps/alea/mean.hpp>
#include <alps/alea/variance.hpp>
#include <alps/alea/covariance.hpp>
#include <alps/alea/autocorr.hpp>
#include <alps/alea/batch.hpp>

#include "gtest/gtest.h"
#include "dataset.hpp"

#include <iostream>

template <typename Acc>
class twogauss_setup
{
public:
    typedef Acc acc_type;
    typedef typename alps::alea::traits<Acc>::value_type value_type;

public:
    twogauss_setup()
        : acc_(2)
    {
        std::vector<value_type> curr(2);
        for (size_t i = 0; i != twogauss_count; ++i) {
            std::copy(twogauss_data[i], twogauss_data[i+1], curr.begin());
            acc_ << curr;
        }
    }

    const Acc &acc() const { return acc_; }

    Acc &acc() { return acc_; }

private:
    Acc acc_;
};


// MEAN

template <typename Acc>
class twogauss_mean_case
    : public ::testing::Test
    , public twogauss_setup<Acc>
{
public:
    typedef typename alps::alea::traits<Acc>::value_type value_type;

    twogauss_mean_case() : twogauss_setup<Acc>() { }

    void test()
    {
        std::vector<value_type> obs_mean = this->acc().mean();
        EXPECT_NEAR(obs_mean[0], twogauss_mean[0], 1e-6);
        EXPECT_NEAR(obs_mean[1], twogauss_mean[1], 1e-6);
    }
};

typedef ::testing::Types<
      alps::alea::mean_acc<double>
    , alps::alea::var_acc<double>
    , alps::alea::cov_acc<double>
    , alps::alea::autocorr_acc<double>
    , alps::alea::batch_acc<double>
    > has_mean;

TYPED_TEST_CASE(twogauss_mean_case, has_mean);
TYPED_TEST(twogauss_mean_case, test) { this->test(); }


// VARIANCE

template <typename Acc>
class twogauss_var_case
    : public ::testing::Test
    , public twogauss_setup<Acc>
{
public:
    typedef typename alps::alea::traits<Acc>::var_type var_type;

    twogauss_var_case() : twogauss_setup<Acc>() { }

    void test()
    {
        std::vector<var_type> obs_var = this->acc().result().var();
        EXPECT_NEAR(obs_var[0], twogauss_var[0], 1e-6);
        EXPECT_NEAR(obs_var[1], twogauss_var[1], 1e-6);
    }
};

typedef ::testing::Types<
      alps::alea::var_acc<double>
    , alps::alea::cov_acc<double>
    //, alps::alea::autocorr_acc<double>
    //, alps::alea::batch_acc<double>
    > has_var;

TYPED_TEST_CASE(twogauss_var_case, has_var);
TYPED_TEST(twogauss_var_case, test) { this->test(); }


// int main(int argc, char **argv)
// {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }
