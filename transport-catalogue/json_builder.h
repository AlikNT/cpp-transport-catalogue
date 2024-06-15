#include "json.h"


namespace json {

class DictItemContext;
class ArrayItemContext;
class KeyItemContext;
class ValueItemContext;


class Builder {
public:

    Builder();

    KeyItemContext Key(std::string value);

    ValueItemContext Value(Node::Value value);

    DictItemContext StartDict();

    ArrayItemContext StartArray();

    Builder& EndDict();

    Builder& EndArray();

    Node Build();

private:
    Node root_;
    std::vector<Node*> nodes_stack_;

    void CheckAfterBuild();
    void CheckCorrectCall();
};

class ItemContext {
public:
    ItemContext(Builder& builder) : builder_(builder) {}

    Builder& GetBuilder();

private:
    Builder& builder_;
};

class ValueItemContext : ItemContext {
public:
    using ItemContext::ItemContext;

    KeyItemContext Key(std::string value);

    ArrayItemContext StartArray();

    Builder& EndDict();

    Builder& EndArray();

    Node Build();
};

class KeyItemContext : public ItemContext {
public:
    using ItemContext::ItemContext;

    DictItemContext Value(Node::Value value);

    ArrayItemContext StartArray();

    DictItemContext StartDict();

};

class DictItemContext : public ItemContext {
public:
    using ItemContext::ItemContext;

    KeyItemContext Key(std::string value);

    Builder& EndDict();
};

class ArrayItemContext : public ItemContext {
public:
    using ItemContext::ItemContext;

    ArrayItemContext& Value(Node::Value value);

    DictItemContext StartDict();

    ArrayItemContext StartArray();

    Builder& EndArray();
};

} // namespace json