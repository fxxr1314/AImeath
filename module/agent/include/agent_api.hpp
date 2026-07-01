#pragma once

#include <string>

class IAgent {
public:
    virtual ~IAgent() = default;

    virtual bool openApp(const std::string& name, const std::string& paramsJson) = 0;
    virtual bool controlApp(const std::string& name, const std::string& commandJson) = 0;
    virtual bool closeApp(const std::string& name) = 0;
    virtual void stop() = 0;
};
