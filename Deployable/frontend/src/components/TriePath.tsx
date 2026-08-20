import { useEffect, useState } from "react";

interface TriePathProps {
  query: string;
  branchCount: number | null; // null while nothing has been searched yet
}

/**
 * The page's signature element. Rather than a generic hero graphic, this
 * renders the actual mechanism the product is built on: each typed
 * character becomes a lit node in a chain, exactly mirroring how the
 * Trie walks one child pointer per character during a real lookup. The
 * final node fans out into a small stub indicating how many words
 * branch onward from the current prefix -- a literal, honest picture of
 * "prefix node -> N descendants," not a decorative animation.
 */
export function TriePath({ query, branchCount }: TriePathProps) {
  const [pulseIndex, setPulseIndex] = useState(-1);

  useEffect(() => {
    setPulseIndex(query.length - 1);
    const t = setTimeout(() => setPulseIndex(-1), 260);
    return () => clearTimeout(t);
  }, [query]);

  if (query.length === 0) {
    return (
      <div className="flex h-12 items-center px-1 text-sm" style={{ color: "var(--text-faint)" }}>
        <span className="font-mono-tabular">root ·</span>
        <span className="ml-2">start typing to walk the trie</span>
      </div>
    );
  }

  const chars = query.split("");

  return (
    <div className="flex h-12 items-center gap-0 overflow-x-auto px-1" aria-hidden="true">
      <NodeDot label="•" dim />
      {chars.map((ch, i) => (
        <div key={i} className="flex items-center">
          <div
            className="h-px w-4 shrink-0 transition-colors duration-200"
            style={{ background: "var(--border-hover)" }}
          />
          <NodeDot label={ch} pulsing={i === pulseIndex} />
        </div>
      ))}
      {branchCount !== null && (
        <div className="ml-3 flex items-center gap-1.5 border-l pl-3" style={{ borderColor: "var(--border)" }}>
          <span className="font-mono-tabular text-xs" style={{ color: "var(--amber)" }}>
            {branchCount}
          </span>
          <span className="text-xs" style={{ color: "var(--text-faint)" }}>
            {branchCount === 1 ? "word beyond here" : "words beyond here"}
          </span>
        </div>
      )}
    </div>
  );
}

function NodeDot({
  label,
  dim,
  pulsing,
}: {
  label: string;
  dim?: boolean;
  pulsing?: boolean;
}) {
  return (
    <div
      className="flex h-8 w-8 shrink-0 items-center justify-center rounded-full border font-mono-tabular text-sm uppercase transition-all duration-200"
      style={{
        borderColor: pulsing ? "var(--accent)" : "var(--border-hover)",
        background: pulsing ? "var(--accent-glow)" : "var(--bg-raised)",
        color: dim ? "var(--text-faint)" : "var(--text)",
        boxShadow: pulsing ? "0 0 0 3px var(--accent-glow)" : "none",
      }}
    >
      {label}
    </div>
  );
}
