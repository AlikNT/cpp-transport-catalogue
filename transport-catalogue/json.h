#pragma once

#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

using namespace std::literals;

namespace json {

class Node;

// Сохраните объявления Dict и Array без изменения
using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;

// Эта ошибка должна выбрасываться при ошибках парсинга JSON
class ParsingError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

using Number = std::variant<int, double>;

class Node {
public:
    /* Реализуйте Node, используя std::variant */
    using Value = std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string>;

    Node() = default;
    Node(Array value);
    Node(Dict value);
    Node(std::nullptr_t value);
    Node(bool value);
    Node(int value);
    Node(double value);
    Node(std::string value);

    bool IsNull() const;
    bool IsBool() const;
    bool IsInt() const;
    bool IsDouble() const;
    bool IsPureDouble() const;
    bool IsString() const;
    bool IsArray() const;
    bool IsMap() const;

    int AsInt() const;
    bool AsBool() const;
    double AsDouble() const;
    const std::string& AsString() const;
    const Array& AsArray() const;
    const Dict& AsMap() const;

    bool operator==(const Node& other) const;
    bool operator!=(const Node& other) const;

    [[nodiscard]] const Value& GetValue() const;

private:
    Value value_ = nullptr;
};

class Document {
public:
    explicit Document(Node root);

    bool operator==(const Document& other);

    bool operator!=(const Document& other);

    const Node& GetRoot() const;

private:
    Node root_;
};

Document Load(std::istream& input);

void Print(const Document& doc, std::ostream& output);

}  // namespace json