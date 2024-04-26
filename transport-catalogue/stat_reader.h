#pragma once

#include <iosfwd>
#include <string_view>

#include "transport_catalogue.h"

namespace request {

struct StatCommand {
    std::string command;
    std::string description;
};

void ParseAndPrintStat(const data::TransportCatalogue& transport_catalogue, std::string_view request,
                       std::ostream& output);
} // namespace request