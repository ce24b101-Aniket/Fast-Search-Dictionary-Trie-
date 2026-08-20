import { useState } from "react";
import { SearchBar } from "../components/SearchBar";
import { ResultPanel } from "../components/ResultPanel";

export function SearchPage() {
  const [selectedWord, setSelectedWord] = useState<string | null>(null);

  return (
    <div className="mx-auto max-w-2xl px-6 py-16">
      <div className="mb-10 text-center">
        <h1 className="font-mono-tabular text-3xl tracking-tight" style={{ color: "var(--text)" }}>
          fastsearch
        </h1>
        <p className="mt-2 text-sm" style={{ color: "var(--text-faint)" }}>
          a trie-backed autocomplete engine — type a prefix to walk the tree
        </p>
      </div>

      <SearchBar onSelectWord={setSelectedWord} />

      <div className="mt-10">
        <ResultPanel word={selectedWord} />
      </div>
    </div>
  );
}
