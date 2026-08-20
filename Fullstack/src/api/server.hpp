#pragma once

#include <memory>

#include "service/search_service.hpp"

namespace httplib {
class Server;
}

namespace fastsearch::api {

// Thin HTTP layer over SearchService. This class contains NO business
// logic of its own (spec section 2: "API routes should not contain
// business logic") -- every handler's job is: parse/validate the HTTP
// request shape, call into SearchService, and translate the result (or
// exception) into a JSON response with the right status code. All the
// actual decisions (what counts as a valid word, how ranking works, the
// DB-then-Trie write order) live in SearchService and below.
class ApiServer {
public:
    explicit ApiServer(SearchService& service);
    ~ApiServer();

    // Blocks, serving HTTP on `port` until the process is killed.
    void listen(const std::string& host, int port);

private:
    SearchService& service_;
    std::unique_ptr<httplib::Server> svr_;

    void registerRoutes();
};

}  // namespace fastsearch::api
