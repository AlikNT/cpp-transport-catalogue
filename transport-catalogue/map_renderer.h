#pragma once

/*
 * В этом файле вы можете разместить код, отвечающий за визуализацию карты маршрутов в формате SVG.
 * Визуализация маршртутов вам понадобится во второй части итогового проекта.
 * Пока можете оставить файл пустым.
 */

#include "transport_catalogue.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <vector>

namespace render {

inline const double EPSILON = 1e-6;
bool IsZero(double value);

class SphereProjector {
public:
    // points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                    double max_width, double max_height, double padding)
            : padding_(padding) //
    {
        // Если точки поверхности сферы не заданы, вычислять нечего
        if (points_begin == points_end) {
            return;
        }

        // Находим точки с минимальной и максимальной долготой
        const auto [left_it, right_it] = std::minmax_element(
                points_begin, points_end,
                [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        // Находим точки с минимальной и максимальной широтой
        const auto [bottom_it, top_it] = std::minmax_element(
                points_begin, points_end,
                [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        // Вычисляем коэффициент масштабирования вдоль координаты x
        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        // Вычисляем коэффициент масштабирования вдоль координаты y
        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            // Коэффициенты масштабирования по ширине и высоте ненулевые,
            // берём минимальный из них
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        } else if (width_zoom) {
            // Коэффициент масштабирования по ширине ненулевой, используем его
            zoom_coeff_ = *width_zoom;
        } else if (height_zoom) {
            // Коэффициент масштабирования по высоте ненулевой, используем его
            zoom_coeff_ = *height_zoom;
        }
    }

    // Проецирует широту и долготу в координаты внутри SVG-изображения
    svg::Point operator()(geo::Coordinates coords) const {
        return {
                (coords.lng - min_lon_) * zoom_coeff_ + padding_,
                (max_lat_ - coords.lat) * zoom_coeff_ + padding_
        };
    }

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

class MapElement : public svg::Drawable {
public:
    MapElement (const request::RenderSettings &r_settings, const SphereProjector &projector);

protected:
    const request::RenderSettings& r_settings_;
    const render::SphereProjector& projector_;
};

class RouteLine : public MapElement {
public:
    RouteLine(const data::Bus *bus_ptr, const svg::Color &color, const request::RenderSettings &r_settings,
              const SphereProjector &projector);

    void Draw(svg::ObjectContainer& container) const override;

private:
    const data::Bus* bus_ptr_;
    const svg::Color& color_;
};

class BusLabelUnderlayer : public MapElement {
public:
    BusLabelUnderlayer(std::string_view text, const geo::Coordinates& coordinates, const request::RenderSettings &r_settings,
                       const SphereProjector &projector);

    void Draw(svg::ObjectContainer& container) const override;

private:
    const std::string text_;
    const geo::Coordinates& coordinates_;
};

class BusLabel : public MapElement {
public:
    BusLabel(const data::Bus *bus_ptr, const svg::Color &color, const geo::Coordinates& coordinates, const request::RenderSettings &r_settings,
             const SphereProjector &projector);

    void Draw(svg::ObjectContainer& container) const override;

private:
    const data::Bus* bus_ptr_;
    const svg::Color& color_;
    const geo::Coordinates& coordinates_;
};

class StopSign : public MapElement {
public:
    StopSign(const geo::Coordinates& coordinates, const request::RenderSettings &r_settings,
             const SphereProjector &projector);

    void Draw(svg::ObjectContainer& container) const override;

private:
    const geo::Coordinates& coordinates_;
};

class StopLabelUnderlayer : public MapElement {
public:
    StopLabelUnderlayer(std::string_view text, const geo::Coordinates &coordinates,
                        const request::RenderSettings &r_settings, const SphereProjector &projector);

    void Draw(svg::ObjectContainer& container) const override;

private:
    const std::string text_;
    const geo::Coordinates& coordinates_;
};

class StopLabel : public MapElement {
public:
    StopLabel(std::string_view text, const geo::Coordinates& coordinates, const request::RenderSettings &r_settings,
             const SphereProjector &projector);

    void Draw(svg::ObjectContainer& container) const override;

private:
    const std::string text_;
    const geo::Coordinates& coordinates_;
};

class MapRenderer {
public:

    MapRenderer(const data::TransportCatalogue &catalogue, const render::SphereProjector &projector,
                const request::RenderSettings &r_settings);

   void RenderMap(svg::Document& doc);

private:
//    const data::TransportCatalogue& catalogue_;
    const SphereProjector& projector_;
    const request::RenderSettings& r_settings_;
    svg::Document document_;
    std::vector<std::unique_ptr<svg::Drawable>> picture_;
    const data::SortedBusesType sorted_buses_;
    const data::SortedStopsType sorted_stops_;

    void RouteLinesRender();

    void BusLabelsRender();

    void StopSignsRender();

    void StopLabelRender();
};

} // namespace render