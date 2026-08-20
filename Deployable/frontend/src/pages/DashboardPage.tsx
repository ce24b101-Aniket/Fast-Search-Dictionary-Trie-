import { useEffect, useState } from "react";
import { Bar, BarChart, ResponsiveContainer, Tooltip, XAxis, YAxis } from "recharts";
import { api, type HistoryEntry, type Stats } from "../lib/api";

export function DashboardPage() {
  const [stats, setStats] = useState<Stats | null>(null);
  const [history, setHistory] = useState<HistoryEntry[]>([]);

  useEffect(() => {
    api.stats().then(setStats);
    api.history(15).then((r) => setHistory(r.history));
  }, []);

  if (!stats) {
    return (
      <div className="px-6 py-10 text-sm" style={{ color: "var(--text-faint)" }}>
        loading dashboard…
      </div>
    );
  }

  const chartData = history
    .slice()
    .reverse()
    .map((h, i) => ({ name: `#${i + 1}`, latency: Number(h.latency_ms.toFixed(3)) }));

  return (
    <div className="mx-auto max-w-4xl space-y-8 px-6 py-10">
      <h1 className="font-mono-tabular text-xl" style={{ color: "var(--text)" }}>
        analytics
      </h1>

      <div className="grid grid-cols-2 gap-4 sm:grid-cols-3">
        <StatCard label="total words" value={stats.total_words.toLocaleString()} />
        <StatCard label="trie nodes" value={stats.trie_node_count.toLocaleString()} />
        <StatCard label="total searches" value={stats.total_searches.toLocaleString()} />
        <StatCard label="hits" value={stats.searches_with_results.toLocaleString()} tone="success" />
        <StatCard label="misses" value={stats.searches_with_no_results.toLocaleString()} tone="danger" />
        <StatCard label="avg latency" value={`${stats.average_latency_ms.toFixed(3)} ms`} />
      </div>

      <div>
        <div className="mb-3 text-xs uppercase tracking-wide" style={{ color: "var(--text-faint)" }}>
          latency, most recent searches (real-time, not simulated)
        </div>
        {chartData.length === 0 ? (
          <p className="text-sm" style={{ color: "var(--text-faint)" }}>
            no searches recorded yet — try the search page first.
          </p>
        ) : (
          <div
            className="rounded-xl border p-4"
            style={{ background: "var(--bg-raised)", borderColor: "var(--border)" }}
          >
            <ResponsiveContainer width="100%" height={220}>
              <BarChart data={chartData}>
                <XAxis dataKey="name" stroke="var(--text-faint)" fontSize={11} tickLine={false} axisLine={false} />
                <YAxis stroke="var(--text-faint)" fontSize={11} tickLine={false} axisLine={false} width={40} />
                <Tooltip
                  contentStyle={{
                    background: "var(--bg-inset)",
                    border: "1px solid var(--border)",
                    borderRadius: 8,
                    fontSize: 12,
                  }}
                  labelStyle={{ color: "var(--text-dim)" }}
                />
                <Bar dataKey="latency" fill="var(--accent)" radius={[3, 3, 0, 0]} />
              </BarChart>
            </ResponsiveContainer>
          </div>
        )}
      </div>

      <div>
        <div className="mb-3 text-xs uppercase tracking-wide" style={{ color: "var(--text-faint)" }}>
          recent queries
        </div>
        <div className="overflow-hidden rounded-xl border" style={{ borderColor: "var(--border)" }}>
          <table className="w-full text-sm">
            <thead>
              <tr style={{ background: "var(--bg-inset)" }}>
                <Th>query</Th>
                <Th>results</Th>
                <Th>latency</Th>
                <Th>when</Th>
              </tr>
            </thead>
            <tbody>
              {history.length === 0 && (
                <tr>
                  <td colSpan={4} className="px-4 py-6 text-center" style={{ color: "var(--text-faint)" }}>
                    nothing searched yet
                  </td>
                </tr>
              )}
              {history.map((h, i) => (
                <tr key={i} style={{ borderTop: "1px solid var(--border)" }}>
                  <td className="px-4 py-2 font-mono-tabular" style={{ color: "var(--text)" }}>
                    {h.query}
                  </td>
                  <td className="px-4 py-2 font-mono-tabular" style={{ color: "var(--text-dim)" }}>
                    {h.result_count}
                  </td>
                  <td className="px-4 py-2 font-mono-tabular" style={{ color: "var(--text-dim)" }}>
                    {h.latency_ms.toFixed(3)} ms
                  </td>
                  <td className="px-4 py-2" style={{ color: "var(--text-faint)" }}>
                    {new Date(h.created_at).toLocaleTimeString()}
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </div>
    </div>
  );
}

function StatCard({ label, value, tone }: { label: string; value: string; tone?: "success" | "danger" }) {
  return (
    <div className="rounded-xl border p-4" style={{ background: "var(--bg-raised)", borderColor: "var(--border)" }}>
      <div className="text-xs uppercase tracking-wide" style={{ color: "var(--text-faint)" }}>
        {label}
      </div>
      <div
        className="mt-1.5 font-mono-tabular text-2xl"
        style={{ color: tone === "success" ? "var(--success)" : tone === "danger" ? "var(--danger)" : "var(--text)" }}
      >
        {value}
      </div>
    </div>
  );
}

function Th({ children }: { children: React.ReactNode }) {
  return (
    <th
      className="px-4 py-2 text-left text-xs font-medium uppercase tracking-wide"
      style={{ color: "var(--text-faint)" }}
    >
      {children}
    </th>
  );
}
