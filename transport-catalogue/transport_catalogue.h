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
};

class TransportCatalogue {
    // Реализуйте класс самостоятельно
    using StopsType = std::unordered_map<std::string_view, const Stop*>;
    using RouteType = std::vector<const Stop*>;
    using RoutesType = std::unordered_map<std::string_view, RouteType>;

public:
    void AddBusRoute(std::string_view bus_name, const std::vector<std::string_view>& stops);
    void AddStop(std::string_view stop_name, const geo::Coordinates& coordinates);
    RouteType GetRoute(std::string_view bus_name) const;
    geo::Coordinates GetStop(std::string_view stop_name) const;
    size_t GetStopsOfBus(std::string_view bus_name) const;
    size_t GetUniqueStopsOfBus(std::string_view bus_name) const;
    double GetBusRouteLength(std::string_view bus_name) const;
    bool GetBusesOfStop(std::string_view stop_name, std::set<std::string_view>& buses) const;

private:
    std::deque<Stop> stops_catalog_;
    std::deque<Bus> buses_catalog_;
    StopsType stops_;
    RoutesType routes_;
};
} // namespace data