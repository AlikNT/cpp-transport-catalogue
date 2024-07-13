#pragma once

#include "router.h"
#include "transport_catalogue.h"

using namespace std::literals;

namespace router {
class TransportCatalogueRouter {
public:
    static constexpr double METERS_IN_KILOMETER = 1000;
    static constexpr double MINUTES_IN_HOUR = 60;

    TransportCatalogueRouter(const data::TransportCatalogue &catalogue, const request::RoutingSettings& routing_settings);

    std::optional<request::StatRouteInfo> BuildRoute(std::string_view from, std::string_view to);

private:
    struct StopVertexes {
        size_t portal;
        size_t hub;
    };

    struct Edges {
        const data::Bus *bus_ptr;
        const data::Stop *stop_from_ptr;
        const data::Stop *stop_to_ptr;
        int span_count;
    };

    const data::TransportCatalogue &catalogue_;
    graph::DirectedWeightedGraph<double> graph_;
    request::RoutingSettings routing_settings_;
    std::unordered_map<const data::Stop *, StopVertexes> stops_vertexes_;
    std::unordered_map<size_t, Edges> edges_;
    const double bus_velocity_;
    std::unique_ptr<graph::Router<double> > router_;

    void CreateVertexes();

    template<typename Iterator>
    void ParseBusRouteOnEdges(Iterator it_begin, Iterator it_end, const data::Bus *bus_ptr);

    void CreateEdges();
};

template<typename Iterator>
void TransportCatalogueRouter::ParseBusRouteOnEdges(Iterator it_begin, Iterator it_end, const data::Bus *bus_ptr) {
    for (auto it_start = it_begin; it_start != it_end; ++it_start) {
        double weight = 0;
        int span_count = 0;
        for (auto it_stop = it_start + 1; it_stop != it_end; ++it_stop) {
            if (*it_start != *it_stop) {
                weight += catalogue_.GetDistance(*(it_stop - 1), *it_stop) / bus_velocity_;
                ++span_count;
                auto edge_id = graph_.AddEdge({
                    stops_vertexes_[*it_start].hub, stops_vertexes_[*it_stop].portal, weight
                });
                edges_[edge_id] = Edges{bus_ptr, *it_start, *it_stop, span_count};
            }
        }
    }
}
} // namespace router
