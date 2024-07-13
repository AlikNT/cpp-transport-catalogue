#pragma once
/*
 * В этом файле вы можете разместить классы/структуры, которые являются частью предметной области (domain)
 * вашего приложения и не зависят от транспортного справочника. Например, Автобусные маршруты и Остановки.
 *
 * Их можно было бы разместить и в transport_catalogue.h, однако вынесение их в отдельный
 * заголовочный файл может оказаться полезным, когда дело дойдёт до визуализации карты маршрутов:
 * визуализатор карты (map_renderer) можно будет сделать независящим от транспортного справочника.
 *
 * Если структура вашего приложения не позволяет так сделать, просто оставьте этот файл пустым.
 *
 */
#include <string>
#include <vector>

#include "geo.h"
#include "svg.h"

namespace data {

struct Stop {
    std::string name;
    geo::Coordinates coordinates;
};

struct Bus {
    std::string name;
    std::vector<const Stop *> route;
    bool is_roundtrip;
};

struct StopsHasher {
    std::size_t operator()(const std::pair<const Stop *, const Stop *> &p) const;

private:
    std::hash<const Stop *> d_hasher;
};

} // namespace data

namespace request {

struct RenderOffset {
    double dx;
    double dy;
};

struct RenderSettings {
    double width;
    double height;
    double padding;
    double line_width;
    double stop_radius;
    int bus_label_font_size;
    RenderOffset bus_label_offset;
    int stop_label_font_size;
    RenderOffset stop_label_offset;
    svg::Color underlayer_color;
    double underlayer_width;
    std::vector<svg::Color> color_palette;
};

struct RoutingSettings {
    int bus_wait_time = 0;
    double bus_velocity = 0;
};


struct StatRequest {
    int id;
    std::string type;
    std::string name;
    std::string from;
    std::string to;
};

struct Route {
    bool is_wait;
    const data::Stop* stop;
    const data::Bus* bus;
    double weight;
    int span_count;
};

struct StatRouteInfo {
    double weight;
    std::vector<Route> route;
};

} // namespace request