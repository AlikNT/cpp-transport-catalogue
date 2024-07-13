#include <numeric>
#include <set>
#include <algorithm>
#include <unordered_set>
#include <stdexcept>

#include "transport_catalogue.h"

namespace data {
void TransportCatalogue::AddBusRoute(std::string_view bus_name, const std::vector<std::string_view> &stops,
                                     bool is_roundtrip) {
    std::vector<const Stop *> route;
    for (auto &stop: stops) {
        if (stops_.count(stop) == 0) {
            AddStop(stop, {});
        }
        route.push_back(stops_[stop]);
    }
    buses_catalog_.push_back(Bus{std::string(bus_name), std::move(route), is_roundtrip});
    buses_.insert({buses_catalog_.back().name, &buses_catalog_.back()});
}

void TransportCatalogue::AddStop(std::string_view stop_name, const geo::Coordinates &coordinates) {
    if (stops_.count(stop_name) == 0) {
        stops_catalog_.push_back(Stop{std::string(stop_name), coordinates});
        stops_[stops_catalog_.back().name] = &stops_catalog_.back();
    } else {
        const_cast<Stop *>(stops_[stop_name])->coordinates = coordinates;
    }
}

const Bus *TransportCatalogue::GetBus(std::string_view bus_name) const {
    if (buses_.count(bus_name) > 0) {
        return buses_.at(bus_name);
    }
    return nullptr;
}

const Stop *TransportCatalogue::GetStop(std::string_view stop_name) const {
    if (stops_.count(stop_name) > 0) {
        return stops_.at(stop_name);
    }
    return nullptr;
}

size_t TransportCatalogue::GetNumberStopsOfBus(const Bus *bus_ptr) const {
    return bus_ptr->route.size();
}

size_t TransportCatalogue::GetNumberUniqueStopsOfBus(const Bus *bus_ptr) const {
    std::unordered_set<std::string_view> unique_stops;
    for (const auto &stop_ptr: bus_ptr->route) {
        unique_stops.insert(stop_ptr->name);
    }
    return unique_stops.size();
}

double TransportCatalogue::GetStraightLength(const Bus *bus_ptr) const {
    const auto it_begin = bus_ptr->route.begin();
    const auto it_end = bus_ptr->route.end();
    double result = 0;
    for (auto it = it_begin; it + 1 != it_end; ++it) {
        result += geo::ComputeDistance((*it)->coordinates, (*(it + 1))->coordinates);
    }
    return result;
}

void TransportCatalogue::SetStopsDistance(const std::string_view stop1, const std::string_view stop2,
                                          const int distance) {
    distances_[std::pair{stops_[stop1], stops_[stop2]}] = distance;
}

int TransportCatalogue::GetDistance(const Stop *stop_ptr_1, const Stop *stop_ptr_2) const {
    if (distances_.count(std::pair(stop_ptr_1, stop_ptr_2)) > 0) {
        return distances_.at(std::pair(stop_ptr_1, stop_ptr_2));
    }
    return distances_.at(std::pair(stop_ptr_2, stop_ptr_1));
}

size_t TransportCatalogue::GetBusesCount() const {
    return buses_.size();
}

size_t TransportCatalogue::GetStopsCount() const {
    return stops_.size();
}

const BusesType &TransportCatalogue::GetBuses() const {
    return buses_;
}

int TransportCatalogue::GetFactLength(const Bus *bus_ptr) const {
    const auto it_begin = bus_ptr->route.begin();
    const auto it_end = bus_ptr->route.end();
    int result = 0;
    for (auto it = it_begin; it + 1 != it_end; ++it) {
        if (distances_.count({*it, *(it + 1)}) > 0) {
            result += distances_.at({*it, *(it + 1)});
        } else {
            result += distances_.at({*(it + 1), *it});
        }
    }
    return result;
}

std::set<std::string_view> TransportCatalogue::GetBusesByStop(const Stop *stop_ptr_arg) const {
    std::set<std::string_view> buses;
    for (auto &[bus_name, bus_ptr]: buses_) {
        for (const auto stop_ptr: bus_ptr->route) {
            if (stop_ptr == stop_ptr_arg) {
                buses.insert(bus_name);
            }
        }
    }
    return buses;
}

std::vector<geo::Coordinates> TransportCatalogue::GetAllCoordinates() const {
    std::vector<geo::Coordinates> coordinates;
    for (const auto &[_, bus_ptr]: buses_) {
        if (bus_ptr->route.empty()) {
            continue;
        }
        for (const auto &stop: bus_ptr->route) {
            coordinates.emplace_back(stop->coordinates);
        }
    }
    return coordinates;
}

SortedBusesType TransportCatalogue::GetSortedBuses() const {
    return SortedBusesType{buses_.begin(), buses_.end()};
}

SortedStopsType TransportCatalogue::GetSortedStops() const {
    SortedStopsType result;
    for (const auto &[_, bus_ptr]: buses_) {
        for (const auto &stop_ptr: bus_ptr->route) {
            result.insert({stop_ptr->name, stop_ptr});
        }
    }
    return result;
}
} // namespace data
