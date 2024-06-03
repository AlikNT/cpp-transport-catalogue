/*
 * Здесь можно разместить код наполнения транспортного справочника данными из JSON,
 * а также код обработки запросов к базе и формирование массива ответов в формате JSON
 */

#include <sstream>
#include "json_reader.h"

namespace request {

std::vector<std::string> ParseRoute(const json::Node &node) {
    std::vector<std::string> stops;
    for (const auto& stop : node.AsArray()) {
        stops.emplace_back(stop.AsString());
    }

    return stops;
}

data::TransportCatalogue MakeCatalogueFromJSON(const json::Document &doc) {
    data::TransportCatalogue catalogue;
    const json::Array &base_requests = doc.GetRoot().AsMap().at("base_requests"s).AsArray();
    for (const auto &requests: base_requests) {
        const auto& request = requests.AsMap();
        const std::string &request_type = request.at("type"s).AsString();
        const std::string &request_name = request.at("name"s).AsString();
        if (request_type == "Bus"s) {
            bool is_roundtrip = request.at("is_roundtrip"s).AsBool();
            std::vector<std::string> stops_tmp = ParseRoute(request.at("stops"s).AsArray());
            std::vector<std::string_view> stops(stops_tmp.begin(), stops_tmp.end());
            if (!is_roundtrip) {
                stops.insert(stops.end(), std::next(stops_tmp.rbegin()), stops_tmp.rend());
            }
            catalogue.AddBusRoute(request_name, stops, is_roundtrip);

        } else if (request_type == "Stop"s) {
            double lat = request.at("latitude"s).AsDouble();
            double lng = request.at("longitude"s).AsDouble();
            catalogue.AddStop(request_name, geo::Coordinates{lat, lng});
            for (const auto& [stop, distance] : request.at("road_distances"s).AsMap()) {
                if (!catalogue.GetStop(stop)) {
                    catalogue.AddStop(stop, {});
                }
                catalogue.SetStopsDistance(request_name, stop, distance.AsInt());
            }
        }
    }
    return catalogue;
}

json::Array& MakeStatOfBus(const StatRequest &stat_request, json::Array &stat_array, const data::TransportCatalogue &catalogue) {
    auto bus_ptr = catalogue.GetBus(stat_request.name);
    if (!bus_ptr || bus_ptr->route.empty()) {
        stat_array.emplace_back(json::Dict{
                {"request_id"s, stat_request.id},
                {"error_message"s, "not found"s}
        });
        return stat_array;
    }
    int fact_route_length = catalogue.GetFactLength(bus_ptr);
    double curvature = fact_route_length / catalogue.GetStraightLength(bus_ptr);
    int stop_count = static_cast<int>(catalogue.GetNumberStopsOfBus(bus_ptr));
    int unique_stop_count = static_cast<int>(catalogue.GetNumberUniqueStopsOfBus(bus_ptr));
    stat_array.emplace_back(json::Dict{
            {"curvature"s, curvature},
            {"request_id"s, stat_request.id},
            {"route_length"s, fact_route_length},
            {"stop_count"s, stop_count},
            {"unique_stop_count"s, unique_stop_count}
    });
    return stat_array;
}

json::Array& MakeStatOfStop(const StatRequest &stat_request, json::Array &stat_array, const data::TransportCatalogue &catalogue) {
    const data::Stop* stop_ptr = catalogue.GetStop(stat_request.name);
    if (!stop_ptr) {
        stat_array.emplace_back(json::Dict{
                {"request_id"s, stat_request.id},
                {"error_message"s, "not found"s}
        });
        return stat_array;
    }
    std::set<std::string_view> buses = catalogue.GetBusesByStop(stop_ptr);
    json::Array buses_array;
    for (auto bus : buses) {
        buses_array.emplace_back(std::string (bus));
    }
    stat_array.emplace_back(json::Dict{
            {"buses"s, buses_array},
            {"request_id"s, stat_request.id}
    });
    return stat_array;
}

json::Array& MakeStatOfMap(const StatRequest &stat_request, json::Array &stat_array, render::MapRenderer &map_renderer) {
    // Получаем параметры отрисовки из объекта json
    svg::Document map_doc;
    map_renderer.RenderMap(map_doc);
    std::ostringstream map_stringstream;
    map_doc.Render(map_stringstream);

    stat_array.emplace_back(json::Dict{
            {"map"s, map_stringstream.str()},
            {"request_id"s, stat_request.id}
    });
    return stat_array;
}

json::Document StatRequestToJSON(const json::Document &doc, const data::TransportCatalogue &catalogue) {
    const json::Array &base_requests = doc.GetRoot().AsMap().at("stat_requests"s).AsArray();
    // Создаем массив нового json
    json::Array stat_array;
    for (const auto &requests: base_requests) {
        const auto& request = requests.AsMap();
        StatRequest stat_request;
        stat_request.id = request.at("id"s).AsInt();
        stat_request.type = request.at("type"s).AsString();
        if (stat_request.type == "Bus"s) {
            stat_request.name = request.at("name"s).AsString();
            stat_array = MakeStatOfBus(stat_request, stat_array, catalogue);
        } else if (stat_request.type == "Stop"s) {
            stat_request.name = request.at("name"s).AsString();
            stat_array = MakeStatOfStop(stat_request, stat_array, catalogue);
        } else if (stat_request.type == "Map"s) {
            // Получаем параметры отрисовки из объекта json
            request::RenderSettings r_settings = request::LoadRenderSettings(doc);
            // Создаем и инициализируем объект для проекции гео координат на плоскость
            std::vector<geo::Coordinates> all_coordinates = catalogue.GetAllCoordinates();
            render::SphereProjector projector(all_coordinates.begin(), all_coordinates.end(),
                                              r_settings.width, r_settings.height, r_settings.padding);
            // Создаем объект, который отвечает за визуализацию транспортного справочника
            render::MapRenderer map_renderer(catalogue, projector, r_settings);

            stat_array = MakeStatOfMap(stat_request, stat_array, map_renderer);
        }
    }
    return json::Document{stat_array};
}

svg::Color ColorFromJsonToSvg(const json::Node &color) {
    svg::Color result;
    if (color.IsString()) {
        return color.AsString();
    }
    uint8_t r = color.AsArray()[0].AsInt();
    uint8_t g = color.AsArray()[1].AsInt();
    uint8_t b = color.AsArray()[2].AsInt();
    if (color.AsArray().size() == 3) {
        return  svg::Rgb{r, g, b};
    }
    double opacity = color.AsArray()[3].AsDouble();
    return svg::Rgba{r, g, b, opacity};
}

RenderSettings LoadRenderSettings(const json::Document &doc) {
    RenderSettings result;
    const json::Dict &render_settings = doc.GetRoot().AsMap().at("render_settings"s).AsMap();
    result.width = render_settings.at("width"s).AsDouble();
    result.height = render_settings.at("height"s).AsDouble();
    result.padding = render_settings.at("padding"s).AsDouble();
    result.line_width = render_settings.at("line_width"s).AsDouble();
    result.stop_radius = render_settings.at("stop_radius"s).AsDouble();
    result.bus_label_font_size = render_settings.at("bus_label_font_size"s).AsInt();
    result.bus_label_offset.dx = render_settings.at("bus_label_offset"s).AsArray()[0].AsDouble();
    result.bus_label_offset.dy = render_settings.at("bus_label_offset"s).AsArray()[1].AsDouble();
    result.stop_label_font_size = render_settings.at("stop_label_font_size"s).AsInt();
    result.stop_label_offset.dx = render_settings.at("stop_label_offset"s).AsArray()[0].AsDouble();
    result.stop_label_offset.dy = render_settings.at("stop_label_offset"s).AsArray()[1].AsDouble();
    json::Node underlayer_color = render_settings.at("underlayer_color"s);
    result.underlayer_color = ColorFromJsonToSvg(underlayer_color);
    result.underlayer_width = render_settings.at("underlayer_width"s).AsDouble();
    json::Array color_palette = render_settings.at("color_palette"s).AsArray();
    for (const auto& color : color_palette) {
        result.color_palette.emplace_back(ColorFromJsonToSvg(color));
    }
    return result;
}
} // namespace request
