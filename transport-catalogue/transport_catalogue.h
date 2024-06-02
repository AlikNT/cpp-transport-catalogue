#pragma once

#include <deque>
#include <set>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>

#include "domain.h"

namespace data {

    using StopsType = std::unordered_map<std::string_view, const Stop*>;
    using BusesType = std::unordered_map<std::string_view, const Bus*>;
    using SortedBusesType = std::map<std::string_view, const Bus*>;
    using SortedStopsType = std::map<std::string_view, const Stop*>;
    using DistanceType = std::unordered_map<std::pair<const Stop*, const Stop*>, int, StopsHasher>;

class TransportCatalogue {
    // Реализуйте класс самостоятельно

public:
    void AddBusRoute(std::string_view bus_name, const std::vector<std::string_view> &stops,
                     bool is_roundtrip);
    void AddStop(std::string_view stop_name, const geo::Coordinates& coordinates);

    const Bus* GetBus(std::string_view bus_name) const;

    SortedBusesType GetSortedBuses() const;

    SortedStopsType GetSortedStops() const;

    const Stop* GetStop(std::string_view stop_name) const;

    size_t GetNumberStopsOfBus(const Bus *bus_ptr) const;

    size_t GetNumberUniqueStopsOfBus(const Bus *bus_ptr) const;

    std::vector<geo::Coordinates> GetAllCoordinates() const;

    double GetStraightLength(const Bus *bus_ptr) const;

    int GetFactLength(const Bus *bus_ptr) const;

    std::set<std::string_view> GetBusesByStop(const Stop *stop_ptr_arg) const;

    void SetStopsDistance(std::string_view stop1, std::string_view stop2, int distance);

private:
    std::deque<Stop> stops_catalog_;
    std::deque<Bus> buses_catalog_;
    StopsType stops_;
    BusesType buses_;
    DistanceType distances_;
};
} // namespace data