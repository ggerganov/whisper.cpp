#include "grammar-parser.h"
#include <whisper.h>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>

namespace qi = boost::spirit::qi;
namespace phx = boost::phoenix;

namespace {
using Iterator = std::string::const_iterator;
using Skipper = boost::spirit::qi::rule<Iterator>;
using Rule = std::vector<whisper_grammar_element>;
using Rules = std::vector<Rule>;

struct WGrammar : public qi::grammar<Iterator, Rules(), Skipper> {
    WGrammar() : WGrammar::base_type(grammar) {
        using namespace qi;
        grammar = (omit[*space] >> *rule)
            [_val = phx::bind(&WGrammar::get_rules, this)];
        rule = (rule_name >> omit[*blank] >> "::=" >> omit[*space] >> alternates >> (+eol | eoi))
            [phx::bind(&WGrammar::add_rule, this, _1, _2)];
        rule_name = (alpha >> *(alnum | char_('-')))
            [_val = phx::bind(&WGrammar::get_symbol_id, this, _1, _2)];
        alternates = (sequence >> omit[*blank] >> *( omit[*blank] >> '|' >> omit[*space] >> sequence))
            [_val = phx::bind(&WGrammar::add_alternate, this, _1, _2)];
        nested_alternates = (nested_sequence >> omit[*space] >> *(omit[*space] >> '|' >> omit[*space] >> nested_sequence))
            [_val = phx::bind(&WGrammar::add_alternate, this, _1, _2)];
        sequence = (repetition % *blank)
            [_val = phx::bind(&WGrammar::merge, this, _1)];
        nested_sequence = (repetition % *space)
            [_val = phx::bind(&WGrammar::merge, this, _1)];
        repetition = (value >> -char_("*+?"))
            [_val = phx::bind(&WGrammar::add_repetition, this, _1, _2)];
        value = literal[_val = _1] | set[_val = _1] |
            rule_name[_val = phx::bind(&WGrammar::add_rule_ref, this, _1)] |
            group[_val = phx::bind(&WGrammar::add_rule_ref, this, _1)];
        literal = lexeme['"' >> *(char_ - '"') >> '"']
            [_val = phx::bind(&WGrammar::add_literal, this, _1)];
        set = ('[' >> -char_('^') >> (*range | *(char_ - ']')) >> ']')
            [_val = phx::bind(&WGrammar::add_char_range, this, _1, _2)];
        range = (char_ - ']') >> '-' >> (char_ - ']');
        group = ('(' >> omit[*space] >> nested_alternates >> omit[*space] >> ')')
            [_val = phx::bind(&WGrammar::add_group, this, _1)];
    }

    Rules get_rules() { return std::move(rules); }

    void add_rule(uint32_t rule_id, Rule& rule) {
        rule.push_back({WHISPER_GRETYPE_END, 0});
        if (rules.size() <= rule_id) {
            rules.resize(rule_id + 1);
        }
        rules[rule_id] = std::move(rule);
    }

    Rule merge(Rules& rules) {
        Rule result;
        for(auto r : rules) result.insert(result.end(), r.begin(), r.end());
        return result;
    }

    Rule add_alternate(Rule& rule, Rules& rules) {
        for (auto& r : rules) {
            rule.push_back({WHISPER_GRETYPE_ALT, 0});
            rule.insert(rule.end(), r.begin(), r.end());
        }
        return std::move(rule);
    }

    uint32_t generate_symbol_id() {
        uint32_t next_id = static_cast<uint32_t>(symbol_ids.size());
        std::string id = "__AUTO_GEN__" + std::to_string(next_id);
        symbol_ids[id] = next_id;
        return next_id;
    }

    uint32_t get_symbol_id(char begin, std::vector<char>& id) {
        id.insert(id.begin(), begin);
        uint32_t next_id = static_cast<uint32_t>(symbol_ids.size());
        auto result = symbol_ids.emplace(std::string(id.begin(), id.end()), next_id);
        // fprintf(stderr, "%s: id{%d} => name{%s} \n", __func__, int(result.first->second), result.first->first.c_str());
        return result.first->second;
    }

    Rule add_repetition(Rule& rule, boost::optional<char> op) {
        if (!op) return std::move(rule);
        // apply transformation to previous symbol according to
        // rewrite rules:
        // S* --> S' ::= S S' |
        // S+ --> S' ::= S S' | S
        // S? --> S' ::= S |
        auto rule_id = generate_symbol_id();
        Rule auto_rule = rule;
        if (*op == '*' || *op == '+') {
            // cause generated rule to recurse
            auto_rule.push_back({WHISPER_GRETYPE_RULE_REF, rule_id});
        }
        // mark start of alternate def
        auto_rule.push_back({WHISPER_GRETYPE_ALT, 0});
        if (*op == '+') {
            // add preceding symbol as alternate only for '+' (otherwise empty)
            auto_rule.insert(auto_rule.end(), rule.begin(), rule.end());
        }
        add_rule(rule_id, auto_rule);
        return Rule(1, {WHISPER_GRETYPE_RULE_REF, rule_id});
    }

    Rule add_rule_ref(uint32_t rule_id) {
        return Rule(1, {WHISPER_GRETYPE_RULE_REF, rule_id});
    }

    uint32_t add_group(Rule& rule) {
        auto rule_id = generate_symbol_id();
        add_rule(rule_id, rule);
        return rule_id;
    }

    Rule add_literal(std::vector<char>& str) {
        Rule result;
        std::string tmp(str.begin(), str.end());
        const char * pos = str.data();
        const char* end = pos + str.size();
        uint32_t value = 0;
        while(pos != end) {
            std::tie(value, pos) = parse_char(pos);
            result.push_back({WHISPER_GRETYPE_CHAR, value});
        }
        return result;
    }

    Rule add_char_range(boost::optional<char> neg, boost::variant<std::vector<std::string>, std::vector<char> >& content) {
        Rule result;
        auto type = neg ? WHISPER_GRETYPE_CHAR_NOT : WHISPER_GRETYPE_CHAR;
        switch(content.which()) {
            case 0: {
                auto& vec = boost::get<std::vector<std::string>>(content);
                for (auto& range : vec) {
                    assert(range.size() == 2);
                    result.push_back({type, (uint32_t)range[0]});
                    result.push_back({WHISPER_GRETYPE_CHAR_RNG_UPPER, (uint32_t)range[1]});
                    type = WHISPER_GRETYPE_CHAR_ALT;
                }
                break;
            }
            case 1: {
                auto& vec = boost::get<std::vector<char>>(content);
                const char* pos = &vec[0];
                const char* end = pos + vec.size();
                uint32_t value = 0;
                while(pos != end) {
                    std::tie(value, pos) = parse_char(pos);
                    result.push_back({type, value});
                    type = WHISPER_GRETYPE_CHAR_ALT;
                }
            }
        }

        return result;
    }
    // TODO: define rules for escape sequences
    static std::pair<uint32_t, const char *> parse_char(const char * src) {
        if (*src == '\\') {
            switch (src[1]) {
                case 'x': return parse_hex(src + 2, 2);
                case 'u': return parse_hex(src + 2, 4);
                case 'U': return parse_hex(src + 2, 8);
                case 't': return std::make_pair('\t', src + 2);
                case 'r': return std::make_pair('\r', src + 2);
                case 'n': return std::make_pair('\n', src + 2);
                case '\\':
                case '"':
                case '[':
                case ']':
                    return std::make_pair(src[1], src + 2);
                default:
                    throw std::runtime_error(std::string("unknown escape at ") + src);
            }
        } else if (*src) {
            return decode_utf8(src);
        }
        throw std::runtime_error("unexpected end of input");
    }
    // TODO: rule for hex values
    static std::pair<uint32_t, const char *> parse_hex(const char * src, int size) {
        const char * pos   = src;
        const char * end   = src + size;
        uint32_t     value = 0;
        for ( ; pos < end && *pos; pos++) {
            value <<= 4;
            char c = *pos;
            if ('a' <= c && c <= 'f') {
                value += c - 'a' + 10;
            } else if ('A' <= c && c <= 'F') {
                value += c - 'A' + 10;
            } else if ('0' <= c && c <= '9') {
                value += c - '0';
            } else {
                break;
            }
        }
        if (pos != end) {
            throw std::runtime_error("expecting " + std::to_string(size) + " hex chars at " + src);
        }
        return std::make_pair(value, pos);
    }
    // TODO: check if this can be replaced with boost::u8_to_u32_iterator<const char*>
    // NOTE: assumes valid utf8 (but checks for overrun)
    // copied from whisper.cpp
    static std::pair<uint32_t, const char *> decode_utf8(const char * src) {
        static const int lookup[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 3, 4 };
        uint8_t  first_byte = static_cast<uint8_t>(*src);
        uint8_t  highbits   = first_byte >> 4;
        int      len        = lookup[highbits];
        uint8_t  mask       = (1 << (8 - len)) - 1;
        uint32_t value      = first_byte & mask;
        const char * end    = src + len; // may overrun!
        const char * pos    = src + 1;
        for ( ; pos < end && *pos; pos++) {
            value = (value << 6) + (static_cast<uint8_t>(*pos) & 0x3F);
        }
        return std::make_pair(value, pos);
    }

    Rules rules;
    std::map<std::string, uint32_t> symbol_ids;
    qi::rule<Iterator, uint32_t()> rule_name;
    qi::rule<Iterator, uint32_t(), Skipper> group;
    qi::rule<Iterator, std::string()> range;
    qi::rule<Iterator, Rule(), Skipper> alternates, value, literal, repetition, sequence, set, nested_alternates, nested_sequence;
    qi::rule<Iterator, Skipper> rule;
    qi::rule<Iterator, Rules(), Skipper> grammar;
};
}

Rules grammar_parser::test_parse(const std::string& src) {
    Skipper comment = '#' >> *(qi::char_ - qi::eol) >> (qi::eol|qi::eoi);
    Rules result;
    Iterator begin = src.begin();
    Iterator end = src.end();
    try {
        boost::spirit::qi::phrase_parse(begin, end, WGrammar(), comment, result);
        if (begin != end) throw std::runtime_error("Parsing failed.");

    } catch (std::exception & err) {
        fprintf(stderr, "%s: error parsing grammar: %s\n", __func__, err.what());
        return {};
    }
    return result;
}
