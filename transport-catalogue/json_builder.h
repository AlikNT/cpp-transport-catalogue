#include "json.h"

namespace json {

class Builder {
private:
    class BaseContext;
    class DictValueContext;
    class DictItemContext;
    class ArrayItemContext;

public:
    Builder();
    DictValueContext Key(std::string value);
    BaseContext Value(Node::Value value);
    DictItemContext StartDict();
    ArrayItemContext StartArray();
    BaseContext EndDict();
    BaseContext EndArray();
    Node Build();

private:
    Node root_;
    std::vector<Node*> nodes_stack_;

    void CheckAfterBuild();
    void CheckCorrectCall();

    class BaseContext {
    public:
        BaseContext(Builder& builder) : builder_(builder) {}

        Node Build();
        DictValueContext Key(std::string value);
        BaseContext Value (Node::Value value);
        DictItemContext StartDict();
        ArrayItemContext StartArray();
        BaseContext EndDict();
        BaseContext EndArray();

    private:
        Builder& builder_;
    };

    class DictValueContext : public BaseContext {
    public:
        DictValueContext(BaseContext base_context) : BaseContext(base_context) {}

        DictItemContext Value(Node::Value value);
        Node Build() = delete;
        DictValueContext Key(std::string value) = delete;
        BaseContext EndArray() = delete;
        BaseContext EndDict() = delete;
    };

    class DictItemContext : public BaseContext {
    public:
        DictItemContext(BaseContext base_context) : BaseContext(base_context) {}

        Node Build() = delete;
        BaseContext Value(Node::Value value) = delete;
        BaseContext EndArray() = delete;
        DictItemContext StartDict() = delete;
        ArrayItemContext StartArray() = delete;
    };

    class ArrayItemContext : public BaseContext {
    public:
        ArrayItemContext(BaseContext base_context) : BaseContext(base_context) {}

        ArrayItemContext Value(Node::Value value);
        Node Build() = delete;
        DictValueContext Key(std::string value) = delete;
        BaseContext EndDict() = delete;
    };
};

} // namespace json