#include <gtest/gtest.h>
#include <codecs/gr-codec.hpp>
#include <random>

namespace grcodec {
    namespace test {

        class N3SequencesDataSet : public ::testing::Test {
            protected:
                // Attributes:
                static const std::size_t    N_ENTRIES = 10,
                                            DIMENSIONS = 3;
                using Type = std::uint16_t;

                std::array<std::vector<Type>,DIMENSIONS> s1,s2,s3;

                const std::size_t K[4] = {2,3,4,5};

                // Constructor and Destructor:
                N3SequencesDataSet(){
                    std::random_device rd;
                    std::mt19937_64 gen(rd());
                    std::uniform_int_distribution<Type> dist(0,N_ENTRIES-1);

                    for (std::size_t i = 0; i < N_ENTRIES; ++i) {
                        for (std::size_t j = 0; j < N_ENTRIES; ++j) {
                            for (std::size_t k = 0; k < N_ENTRIES; ++k) {
                                s1[0].push_back(i);
                                s1[1].push_back(j);
                                s1[2].push_back(k);

                                s2[0].push_back(N_ENTRIES - i - 1);
                                s2[1].push_back(N_ENTRIES - j - 1);
                                s2[2].push_back(N_ENTRIES - k - 1);

                                s3[0].push_back(dist(gen));
                                s3[1].push_back(dist(gen));
                                s3[2].push_back(dist(gen));
                            }       
                        }   
                    }
                    
                }
                ~N3SequencesDataSet() override {}
                // Methods:
                void SetUp() override {}
                void TearDown() override {}
                static bool are_equal(const std::vector<Type> &v1, const std::vector<Type> &v2) {
                    bool ans = v1.size() == v2.size();
                    for (size_t i = 0; ans && i < v1.size(); ++i) {
                        ans &= v1[i] == v2[i];
                    }
                    return ans;
                }
        };
        

        // k = 2
        TEST_F(N3SequencesDataSet,Asc_k2) {
            for (size_t i = 0; i < DIMENSIONS; ++i) {
                samg::grcodec::RiceRuns<Type> rr(K[0]);
                rr.encode(s1[i]);
                std::vector<Type> ans = rr.decode();
                EXPECT_TRUE(are_equal(ans,s1[i]));
            }
        }

        TEST_F(N3SequencesDataSet,Desc_k2) {
            for (size_t i = 0; i < DIMENSIONS; ++i) {
                samg::grcodec::RiceRuns<Type> rr(K[0]);
                rr.encode(s2[i]);
                std::vector<Type> ans = rr.decode();
                EXPECT_TRUE(are_equal(ans,s2[i]));
            }
        }

        TEST_F(N3SequencesDataSet,Rand_k2) {
            for (size_t i = 0; i < DIMENSIONS; ++i) {
                samg::grcodec::RiceRuns<Type> rr(K[0]);
                rr.encode(s3[i]);
                std::vector<Type> ans = rr.decode();
                EXPECT_TRUE(are_equal(ans,s3[i]));
            }
        }


        // k = 3
        TEST_F(N3SequencesDataSet,Asc_k3) {
            for (size_t i = 0; i < DIMENSIONS; ++i) {
                samg::grcodec::RiceRuns<Type> rr(K[1]);
                rr.encode(s1[i]);
                std::vector<Type> ans = rr.decode();
                EXPECT_TRUE(are_equal(ans,s1[i]));
            }
        }

        TEST_F(N3SequencesDataSet,Desc_k3) {
            for (size_t i = 0; i < DIMENSIONS; ++i) {
                samg::grcodec::RiceRuns<Type> rr(K[1]);
                rr.encode(s2[i]);
                std::vector<Type> ans = rr.decode();
                EXPECT_TRUE(are_equal(ans,s2[i]));
            }
        }

        TEST_F(N3SequencesDataSet,Rand_k3) {
            for (size_t i = 0; i < DIMENSIONS; ++i) {
                samg::grcodec::RiceRuns<Type> rr(K[1]);
                rr.encode(s3[i]);
                std::vector<Type> ans = rr.decode();
                EXPECT_TRUE(are_equal(ans,s3[i]));
            }
        }


        // k = 4
        TEST_F(N3SequencesDataSet,Asc_k4) {
            for (size_t i = 0; i < DIMENSIONS; ++i) {
                samg::grcodec::RiceRuns<Type> rr(K[2]);
                rr.encode(s1[i]);
                std::vector<Type> ans = rr.decode();
                EXPECT_TRUE(are_equal(ans,s1[i]));
            }
        }

        TEST_F(N3SequencesDataSet,Desc_k4) {
            for (size_t i = 0; i < DIMENSIONS; ++i) {
                samg::grcodec::RiceRuns<Type> rr(K[2]);
                rr.encode(s2[i]);
                std::vector<Type> ans = rr.decode();
                EXPECT_TRUE(are_equal(ans,s2[i]));
            }
        }

        TEST_F(N3SequencesDataSet,Rand_k4) {
            for (size_t i = 0; i < DIMENSIONS; ++i) {
                samg::grcodec::RiceRuns<Type> rr(K[2]);
                rr.encode(s3[i]);
                std::vector<Type> ans = rr.decode();
                EXPECT_TRUE(are_equal(ans,s3[i]));
            }
        }


        // k = 5
        TEST_F(N3SequencesDataSet,Asc_k5) {
            for (size_t i = 0; i < DIMENSIONS; ++i) {
                samg::grcodec::RiceRuns<Type> rr(K[3]);
                rr.encode(s1[i]);
                std::vector<Type> ans = rr.decode();
                EXPECT_TRUE(are_equal(ans,s1[i]));
            }
        }

        TEST_F(N3SequencesDataSet,Desc_k5) {
            for (size_t i = 0; i < DIMENSIONS; ++i) {
                samg::grcodec::RiceRuns<Type> rr(K[3]);
                rr.encode(s2[i]);
                std::vector<Type> ans = rr.decode();
                EXPECT_TRUE(are_equal(ans,s2[i]));
            }
        }

        TEST_F(N3SequencesDataSet,Rand_k5) {
            for (size_t i = 0; i < DIMENSIONS; ++i) {
                samg::grcodec::RiceRuns<Type> rr(K[3]);
                rr.encode(s3[i]);
                std::vector<Type> ans = rr.decode();
                EXPECT_TRUE(are_equal(ans,s3[i]));
            }
        }
    }
}
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
