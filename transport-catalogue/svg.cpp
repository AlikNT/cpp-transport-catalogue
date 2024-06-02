#include "svg.h"

/*
 * Место для вашей svg-библиотеки
 */

#include <sstream>
#include <utility>
#include <unordered_map>

namespace svg {

using namespace std::literals;

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();

    // Делегируем вывод тега своим подклассам
    RenderObject(context);

    context.out << std::endl;
}

// ---------- Circle ------------------

Circle& Circle::SetCenter(Point center)  {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius)  {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\""sv;
    RenderAttrs(context.out);
    out << "/>"sv;
}

Polyline &Polyline::AddPoint(Point point) {
    points_.push_back(point);
    return *this;
}

void Polyline::RenderObject(const RenderContext &context) const {
    auto& out = context.out;
    out << "<polyline points=\""sv;
    for (auto it = points_.begin(); it != points_.end(); ++it) {
        out << (*it).x << ","sv << (*it).y;
        if (it + 1 != points_.end()) {
            out << " "sv;
        }
    }
    out << "\""sv;
    RenderAttrs(context.out);
    out << "/>"sv;
}

void Text::RenderObject(const RenderContext &context) const {
    auto& out = context.out;
    out << "<text x=\""sv << pos_.x << "\" y=\""sv << pos_.y << "\" "sv;
    out << "dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\" "sv;
    out << "font-size=\""sv << size_ << "\""sv;
    if (!font_family_.empty()) {
        out << " font-family=\""sv << font_family_ << "\""sv;
    }
    if (!font_weight_.empty()) {
        out << " font-weight=\""sv << font_weight_ << "\""sv;
    }
    RenderAttrs(context.out);
    out << ">"sv;
    std::unordered_map<char, std::string_view > escaping_map {
            {'"', "&quot;"sv},
            {'<', "&lt;"sv},
            {'>', "&gt;"sv},
            {'\'', "&apos;"sv},
            {'&', "&amp;"sv}
    };
    for (auto s : data_) {
        if (escaping_map.count(s)) {
            out << escaping_map[s];
        } else {
            out << s;
        }
    }
    out << "</text>"sv;
}

Text &Text::SetPosition(Point pos) {
    pos_ = pos;
    return *this;
}

Text &Text::SetOffset(Point offset) {
    offset_ = offset;
    return *this;
}

Text &Text::SetFontSize(uint32_t size) {
    size_ = size;
    return *this;
}

Text &Text::SetFontFamily(std::string font_family) {
    font_family_ = std::move(font_family);
    return *this;
}

Text &Text::SetFontWeight(std::string font_weight) {
    font_weight_ = std::move(font_weight);
    return *this;
}

Text &Text::SetData(std::string data) {
    data_ = std::move(data);
    return *this;
}

void Document::AddPtr(std::unique_ptr<Object> &&obj) {
    objects_.emplace_back(std::move(obj));
}

void Document::Render(std::ostream &out) const {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;

    RenderContext render_context{out, 2, 2};
    for (auto& obj : objects_) {
        obj->Render(render_context);
    }

    out << "</svg>"sv;
}

std::ostream &operator<<(std::ostream &out, const StrokeLineCap &data) {
    switch (data) {
        case StrokeLineCap::BUTT:
            out << "butt"sv;
            break;
        case StrokeLineCap::ROUND:
            out << "round"sv;
            break;
        case StrokeLineCap::SQUARE:
            out << "square"sv;
    }
    return out;
}

std::ostream &operator<<(std::ostream &out, StrokeLineJoin data) {
    switch (data) {
        case StrokeLineJoin::ARCS:
            out << "arcs"sv;
            break;
        case StrokeLineJoin::BEVEL:
            out << "bevel"sv;
            break;
        case StrokeLineJoin::MITER:
            out << "miter"sv;
            break;
        case StrokeLineJoin::MITER_CLIP:
            out << "miter-clip"sv;
            break;
        case StrokeLineJoin::ROUND:
            out << "round"sv;
            break;
    }
    return out;
}

std::ostream& ColorRepresentation::operator()(std::monostate) {
    out << "none"s;
    return out;
}

std::ostream& ColorRepresentation::operator()(std::string color) {
    out << color;
    return out;
}

std::ostream& ColorRepresentation::operator()(const Rgb& color) {
    out << "rgb("s << static_cast<int>(color.red) << ","s << static_cast<int>(color.green) << ","s
        << static_cast<int>(color.blue) << ")"s;
    return out;
}

std::ostream& ColorRepresentation::operator()(const Rgba& color) {
    out << "rgba("s << static_cast<int>(color.red) << ","s << static_cast<int>(color.green) << ","s
        << static_cast<int>(color.blue) << ","s << color.opacity << ")"s;
    return out;
}


std::ostream &operator<<(std::ostream &out, Color color) {
    return std::visit(ColorRepresentation{out}, color);
}
}  // namespace svg