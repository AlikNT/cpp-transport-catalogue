#include "json.h"

#include <utility>

using namespace std;

namespace json {

namespace {

Node LoadNode(istream& input);

Number LoadNumber(std::istream& input) {
    using namespace std::literals;

    std::string parsed_num;

    // Считывает в parsed_num очередной символ из input
    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) {
            throw ParsingError("Failed to read number from stream"s);
        }
    };

    // Считывает одну или более цифр в parsed_num из input
    auto read_digits = [&input, read_char] {
        if (!std::isdigit(input.peek())) {
            throw ParsingError("A digit is expected"s);
        }
        while (std::isdigit(input.peek())) {
            read_char();
        }
    };

    if (input.peek() == '-') {
        read_char();
    }
    // Парсим целую часть числа
    if (input.peek() == '0') {
        read_char();
        // После 0 в JSON не могут идти другие цифры
    } else {
        read_digits();
    }

    bool is_int = true;
    // Парсим дробную часть числа
    if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
    }

    // Парсим экспоненциальную часть числа
    if (int ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-') {
            read_char();
        }
        read_digits();
        is_int = false;
    }

    try {
        if (is_int) {
            // Сначала пробуем преобразовать строку в int
            try {
                return std::stoi(parsed_num);
            } catch (...) {
                // В случае неудачи, например, при переполнении,
                // код ниже попробует преобразовать строку в double
            }
        }
        return std::stod(parsed_num);
    } catch (...) {
        throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
    }
}

// Считывает содержимое строкового литерала JSON-документа
// Функцию следует использовать после считывания открывающего символа ":
std::string LoadString(std::istream& input) {
    using namespace std::literals;

    auto it = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();
    std::string s;
    while (true) {
        if (it == end) {
            // Поток закончился до того, как встретили закрывающую кавычку?
            throw ParsingError("String parsing error");
        }
        const char ch = *it;
        if (ch == '"') {
            // Встретили закрывающую кавычку
            ++it;
            break;
        } else if (ch == '\\') {
            // Встретили начало escape-последовательности
            ++it;
            if (it == end) {
                // Поток завершился сразу после символа обратной косой черты
                throw ParsingError("String parsing error");
            }
            const char escaped_char = *(it);
            // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
            switch (escaped_char) {
                case 'n':
                    s.push_back('\n');
                    break;
                case 't':
                    s.push_back('\t');
                    break;
                case 'r':
                    s.push_back('\r');
                    break;
                case '"':
                    s.push_back('"');
                    break;
                case '\\':
                    s.push_back('\\');
                    break;
                default:
                    // Встретили неизвестную escape-последовательность
                    throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
            }
        } else if (ch == '\n' || ch == '\r') {
            // Строковый литерал внутри- JSON не может прерываться символами \r или \n
            throw ParsingError("Unexpected end of line"s);
        } else {
            // Просто считываем очередной символ и помещаем его в результирующую строку
            s.push_back(ch);
        }
        ++it;
    }

    return s;
}

Node LoadArray(istream& input) {
    Array result;
    while (true) {
        char c;
        input >> c;
        // Если дошли до конца потока и не обнаружили ']' выбрасываем ошибку
        if (!input) {
            throw ParsingError("Array parsing error"s);
        }
        if (c != ',') {
            input.putback(c);
        }
        if (c == ']') {
            break;
        }
        result.push_back(LoadNode(input));
    }
    input.seekg(1, std::ios::cur);
    return {std::move(result)};
}

Node LoadDict(istream& input) {
    Dict dict;
    for (char c; input >> c && c != '}';) {
        // Первый символ должен быть '"' или ','. Если нет, выбрасываем ошибку.
        if (c == '"') {
            string key = LoadString(input);
            input >> c;
            // После '"' обязательно должен следовать ':', если нет, выбрасываем ошибку
            if (c == ':') {
                if (dict.count(key) > 0) {
                    throw ParsingError("Duplicate key : "s + key);
                }
                dict.insert({std::move(key), LoadNode(input)});
            } else {
                throw ParsingError("':' is expected"s);
            }
        } else if (c != ',') {
            throw ParsingError("',' is expected"s);
        }
    }
    if (!input) {
        throw ParsingError("Dictionary parsing error"s);
    }
    return {std::move(dict)};
}

bool LoadBool(istream &input) {
    string value;
    while (true) {
        char c;
        input >> c;
        if (!input || !isalpha(c)) {
            input.putback(c);
            break;
        }
        value.push_back(c);
    }
    if (value == "true"s) {
        return true;
    } else if (value == "false") {
        return false;
    } else {
        throw ParsingError("\"true\" or \"false\' is expected"s);
    }
}

nullptr_t LoadNull(istream& input) {
    string value;
    while (true) {
        char c;
        input >> c;
        if (!input || !isalpha(c)) {
            input.putback(c);
            break;
        }
        value.push_back(c);
    }
    if (value == "null"s) {
        return nullptr;
    } else {
        throw ParsingError("\"null\" is expected"s);
    }
}

Node LoadNode(istream& input) {
    char c;
    input >> c;

    if (c == '[') {
        return LoadArray(input);
    } else if (c == '{') {
        return LoadDict(input);
    } else if (c == '"') {
        return {LoadString(input)};
    } else if (c == 't' || c == 'f') {
        input.putback(c);
        return {LoadBool(input)};
    } else if (c == 'n') {
        input.putback(c);
        return {LoadNull(input)};
    } else if (c == '-' || isdigit(c)) {
        input.putback(c);
        Number number = LoadNumber(input);
        if (holds_alternative<double>(number)) {
            return {get<double>(number)};
        } else {
            return {get<int>(number)};
        }
    } else {
        throw ParsingError("Error parsing"s);
    }
}

// Контекст вывода, хранит ссылку на поток вывода и текущий отсуп
struct PrintContext {
    PrintContext(ostream& out)
            : out(out){
    }

    PrintContext(ostream& out, int indent_step, int indent)
            : out(out)
            , indent_step(indent_step)
            , indent(indent) {
    }

    void PrintIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    // Возвращает новый контекст вывода с увеличенным смещением
    [[nodiscard]] PrintContext Indented() const {
        return {out, indent_step, indent_step + indent};
    }

    ostream& out;
    int indent_step = 4;
    int indent = 0;
};

// Перегрузка функции PrintValue для вывода значений null
void PrintValue(nullptr_t, const PrintContext& ctx) {
    ctx.out << "null"sv;
}

void PrintValue(bool value, const PrintContext& ctx) {
    ctx.out << (value ? "true"s : "false"s);
}

void PrintValue(const string& value, const PrintContext& ctx) {
    ctx.out << "\""s;
    for (const auto c : value) {
        switch (c) {
            case '\\':
                ctx.out << R"(\\)";
                break;
            case '"':
                ctx.out << R"(\")";
                break;
            case 13:
                ctx.out << R"(\r)";
                break;
            case 10:
                ctx.out << R"(\n)";
                break;
            case 9:
                ctx.out << R"(\t)";
                break;
            default:
                ctx.out << c;
        }
    }
    ctx.out << "\""s;
}

void PrintNode(const Node&, const PrintContext&);

void PrintValue(const Array & values, const PrintContext& ctx) {
    ctx.out << "["s << endl;
    const PrintContext& ctx_values = ctx.Indented();
    for (auto it = values.begin(); it != values.end(); ++it) {
        ctx_values.PrintIndent();
        PrintNode((*it), ctx_values);
        if (next(it) != values.end()) {
            ctx.out << ","s;
        }
        ctx.out << endl;
    }
    ctx.PrintIndent();
    ctx.out << "]"s;
}

void PrintValue(const Dict& values, const PrintContext& ctx) {
    ctx.out << "{"s << endl;
    const PrintContext& ctx_values = ctx.Indented();
    for (auto it = values.begin(); it != values.end(); ++it) {
        ctx_values.PrintIndent();
        ctx.out << "\""s << (*it).first << "\": "s;
        PrintNode(it->second, ctx_values);
        if (next(it) != values.end()) {
            ctx.out << ","s;
        }
        ctx.out << endl;
    }
    ctx.PrintIndent();
    ctx.out << "}"s;
}

// Шаблон, подходящий для вывода double и int
template <typename Value>
void PrintValue(const Value& value, const PrintContext& ctx) {
    ctx.out << value;
}

void PrintNode(const Node& node, const PrintContext& ctx) {
    std::visit(
            [&ctx](const auto& value){ PrintValue(value, ctx); },
            node.GetValue());
}

}  // namespace

bool Node::IsNull() const {
    return std::holds_alternative<std::nullptr_t>(*this);
}

bool Node::IsBool() const {
    return std::holds_alternative<bool>(*this);;
}

bool Node::IsInt() const {
    return std::holds_alternative<int>(*this);
}

bool Node::IsDouble() const {
    return std::holds_alternative<double>(*this) || std::holds_alternative<int>(*this);
}

bool Node::IsPureDouble() const {
    return std::holds_alternative<double>(*this);
}

bool Node::IsString() const {
    return std::holds_alternative<std::string>(*this);
}

bool Node::IsArray() const {
    return std::holds_alternative<Array>(*this);
}

bool Node::IsMap() const {
    return std::holds_alternative<Dict>(*this);
}

int Node::AsInt() const {
    if (!IsInt()) {
        throw std::logic_error("Type is not an int."s);
    }
    return std::get<int>(*this);
}

bool Node::AsBool() const {
    if (!IsBool()) {
        throw std::logic_error("Type is not a bool."s);
    }
    return std::get<bool>(*this);
}

double Node::AsDouble() const {
    if (IsInt()) {
        return get<int>(*this);
    }
    if (IsDouble()) {
        return get<double>(*this);
    }
    throw std::logic_error("Type is not a double."s);
}

const Array& Node::AsArray() const {
    if (!IsArray()) {
        throw std::logic_error("Type is not an array."s);
    }
    return std::get<Array>(*this);
}

const Dict& Node::AsMap() const {
    if (!IsMap()) {
        throw std::logic_error("Type is not a map."s);
    }
    return std::get<Dict>(*this);
}

const string& Node::AsString() const {
    if (!IsString()) {
        throw std::logic_error("Type is not a string."s);
    }
    return std::get<std::string>(*this);
}

const Node::Value& Node::GetValue() const {
    return *this;
}

bool Node::operator==(const Node &other) const {
    return value_ == other.value_;
}

bool Node::operator!=(const Node &other) const {
    return !(*this == other);
}

Document::Document(Node root)
        : root_(std::move(root)) {
}

const Node& Document::GetRoot() const {
    return root_;
}

bool Document::operator==(const Document &other) {
    return root_ == other.root_;
}

bool Document::operator!=(const Document &other) {
    return !(*this == other);
}

Document Load(istream& input) {
    return Document{LoadNode(input)};
}

void Print(const Document& doc, std::ostream& output) {
    // Реализуйте функцию самостоятельно
    PrintNode(doc.GetRoot(), PrintContext(output));
}

}  // namespace json