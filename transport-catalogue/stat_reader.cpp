#include <iomanip>
#include <set>

#include "stat_reader.h"
#include "input_reader.h"

using namespace std::literals;

namespace detail {
request::StatCommand ParseRequest(std::string_view line) {
    const size_t first_space_pos = line.find_first_of(' ');
    std::string_view command = Trim(line.substr(0, first_space_pos));
    std::string_view description = Trim(line.substr(first_space_pos + 1));
    return { std::string(command), std::string(description) };
}
} // namespace detail

namespace request {

void PrintStatOfBus(const StatCommand& stat_command, const data::TransportCatalogue& transport_catalogue, std::ostream& output) {
    auto bus_ptr = transport_catalogue.GetRoute(stat_command.description);
    if (!bus_ptr || bus_ptr->route.empty()) {
        output << "Bus "s << stat_command.description << ": not found"s << std::endl;
    }
    else {
        std::string output_string = "Bus "s + stat_command.description + ": "s + std::to_string(transport_catalogue.GetStopsOfBus(bus_ptr));
        output_string += " stops on route, "s + std::to_string(transport_catalogue.GetUniqueStopsOfBus(bus_ptr));
        output_string += " unique stops, ";
        int fact_route_length = transport_catalogue.GetBusRouteFactLength(bus_ptr);
        double curvature = fact_route_length / transport_catalogue.GetBusRouteStraightLength(bus_ptr);
        output << output_string << std::setprecision(6) << fact_route_length << " route length, "s << curvature << " curvature"s <<  std::endl;
    }
}

void PrintStatOfStop(const StatCommand& stat_command, const data::TransportCatalogue& transport_catalogue, std::ostream& output) {
    const data::Stop* stop_ptr = transport_catalogue.GetStop(stat_command.description);
    if (!stop_ptr) {
        output << "Stop "s << stat_command.description << ": not found"s << std::endl;
        return;
    }
    std::set<std::string_view> buses = transport_catalogue.GetBusesOfStop(stop_ptr);
    if (buses.empty()) {
        output << "Stop "s << stat_command.description << ": no buses"s << std::endl;
        return;
    }
    output << "Stop "s << stat_command.description << ": buses"s;
    for (auto bus : buses) {
        output << " "s << bus;
    }
    output << std::endl;
}

void ParseAndPrintStat(const data::TransportCatalogue& transport_catalogue, std::string_view request, std::ostream& output) {
    // Реализуйте самостоятельно
    const auto& stat_command = detail::ParseRequest(request);
    if (stat_command.command == "Bus"s) {
        PrintStatOfBus(stat_command, transport_catalogue, output);
    } else if (stat_command.command == "Stop"s) {
        PrintStatOfStop(stat_command, transport_catalogue, output);
    }
}
} // namespace request