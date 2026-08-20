import { useEffect, useState } from "react";
import { api, type SearchResponse } from "../lib/api";

export function ResultPanel({ word }: { word: string | null }) {
  const [result, setResult] = useState<SearchResponse | null>(null);
  const [related, setRelated] = useState<{ word: string; frequency: number }[]>([]);
  const [loading, setLoading] = useState(false);

  useEffect(() => {
    if (!word) {
      setResult(null);
      setRelated([]);
      return;
    }
    let cancelled = false;
    setLoading(true);
    Promise.all([
      api.search(word),
      api.autocomplete(word.slice(0, Math.max(1, word.length - 1)), 6),
    ])
      .then(([searchRes, relatedRes]) => {
        if (cancelled) return;
        setResult(searchRes);
        setRelated(relatedRes.results.filter((r) => r.word !== word));
      })
      .finally(() => !cancelled && setLoading(false));
    return () => {
      cancelled = true;
    };
  }, [word]);

  if (!word) {
    return (
      <EmptyState
        title="Nothing searched yet"
        body="Pick a suggestion above, or press Enter with a word typed, and its record will show up here."
      />
    );
  }

  if (loading || !result) {
    return <div className="py-8 text-sm" style={{ color: "var(--text-faint)" }}>looking it up…</div>;
  }

  return (
    <div className="space-y-6">
      <div
        className="rounded-xl border p-6"
        style={{ background: "var(--bg-raised)", borderColor: "var(--border)" }}
      >
        <div className="flex items-baseline justify-between">
          <h2 className="font-mono-tabular text-2xl" style={{ color: "var(--text)" }}>
            {result.word}
          </h2>
          <Badge tone={result.found ? "success" : "danger"}>
            {result.found ? "found" : "not found"}
          </Badge>
        </div>

        <div className="mt-5 grid grid-cols-2 gap-4 sm:grid-cols-3">
          <Stat label="frequency" value={result.frequency.toLocaleString()} />
          <Stat label="search latency" value={`${result.latency_ms.toFixed(3)} ms`} />
        </div>
      </div>

      {related.length > 0 && (
        <div>
          <div className="mb-2 text-xs uppercase tracking-wide" style={{ color: "var(--text-faint)" }}>
            related suggestions
          </div>
          <div className="flex flex-wrap gap-2">
            {related.map((r) => (
              <span
                key={r.word}
                className="rounded-full border px-3 py-1 font-mono-tabular text-xs"
                style={{ borderColor: "var(--border)", color: "var(--text-dim)" }}
              >
                {r.word}
              </span>
            ))}
          </div>
        </div>
      )}
    </div>
  );
}

function Stat({ label, value }: { label: string; value: string }) {
  return (
    <div>
      <div className="text-xs uppercase tracking-wide" style={{ color: "var(--text-faint)" }}>
        {label}
      </div>
      <div className="mt-1 font-mono-tabular text-lg" style={{ color: "var(--text)" }}>
        {value}
      </div>
    </div>
  );
}

function Badge({ children, tone }: { children: React.ReactNode; tone: "success" | "danger" }) {
  return (
    <span
      className="rounded-full px-2.5 py-1 text-xs font-medium"
      style={{
        background: tone === "success" ? "rgba(92,201,143,0.12)" : "rgba(224,106,106,0.12)",
        color: tone === "success" ? "var(--success)" : "var(--danger)",
      }}
    >
      {children}
    </span>
  );
}

export function EmptyState({ title, body }: { title: string; body: string }) {
  return (
    <div
      className="rounded-xl border border-dashed p-8 text-center"
      style={{ borderColor: "var(--border)" }}
    >
      <div className="text-sm font-medium" style={{ color: "var(--text-dim)" }}>
        {title}
      </div>
      <div className="mt-1.5 text-sm" style={{ color: "var(--text-faint)" }}>
        {body}
      </div>
    </div>
  );
}
