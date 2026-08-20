import { useEffect, useState } from "react";
import { api, ApiError, type WordRecord } from "../lib/api";

export function AdminPage() {
  const [newWord, setNewWord] = useState("");
  const [lookupWord, setLookupWord] = useState("");
  const [record, setRecord] = useState<WordRecord | null>(null);
  const [freqInput, setFreqInput] = useState("");
  const [message, setMessage] = useState<{ tone: "success" | "danger"; text: string } | null>(null);
  const [stats, setStats] = useState<{ total_words: number; trie_node_count: number } | null>(null);

  function refreshStats() {
    api.stats().then((s) => setStats({ total_words: s.total_words, trie_node_count: s.trie_node_count }));
  }

  useEffect(refreshStats, []);

  function showMessage(tone: "success" | "danger", text: string) {
    setMessage({ tone, text });
    setTimeout(() => setMessage(null), 3500);
  }

  async function handleAdd() {
    if (!newWord.trim()) return;
    try {
      const rec = await api.addWord(newWord.trim());
      showMessage("success", `added "${rec.word}"`);
      setNewWord("");
      refreshStats();
    } catch (e) {
      showMessage("danger", e instanceof ApiError ? e.message : "failed to add word");
    }
  }

  async function handleLookup() {
    if (!lookupWord.trim()) return;
    try {
      const rec = await api.getWord(lookupWord.trim());
      setRecord(rec);
      setFreqInput(String(rec.frequency));
    } catch (e) {
      setRecord(null);
      showMessage("danger", e instanceof ApiError ? e.message : "word not found");
    }
  }

  async function handleUpdateFrequency() {
    if (!record) return;
    const freq = Number(freqInput);
    if (!Number.isInteger(freq) || freq < 0) {
      showMessage("danger", "frequency must be a non-negative whole number");
      return;
    }
    try {
      const rec = await api.updateFrequency(record.word, freq);
      setRecord(rec);
      showMessage("success", `updated frequency for "${rec.word}"`);
    } catch (e) {
      showMessage("danger", e instanceof ApiError ? e.message : "update failed");
    }
  }

  async function handleDelete() {
    if (!record) return;
    try {
      await api.deleteWord(record.word);
      showMessage("success", `deleted "${record.word}"`);
      setRecord(null);
      setLookupWord("");
      refreshStats();
    } catch (e) {
      showMessage("danger", e instanceof ApiError ? e.message : "delete failed");
    }
  }

  return (
    <div className="mx-auto max-w-2xl space-y-8 px-6 py-10">
      <div>
        <h1 className="font-mono-tabular text-xl" style={{ color: "var(--text)" }}>
          dictionary management
        </h1>
        {stats && (
          <p className="mt-1 text-sm" style={{ color: "var(--text-faint)" }}>
            {stats.total_words.toLocaleString()} words · {stats.trie_node_count.toLocaleString()} trie nodes
          </p>
        )}
      </div>

      {message && (
        <div
          className="rounded-lg border px-4 py-2.5 text-sm"
          style={{
            borderColor: message.tone === "success" ? "var(--success)" : "var(--danger)",
            color: message.tone === "success" ? "var(--success)" : "var(--danger)",
            background: message.tone === "success" ? "rgba(92,201,143,0.08)" : "rgba(224,106,106,0.08)",
          }}
        >
          {message.text}
        </div>
      )}

      <Section title="add a word">
        <div className="flex gap-2">
          <TextInput value={newWord} onChange={setNewWord} placeholder="e.g. rebar" onEnter={handleAdd} />
          <Button onClick={handleAdd}>Add</Button>
        </div>
      </Section>

      <Section title="find, update, or remove a word">
        <div className="flex gap-2">
          <TextInput
            value={lookupWord}
            onChange={setLookupWord}
            placeholder="e.g. concrete"
            onEnter={handleLookup}
          />
          <Button onClick={handleLookup}>Look up</Button>
        </div>

        {record && (
          <div
            className="mt-4 rounded-lg border p-4"
            style={{ borderColor: "var(--border)", background: "var(--bg-inset)" }}
          >
            <div className="flex items-center justify-between">
              <span className="font-mono-tabular text-base" style={{ color: "var(--text)" }}>
                {record.word}
              </span>
              <span className="text-xs" style={{ color: "var(--text-faint)" }}>
                updated {new Date(record.updated_at).toLocaleString()}
              </span>
            </div>
            <div className="mt-3 flex items-center gap-2">
              <label className="text-xs" style={{ color: "var(--text-faint)" }}>
                frequency
              </label>
              <input
                type="number"
                min={0}
                value={freqInput}
                onChange={(e) => setFreqInput(e.target.value)}
                className="w-24 rounded-md border px-2 py-1 font-mono-tabular text-sm outline-none"
                style={{ borderColor: "var(--border)", background: "var(--bg-raised)", color: "var(--text)" }}
              />
              <Button onClick={handleUpdateFrequency} small>
                Save
              </Button>
              <Button onClick={handleDelete} small danger>
                Delete
              </Button>
            </div>
          </div>
        )}
      </Section>
    </div>
  );
}

function Section({ title, children }: { title: string; children: React.ReactNode }) {
  return (
    <div>
      <div className="mb-2 text-xs uppercase tracking-wide" style={{ color: "var(--text-faint)" }}>
        {title}
      </div>
      {children}
    </div>
  );
}

function TextInput({
  value,
  onChange,
  placeholder,
  onEnter,
}: {
  value: string;
  onChange: (v: string) => void;
  placeholder: string;
  onEnter: () => void;
}) {
  return (
    <input
      value={value}
      onChange={(e) => onChange(e.target.value)}
      onKeyDown={(e) => e.key === "Enter" && onEnter()}
      placeholder={placeholder}
      className="flex-1 rounded-lg border px-3 py-2 font-mono-tabular text-sm outline-none"
      style={{ borderColor: "var(--border)", background: "var(--bg-raised)", color: "var(--text)" }}
    />
  );
}

function Button({
  children,
  onClick,
  small,
  danger,
}: {
  children: React.ReactNode;
  onClick: () => void;
  small?: boolean;
  danger?: boolean;
}) {
  return (
    <button
      onClick={onClick}
      className={`rounded-lg font-medium transition-colors ${small ? "px-3 py-1 text-xs" : "px-4 py-2 text-sm"}`}
      style={{
        background: danger ? "rgba(224,106,106,0.12)" : "var(--accent-glow)",
        color: danger ? "var(--danger)" : "var(--accent)",
      }}
    >
      {children}
    </button>
  );
}
