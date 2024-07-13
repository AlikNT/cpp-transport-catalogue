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
    for (const auto &stop: node.AsArray()) {
        stops.emplace_back(stop.AsString());
    }

    return stops;
}

data::TransportCatalogue MakeCatalogueFromJSON(const json::Document &doc) {
    data::TransportCatalogue catalogue;
    const json::Array &base_requests = doc.GetRoot().AsDict().at("base_requests"s).AsArray();
    for (const auto &requests: base_requests) {
        const auto &request = requests.AsDict();
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
            for (const auto &[stop, distance]: request.at("road_distances"s).AsDict()) {
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

router::TransportCatalogueRouter MakeCatatalogueRouter(const json::Document &doc, const data::TransportCatalogue &catalogue) {
    const RoutingSettings routing_settings = LoadRoutingSettings(doc);
    return router::TransportCatalogueRouter{catalogue, routing_settings};
}

json::Node MakeStatOfStop(const StatRequest &stat_request, const data::TransportCatalogue &catalogue) {
    const data::Stop *stop_ptr = catalogue.GetStop(stat_request.name);
    if (!stop_ptr) {
        return json::Builder{}
                .StartDict()
                .Key("error_message"s).Value("not found"s)
                .Key("request_id"s).Value(stat_request.id)
                .EndDict()
                .Build();
    }
    const std::set<std::string_view> buses = catalogue.GetBusesByStop(stop_ptr);
    auto json_builder = json::Builder{};
    json_builder.StartDict()
        .Key("buses"s)
        .StartArray();
    for (auto bus: buses) {
        json_builder.Value(std::string(bus));
    }
    json_builder.EndArray()
        .Key("request_id"s).Value(stat_request.id)
        .EndDict();
    return json_builder.Build();
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

json::Node MakeStatOfRoute(const StatRequest &stat_request, router::TransportCatalogueRouter &router) {
    auto route = router.BuildRoute(stat_request.from, stat_request.to);
    auto json_builder = json::Builder{};
    if (!route.has_value()) {
        return json::Builder{}.StartDict()
            .Key("request_id"s).Value(stat_request.id)
            .Key("error_message"s).Value("not found"s)
            .EndDict()
            .Build();
    }
    json_builder.StartDict()
        .Key("items"s)
        .StartArray();
    for (const auto&[is_wait, stop, bus, weight, span_count]: route->route) {
        if (is_wait) {
            json_builder.StartDict()
                .Key("stop_name"s).Value(stop->name)
                .Key("time"s).Value(weight)
                .Key("type"s).Value("Wait"s)
                .EndDict();
        } else {
            json_builder.StartDict()
                .Key("bus"s).Value(bus->name)
                .Key("span_count"s).Value(span_count)
                .Key("time"s).Value(weight)
                .Key("type"s).Value("Bus"s)
                .EndDict();
        }
    }
    json_builder.EndArray()
        .Key("request_id").Value(stat_request.id)
        .Key("total_time"s).Value(route->weight)
        .EndDict();
    return json_builder.Build();
}



json::Document StatRequestsToJSON(const json::Document &doc, const data::TransportCatalogue &catalogue,
                                  router::TransportCatalogueRouter &router) {
    const json::Array &stat_requests = doc.GetRoot().AsDict().at("stat_requests"s).AsArray();
    auto json_builder = json::Builder{};
    json_builder.StartArray();
    for (const auto &requests: stat_requests) {
        const auto &request = requests.AsDict();
        StatRequest stat_request;
        stat_request.id = request.at("id"s).AsInt();
        stat_request.type = request.at("type"s).AsString();
        if (stat_request.type == "Bus"s) {
            stat_request.name = request.at("name"s).AsString();
            json_builder.Value(MakeStatOfBus(stat_request, catalogue).GetValue());
        } else if (stat_request.type == "Stop"s) {
            stat_request.name = request.at("name"s).AsString();
            json_builder.Value(MakeStatOfStop(stat_request, catalogue).GetValue());
        } else if (stat_request.type == "Map"s) {
            // Получаем параметры отрисовки из объекта json
            request::RenderSettings r_settings = request::LoadRenderSettings(doc);

            // Создаем и инициализируем объект для проекции гео координат на плоскость
            std::vector<geo::Coordinates> all_coordinates = catalogue.GetAllCoordinates();
            render::SphereProjector projector(all_coordinates.begin(), all_coordinates.end(),
                                              r_settings.width, r_settings.height, r_settings.padding);

            // Создаем объект, который отвечает за визуализацию транспортного справочника
            render::MapRenderer map_renderer(catalogue, projector, r_settings);

            json_builder.Value(MakeStatOfMap(stat_request, map_renderer).GetValue());
        } else if (stat_request.type == "Route"s) {
            stat_request.from = request.at("from"s).AsString();
            stat_request.to = request.at("to"s).AsString();
            json_builder.Value(MakeStatOfRoute(stat_request, router).GetValue());
        }
    }
    json_builder.EndArray();
    return json::Document{json_builder.Build()};
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
        return svg::Rgb{r, g, b};
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
    for (const auto &color: color_palette) {
        result.color_palette.emplace_back(ColorFromJsonToSvg(color));
    }
    return result;
}

RoutingSettings LoadRoutingSettings(const json::Document &doc) {
    RoutingSettings result;
    const json::Dict &routing_settings = doc.GetRoot().AsDict().at("routing_settings"s).AsDict();
    result.bus_wait_time = routing_settings.at("bus_wait_time"s).AsInt();
    result.bus_velocity = routing_settings.at("bus_velocity"s).AsDouble();
    return result;
}
} // namespace request
