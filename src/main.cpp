#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include "json.hpp"
#include "tagfilterdb/service1.hpp"
#include "tagfilterdb/logging.hpp"

using json = nlohmann::json;

void set_cors_headers(httplib::Response &res) {
    res.set_header("Access-Control-Allow-Origin", "*"); // Allow requests from any origin
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS"); // Allowed methods
    res.set_header("Access-Control-Allow-Headers", "Content-Type"); // Allowed headers
}

int main() {
    using namespace httplib;
    // HTTP Server
    Server svr;

    // Route to return a simple greeting message in JSON format
    svr.Get("/hi", [](const Request &, Response &res) {
        set_cors_headers(res);  // Set CORS headers
        json response_json = {
            {"message", "Hello World!"}
        };
        
        res.set_content(response_json.dump(), "application/json");
    });

    // Route to return numbers as JSON
    svr.Get(R"(/numbers/(\d+))", [&](const Request& req, Response& res) {
        set_cors_headers(res);  // Set CORS headers
        auto numbers = req.matches[1];
        json response_json = {
            {"number", numbers}
        };
        
        res.set_content(response_json.dump(), "application/json");
    });

    // Route to return user ID as JSON
    svr.Get("/users/:id", [&](const Request& req, Response& res) {
        set_cors_headers(res);  // Set CORS headers
        auto user_id = req.path_params.at("id");
        json response_json = {
            {"user_id", user_id}
        };
      
        res.set_content(response_json.dump(), "application/json");
    });

    svr.Get("/compiler/details", [](const Request &, Response &res) {
         set_cors_headers(res);  // Set CORS headers
        tin_compiler::CompilerService cs;
        json response_json;
        response_json["Detail"] = cs.toJson();
       
        res.set_content(response_json.dump(), "application/json");
    });

    // Route to return body and parameters in JSON format
    svr.Post("/compiler/plainText", [](const Request& req, Response& res) {
        set_cors_headers(res);

        json response_json;
        if (req.has_header("Content-Length")) {
            response_json["content_length"] = req.get_header_value("Content-Length");
        }
        LOG_DEBUG(req.body)
        tin_compiler::CompilerService cs;

        cs.setInput(req.body);
        auto ast = cs.Parse(cs.Tokenize());
        if (ast != nullptr) {
            response_json["AST"] = tin_compiler::ASTNode::toJson(ast);
        } else {
            response_json["AST"] = "NULL";
        }

        res.set_content("", "text/plain");
        res.set_content(response_json.dump(), "application/json");
    });

    // Start the server
    svr.listen("0.0.0.0", 8080);
}
