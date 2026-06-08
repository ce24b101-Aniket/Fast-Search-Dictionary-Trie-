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

public:
    FastDictionary() {
        root = new TrieNode();
    }

    // Insert a word into the trie
    void insert(string word) {
        TrieNode* curr = root;
        for (char ch : word) {
            int index = ch - 'a';
            if (!curr->children[index])
                curr->children[index] = new TrieNode();
            curr = curr->children[index];
        }
        curr->isEndOfWord = true;
    }

    // Search if word exists
    bool search(string word) {
        TrieNode* curr = root;
        for (char ch : word) {
            int index = ch - 'a';
            if (!curr->children[index]) return false;
            curr = curr->children[index];
        }
        return curr->isEndOfWord;
    }

    // Check if any word starts with the given prefix
    bool startsWith(string prefix) {
        TrieNode* curr = root;
        for (char ch : prefix) {
            int index = ch - 'a';
            if (!curr->children[index]) return false;
            curr = curr->children[index];
        }
        return true;
    }
};

int main() {
    FastDictionary dict;
    
    // Simulating a database of words
    vector<string> words = {"cement", "concrete", "column", "beam", "bridge", "brick"};
    for (string w : words) dict.insert(w);

    cout << "--- Professional Dictionary Search ---\n";
    
    string query = "con";
    cout << "Search 'concrete': " << (dict.search("concrete") ? "Found" : "Not Found") << endl;
    cout << "Suggestions for 'con': " << (dict.startsWith("con") ? "Available" : "None") << endl;
    cout << "Suggestions for 'xyz': " << (dict.startsWith("xyz") ? "Available" : "None") << endl;

    return 0;
}