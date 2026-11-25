#include "scenario.h"

#include <cctype>
#include <fstream>
#include <map>
#include <sstream>
#include <utility>
#include <utility>

namespace scenario {
namespace {

struct Value {
    enum class Kind { Null, Boolean, Number, String, Array, Object } kind = Kind::Null;
    bool boolean = false;
    double number = 0;
    std::string string;
    std::vector<Value> array;
    std::map<std::string, Value> object;
};

class Parser {
public:
    explicit Parser(const std::string& source) : source_(source) {}
    bool parse(Value& out, std::string& error) {
        skip();
        if (!value(out, error)) return false;
        skip();
        if (pos_ != source_.size()) return fail(error, "unexpected text after JSON value");
        return true;
    }
private:
    bool fail(std::string& error, const char* message) const {
        std::ostringstream out; out << message << " at byte " << pos_; error = out.str(); return false;
    }
    void skip() { while (pos_ < source_.size() && std::isspace(static_cast<unsigned char>(source_[pos_]))) ++pos_; }
    bool take(char c) { skip(); if (pos_ < source_.size() && source_[pos_] == c) { ++pos_; return true; } return false; }
    bool string(std::string& out, std::string& error) {
        if (!take('"')) return fail(error, "expected string");
        while (pos_ < source_.size()) {
            char c = source_[pos_++];
            if (c == '"') return true;
            if (static_cast<unsigned char>(c) < 0x20) return fail(error, "control character in string");
            if (c != '\\') { out.push_back(c); continue; }
            if (pos_ >= source_.size()) return fail(error, "unfinished escape");
            char e = source_[pos_++];
            switch (e) {
                case '"': case '\\': case '/': out.push_back(e); break;
                case 'b': out.push_back('\b'); break; case 'f': out.push_back('\f'); break;
                case 'n': out.push_back('\n'); break; case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                default: return fail(error, "unsupported string escape");
            }
        }
        return fail(error, "unterminated string");
    }
    bool value(Value& out, std::string& error) {
        skip(); if (pos_ >= source_.size()) return fail(error, "expected JSON value");
        char c = source_[pos_];
        if (c == '"') { out.kind = Value::Kind::String; return string(out.string, error); }
        if (c == '{') {
            ++pos_; out.kind = Value::Kind::Object; skip(); if (take('}')) return true;
            do { std::string key; if (!string(key, error) || !take(':')) return fail(error, "expected object key and colon"); Value child; if (!value(child, error)) return false; out.object[key] = std::move(child); skip(); if (take('}')) return true; } while (take(','));
            return fail(error, "expected comma or object end");
        }
        if (c == '[') {
            ++pos_; out.kind = Value::Kind::Array; skip(); if (take(']')) return true;
            do { Value child; if (!value(child, error)) return false; out.array.push_back(std::move(child)); skip(); if (take(']')) return true; } while (take(','));
            return fail(error, "expected comma or array end");
        }
        if (source_.compare(pos_, 4, "true") == 0) { pos_ += 4; out.kind = Value::Kind::Boolean; out.boolean = true; return true; }
        if (source_.compare(pos_, 5, "false") == 0) { pos_ += 5; out.kind = Value::Kind::Boolean; return true; }
        if (source_.compare(pos_, 4, "null") == 0) { pos_ += 4; return true; }
        std::size_t start = pos_;
        if (source_[pos_] == '-') ++pos_;
        if (pos_ < source_.size() && source_[pos_] == '0') ++pos_;
        else {
            std::size_t integerStart = pos_;
            while (pos_ < source_.size() && std::isdigit(static_cast<unsigned char>(source_[pos_]))) ++pos_;
            if (integerStart == pos_) return fail(error, "invalid JSON value");
        }
        if (pos_ < source_.size() && source_[pos_] == '.') {
            ++pos_; std::size_t fractionStart = pos_;
            while (pos_ < source_.size() && std::isdigit(static_cast<unsigned char>(source_[pos_]))) ++pos_;
            if (fractionStart == pos_) return fail(error, "invalid number fraction");
        }
        if (pos_ < source_.size() && (source_[pos_] == 'e' || source_[pos_] == 'E')) {
            ++pos_; if (pos_ < source_.size() && (source_[pos_] == '+' || source_[pos_] == '-')) ++pos_;
            std::size_t exponentStart = pos_;
            while (pos_ < source_.size() && std::isdigit(static_cast<unsigned char>(source_[pos_]))) ++pos_;
            if (exponentStart == pos_) return fail(error, "invalid number exponent");
        }
        try { out.number = std::stod(source_.substr(start, pos_ - start)); }
        catch (...) { return fail(error, "invalid number"); }
        out.kind = Value::Kind::Number; return true;
    }
    const std::string& source_; std::size_t pos_ = 0;
};

const Value* get(const Value& v, const char* key) {
    auto it = v.object.find(key); return it == v.object.end() ? nullptr : &it->second;
}
std::string text(const Value& v, const char* fallback) { return v.kind == Value::Kind::String ? v.string : fallback; }
double number(const Value& v, double fallback) { return v.kind == Value::Kind::Number ? v.number : fallback; }
int integer(const Value& v, int fallback) { return static_cast<int>(number(v, fallback)); }

} // namespace

bool load(const std::string& path, Data& out, std::string& error) {
    std::ifstream file(path, std::ios::binary);
    if (!file) { error = "Could not open scenario file: " + path; return false; }
    std::ostringstream contents; contents << file.rdbuf();
    Value root;
    if (!Parser(contents.str()).parse(root, error)) return false;
    if (root.kind != Value::Kind::Object) { error = "Scenario root must be an object"; return false; }
    Data parsed;
    if (const Value* label = get(root, "label")) parsed.label = text(*label, parsed.label.c_str());
    const Value* events = get(root, "events");
    if (!events || events->kind != Value::Kind::Array) { error = "Scenario must contain an events array"; return false; }
    for (const Value& value : events->array) {
        if (value.kind != Value::Kind::Object) { error = "Each event must be an object"; return false; }
        Event e;
        if (const Value* v = get(value, "id")) e.id = text(*v, "event");
        if (const Value* v = get(value, "location")) e.location = text(*v, "");
        if (const Value* v = get(value, "place")) e.place = text(*v, "");
        if (const Value* v = get(value, "date")) e.date = text(*v, "");
        if (const Value* v = get(value, "headline")) e.headline = text(*v, "");
        if (const Value* v = get(value, "type")) e.mode = text(*v, "protest") == "voting" ? simulation::Mode::Voting : simulation::Mode::Protest;
        if (const Value* v = get(value, "crowd_size")) e.crowdSize = integer(*v, e.crowdSize);
        if (const Value* v = get(value, "duration")) e.duration = static_cast<float>(number(*v, e.duration));
        if (const Value* v = get(value, "seed")) e.seed = static_cast<unsigned>(integer(*v, static_cast<int>(e.seed)));
        if (const Value* v = get(value, "stations")) e.stations = integer(*v, e.stations);
        if (const Value* v = get(value, "turnout")) e.turnout = static_cast<float>(number(*v, e.turnout));
        if (const Value* v = get(value, "year_card")) e.yearCard = integer(*v, e.yearCard);
        if (const Value* districts = get(value, "districts")) {
            if (districts->kind != Value::Kind::Array) { error = "Event districts must be an array"; return false; }
            for (const Value& d : districts->array) {
                const Value* name = get(d, "name");
                if (d.kind != Value::Kind::Object || !name || name->kind != Value::Kind::String) { error = "Each district needs a name"; return false; }
                simulation::District district; district.name = name->string;
                if (const Value* v = get(d, "turnout")) district.turnout = static_cast<float>(number(*v, district.turnout));
                if (const Value* v = get(d, "expected_voters")) district.expectedVoters = integer(*v, district.expectedVoters);
                if (const Value* v = get(d, "map_x")) district.mapX = static_cast<float>(number(*v, district.mapX));
                if (const Value* v = get(d, "map_y")) district.mapY = static_cast<float>(number(*v, district.mapY));
                e.districts.push_back(std::move(district));
            }
        }
        if (const Value* barriers = get(value, "barriers")) {
            if (barriers->kind != Value::Kind::Array) { error = "Event barriers must be an array"; return false; }
            for (const Value& b : barriers->array) {
                if (b.kind != Value::Kind::Object) { error = "Each barrier must be an object"; return false; }
                simulation::Barrier barrier;
                if (const Value* v = get(b, "x")) barrier.x = static_cast<float>(number(*v, barrier.x));
                if (const Value* v = get(b, "y")) barrier.y = static_cast<float>(number(*v, barrier.y));
                if (const Value* v = get(b, "radius")) barrier.radius = static_cast<float>(number(*v, barrier.radius));
                if (barrier.radius <= 0) { error = "Barrier radius must be positive"; return false; }
                e.barriers.push_back(barrier);
            }
        }
        if (e.crowdSize < 0 || e.duration <= 0 || e.stations < 1 || e.turnout < 0 || e.turnout > 1) { error = "Event values are outside valid bounds"; return false; }
        for (const auto& d : e.districts) if (d.turnout < 0 || d.turnout > 1 || d.expectedVoters < 0 || d.mapX < 0 || d.mapX > 1 || d.mapY < 0 || d.mapY > 1) { error = "District values are outside valid bounds"; return false; }
        parsed.events.push_back(std::move(e));
    }
    out = std::move(parsed); error.clear(); return true;
}

simulation::Config simulationConfig(const Event& event) {
    simulation::Config c; c.mode = event.mode; c.agents = event.crowdSize; c.turnout = event.turnout;
    c.seed = event.seed; c.stations = event.stations; c.districts = event.districts; c.barriers = event.barriers; return c;
}

} // namespace scenario
