#include "input_reader.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <regex>

using namespace std::literals;

namespace input {

/**
 * Парсит координаты (широта, долгота) из вектора параметров
 */
geo::Coordinates ParseCoordinates(const std::vector<std::string_view>& params) {
    double lat = std::stod(std::string(params[0]));
    double lng = std::stod(std::string(params[1]));
    return {lat, lng};
}

} // namespace input


namespace detail {
/**
 * Удаляет пробелы в начале и конце строки
 */
std::string_view Trim(std::string_view string) {
    const auto start = string.find_first_not_of(' ');
    if (start == string.npos) {
        return {};
    }
    return string.substr(start, string.find_last_not_of(' ') + 1 - start);
}

/**
 * Разбивает строку string на n строк, с помощью указанного символа-разделителя delim
 */
std::vector<std::string_view> Split(std::string_view string, char delim) {
    std::vector<std::string_view> result;

    size_t pos = 0;
    while ((pos = string.find_first_not_of(' ', pos)) < string.length()) {
        auto delim_pos = string.find(delim, pos);
        if (delim_pos == string.npos) {
            delim_pos = string.size();
        }
        if (auto substr = Trim(string.substr(pos, delim_pos - pos)); !substr.empty()) {
            result.push_back(substr);
        }
        pos = delim_pos + 1;
    }

    return result;
}
} // namespace detail

namespace input {
/**
 * Парсит маршрут.
 * Для кольцевого маршрута (A>B>C>A) возвращает массив названий остановок [A,B,C,A]
 * Для некольцевого маршрута (A-B-C-D) возвращает массив названий остановок [A,B,C,D,C,B,A]
 */
std::vector<std::string_view> ParseRoute(std::string_view route) {
    if (route.find('>') != route.npos) {
        return detail::Split(route, '>');
    }

    auto stops = detail::Split(route, '-');
    std::vector<std::string_view> results(stops.begin(), stops.end());
    results.insert(results.end(), std::next(stops.rbegin()), stops.rend());

    return results;
}
/**
 * Парсит фактический расстояния до других остановок
 */
std::vector<std::pair<std::string, int>> ParseDistances(const std::vector<std::string_view>& params) {
    std::regex distance_pattern(R"(^\d+(?=m))");
    std::regex stop_pattern(R"(to (.+))");
    std::smatch distance_match;
    std::smatch stop_match;

    std::vector<std::pair<std::string, int>> results;

    for (size_t i = 2; i < params.size(); ++i) {
        std::string input = std::string(params[i]);
        if (!std::regex_search(input, distance_match, distance_pattern)) {
            return {};
        }
        if (!std::regex_search(input, stop_match, stop_pattern)) {
            return {};
        }
        results.emplace_back(stop_match[1], std::stoi(distance_match[0]));
    }
    return results;
}

std::vector<std::string_view> ParseStopLine(std::string_view stop_line) {
    return detail::Split(stop_line, ',');
}

CommandDescription ParseCommandDescription(std::string_view line) {
    auto colon_pos = line.find(':');
    if (colon_pos == line.npos) {
        return {};
    }

    auto space_pos = line.find(' ');
    if (space_pos >= colon_pos) {
        return {};
    }

    auto not_space = line.find_first_not_of(' ', space_pos);
    if (not_space >= colon_pos) {
        return {};
    }

    return { std::string(line.substr(0, space_pos)),
             std::string(line.substr(not_space, colon_pos - not_space)),
             std::string(line.substr(colon_pos + 1)) };
}

void InputReader::ParseLine(std::string_view line) {
    auto command_description = ParseCommandDescription(line);
    if (command_description) {
        commands_.push_back(std::move(command_description));
    }
}

void InputReader::ApplyCommands([[maybe_unused]] data::TransportCatalogue& catalog) const {
    // Реализуйте метод самостоятельно
    for (auto& command : commands_) {
        if (command.command == "Stop"s) {
            std::vector<std::string_view> params = ParseStopLine(command.description);
            catalog.AddStop(command.id, ParseCoordinates(params));
            for (const auto& [stop, distance] : ParseDistances(params)) {
                if (!catalog.GetStop(stop)) {
                    catalog.AddStop(stop, {});
                }
                catalog.SetStopsDistance(command.id, stop, distance);
            }
        } else if (command.command == "Bus"s) {
            catalog.AddBusRoute(command.id, ParseRoute(command.description));
        }
    }
}
} // namespace input