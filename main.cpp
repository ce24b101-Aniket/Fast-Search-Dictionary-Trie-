#include <bits/stdc++.h>
using namespace std;

// Node structure for the Trie
struct TrieNode {
    TrieNode* children[26];
    bool isEndOfWord;

    TrieNode() {
        isEndOfWord = false;
        for (int i = 0; i < 26; i++)
            children[i] = nullptr;
    }
};

class FastDictionary {
private:
    TrieNode* root;

    // Recursively free every node -- fixes the memory leak in the
    // original version, where nodes were allocated with `new` but
    // never released.
    void destroy(TrieNode* node) {
        if (!node) return;
        for (int i = 0; i < 26; i++)
            destroy(node->children[i]);
        delete node;
    }

    // DFS from a given node, collecting every complete word found
    // beneath it. `prefix` is the string built so far on the path
    // from the root down to `node`.
    void collectWords(TrieNode* node, string prefix, vector<string>& results) {
        if (!node) return;
        if (node->isEndOfWord)
            results.push_back(prefix);
        for (int i = 0; i < 26; i++) {
            if (node->children[i])
                collectWords(node->children[i], prefix + char('a' + i), results);
        }
    }

public:
    FastDictionary() {
        root = new TrieNode();
    }

    ~FastDictionary() {
        destroy(root);
    }

    // Insert a word into the trie -- O(L), L = word length
    void insert(const string& word) {
        TrieNode* curr = root;
        for (char ch : word) {
            int index = ch - 'a';
            if (!curr->children[index])
                curr->children[index] = new TrieNode();
            curr = curr->children[index];
        }
        curr->isEndOfWord = true;
    }

    // Search if an exact word exists -- O(L)
    bool search(const string& word) {
        TrieNode* curr = root;
        for (char ch : word) {
            int index = ch - 'a';
            if (!curr->children[index]) return false;
            curr = curr->children[index];
        }
        return curr->isEndOfWord;
    }

    // Check if any word starts with the given prefix -- O(L)
    bool startsWith(const string& prefix) {
        return findPrefixNode(prefix) != nullptr;
    }

    // Returns every word in the dictionary that starts with `prefix`.
    // This is the actual autocomplete feature: O(P + K) where P is the
    // prefix length and K is the total length of characters across all
    // matching suggestions.
    vector<string> getSuggestions(const string& prefix) {
        vector<string> results;
        TrieNode* node = findPrefixNode(prefix);
        if (node) collectWords(node, prefix, results);
        return results;
    }

private:
    TrieNode* findPrefixNode(const string& prefix) {
        TrieNode* curr = root;
        for (char ch : prefix) {
            int index = ch - 'a';
            if (!curr->children[index]) return nullptr;
            curr = curr->children[index];
        }
        return curr;
    }
};

int main() {
    FastDictionary dict;

    // Simulating a civil-construction material lexicon
    vector<string> words = {
        "cement", "concrete", "concreting", "column", "conduit",
        "beam", "bridge", "brick", "bitumen", "boulder",
        "gravel", "girder", "grout", "granite",
        "steel", "sand", "slab", "sealant",
        "asphalt", "aggregate", "admixture"
    };
    for (const string& w : words) dict.insert(w);

    cout << "--- Civil Material Lexicon: Fast Search & Autocomplete ---\n\n";

    cout << "Search 'concrete': "
         << (dict.search("concrete") ? "Found" : "Not Found") << endl;
    cout << "Search 'con': "
         << (dict.search("con") ? "Found" : "Not Found")
         << "  (exists only as a prefix, not a complete word)\n\n";

    vector<string> prefixesToTry = {"con", "b", "gr", "xyz"};
    for (const string& prefix : prefixesToTry) {
        vector<string> suggestions = dict.getSuggestions(prefix);
        cout << "Autocomplete suggestions for '" << prefix << "': ";
        if (suggestions.empty()) {
            cout << "None\n";
        } else {
            for (size_t i = 0; i < suggestions.size(); i++) {
                cout << suggestions[i];
                if (i + 1 < suggestions.size()) cout << ", ";
            }
            cout << endl;
        }
    }

    return 0;
}
