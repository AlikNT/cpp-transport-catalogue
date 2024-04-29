#include <iomanip>
#include "stat_reader.h"

#include <set>

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
		std::string output_string = "Bus "s + stat_command.description + ": "s + std::to_string(transport_catalogue.GetStopsOfBus(stat_command.description));
		output_string += " stops on route, "s + std::to_string(transport_catalogue.GetUniqueStopsOfBus(stat_command.description));
		output_string += " unique stops, ";
		output << output_string << std::setprecision(6) << transport_catalogue.GetBusRouteLength(stat_command.description) << " route length"s << std::endl;
	}
}

void PrintStatOfStop(const StatCommand& stat_command, const data::TransportCatalogue& transport_catalogue, std::ostream& output) {
    std::set<std::string_view> buses;
    if (!transport_catalogue.GetBusesOfStop(stat_command.description, buses)) {
        output << "Stop "s << stat_command.description << ": not found"s << std::endl;
    }
    else {
        if (buses.empty()) {
            output << "Stop "s << stat_command.description << ": no buses"s << std::endl;
        }
        else {
            output << "Stop "s << stat_command.description << ": buses"s;
            for (auto bus : buses) {
                output << " "s << bus;
            }
            output << std::endl;
        }
    }
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