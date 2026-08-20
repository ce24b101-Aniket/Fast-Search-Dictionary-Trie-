#include "core/normalizer.hpp"

#include <algorithm>
#include <cctype>

namespace fastsearch {

namespace {

bool isAsciiSpace(unsigned char c) {
    return std::isspace(c) != 0;
}

}  // namespace

NormalizationResult Normalizer::normalize(const std::string& input) {
    NormalizationResult result;

    // Trim leading/trailing whitespace only. Internal whitespace (e.g.
    // "con crete") is left in place so it gets caught by the character
    // check below rather than silently collapsed or stripped -- a
    // multi-word "word" is invalid input, not something to fix for the
    // caller.
    std::size_t start = 0;
    std::size_t end = input.size();
    while (start < end && isAsciiSpace(static_cast<unsigned char>(input[start]))) {
        ++start;
    }
    while (end > start && isAsciiSpace(static_cast<unsigned char>(input[end - 1]))) {
        --end;
    }
    std::string trimmed = input.substr(start, end - start);

    if (trimmed.empty()) {
        result.valid = false;
        result.error = "word must not be empty";
        return result;
    }

    if (trimmed.size() > kMaxWordLength) {
        result.valid = false;
        result.error = "word exceeds maximum length of " +
                        std::to_string(kMaxWordLength) + " characters";
        return result;
    }

    std::string lowered;
    lowered.reserve(trimmed.size());
    for (char c : trimmed) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (uc >= 'A' && uc <= 'Z') {
            lowered.push_back(static_cast<char>(uc - 'A' + 'a'));
        } else {
            lowered.push_back(static_cast<char>(uc));
        }
    }

    bool allLowerAlpha = std::all_of(lowered.begin(), lowered.end(), [](unsigned char c) {
        return c >= 'a' && c <= 'z';
    });

    if (!allLowerAlpha) {
        result.valid = false;
        result.error = "word must contain only letters a-z (after case folding)";
        return result;
    }

    result.value = std::move(lowered);
    result.valid = true;
    return result;
}

}  // namespace fastsearch
