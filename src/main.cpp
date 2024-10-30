#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include "json.hpp"
#include "tagfilterdb/service1.hpp"
#include "tagfilterdb/logging.hpp"

using json = nlohmann::json;

int main() {
    using namespace httplib;
    // HTTP Server
    Server svr;

    // Route to return a simple greeting message in JSON format
    svr.Get("/hi", [](const Request &, Response &res) {
        json response_json = {
            {"message", "Hello World!"}
        };
        res.set_content(response_json.dump(), "application/json");
    });

    // Route to return numbers as JSON
    svr.Get(R"(/numbers/(\d+))", [&](const Request& req, Response& res) {
        auto numbers = req.matches[1];
        json response_json = {
            {"number", numbers}
        };
        res.set_content(response_json.dump(), "application/json");
    });

    // Route to return user ID as JSON
    svr.Get("/users/:id", [&](const Request& req, Response& res) {
        auto user_id = req.path_params.at("id");
        json response_json = {
            {"user_id", user_id}
        };
        res.set_content(response_json.dump(), "application/json");
    });

     svr.Get("/compiler/details", [](const Request &, Response &res) {
        tin_compiler::CompilerService cs;
        json response_json;
        response_json["Detail"] = cs.toJson();
        res.set_content(response_json.dump(), "application/json");
    });

    // Route to return body and parameters in JSON format
    svr.Get("/body-header-param", [](const Request& req, Response& res) {
        json response_json;
        if (req.has_header("Content-Length")) {
            response_json["content_length"] = req.get_header_value("Content-Length");
        }
        std::string body = "SELECT ((   id,name,email)) FROM user WHERE salary= 10000 JOIN salary ON id =employeId   $";
        LOG_DEBUG(body)
        tin_compiler::CompilerService cs;
        cs.setInput(body);
        auto ast =  cs.Parse(cs.Tokenize());
        if (ast != nullptr) {
            response_json["AST"] = tin_compiler::ASTNode::toJson(ast);
        } else {
            response_json["AST"] = "NULL";
        }

        res.set_content(response_json.dump(), "application/json");
    });

    // Route to stop the server
    svr.Get("/stop", [&](const Request& req, Response& res) {
        svr.stop();
        json response_json = {
            {"status", "Server stopped."}
        };
        res.set_content(response_json.dump(), "application/json");
    });

    // Start the server
    svr.listen("0.0.0.0", 8080);
}
