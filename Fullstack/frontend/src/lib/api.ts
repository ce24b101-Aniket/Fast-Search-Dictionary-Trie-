// Thin typed wrapper around the FastSearch REST API (see
// ../../../docs/api.md once written, or src/api/server.cpp for the
// authoritative route definitions). Every function here maps 1:1 to one
// backend endpoint -- no business logic lives in this file, matching
// the same "thin layer" discipline the C++ API server follows.

const BASE_URL = import.meta.env.VITE_API_BASE_URL ?? "http://localhost:8080";

export class ApiError extends Error {
  status: number;
  constructor(status: number, message: string) {
    super(message);
    this.status = status;
  }
}

async function request<T>(path: string, init?: RequestInit): Promise<T> {
  const res = await fetch(`${BASE_URL}${path}`, {
    headers: { "Content-Type": "application/json" },
    ...init,
  });
  if (!res.ok) {
    let message = res.statusText;
    try {
      const body = await res.json();
      if (body?.error) message = body.error;
    } catch {
      // response wasn't JSON (e.g. a network-level failure) -- fall
      // back to the HTTP status text rather than throwing a second,
      // more confusing error out of the catch block.
    }
    throw new ApiError(res.status, message);
  }
  if (res.status === 204) return undefined as T;
  return res.json() as Promise<T>;
}

export interface SearchResponse {
  word: string;
  found: boolean;
  frequency: number;
  latency_ms: number;
}

export interface Suggestion {
  word: string;
  frequency: number;
}

export interface AutocompleteResponse {
  prefix: string;
  results: Suggestion[];
  count: number;
  latency_ms: number;
}

export interface WordRecord {
  word: string;
  frequency: number;
  created_at: string;
  updated_at: string;
}

export interface Stats {
  total_words: number;
  trie_node_count: number;
  total_searches: number;
  searches_with_results: number;
  searches_with_no_results: number;
  average_latency_ms: number;
}

export interface HistoryEntry {
  query: string;
  result_count: number;
  latency_ms: number;
  created_at: string;
}

export interface HealthResponse {
  status: string;
  database: string;
  trie: string;
  uptime_seconds: number;
}

export const api = {
  health: () => request<HealthResponse>("/api/health"),

  search: (word: string) =>
    request<SearchResponse>(`/api/search?word=${encodeURIComponent(word)}`),

  autocomplete: (prefix: string, limit = 10) =>
    request<AutocompleteResponse>(
      `/api/autocomplete?prefix=${encodeURIComponent(prefix)}&limit=${limit}`
    ),

  getWord: (word: string) =>
    request<WordRecord>(`/api/words/${encodeURIComponent(word)}`),

  addWord: (word: string) =>
    request<WordRecord>("/api/words", {
      method: "POST",
      body: JSON.stringify({ word }),
    }),

  updateFrequency: (word: string, frequency: number) =>
    request<WordRecord>(`/api/words/${encodeURIComponent(word)}`, {
      method: "PUT",
      body: JSON.stringify({ frequency }),
    }),

  deleteWord: (word: string) =>
    request<void>(`/api/words/${encodeURIComponent(word)}`, {
      method: "DELETE",
    }),

  stats: () => request<Stats>("/api/stats"),

  history: (limit = 20) =>
    request<{ history: HistoryEntry[]; count: number }>(
      `/api/history?limit=${limit}`
    ),
};
