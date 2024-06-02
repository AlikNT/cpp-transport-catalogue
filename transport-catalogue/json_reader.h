#pragma once

#include <iostream>

#include "json.h"
#include "transport_catalogue.h"
#include "map_renderer.h"

/*
 * Здесь можно разместить код наполнения транспортного справочника данными из JSON,
 * а также код обработки запросов к базе и формирование массива ответов в формате JSON
 */
using namespace std::literals;

namespace request {

std::vector<std::string> ParseRoute(const json::Node &node);

void MakeCatalogueFromJSON(const json::Document& doc, data::TransportCatalogue& catalogue);

void MakeStatOfBus(const StatRequest& stat_request, json::Array& stat_array, const data::TransportCatalogue &catalogue);

void MakeStatOfStop(const StatRequest& stat_request, json::Array& stat_array, const data::TransportCatalogue &catalogue);

void MakeStatOfMap(const StatRequest &stat_request, json::Array &stat_array, render::MapRenderer &map_renderer);

json::Document StatRequestToJSON(const json::Document &doc, const data::TransportCatalogue &catalogue);

svg::Color ColorFromJsonToSvg(const json::Node &color);

RenderSettings LoadRenderSettings(const json::Document& doc);

} // namespace request



