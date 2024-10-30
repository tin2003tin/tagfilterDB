#ifndef COMPILER_HANDLER_HPP
#define COMPILER_HANDLER_HPP

#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>

class CompilerHandler;
typedef void (CompilerHandler::*HandlerMethod)(std::vector<std::string> &input, std::vector<std::string> &output);

#define HANDLER(cls) class cls : public CompilerHandler

#define FUNC(func) void func(std::vector<std::string> &input, std::vector<std::string> &output)

#define REGISTER(func) std::make_pair(#func, static_cast<HandlerMethod>(&func))

#define DECLARE(...)                                                              \
    std::unordered_map<std::string, HandlerMethod> getFunctionNames() override    \
    {                                                                             \
        std::unordered_map<std::string, HandlerMethod> functions = {__VA_ARGS__}; \
        return functions;                                                         \
    }

class CompilerHandler
{
public:
    virtual ~CompilerHandler() = default;
    virtual std::unordered_map<std::string, HandlerMethod> getFunctionNames() = 0;
};

#endif