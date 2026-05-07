#ifndef TASK_HPP
#define TASK_HPP

#include <string>
#include <sstream>

enum class Action {
    ENCRYPT,
    DECRYPT
};

struct Task {
    std::string filePath;
    Action action;

    // Clean constructor
    Task(Action act, const std::string &path)
        : filePath(path), action(act) {}

    // Convert to string(for queue usage)
    std::string toString() const {
        std::ostringstream oss;
        oss << filePath << "," << (action == Action::ENCRYPT ? "ENCRYPT" : "DECRYPT");
        return oss.str();
    }

    // Parse from string
    static Task fromString(const std::string &taskData){
        std::istringstream iss(taskData);
        std::string filePath, actionStr;

        if (std::getline(iss, filePath, ',') && std::getline(iss, actionStr)){
            Action action = (actionStr == "ENCRYPT") ? Action::ENCRYPT : Action::DECRYPT;
            return Task(action, filePath);
        } else {
            throw std::runtime_error("Invalid task data format");
        }
    }
};

#endif