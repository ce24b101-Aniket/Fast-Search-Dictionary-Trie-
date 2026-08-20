import { useState } from "react";
import { SearchPage } from "./pages/SearchPage";
import { AdminPage } from "./pages/AdminPage";
import { DashboardPage } from "./pages/DashboardPage";

type View = "search" | "admin" | "dashboard";

const NAV: { id: View; label: string }[] = [
  { id: "search", label: "search" },
  { id: "admin", label: "manage" },
  { id: "dashboard", label: "analytics" },
];

export default function App() {
  const [view, setView] = useState<View>("search");

  return (
    <div className="min-h-full">
      <nav
        className="sticky top-0 z-20 flex items-center justify-between border-b px-6 py-3 backdrop-blur"
        style={{ borderColor: "var(--border)", background: "rgba(10,14,20,0.85)" }}
      >
        <div className="flex items-center gap-1.5">
          <div className="h-2 w-2 rounded-full" style={{ background: "var(--accent)" }} />
          <span className="font-mono-tabular text-sm" style={{ color: "var(--text)" }}>
            fastsearch
          </span>
        </div>
        <div className="flex gap-1">
          {NAV.map((n) => (
            <button
              key={n.id}
              onClick={() => setView(n.id)}
              className="rounded-lg px-3 py-1.5 text-sm transition-colors"
              style={{
                color: view === n.id ? "var(--text)" : "var(--text-faint)",
                background: view === n.id ? "var(--bg-raised)" : "transparent",
              }}
            >
              {n.label}
            </button>
          ))}
        </div>
      </nav>

      {view === "search" && <SearchPage />}
      {view === "admin" && <AdminPage />}
      {view === "dashboard" && <DashboardPage />}
    </div>
  );
}
