/*
 * Здесь можно разместить код наполнения транспортного справочника данными из JSON,
 * а также код обработки запросов к базе и формирование массива ответов в формате JSON
 */

#include <sstream>
#include "json_reader.h"
#include "json_builder.h"

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
    const json::Array &base_requests = doc.GetRoot().AsDict().at("base_requests"s).AsArray();
    for (const auto &requests: base_requests) {
        const auto& request = requests.AsDict();
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
            for (const auto& [stop, distance] : request.at("road_distances"s).AsDict()) {
                if (!catalogue.GetStop(stop)) {
                    catalogue.AddStop(stop, {});
                }
                catalogue.SetStopsDistance(request_name, stop, distance.AsInt());
            }
        }
    }
    return catalogue;
}

json::Node MakeStatOfBus(const StatRequest &stat_request, const data::TransportCatalogue &catalogue) {
    auto bus_ptr = catalogue.GetBus(stat_request.name);
    if (!bus_ptr || bus_ptr->route.empty()) {
        return json::Builder{}
            .StartDict()
                .Key("request_id"s).Value(stat_request.id)
                .Key("error_message"s).Value("not found"s)
            .EndDict()
            .Build();
    }
    int fact_route_length = catalogue.GetFactLength(bus_ptr);
    double curvature = fact_route_length / catalogue.GetStraightLength(bus_ptr);
    int stop_count = static_cast<int>(catalogue.GetNumberStopsOfBus(bus_ptr));
    int unique_stop_count = static_cast<int>(catalogue.GetNumberUniqueStopsOfBus(bus_ptr));
    return json::Builder{}
        .StartDict()
            .Key("curvature"s).Value(curvature)
            .Key("request_id"s).Value(stat_request.id)
            .Key("route_length"s).Value(fact_route_length)
            .Key("stop_count"s).Value(stop_count)
            .Key("unique_stop_count"s).Value(unique_stop_count)
        .EndDict()
        .Build();
}

json::Node MakeStatOfStop(const StatRequest &stat_request, const data::TransportCatalogue &catalogue) {
    const data::Stop* stop_ptr = catalogue.GetStop(stat_request.name);
    if (!stop_ptr) {
        return json::Builder{}
            .StartDict()
                .Key("error_message"s).Value("not found"s)
                .Key("request_id"s).Value(stat_request.id)
            .EndDict()
            .Build();
    }
    std::set<std::string_view> buses = catalogue.GetBusesByStop(stop_ptr);
    json::Array buses_array;
    for (auto bus : buses) {
        buses_array.emplace_back(std::string (bus));
    }
    return json::Builder{}
        .StartDict()
            .Key("buses"s).Value(std::move(buses_array))
            .Key("request_id"s).Value(stat_request.id)
        .EndDict()
        .Build();
}

json::Node MakeStatOfMap(const StatRequest &stat_request, render::MapRenderer &map_renderer) {
    // Получаем параметры отрисовки из объекта json
    svg::Document map_doc;
    map_renderer.RenderMap(map_doc);
    std::ostringstream map_sstr;
    map_doc.Render(map_sstr);
    return json::Builder{}
        .StartDict()
            .Key("map"s).Value(std::move(map_sstr.str()))
            .Key("request_id"s).Value(stat_request.id)
        .EndDict()
        .Build();
}

json::Document StatRequestToJSON(const json::Document& doc, const data::TransportCatalogue& catalogue) {
    auto x = json::Builder{};
    const json::Array &base_requests = doc.GetRoot().AsDict().at("stat_requests"s).AsArray();
    json::Array stat_array;
    for (const auto& requests : base_requests) {
        const auto& request = requests.AsDict();
        StatRequest stat_request;
        stat_request.id = request.at("id"s).AsInt();
        stat_request.type = request.at("type"s).AsString();
        if (stat_request.type == "Bus"s) {
            stat_request.name = request.at("name"s).AsString();
            stat_array.emplace_back(MakeStatOfBus(stat_request, catalogue));
        } else if (stat_request.type == "Stop"s) {
            stat_request.name = request.at("name"s).AsString();
            stat_array.emplace_back(MakeStatOfStop(stat_request, catalogue));
        } else if (stat_request.type == "Map"s) {
            // Получаем параметры отрисовки из объекта json
            request::RenderSettings r_settings = request::LoadRenderSettings(doc);

            // Создаем и инициализируем объект для проекции гео координат на плоскость
            std::vector<geo::Coordinates> all_coordinates = catalogue.GetAllCoordinates();
            render::SphereProjector projector(all_coordinates.begin(), all_coordinates.end(),
                                              r_settings.width, r_settings.height, r_settings.padding);

            // Создаем объект, который отвечает за визуализацию транспортного справочника
            render::MapRenderer map_renderer(catalogue, projector, r_settings);

            stat_array.emplace_back(MakeStatOfMap(stat_request, map_renderer));
        }
    }
    return json::Document{json::Builder{}.Value(stat_array).Build()};
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
    const json::Dict &render_settings = doc.GetRoot().AsDict().at("render_settings"s).AsDict();
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
