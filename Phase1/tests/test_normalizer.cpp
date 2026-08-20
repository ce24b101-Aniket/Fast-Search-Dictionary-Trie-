#include "core/normalizer.hpp"

#include <gtest/gtest.h>

using fastsearch::Normalizer;

TEST(Normalizer, LowercasesInput) {
    auto r = Normalizer::normalize("CONCRETE");
    EXPECT_TRUE(r.valid);
    EXPECT_EQ(r.value, "concrete");
}

TEST(Normalizer, TrimsWhitespace) {
    auto r = Normalizer::normalize("  concrete  ");
    EXPECT_TRUE(r.valid);
    EXPECT_EQ(r.value, "concrete");
}

TEST(Normalizer, MixedCase) {
    auto r = Normalizer::normalize("CoNcReTe");
    EXPECT_TRUE(r.valid);
    EXPECT_EQ(r.value, "concrete");
}

TEST(Normalizer, EmptyStringIsInvalid) {
    auto r = Normalizer::normalize("");
    EXPECT_FALSE(r.valid);
}

TEST(Normalizer, WhitespaceOnlyIsInvalid) {
    auto r = Normalizer::normalize("    ");
    EXPECT_FALSE(r.valid);
}

TEST(Normalizer, InternalWhitespaceIsInvalid) {
    auto r = Normalizer::normalize("con crete");
    EXPECT_FALSE(r.valid);
}

TEST(Normalizer, DigitsAreInvalid) {
    auto r = Normalizer::normalize("concrete123");
    EXPECT_FALSE(r.valid);
}

TEST(Normalizer, PunctuationIsInvalid) {
    auto r = Normalizer::normalize("con-crete");
    EXPECT_FALSE(r.valid);
}

TEST(Normalizer, ExceedsMaxLengthIsInvalid) {
    std::string tooLong(fastsearch::kMaxWordLength + 1, 'a');
    auto r = Normalizer::normalize(tooLong);
    EXPECT_FALSE(r.valid);
}

TEST(Normalizer, ExactlyMaxLengthIsValid) {
    std::string exact(fastsearch::kMaxWordLength, 'a');
    auto r = Normalizer::normalize(exact);
    EXPECT_TRUE(r.valid);
}
