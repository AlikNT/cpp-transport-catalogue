#include <numeric>
#include <set>
#include <algorithm>
#include <unordered_set>
#include <stdexcept>

#include "transport_catalogue.h"

namespace data {
void TransportCatalogue::AddBusRoute(std::string_view bus_name, const std::vector<std::string_view>& stops) {
    std::vector <const Stop*> route;
    for (auto& stop : stops) {
        if (stops_.count(stop) == 0) {
            AddStop(stop, {});
        }
        route.push_back(stops_[stop]);
    }
    buses_catalog_.push_back(Bus{std::string(bus_name), std::move(route) });
    buses_.insert({buses_catalog_.back().name, &buses_catalog_.back()});
}

void TransportCatalogue::AddStop(std::string_view stop_name, const geo::Coordinates& coordinates) {
    if (stops_.count(stop_name) == 0) {
        stops_catalog_.push_back(Stop{std::string(stop_name), coordinates});
        stops_[stops_catalog_.back().name] = &stops_catalog_.back();
    } else {
        const_cast<Stop*>(stops_[stop_name])->coordinates = coordinates;
    }
}

const Bus* TransportCatalogue::GetRoute(std::string_view bus_name) const {
    if (buses_.count(bus_name) > 0) {
        return buses_.at(bus_name);
    }
    return nullptr;
}

const Stop* TransportCatalogue::GetStop(std::string_view stop_name) const {
    if (stops_.count(stop_name) > 0) {
        return stops_.at(stop_name);
    }
    return nullptr;
}

size_t TransportCatalogue::GetStopsOfBus(const Bus *bus_ptr) const {
    return bus_ptr->route.size();
}

size_t TransportCatalogue::GetUniqueStopsOfBus(const Bus *bus_ptr) const {
    std::unordered_set<std::string_view> unique_stops;
    for (const auto& stop_ptr : bus_ptr->route) {
        unique_stops.insert(stop_ptr->name);
    }
    return unique_stops.size();
}

double TransportCatalogue::GetBusRouteStraightLength(const Bus *bus_ptr) const {
    const auto it_begin = bus_ptr->route.begin();
    const auto it_end = bus_ptr->route.end();
    double result = 0;
    for (auto it = it_begin; it + 1 != it_end; ++it) {
        result += geo::ComputeDistance((*it)->coordinates, (*(it + 1))->coordinates);
    }
    return result;
}

void TransportCatalogue::SetStopsDistance(std::string_view stop1, std::string_view stop2, int distance) {
    distances_[std::pair{std::string_view(stops_[stop1]->name), std::string_view(stops_[stop2]->name)}] = distance;
}


int TransportCatalogue::GetBusRouteFactLength(const Bus *bus_ptr) const {
    const auto it_begin = bus_ptr->route.begin();
    const auto it_end = bus_ptr->route.end();
    int result = 0;
    for (auto it = it_begin; it + 1 != it_end; ++it) {
        if (distances_.count({(*it)->name, (*(it + 1))->name}) > 0) {
            result += distances_.at({(*it)->name, (*(it + 1))->name});
        } else {
            result += distances_.at({(*(it + 1))->name, (*it)->name});
        }
    }
    return result;
}

std::set<std::string_view> TransportCatalogue::GetBusesOfStop(const Stop* stop_ptr_arg) const {
    std::set<std::string_view> buses;
    for (auto& [bus_name, bus_ptr] : buses_) {
        for (const auto stop_ptr : bus_ptr->route) {
            if (stop_ptr == stop_ptr_arg) {
                buses.insert(bus_name);
            }
        }
    }
    return buses;
}

std::size_t StopsHasher::operator()(const std::pair<std::string_view, std::string_view>& p) const {
    size_t h_s1 = d_hasher(p.first);
    size_t h_s2 = d_hasher(p.second) * 37;

    return h_s1 + h_s2;
}
} // namespace data
