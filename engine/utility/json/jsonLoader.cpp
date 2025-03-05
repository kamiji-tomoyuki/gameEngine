#include "JsonLoader.h"
#include <fstream>
#include <iostream>

bool JsonLoader::LoadFromFile(const std::string& filePath) {
    const std::string fullPath = "resources/jsons/" + filePath;

    std::ifstream file(fullPath);
    if (!file.is_open()) {
        return false;
    }
    try {
        file >> jsonData_;
        return true;
    }
    catch (const std::exception& e) {
        return false;
    }
}

bool JsonLoader::SaveToFile(const std::string& filePath) const {
    const std::string fullPath = "resources/jsons/" + filePath;

    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    try {
        file << jsonData_.dump(4);
        return true;
    }
    catch (const std::exception& e) {
        return false;
    }
}

std::optional<nlohmann::json> JsonLoader::GetValue(const std::string& key) const {
    if (jsonData_.contains(key)) {
        return jsonData_.at(key);
    }
    return std::nullopt;
}

void JsonLoader::SetValue(const std::string& key, const nlohmann::json& value) {
    jsonData_[key] = value;
}

void JsonLoader::RemoveValue(const std::string& key) {
    jsonData_.erase(key);
}

void JsonLoader::Clear() {
    jsonData_.clear();
}

bool JsonLoader::GetName(const std::string& filePath, const std::string& targetName) const {
    const std::string fullPath = "resources/jsons/" + filePath;

    std::ifstream file(fullPath);
    if (!file.is_open()) {
        return false;
    }
    nlohmann::json jsonData;
    try {
        file >> jsonData;
    }
    catch (const std::exception&) {
        return false;
    }
    if (!jsonData.contains("objects") || !jsonData["objects"].is_array()) {
        return false;
    }
    for (const auto& obj : jsonData["objects"]) {
        if (obj.contains("name") && obj["name"] == targetName) {
            return true;
        }
    }
    return false;
}

std::optional<std::vector<float>> JsonLoader::GetWorldTransform(const std::string& filePath, const std::string& targetName) const {
    const std::string fullPath = "resources/jsons/" + filePath;

    std::ifstream file(fullPath);
    if (!file.is_open()) {
        return std::nullopt;
    }
    nlohmann::json jsonData;
    try {
        file >> jsonData;
    }
    catch (const std::exception&) {
        return std::nullopt;
    }
    if (!jsonData.contains("objects") || !jsonData["objects"].is_array()) {
        return std::nullopt;
    }
    for (const auto& obj : jsonData["objects"]) {
        if (obj.contains("name") && obj["name"] == targetName) {
            if (obj.contains("transform") && obj["transform"].contains("translation")) {
                std::vector<float> position = obj["transform"]["translation"];
                if (position.size() == 3) {
                    return position;
                }
            }
        }
    }
    return std::nullopt;
}