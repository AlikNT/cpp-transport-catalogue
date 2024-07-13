#include "transport_router.h"

router::TransportCatalogueRouter::TransportCatalogueRouter(const data::TransportCatalogue &catalogue, const request::RoutingSettings& routing_settings)
    : catalogue_(catalogue)
      , graph_(catalogue_.GetStopsCount() * 2)
      , routing_settings_(routing_settings)
      , bus_velocity_(routing_settings_.bus_velocity * METERS_IN_KILOMETER / MINUTES_IN_HOUR) {
    CreateVertexes();
    CreateEdges();
    router_ = std::make_unique<graph::Router<double> >(graph_);
}

std::optional<request::StatRouteInfo> router::TransportCatalogueRouter::BuildRoute(const std::string_view from, const std::string_view to) {
    const auto from_ptr = catalogue_.GetStop(from);
    const auto to_ptr = catalogue_.GetStop(to);
    if (stops_vertexes_.count(from_ptr) == 0 || stops_vertexes_.count(to_ptr) == 0) {
        return std::nullopt;
    }
    const auto route = router_->BuildRoute(stops_vertexes_[from_ptr].portal, stops_vertexes_[to_ptr].portal);
    request::StatRouteInfo result;
    if (!route.has_value()) {
        return std::nullopt;
    }
    for (const auto &edge_id: route->edges) {
        if (edges_[edge_id].stop_from_ptr == edges_[edge_id].stop_to_ptr) {
            result.route.emplace_back(request::Route{
                true, edges_[edge_id].stop_from_ptr, nullptr, graph_.GetEdge(edge_id).weight, 0});
        } else {
            result.route.emplace_back(request::Route{false, nullptr, edges_[edge_id].bus_ptr, graph_.GetEdge(edge_id).weight, edges_[edge_id].span_count});
        }
    }
    result.weight = route->weight;
    return result;
}

void router::TransportCatalogueRouter::CreateVertexes() {
    graph::VertexId vertex_id = 0;
    const auto &buses = catalogue_.GetBuses();
    for (const auto &[_, bus_ptr]: buses) {
        for (auto &stop_ptr: bus_ptr->route) {
            if (stops_vertexes_.insert(std::make_pair(stop_ptr, StopVertexes{vertex_id, vertex_id + 1})).second) {
                auto edge_id = graph_.AddEdge({vertex_id++, vertex_id++, routing_settings_.bus_wait_time * 1.0});
                edges_[edge_id] = Edges{bus_ptr, stop_ptr, stop_ptr, 0};
            }
        }
    }
}

void router::TransportCatalogueRouter::CreateEdges() {
    const auto &buses = catalogue_.GetBuses();
    for (const auto &[_, bus_ptr]: buses) {
        if (bus_ptr->is_roundtrip) {
            ParseBusRouteOnEdges(bus_ptr->route.begin(), bus_ptr->route.end(), bus_ptr);
        } else {
            const auto it_middle = bus_ptr->route.begin() + static_cast<int>(bus_ptr->route.size() / 2);
            ParseBusRouteOnEdges(bus_ptr->route.begin(), it_middle + 1, bus_ptr);
            ParseBusRouteOnEdges(it_middle, bus_ptr->route.end(), bus_ptr);
        }
    }
}
