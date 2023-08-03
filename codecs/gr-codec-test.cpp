#include <gtest/gtest.h>
#include <codecs/gr-codec.hpp>
#include <random>

// To compile: g++-11 -ggdb -g3 -I ~/include/ -L ~/lib/ gr-codec-test.cpp -o gr-codec-test -lsdsl -lgtest
namespace grcodec {
    namespace test {

        class N3SequencesDataSet : public ::testing::Test {
            protected:
                // Attributes:
                static const std::size_t    N_ENTRIES = 10,
                                            DIMENSIONS = 3;
                using Type = std::uint16_t;

                std::array<std::queue<Type>,DIMENSIONS> s1,s2,s3;

                const std::size_t K[4] = {2,3,4,5};

                // Constructor and Destructor:
                N3SequencesDataSet(){
                    std::random_device rd;
                    std::mt19937_64 gen(rd());
                    std::uniform_int_distribution<Type> dist(0,N_ENTRIES-1);

                    for (std::size_t i = 0; i < N_ENTRIES; ++i) {
                        for (std::size_t j = 0; j < N_ENTRIES; ++j) {
                            for (std::size_t k = 0; k < N_ENTRIES; ++k) {
                                s1[0].push(i);
                                s1[1].push(j);
                                s1[2].push(k);

                                s2[0].push(N_ENTRIES - i - 1);
                                s2[1].push(N_ENTRIES - j - 1);
                                s2[2].push(N_ENTRIES - k - 1);

                                s3[0].push(dist(gen));
                                s3[1].push(dist(gen));
                                s3[2].push(dist(gen));
                            }       
                        }   
                    }
                    
                }
                ~N3SequencesDataSet() override {}
                // Methods:
                void SetUp() override {}
                void TearDown() override {}
                static bool are_equal( std::queue<Type> &v1, std::queue<Type> &v2 ) {
                    bool ans = v1.size() == v2.size();
                    while( ans && !v1.empty() && !v2.empty() ) {
                        ans &= v1.front() == v2.front();
                        v1.pop();
                        v2.pop();
                    }
                    return ans;
                }
        };
        

        // k = 2
        TEST_F(N3SequencesDataSet,Asc_k2) {
            for (std::size_t i = 0; i < DIMENSIONS; ++i) {
                sdsl::bit_vector bv = samg::grcodec::RiceRuns<Type>::encode(s1[i],K[0]);
                std::queue<Type> ans = samg::grcodec::RiceRuns<Type>::decode(bv,K[0]);
                EXPECT_TRUE(are_equal(ans,s1[i]));
            }
        }

        TEST_F(N3SequencesDataSet,Desc_k2) {
            for (std::size_t i = 0; i < DIMENSIONS; ++i) {
                sdsl::bit_vector bv = samg::grcodec::RiceRuns<Type>::encode(s2[i],K[0]);
                std::queue<Type> ans = samg::grcodec::RiceRuns<Type>::decode(bv,K[0]);
                EXPECT_TRUE(are_equal(ans,s2[i]));
            }
        }

        TEST_F(N3SequencesDataSet,Rand_k2) {
            for (std::size_t i = 0; i < DIMENSIONS; ++i) {
                sdsl::bit_vector bv = samg::grcodec::RiceRuns<Type>::encode(s3[i],K[0]);
                std::queue<Type> ans = samg::grcodec::RiceRuns<Type>::decode(bv,K[0]);
                EXPECT_TRUE(are_equal(ans,s3[i]));
            }
        }


        // k = 3
        TEST_F(N3SequencesDataSet,Asc_k3) {
            for (std::size_t i = 0; i < DIMENSIONS; ++i) {
                sdsl::bit_vector bv = samg::grcodec::RiceRuns<Type>::encode(s1[i],K[1]);
                std::queue<Type> ans = samg::grcodec::RiceRuns<Type>::decode(bv,K[1]);
                EXPECT_TRUE(are_equal(ans,s1[i]));
            }
        }

        TEST_F(N3SequencesDataSet,Desc_k3) {
            for (std::size_t i = 0; i < DIMENSIONS; ++i) {
                sdsl::bit_vector bv = samg::grcodec::RiceRuns<Type>::encode(s2[i],K[1]);
                std::queue<Type> ans = samg::grcodec::RiceRuns<Type>::decode(bv,K[1]);
                EXPECT_TRUE(are_equal(ans,s2[i]));
            }
        }

        TEST_F(N3SequencesDataSet,Rand_k3) {
            for (std::size_t i = 0; i < DIMENSIONS; ++i) {
                sdsl::bit_vector bv = samg::grcodec::RiceRuns<Type>::encode(s3[i],K[1]);
                std::queue<Type> ans = samg::grcodec::RiceRuns<Type>::decode(bv,K[1]);
                EXPECT_TRUE(are_equal(ans,s3[i]));
            }
        }


        // k = 4
        TEST_F(N3SequencesDataSet,Asc_k4) {
            for (std::size_t i = 0; i < DIMENSIONS; ++i) {
                sdsl::bit_vector bv = samg::grcodec::RiceRuns<Type>::encode(s1[i],K[2]);
                std::queue<Type> ans = samg::grcodec::RiceRuns<Type>::decode(bv,K[2]);
                EXPECT_TRUE(are_equal(ans,s1[i]));
            }
        }

        TEST_F(N3SequencesDataSet,Desc_k4) {
            for (std::size_t i = 0; i < DIMENSIONS; ++i) {
                sdsl::bit_vector bv = samg::grcodec::RiceRuns<Type>::encode(s2[i],K[2]);
                std::queue<Type> ans = samg::grcodec::RiceRuns<Type>::decode(bv,K[2]);
                EXPECT_TRUE(are_equal(ans,s2[i]));
            }
        }

        TEST_F(N3SequencesDataSet,Rand_k4) {
            for (std::size_t i = 0; i < DIMENSIONS; ++i) {
                sdsl::bit_vector bv = samg::grcodec::RiceRuns<Type>::encode(s3[i],K[2]);
                std::queue<Type> ans = samg::grcodec::RiceRuns<Type>::decode(bv,K[2]);
                EXPECT_TRUE(are_equal(ans,s3[i]));
            }
        }


        // k = 5
        TEST_F(N3SequencesDataSet,Asc_k5) {
            for (std::size_t i = 0; i < DIMENSIONS; ++i) {
                sdsl::bit_vector bv = samg::grcodec::RiceRuns<Type>::encode(s1[i],K[3]);
                std::queue<Type> ans = samg::grcodec::RiceRuns<Type>::decode(bv,K[3]);
                EXPECT_TRUE(are_equal(ans,s1[i]));
            }
        }

        TEST_F(N3SequencesDataSet,Desc_k5) {
            for (std::size_t i = 0; i < DIMENSIONS; ++i) {
                sdsl::bit_vector bv = samg::grcodec::RiceRuns<Type>::encode(s2[i],K[3]);
                std::queue<Type> ans = samg::grcodec::RiceRuns<Type>::decode(bv,K[3]);
                EXPECT_TRUE(are_equal(ans,s2[i]));
            }
        }

        TEST_F(N3SequencesDataSet,Rand_k5) {
            for (std::size_t i = 0; i < DIMENSIONS; ++i) {
                sdsl::bit_vector bv = samg::grcodec::RiceRuns<Type>::encode(s3[i],K[3]);
                std::queue<Type> ans = samg::grcodec::RiceRuns<Type>::decode(bv,K[3]);
                EXPECT_TRUE(are_equal(ans,s3[i]));
            }
        }

        // k = 3 with `next` function:
        TEST_F(N3SequencesDataSet,Asc_k3_wnext) {
            for (std::size_t i = 0; i < DIMENSIONS; ++i) {
                samg::grcodec::RiceRuns<Type> codec(
                    samg::grcodec::RiceRuns<Type>::encode(s1[i],K[1]),
                    K[1]
                );
                while( !s1[i].empty() && codec.has_more() ) {
                    // std::cout << "1";
                    EXPECT_EQ(s1[i].front(), codec.next());
                    s1[i].pop();
                }
                EXPECT_EQ(!s1[i].empty(), codec.has_more());
            }
        }

        TEST_F(N3SequencesDataSet,Desc_k3_wnext) {
            for (std::size_t i = 0; i < DIMENSIONS; ++i) {
                samg::grcodec::RiceRuns<Type> codec(
                    samg::grcodec::RiceRuns<Type>::encode(s2[i],K[1]),
                    K[1]
                );
                while( !s2[i].empty() && codec.has_more() ) {
                    // std::cout << "2";
                    EXPECT_EQ(s2[i].front(), codec.next());
                    s2[i].pop();
                }
                EXPECT_EQ(!s2[i].empty(), codec.has_more());
            }
        }

        TEST_F(N3SequencesDataSet,Rand_k3_wnext) {
            for (std::size_t i = 0; i < DIMENSIONS; ++i) {
                samg::grcodec::RiceRuns<Type> codec(
                    samg::grcodec::RiceRuns<Type>::encode(s3[i],K[1]),
                    K[1]
                );
                while( !s3[i].empty() && codec.has_more() ) {
                    // std::cout << "3";
                    EXPECT_EQ(s3[i].front(), codec.next());
                    s3[i].pop();
                }
                EXPECT_EQ(!s3[i].empty(), codec.has_more());
            }
        }

    }
}
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
