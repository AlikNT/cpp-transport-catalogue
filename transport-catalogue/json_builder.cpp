#include "json_builder.h"

using namespace std::literals;

namespace json {

json::Builder::Builder() {
    nodes_stack_.emplace_back(&root_);
}

KeyItemContext json::Builder::Key(std::string value) {
    CheckAfterBuild();
    if (!nodes_stack_.back()->IsDict()) {
        throw std::logic_error("Key must be called only inside Dict and not just after another key."s);
    }
    auto &dict = std::get<Dict>(nodes_stack_.back()->GetValue());
    auto [it, inserted] = dict.insert({{std::move(value)},Node{}});
    if (!inserted) {
        throw std::logic_error("Duplicate key."s);
    }
    nodes_stack_.emplace_back(&(it->second));

    return KeyItemContext{*this};
}

ValueItemContext Builder::Value(Node::Value value) {
    CheckAfterBuild();
    CheckCorrectCall();
    if (nodes_stack_.back()->IsArray()) {
        auto &array = std::get<Array>(nodes_stack_.back()->GetValue());
        Node node{};
        node.GetValue() = value;
        array.emplace_back(std::move(node));
    } else {
        nodes_stack_.back()->GetValue() = std::move(value);
        nodes_stack_.pop_back();
    }
    return ValueItemContext{*this};
}

DictItemContext json::Builder::StartDict() {
    CheckAfterBuild();
    CheckCorrectCall();
    auto back_node_ptr = nodes_stack_.back();
    if (back_node_ptr->IsArray()) {
        auto &array = std::get<Array>(back_node_ptr->GetValue());
        array.emplace_back(Dict{});
        nodes_stack_.emplace_back(&(array.back()));
    } else {
        nodes_stack_.back()->GetValue() = Dict{};
    }
    return DictItemContext{*this};
}

ArrayItemContext json::Builder::StartArray() {
    CheckAfterBuild();
    CheckCorrectCall();
    auto back_node_ptr = nodes_stack_.back();
    if (back_node_ptr->IsArray()) {
        auto &node = std::get<Array>(back_node_ptr->GetValue());
        node.emplace_back(Array{});
        nodes_stack_.emplace_back(&(node.back()));
    } else {
        back_node_ptr->GetValue() = Array{};
    }
    return ArrayItemContext{*this};
}

json::Builder &json::Builder::EndDict() {
    CheckAfterBuild();
    if (nodes_stack_.back()->IsArray()) {
        throw std::logic_error("EndDict must be called only after StartDict"s);
    }
    nodes_stack_.pop_back();
    return *this;
}

json::Builder &json::Builder::EndArray() {
    CheckAfterBuild();
    if (nodes_stack_.back()->IsDict()) {
        throw std::logic_error("EndArray must be called only after StartArray"s);
    }
    nodes_stack_.pop_back();
    return *this;
}

json::Node json::Builder::Build() {
    if (root_.IsNull()) {
        throw std::logic_error("Build must not be called just after constructor."s);
    }
    if (!nodes_stack_.empty()) {
        throw std::logic_error("Dict or Array is not fully completed."s);
    }
    return std::move(root_);
}

void Builder::CheckAfterBuild() {
    if (nodes_stack_.empty()) {
        throw std::logic_error("Object is already completed, expected Build method."s);
    }
}

void Builder::CheckCorrectCall() {
    if (!root_.IsNull() && !nodes_stack_.back()->IsNull() && !nodes_stack_.back()->IsArray()) {
        throw std::logic_error("Value, StarDict, StartArray must be called only after constructor, Key or previous Array element.");
    }
}

DictItemContext KeyItemContext::Value(Node::Value value) {
    GetBuilder().Value(std::move(value));
    return DictItemContext{GetBuilder()};
}

ArrayItemContext KeyItemContext::StartArray() {
    return GetBuilder().StartArray();
}

DictItemContext KeyItemContext::StartDict() {
    return GetBuilder().StartDict();
}

ArrayItemContext ValueItemContext::StartArray() {
    return GetBuilder().StartArray();
}

KeyItemContext ValueItemContext::Key(std::string value) {
    return GetBuilder().Key(std::move(value));
}

Builder &ValueItemContext::EndDict() {
    return GetBuilder().EndDict();
}

Builder &ValueItemContext::EndArray() {
    return GetBuilder().EndArray();
}

Node ValueItemContext::Build() {
    return GetBuilder().Build();
}

KeyItemContext DictItemContext::Key(std::string value) {
    return GetBuilder().Key(std::move(value));
}

Builder &DictItemContext::EndDict() {
    return GetBuilder().EndDict();
}

ArrayItemContext &ArrayItemContext::Value(Node::Value value) {
    GetBuilder().Value(std::move(value));
    return *this;
}

DictItemContext ArrayItemContext::StartDict() {
    return GetBuilder().StartDict();
}

Builder &ArrayItemContext::EndArray() {
    return GetBuilder().EndArray();
}

ArrayItemContext ArrayItemContext::StartArray() {
    return GetBuilder().StartArray();
}

Builder &ItemContext::GetBuilder() {
    return builder_;
}
} // namespace json