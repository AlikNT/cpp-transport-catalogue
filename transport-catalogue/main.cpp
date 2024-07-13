#include <iostream>
#include <sstream>

#include "json.h"
#include "transport_catalogue.h"
#include "json_reader.h"
#include "map_renderer.h"

using namespace std;

int main() {
    // Загружаем json из stdin
    string stdin_str((istreambuf_iterator<char>(cin)),
                         std::istreambuf_iterator<char>());
    istringstream input_stream(stdin_str);

    // Парсим json, создаем объект json и объект транспортного каталога
    const auto json_requests_doc = json::Load(input_stream);
    data::TransportCatalogue catalogue = request::MakeCatalogueFromJSON(json_requests_doc);

    // Парсим "routing_settings", создаем объект TransportCatalogueRouter
    router::TransportCatalogueRouter router = request::MakeCatatalogueRouter(json_requests_doc, catalogue);

    // Парсим запросы к каталогу, создаем json документ с ответами и отправляем его в stdout
    const auto json_stat_doc = request::StatRequestsToJSON(json_requests_doc, catalogue, router);
    json::Print(json_stat_doc, std::cout);
}