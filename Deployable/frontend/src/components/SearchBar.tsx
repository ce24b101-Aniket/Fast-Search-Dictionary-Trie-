import { useEffect, useRef, useState } from "react";
import { api, type Suggestion } from "../lib/api";
import { useDebounce } from "../lib/useDebounce";
import { TriePath } from "./TriePath";

interface SearchBarProps {
  onSelectWord: (word: string) => void;
}

type Status = "idle" | "loading" | "ready" | "error" | "empty";

export function SearchBar({ onSelectWord }: SearchBarProps) {
  const [query, setQuery] = useState("");
  const [suggestions, setSuggestions] = useState<Suggestion[]>([]);
  const [status, setStatus] = useState<Status>("idle");
  const [highlighted, setHighlighted] = useState(0);
  const [open, setOpen] = useState(false);
  const debounced = useDebounce(query, 250);
  const inputRef = useRef<HTMLInputElement>(null);

  useEffect(() => {
    const trimmed = debounced.trim();
    if (trimmed.length === 0) {
      setSuggestions([]);
      setStatus("idle");
      return;
    }

    let cancelled = false;
    setStatus("loading");
    api
      .autocomplete(trimmed, 8)
      .then((res) => {
        if (cancelled) return;
        setSuggestions(res.results);
        setStatus(res.results.length === 0 ? "empty" : "ready");
        setHighlighted(0);
        setOpen(true);
      })
      .catch(() => {
        if (cancelled) return;
        setStatus("error");
        setSuggestions([]);
      });

    return () => {
      cancelled = true;
    };
  }, [debounced]);

  function commitSelection(word: string) {
    setOpen(false);
    setQuery(word);
    onSelectWord(word);
  }

  function handleKeyDown(e: React.KeyboardEvent<HTMLInputElement>) {
    if (e.key === "ArrowDown") {
      e.preventDefault();
      if (open && suggestions.length > 0) {
        setHighlighted((h) => (h + 1) % suggestions.length);
      }
    } else if (e.key === "ArrowUp") {
      e.preventDefault();
      if (open && suggestions.length > 0) {
        setHighlighted((h) => (h - 1 + suggestions.length) % suggestions.length);
      }
    } else if (e.key === "Enter") {
      e.preventDefault();
      if (open && suggestions.length > 0) {
        commitSelection(suggestions[highlighted].word);
      } else if (query.trim().length > 0) {
        commitSelection(query.trim());
      }
    } else if (e.key === "Escape") {
      setOpen(false);
    }
  }

  return (
    <div className="relative w-full">
      <div
        className="flex items-center gap-3 rounded-xl border px-4 py-3.5 transition-colors focus-within:border-[var(--accent)]"
        style={{ background: "var(--bg-raised)", borderColor: "var(--border)" }}
      >
        <SearchGlyph />
        <input
          ref={inputRef}
          value={query}
          onChange={(e) => {
            setQuery(e.target.value);
            setOpen(true);
          }}
          onKeyDown={handleKeyDown}
          onFocus={() => suggestions.length > 0 && setOpen(true)}
          placeholder="search the dictionary…"
          aria-label="Search"
          aria-autocomplete="list"
          aria-expanded={open}
          className="w-full bg-transparent font-mono-tabular text-lg outline-none placeholder:text-[var(--text-faint)]"
          style={{ color: "var(--text)" }}
        />
        {status === "loading" && <Spinner />}
      </div>

      <div className="mt-1.5">
        <TriePath
          query={query}
          branchCount={status === "ready" || status === "empty" ? suggestions.length : null}
        />
      </div>

      {open && (status === "ready" || status === "empty" || status === "error") && (
        <div
          className="absolute z-10 mt-2 w-full overflow-hidden rounded-xl border shadow-2xl"
          style={{ background: "var(--bg-raised)", borderColor: "var(--border)" }}
          role="listbox"
        >
          {status === "error" && (
            <StateRow tone="danger">
              couldn't reach the search backend — check that the API server is running
            </StateRow>
          )}
          {status === "empty" && (
            <StateRow tone="dim">no words start with "{debounced.trim()}"</StateRow>
          )}
          {status === "ready" &&
            suggestions.map((s, i) => (
              <button
                key={s.word}
                role="option"
                aria-selected={i === highlighted}
                onMouseEnter={() => setHighlighted(i)}
                onClick={() => commitSelection(s.word)}
                className="flex w-full items-center justify-between px-4 py-2.5 text-left transition-colors"
                style={{
                  background: i === highlighted ? "var(--bg-inset)" : "transparent",
                }}
              >
                <HighlightedWord word={s.word} prefix={debounced.trim()} />
                <span className="font-mono-tabular text-xs" style={{ color: "var(--amber)" }}>
                  {s.frequency}
                </span>
              </button>
            ))}
        </div>
      )}
    </div>
  );
}

function HighlightedWord({ word, prefix }: { word: string; prefix: string }) {
  if (!word.toLowerCase().startsWith(prefix.toLowerCase()) || prefix.length === 0) {
    return <span className="font-mono-tabular text-sm">{word}</span>;
  }
  const matched = word.slice(0, prefix.length);
  const rest = word.slice(prefix.length);
  return (
    <span className="font-mono-tabular text-sm">
      <span style={{ color: "var(--accent)" }}>{matched}</span>
      <span style={{ color: "var(--text)" }}>{rest}</span>
    </span>
  );
}

function StateRow({ children, tone }: { children: React.ReactNode; tone: "danger" | "dim" }) {
  return (
    <div
      className="px-4 py-3 text-sm"
      style={{ color: tone === "danger" ? "var(--danger)" : "var(--text-faint)" }}
    >
      {children}
    </div>
  );
}

function Spinner() {
  return (
    <div
      className="h-4 w-4 shrink-0 animate-spin rounded-full border-2"
      style={{ borderColor: "var(--border-hover)", borderTopColor: "var(--accent)" }}
    />
  );
}

function SearchGlyph() {
  return (
    <svg width="18" height="18" viewBox="0 0 18 18" fill="none" aria-hidden="true" className="shrink-0">
      <circle cx="8" cy="8" r="5.5" stroke="var(--text-faint)" strokeWidth="1.5" />
      <line x1="12.2" y1="12.2" x2="16" y2="16" stroke="var(--text-faint)" strokeWidth="1.5" strokeLinecap="round" />
    </svg>
  );
}
