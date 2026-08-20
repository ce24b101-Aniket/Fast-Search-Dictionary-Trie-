#include "core/trie.hpp"

#include <algorithm>
#include <queue>

namespace fastsearch {

Trie::Trie() : root_(std::make_unique<TrieNode>()), wordCount_(0), nodeCount_(1) {
    // nodeCount_ starts at 1 to account for the root node itself.
}

int Trie::charToIndex(char c) {
    if (c < 'a' || c > 'z') {
        throw std::invalid_argument(
            "Trie: character '" + std::string(1, c) +
            "' is outside the supported a-z alphabet. Normalize input "
            "with Normalizer::normalize() before calling the Trie.");
    }
    return c - 'a';
}

void Trie::insert(const std::string& word) {
    if (word.empty()) {
        throw std::invalid_argument("Trie::insert: word must not be empty");
    }

    TrieNode* curr = root_.get();
    for (char ch : word) {
        int idx = charToIndex(ch);
        if (!curr->children[idx]) {
            curr->children[idx] = std::make_unique<TrieNode>();
            ++nodeCount_;
        }
        curr = curr->children[idx].get();
    }

    if (!curr->isEndOfWord) {
        curr->isEndOfWord = true;
        ++wordCount_;
    }
    // If it was already a word, this insert is a no-op beyond the walk
    // itself -- idempotent by design.
}

const TrieNode* Trie::findNode(const std::string& prefix) const {
    const TrieNode* curr = root_.get();
    for (char ch : prefix) {
        int idx = charToIndex(ch);
        if (!curr->children[idx]) return nullptr;
        curr = curr->children[idx].get();
    }
    return curr;
}

TrieNode* Trie::findNodeMutable(const std::string& prefix) {
    TrieNode* curr = root_.get();
    for (char ch : prefix) {
        int idx = charToIndex(ch);
        if (!curr->children[idx]) return nullptr;
        curr = curr->children[idx].get();
    }
    return curr;
}

bool Trie::search(const std::string& word) const {
    if (word.empty()) return false;
    const TrieNode* node = findNode(word);
    return node != nullptr && node->isEndOfWord;
}

bool Trie::contains(const std::string& word) const { return search(word); }

bool Trie::startsWith(const std::string& prefix) const {
    return findNode(prefix) != nullptr;
}

bool Trie::hasAnyChild(const TrieNode* node) noexcept {
    for (const auto& child : node->children) {
        if (child) return true;
    }
    return false;
}

bool Trie::removeHelper(TrieNode* node, const std::string& word, std::size_t depth,
                         bool& removed) {
    if (depth == word.size()) {
        // Reached the node that should represent the end of `word`.
        if (!node->isEndOfWord) {
            return false;  // word was never present; nothing to prune.
        }
        node->isEndOfWord = false;
        removed = true;
        --wordCount_;
        // This node should be pruned by its parent only if it now has
        // no children left (i.e. it isn't a prefix of some other word).
        return !hasAnyChild(node);
    }

    int idx = charToIndex(word[depth]);
    TrieNode* child = node->children[idx].get();
    if (!child) {
        return false;  // word not present.
    }

    bool shouldPruneChild = removeHelper(child, word, depth + 1, removed);
    if (shouldPruneChild) {
        node->children[idx].reset();
        --nodeCount_;
    }

    // This node itself should be pruned only if it's not a terminal word
    // for something else AND has no remaining children.
    return !node->isEndOfWord && !hasAnyChild(node);
}

bool Trie::remove(const std::string& word) {
    if (word.empty()) return false;
    bool removed = false;
    // The root itself is never pruned -- ignore the return value at the
    // top level.
    removeHelper(root_.get(), word, 0, removed);
    return removed;
}

void Trie::collectWords(const TrieNode* node, std::string& current,
                         std::vector<std::string>& results) {
    if (node->isEndOfWord) {
        results.push_back(current);
    }
    for (int i = 0; i < kAlphabetSize; ++i) {
        if (node->children[i]) {
            current.push_back(static_cast<char>('a' + i));
            collectWords(node->children[i].get(), current, results);
            current.pop_back();  // backtrack -- avoids reallocating/copying
                                  // the string at every recursive call.
        }
    }
}

std::vector<std::string> Trie::getSuggestions(const std::string& prefix) const {
    std::vector<std::string> results;
    const TrieNode* node = findNode(prefix);
    if (!node) return results;

    std::string current = prefix;
    collectWords(node, current, results);
    return results;
}

void Trie::collectSuggestions(const TrieNode* node, std::string& current,
                               std::vector<Suggestion>& results) {
    if (node->isEndOfWord) {
        results.push_back(Suggestion{current, node->frequency});
    }
    for (int i = 0; i < kAlphabetSize; ++i) {
        if (node->children[i]) {
            current.push_back(static_cast<char>('a' + i));
            collectSuggestions(node->children[i].get(), current, results);
            current.pop_back();
        }
    }
}

namespace {

// "a is better than b": higher frequency wins; equal frequency breaks
// ties lexicographically ascending (matches the spec's tie-break rule).
// Used as the Compare for a std::priority_queue the same way
// std::greater<T> would be for plain numbers -- it turns the queue into
// a min-heap ordered by "goodness," so top() is always the worst of the
// current best-k candidates and can be evicted in O(log k).
bool isBetter(const fastsearch::Suggestion& a, const fastsearch::Suggestion& b) {
    if (a.frequency != b.frequency) return a.frequency > b.frequency;
    return a.word < b.word;
}

}  // namespace

std::vector<Suggestion> Trie::autocomplete(const std::string& prefix, std::size_t k) const {
    std::vector<Suggestion> topK;
    if (k == 0) return topK;

    const TrieNode* node = findNode(prefix);
    if (!node) return topK;

    std::vector<Suggestion> allMatches;
    std::string current = prefix;
    collectSuggestions(node, current, allMatches);

    // Min-heap of size <= k, ordered so the worst-so-far sits at top().
    // For each of the M matches we do at most one O(log k) push/pop,
    // giving O(M log k) total -- versus O(M log M) for collecting
    // everything and sorting it outright.
    std::priority_queue<Suggestion, std::vector<Suggestion>, decltype(&isBetter)> heap(isBetter);

    for (const auto& candidate : allMatches) {
        if (heap.size() < k) {
            heap.push(candidate);
        } else if (isBetter(candidate, heap.top())) {
            heap.pop();
            heap.push(candidate);
        }
    }

    topK.reserve(heap.size());
    while (!heap.empty()) {
        topK.push_back(heap.top());
        heap.pop();
    }
    // Heap pops worst-first, so reverse to get best-first order.
    std::reverse(topK.begin(), topK.end());
    return topK;
}

long Trie::getFrequency(const std::string& word) const {
    if (word.empty()) return 0;
    const TrieNode* node = findNode(word);
    if (!node || !node->isEndOfWord) return 0;
    return node->frequency;
}

bool Trie::incrementFrequency(const std::string& word) {
    if (word.empty()) return false;
    TrieNode* node = findNodeMutable(word);
    if (!node || !node->isEndOfWord) return false;
    ++node->frequency;
    return true;
}

bool Trie::setFrequency(const std::string& word, long frequency) {
    if (word.empty() || frequency < 0) return false;
    TrieNode* node = findNodeMutable(word);
    if (!node || !node->isEndOfWord) return false;
    node->frequency = frequency;
    return true;
}

std::size_t Trie::size() const noexcept { return wordCount_; }

std::size_t Trie::nodeCount() const noexcept { return nodeCount_; }

void Trie::clear() {
    root_ = std::make_unique<TrieNode>();
    wordCount_ = 0;
    nodeCount_ = 1;
}

}  // namespace fastsearch
