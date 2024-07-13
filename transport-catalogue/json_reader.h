#pragma once

#include <iostream>

#include "json.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"

/*
 * Здесь можно разместить код наполнения транспортного справочника данными из JSON,
 * а также код обработки запросов к базе и формирование массива ответов в формате JSON
 */
using namespace std::literals;

namespace request {

std::vector<std::string> ParseRoute(const json::Node &node);

data::TransportCatalogue MakeCatalogueFromJSON(const json::Document &doc);

router::TransportCatalogueRouter MakeCatatalogueRouter(const json::Document& doc, const data::TransportCatalogue &catalogue);

json::Node MakeStatOfBus(const StatRequest& stat_request, const data::TransportCatalogue &catalogue);

json::Node MakeStatOfStop(const StatRequest& stat_request, const data::TransportCatalogue &catalogue);

json::Node MakeStatOfMap(const StatRequest &stat_request, render::MapRenderer &map_renderer);

json::Node MakeStatOfRoute(const StatRequest &stat_request, router::TransportCatalogueRouter &router);

json::Document StatRequestsToJSON(const json::Document &doc, const data::TransportCatalogue &catalogue, router::TransportCatalogueRouter &router);

svg::Color ColorFromJsonToSvg(const json::Node &color);

RenderSettings LoadRenderSettings(const json::Document& doc);

RoutingSettings LoadRoutingSettings(const json::Document& doc);

} // namespace request



