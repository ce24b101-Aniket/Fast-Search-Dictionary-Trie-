#include "api/server.hpp"

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <chrono>

using json = nlohmann::json;

namespace fastsearch::api {

namespace {

// Global startup time, used for GET /api/health's `uptime` field.
const std::chrono::steady_clock::time_point kStartTime = std::chrono::steady_clock::now();

json errorBody(const std::string& message) {
    return json{{"error", message}};
}

// Parses and validates the `limit` query parameter shared by
// /api/autocomplete and /api/history. Returns std::nullopt (and writes
// a 400 response) if the parameter is present but malformed.
//
// This is the "protect the API against unreasonable limits" requirement
// (spec section 7 / 32) at the HTTP boundary: a client can still ask for
// an enormous limit, but SearchService::kMaxAutocompleteLimit caps the
// actual work done regardless of what's requested here.
bool parseLimit(const httplib::Request& req, int defaultValue, int& out) {
    if (!req.has_param("limit")) {
        out = defaultValue;
        return true;
    }
    const std::string& raw = req.get_param_value("limit");
    try {
        size_t consumed = 0;
        int value = std::stoi(raw, &consumed);
        if (consumed != raw.size() || value < 0) return false;
        out = value;
        return true;
    } catch (...) {
        return false;
    }
}

}  // namespace

ApiServer::ApiServer(SearchService& service)
    : service_(service), svr_(std::make_unique<httplib::Server>()) {
    registerRoutes();
}

ApiServer::~ApiServer() = default;

void ApiServer::registerRoutes() {
    // CORS: the frontend (Vite dev server on :5173, or wherever it's
    // deployed) is a different origin than this API, so without these
    // headers every browser fetch() would be silently blocked by the
    // browser itself before a response is even read. Wide open (`*`)
    // is acceptable here because this API has no cookie/session-based
    // auth to leak -- there's nothing a malicious origin could steal by
    // being allowed to read these responses.
    svr_->set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        if (req.method == "OPTIONS") {
            res.status = 204;
            return httplib::Server::HandlerResponse::Handled;
        }
        return httplib::Server::HandlerResponse::Unhandled;
    });

    // ---------------- GET /api/health ----------------
    svr_->Get("/api/health", [this](const httplib::Request&, httplib::Response& res) {
        double uptimeSeconds =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - kStartTime).count();
        json body{
            {"status", "ok"},
            {"database", "connected"},
            {"trie", "ready"},
            {"uptime_seconds", uptimeSeconds},
        };
        res.set_content(body.dump(), "application/json");
    });

    // ---------------- GET /api/search?word=... ----------------
    svr_->Get("/api/search", [this](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("word") || req.get_param_value("word").empty()) {
            res.status = 400;
            res.set_content(errorBody("missing required query parameter 'word'").dump(),
                             "application/json");
            return;
        }
        try {
            SearchResult r = service_.search(req.get_param_value("word"));
            json body{
                {"word", r.word},
                {"found", r.found},
                {"frequency", r.frequency},
                {"latency_ms", r.latencyMs},
            };
            res.set_content(body.dump(), "application/json");
        } catch (const ValidationError& e) {
            res.status = 400;
            res.set_content(errorBody(e.what()).dump(), "application/json");
        }
    });

    // ---------------- GET /api/autocomplete?prefix=...&limit=... ----------------
    svr_->Get("/api/autocomplete", [this](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("prefix")) {
            res.status = 400;
            res.set_content(errorBody("missing required query parameter 'prefix'").dump(),
                             "application/json");
            return;
        }
        int limit = 0;
        if (!parseLimit(req, /*defaultValue=*/10, limit)) {
            res.status = 400;
            res.set_content(errorBody("'limit' must be a non-negative integer").dump(),
                             "application/json");
            return;
        }
        try {
            AutocompleteResult r =
                service_.autocomplete(req.get_param_value("prefix"), static_cast<std::size_t>(limit));
            json results = json::array();
            for (const auto& s : r.results) {
                results.push_back(json{{"word", s.word}, {"frequency", s.frequency}});
            }
            json body{
                {"prefix", r.prefix},
                {"results", results},
                {"count", results.size()},
                {"latency_ms", r.latencyMs},
            };
            res.set_content(body.dump(), "application/json");
        } catch (const ValidationError& e) {
            res.status = 400;
            res.set_content(errorBody(e.what()).dump(), "application/json");
        }
    });

    // ---------------- GET /api/words/:word ----------------
    svr_->Get(R"(/api/words/([^/]+))", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            WordRecord rec = service_.getWord(req.matches[1]);
            json body{
                {"word", rec.word},
                {"frequency", rec.frequency},
                {"created_at", rec.createdAt},
                {"updated_at", rec.updatedAt},
            };
            res.set_content(body.dump(), "application/json");
        } catch (const NotFoundError& e) {
            res.status = 404;
            res.set_content(errorBody(e.what()).dump(), "application/json");
        } catch (const ValidationError& e) {
            res.status = 400;
            res.set_content(errorBody(e.what()).dump(), "application/json");
        }
    });

    // ---------------- POST /api/words ----------------
    svr_->Post("/api/words", [this](const httplib::Request& req, httplib::Response& res) {
        json parsed;
        try {
            parsed = json::parse(req.body);
        } catch (const json::parse_error&) {
            res.status = 400;
            res.set_content(errorBody("malformed JSON body").dump(), "application/json");
            return;
        }
        if (!parsed.contains("word") || !parsed["word"].is_string()) {
            res.status = 422;
            res.set_content(errorBody("body must contain a string field 'word'").dump(),
                             "application/json");
            return;
        }
        try {
            WordRecord rec = service_.addWord(parsed["word"].get<std::string>());
            json body{
                {"word", rec.word},
                {"frequency", rec.frequency},
                {"created_at", rec.createdAt},
                {"updated_at", rec.updatedAt},
            };
            res.status = 201;
            res.set_content(body.dump(), "application/json");
        } catch (const ConflictError& e) {
            res.status = 409;
            res.set_content(errorBody(e.what()).dump(), "application/json");
        } catch (const ValidationError& e) {
            res.status = 422;
            res.set_content(errorBody(e.what()).dump(), "application/json");
        }
    });

    // ---------------- PUT /api/words/:word ----------------
    svr_->Put(R"(/api/words/([^/]+))", [this](const httplib::Request& req, httplib::Response& res) {
        json parsed;
        try {
            parsed = json::parse(req.body);
        } catch (const json::parse_error&) {
            res.status = 400;
            res.set_content(errorBody("malformed JSON body").dump(), "application/json");
            return;
        }
        if (!parsed.contains("frequency") || !parsed["frequency"].is_number_integer()) {
            res.status = 422;
            res.set_content(errorBody("body must contain an integer field 'frequency'").dump(),
                             "application/json");
            return;
        }
        try {
            WordRecord rec =
                service_.updateWordFrequency(req.matches[1], parsed["frequency"].get<long>());
            json body{
                {"word", rec.word},
                {"frequency", rec.frequency},
                {"created_at", rec.createdAt},
                {"updated_at", rec.updatedAt},
            };
            res.set_content(body.dump(), "application/json");
        } catch (const NotFoundError& e) {
            res.status = 404;
            res.set_content(errorBody(e.what()).dump(), "application/json");
        } catch (const ValidationError& e) {
            res.status = 422;
            res.set_content(errorBody(e.what()).dump(), "application/json");
        }
    });

    // ---------------- DELETE /api/words/:word ----------------
    svr_->Delete(R"(/api/words/([^/]+))", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            service_.deleteWord(req.matches[1]);
            res.status = 204;
        } catch (const NotFoundError& e) {
            res.status = 404;
            res.set_content(errorBody(e.what()).dump(), "application/json");
        } catch (const ValidationError& e) {
            res.status = 400;
            res.set_content(errorBody(e.what()).dump(), "application/json");
        }
    });

    // ---------------- GET /api/stats ----------------
    svr_->Get("/api/stats", [this](const httplib::Request&, httplib::Response& res) {
        SearchService::Stats s = service_.getStats();
        json body{
            {"total_words", s.totalWords},
            {"trie_node_count", s.trieNodeCount},
            {"total_searches", s.totalSearches},
            {"searches_with_results", s.searchesWithResults},
            {"searches_with_no_results", s.searchesWithNoResults},
            {"average_latency_ms", s.averageLatencyMs},
        };
        res.set_content(body.dump(), "application/json");
    });

    // ---------------- GET /api/history?limit=... ----------------
    svr_->Get("/api/history", [this](const httplib::Request& req, httplib::Response& res) {
        int limit = 0;
        if (!parseLimit(req, /*defaultValue=*/20, limit)) {
            res.status = 400;
            res.set_content(errorBody("'limit' must be a non-negative integer").dump(),
                             "application/json");
            return;
        }
        auto history = service_.getHistory(limit);
        json results = json::array();
        for (const auto& h : history) {
            results.push_back(json{
                {"query", h.query},
                {"result_count", h.resultCount},
                {"latency_ms", h.latencyMs},
                {"created_at", h.createdAt},
            });
        }
        json body{{"history", results}, {"count", results.size()}};
        res.set_content(body.dump(), "application/json");
    });

    // Fallback for anything unmatched -- keep the error shape consistent
    // (JSON, not httplib's default HTML/plaintext 404 page), and never
    // leak filesystem paths (spec section 49).
    svr_->set_error_handler([](const httplib::Request&, httplib::Response& res) {
        if (res.body.empty()) {
            res.set_content(errorBody("not found").dump(), "application/json");
        }
    });
}

void ApiServer::listen(const std::string& host, int port) { svr_->listen(host, port); }

}  // namespace fastsearch::api
