#include "map_renderer.h"

/*
 * В этом файле вы можете разместить код, отвечающий за визуализацию карты маршрутов в формате SVG.
 * Визуализация маршрутов вам понадобится во второй части итогового проекта.
 * Пока можете оставить файл пустым.
 */

namespace render {

using namespace std::literals;

MapElement::MapElement(const request::RenderSettings &r_settings, const SphereProjector &projector)
    : r_settings_(r_settings)
    , projector_(projector) {
}

RouteLine::RouteLine(const data::Bus *bus_ptr, const svg::Color &color,
                     const request::RenderSettings &r_settings, const render::SphereProjector &projector)
        : MapElement(r_settings, projector)
        , bus_ptr_(bus_ptr)
        , color_(color) {
}

void RouteLine::Draw(svg::ObjectContainer &container) const {
    svg::Polyline polyline;
    for (const auto stop_ptr: bus_ptr_->route) {
        polyline.AddPoint(projector_(stop_ptr->coordinates));
    }
    container.Add(polyline.SetStrokeColor(color_).SetStrokeWidth(r_settings_.line_width).SetStrokeLineCap(
            svg::StrokeLineCap::ROUND).SetStrokeLineJoin(svg::StrokeLineJoin::ROUND).SetFillColor({}));
}

BusLabelUnderlayer::BusLabelUnderlayer(std::string_view text, const geo::Coordinates& coordinates, const request::RenderSettings &r_settings,
                                       const SphereProjector &projector)
        : MapElement (r_settings, projector)
        , text_(text)
        , coordinates_(coordinates) {
}

void BusLabelUnderlayer::Draw(svg::ObjectContainer &container) const {
    svg::Text text;
    svg::Point position = projector_(coordinates_);
    svg::Point offset {static_cast<double>(r_settings_.bus_label_offset.dx), static_cast<double>(r_settings_.bus_label_offset.dy)};
    text.SetData(text_).SetPosition(position).SetOffset(offset).SetFontFamily("Verdana"s).SetFontSize(
            r_settings_.bus_label_font_size).SetFontWeight("bold"s).SetFillColor(
            r_settings_.underlayer_color).SetStrokeColor(r_settings_.underlayer_color).SetStrokeWidth(
            r_settings_.underlayer_width).SetStrokeLineCap(svg::StrokeLineCap::ROUND).SetStrokeLineJoin(
            svg::StrokeLineJoin::ROUND);
    container.Add(text);
}

BusLabel::BusLabel(const data::Bus *bus_ptr, const svg::Color &color, const geo::Coordinates& coordinates, const request::RenderSettings &r_settings,
                   const SphereProjector &projector)
        : MapElement (r_settings, projector)
        , bus_ptr_(bus_ptr)
        , color_(color)
        , coordinates_(coordinates) {
}

void BusLabel::Draw(svg::ObjectContainer &container) const {
    svg::Text text;
    svg::Point position = projector_(coordinates_);
    svg::Point offset {static_cast<double>(r_settings_.bus_label_offset.dx), static_cast<double>(r_settings_.bus_label_offset.dy)};
    text.SetData(bus_ptr_->name).SetPosition(position).SetOffset(offset).SetFontFamily("Verdana"s).SetFontSize(
            r_settings_.bus_label_font_size).SetFontWeight("bold"s).SetFillColor(
            color_);
    container.Add(text);
}

StopSign::StopSign(const geo::Coordinates& coordinates, const request::RenderSettings &r_settings,
                   const SphereProjector &projector)
        : MapElement(r_settings, projector)
        , coordinates_(coordinates) {
}

void StopSign::Draw(svg::ObjectContainer &container) const {
    svg::Circle circle;
    svg::Point position = projector_(coordinates_);
    circle.SetRadius(r_settings_.stop_radius).SetCenter(position).SetFillColor("white"s);
    container.Add(circle);
}

StopLabelUnderlayer::StopLabelUnderlayer(std::string_view text, const geo::Coordinates &coordinates,
                                         const request::RenderSettings &r_settings, const SphereProjector &projector)
         : MapElement(r_settings, projector)
         , text_(text)
         , coordinates_(coordinates) {
}

void StopLabelUnderlayer::Draw(svg::ObjectContainer &container) const {
    svg::Text text;
    svg::Point position = projector_(coordinates_);
    svg::Point offset {static_cast<double>(r_settings_.stop_label_offset.dx), static_cast<double>(r_settings_.stop_label_offset.dy)};
    text.SetData(text_).SetPosition(position).SetOffset(offset).SetFontFamily("Verdana"s).SetFontSize(
            r_settings_.stop_label_font_size).SetFillColor(r_settings_.underlayer_color).SetStrokeColor(
            r_settings_.underlayer_color).SetStrokeWidth(r_settings_.underlayer_width).SetStrokeLineCap(
            svg::StrokeLineCap::ROUND).SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    container.Add(text);
}

StopLabel::StopLabel(std::string_view text, const geo::Coordinates &coordinates,
                     const request::RenderSettings &r_settings, const SphereProjector &projector)
         : MapElement(r_settings, projector)
         , text_(text)
         , coordinates_(coordinates) {
}

void StopLabel::Draw(svg::ObjectContainer &container) const {
    svg::Text text;
    svg::Point position = projector_(coordinates_);
    svg::Point offset {static_cast<double>(r_settings_.stop_label_offset.dx), static_cast<double>(r_settings_.stop_label_offset.dy)};
    text.SetData(text_).SetPosition(position).SetOffset(offset).SetFontFamily("Verdana"s).SetFontSize(
            r_settings_.stop_label_font_size).SetFillColor("black"s);
    container.Add(text);
}

MapRenderer::MapRenderer(const data::TransportCatalogue &catalogue, const render::SphereProjector &projector,
                                 const request::RenderSettings &r_settings)
        : projector_(projector)
        , r_settings_(r_settings)
        , sorted_buses_(catalogue.GetSortedBuses())
        , sorted_stops_(catalogue.GetSortedStops()) {

    RouteLinesRender();
    BusLabelsRender();
    StopSignsRender();
    StopLabelRender();
}

void MapRenderer::RouteLinesRender() {
    size_t color_size = r_settings_.color_palette.size();
    size_t bus_count = 0;
    for (const auto& [_, bus_ptr] : sorted_buses_) {
        // Если нет остановок у маршрута, ничего не выводим
        if (bus_ptr->route.empty()) {
            continue;
        }
        // Вывод линии маршрута
        picture_.emplace_back(std::make_unique<RouteLine>(bus_ptr, r_settings_.color_palette[bus_count % color_size], r_settings_, projector_));
        ++bus_count;
    }
}

void MapRenderer::BusLabelsRender() {
    size_t color_size = r_settings_.color_palette.size();
    size_t bus_count = 0;
    for (const auto& [_, bus_ptr] : sorted_buses_) {
        // Если нет остановок у маршрута, ничего не выводим
        if (bus_ptr->route.empty()) {
            continue;
        }
        auto route = bus_ptr->route;
        // Вывод названия маршрута на первой остановке делаем в любом случае
        picture_.emplace_back(
                std::make_unique<BusLabelUnderlayer>(bus_ptr->name, route[0]->coordinates,
                                                     r_settings_, projector_));
        picture_.emplace_back(
                std::make_unique<BusLabel>(bus_ptr, r_settings_.color_palette[bus_count % color_size], route[0]->coordinates,
                                           r_settings_, projector_));
        // Вывод названия маршрута на последней остановке делаем, если маршрут не кольцевой
        if (!bus_ptr->is_roundtrip && route[0] != route[route.size() / 2]) {
            picture_.emplace_back(
                    std::make_unique<BusLabelUnderlayer>(bus_ptr->name, route[route.size() / 2]->coordinates,
                                                         r_settings_, projector_));
            picture_.emplace_back(
                    std::make_unique<BusLabel>(bus_ptr, r_settings_.color_palette[bus_count % color_size], route[route.size() / 2]->coordinates,
                                               r_settings_, projector_));
        }
        ++bus_count;
    }
}

void MapRenderer::StopSignsRender() {
    for (const auto& [_, stop_ptr] : sorted_stops_) {
        picture_.emplace_back(std::make_unique<StopSign>(stop_ptr->coordinates, r_settings_, projector_));
    }
}

void MapRenderer::StopLabelRender() {
    for (const auto& [_, stop_ptr] : sorted_stops_) {
        picture_.emplace_back(
                std::make_unique<StopLabelUnderlayer>(stop_ptr->name, stop_ptr->coordinates, r_settings_, projector_));
        picture_.emplace_back(
                std::make_unique<StopLabel>(stop_ptr->name, stop_ptr->coordinates, r_settings_, projector_));
    }
}

void MapRenderer::RenderMap(svg::Document& doc) {
    for (const auto &object: picture_) {
        object->Draw(doc);
    }
}

bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}
} // namespace render