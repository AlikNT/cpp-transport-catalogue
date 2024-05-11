#pragma once

#include <deque>
#include <set>
#include <string>
#include <vector>
#include <unordered_map>

#include "geo.h"


namespace data {
struct Stop {
    std::string name;
    geo::Coordinates coordinates;
};

struct Bus {
    std::string name;
    std::vector <const Stop*> route;
};

struct StopsHasher {
    std::size_t operator()(const std::pair<std::string_view, std::string_view>& p) const;
private:
    std::hash<std::string_view> d_hasher;
};

class TransportCatalogue {
    // Реализуйте класс самостоятельно
    using StopsType = std::unordered_map<std::string_view, const Stop*>;
    using BusesType = std::unordered_map<std::string_view, const Bus*>;
    using DistanceType = std::unordered_map<std::pair<std::string_view, std::string_view>, int, StopsHasher>;

public:
    void AddBusRoute(std::string_view bus_name, const std::vector<std::string_view>& stops);

    void AddStop(std::string_view stop_name, const geo::Coordinates& coordinates);

    const Bus* GetRoute(std::string_view bus_name) const;

    const Stop* GetStop(std::string_view stop_name) const;

    size_t GetStopsOfBus(std::string_view bus_name) const;

    size_t GetUniqueStopsOfBus(std::string_view bus_name) const;

    double GetBusRouteStraightLength(std::string_view bus_name) const;

    int GetBusRouteFactLength(std::string_view bus_name) const;

    bool GetBusesOfStop(std::string_view stop_name, std::set<std::string_view>& buses) const;

    void SetStopsDistance(std::string_view stop1, std::string_view stop2, int distance);

private:
    std::deque<Stop> stops_catalog_;
    std::deque<Bus> buses_catalog_;
    StopsType stops_;
    BusesType buses_;
    DistanceType distances_;
};
} // namespace data