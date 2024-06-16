#include "json_builder.h"

using namespace std::literals;

namespace json {

json::Builder::Builder()
    : root_()
    , nodes_stack_{&root_} {
}

Builder::DictValueContext Builder::Key(std::string value) {
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

    return BaseContext{*this};
}

Builder::BaseContext Builder::Value(Node::Value value) {
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
    return *this;
}

Builder::DictItemContext json::Builder::StartDict() {
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
    return BaseContext{*this};
}

Builder::ArrayItemContext json::Builder::StartArray() {
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
    return BaseContext{*this};
}

Builder::BaseContext json::Builder::EndDict() {
    CheckAfterBuild();
    if (nodes_stack_.back()->IsArray()) {
        throw std::logic_error("EndDict must be called only after StartDict"s);
    }
    nodes_stack_.pop_back();
    return *this;
}

Builder::BaseContext json::Builder::EndArray() {
    CheckAfterBuild();
    if (nodes_stack_.back()->IsDict()) {
        throw std::logic_error("EndArray must be called only after StartArray"s);
    }
    nodes_stack_.pop_back();
    return *this;
}

Node json::Builder::Build() {
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

Node Builder::BaseContext::Build() {
    return builder_.Build();
}

Builder::DictValueContext Builder::BaseContext::Key(std::string value) {
    return builder_.Key(std::move(value));
}

Builder::BaseContext Builder::BaseContext::Value(Node::Value value) {
    return builder_.Value(std::move(value));
}

Builder::DictItemContext Builder::BaseContext::StartDict() {
    return builder_.StartDict();
}

Builder::ArrayItemContext Builder::BaseContext::StartArray() {
    return builder_.StartArray();
}

Builder::BaseContext Builder::BaseContext::EndDict() {
    return builder_.EndDict();
}

Builder::BaseContext Builder::BaseContext::EndArray() {
    return builder_.EndArray();
}

Builder::DictItemContext Builder::DictValueContext::Value(Node::Value value) {
    return BaseContext::Value(std::move(value));
}

Builder::ArrayItemContext Builder::ArrayItemContext::Value(Node::Value value) {
    return BaseContext::Value(std::move(value));
}
} // namespace json