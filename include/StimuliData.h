// StimuliData.h
#pragma once
#include <string>
#include <vector>
#include <boost/json.hpp> // JSON library

struct StimuliData {
    std::string receptorType;
    std::vector<double> values;
};

// Serialization
inline std::string serializeStimuliData(const StimuliData& data) {
    // Build a JSON object
    boost::json::object obj;
    obj["receptorType"] = data.receptorType;

    // Convert the std::vector<double> into a boost::json::array
    boost::json::array arr;
    arr.reserve(data.values.size());
    for (double v : data.values) {
        arr.push_back(v);
    }
    obj["values"] = std::move(arr);

    // Serialize the object to a string
    return boost::json::serialize(obj);
}

// Deserialization
inline StimuliData deserializeStimuliData(const std::string& jsonString) {
    StimuliData data;

    // Parse into a boost::json::value, then get the object view
    boost::json::value jv = boost::json::parse(jsonString);
    const auto& obj = jv.as_object();

    // Extract receptorType (as_string returns a boost::json::string)
    data.receptorType = std::string(obj.at("receptorType").as_string());

    // Extract the values array
    const auto& arr = obj.at("values").as_array();
    data.values.reserve(arr.size());
    for (const auto& el : arr) {
        data.values.push_back(el.as_double());
    }

    return data;
}