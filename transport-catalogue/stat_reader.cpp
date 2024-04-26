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
void ParseAndPrintStat(const data::TransportCatalogue& transport_catalogue, std::string_view request, std::ostream& output) {
    // Реализуйте самостоятельно
    const auto& stat_command = detail::ParseRequest(request);
    if (stat_command.command == "Bus"s) {
        if (transport_catalogue.GetRoute(stat_command.description).empty()) {
            output << request << ": not found"s << std::endl;
        }
        else {
            std::string output_string = std::string(request) + ": "s + std::to_string(transport_catalogue.GetStopsOfBus(stat_command.description));
            output_string += " stops on route, "s + std::to_string(transport_catalogue.GetUniqueStopsOfBus(stat_command.description));
            output_string += " unique stops, ";
            output << output_string << std::setprecision(6) << transport_catalogue.GetBusRouteLength(stat_command.description) << " route length"s << std::endl;
        }
    } else if (stat_command.command == "Stop"s) {
        std::set<std::string_view> buses;
        if (!transport_catalogue.GetBusesOfStop(stat_command.description, buses)) {
            output << request << ": not found"s << std::endl;
        } else {
            if (buses.empty()) {
                output << request << ": no buses"s << std::endl;
            } else {
                output << request << ": buses"s;
                for (auto bus : buses) {
                    output << " "s << bus;
                }
                output << std::endl;
            }
        }
    }
}
} // namespace request