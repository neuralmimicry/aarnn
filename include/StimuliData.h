// StimuliData.h

#include <string>
#include <vector>
#include <nlohmann/json.hpp> // JSON library

struct StimuliData {
    std::string receptorType;
    std::vector<double> values;
};

// Serialization
std::string serializeStimuliData(const StimuliData& data) {
    nlohmann::json j;
    j["receptorType"] = data.receptorType;
    j["values"] = data.values;
    return j.dump();
}

// Deserialization
StimuliData deserializeStimuliData(const std::string& jsonString) {
    StimuliData data;
    auto j = nlohmann::json::parse(jsonString);
    data.receptorType = j["receptorType"];
    data.values = j["values"].get<std::vector<double>>();
    return data;
}
