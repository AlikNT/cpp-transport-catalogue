#include <numeric>
#include <set>
#include "transport_catalogue.h"

#include <algorithm>
#include <unordered_set>


namespace data {
void TransportCatalogue::AddBusRoute(std::string_view bus_name, const std::vector<std::string_view>& stops) {
    buses_catalog_.push_back(Bus{std::string(bus_name)});
    RouteType result;
    for (auto& stop : stops) {
        result.push_back(stops_[stop]);
    }
    routes_.insert({ buses_catalog_.back().name, std::move(result) });
}

void TransportCatalogue::AddStop(std::string_view stop_name, const geo::Coordinates& coordinates) {
    stops_catalog_.push_back(Stop{ std::string(stop_name), coordinates });
    stops_.insert({ stops_catalog_.back().name, &stops_catalog_.back() });
}

TransportCatalogue::RouteType TransportCatalogue::GetRoute(std::string_view bus_name) const {
    if (routes_.count(bus_name) > 0) {
        return routes_.at(bus_name);
    }
    return {};
}

geo::Coordinates TransportCatalogue::GetStop(std::string_view stop_name) const {
    return stops_.at(stop_name)->coordinates;
}

size_t TransportCatalogue::GetStopsOfBus(std::string_view bus_name) const {
    return routes_.at(bus_name).size();
}

size_t TransportCatalogue::GetUniqueStopsOfBus(const std::string_view bus_name) const {
    std::unordered_set<std::string_view> unique_stops;
    for (const auto& stop_ptr : routes_.at(bus_name)) {
        unique_stops.insert(stop_ptr->name);
    }
    return unique_stops.size();
}

double TransportCatalogue::GetBusRouteLength(const std::string_view bus_name) const {
    const auto it_begin = routes_.at(bus_name).begin();
    const auto it_end = routes_.at((bus_name)).end();
    double result = 0;
    for (auto it = it_begin; it + 1 != it_end; ++it) {
        result += ComputeDistance((*it)->coordinates, (*(it + 1))->coordinates);
    }
    return result;
}

bool TransportCatalogue::GetBusesOfStop(std::string_view stop_name, std::set<std::string_view>& buses) const {
    if (stops_.count(stop_name) == 0) {
        return false;
    }
    for (auto& [bus, stops] : routes_) {
        for (const auto stop_value : stops) {
            if (stop_value->name == stop_name) {
                buses.insert(bus);
            }
        }
    }
    return true;
}
} // namespace data
