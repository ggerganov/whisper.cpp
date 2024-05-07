#include <grammar-parser.h>
#include <set>
#include <queue>

auto src =
R"(  root   ::= init "  " (command | question) "."
prompt ::= init

test ::= "你好"

# leading space is very important!
init ::= " Ok Whisper, start listening for commands." #asdfda

command ::= "Turn " ("on" | "off") " " device | "Set " device " to " value |
            "Increase " device " by " value | "Decrease " device " by " value |
            "Play " media | "Stop " media | "Schedule " task " at " time | "Cancel " task |
            "Remind me to " task " at " time | "Show me " device | "Hide " device

question ::= "What is the " device " status?" | "What is the current " device " value?" |
             "What is the " device " temperature?" | "What is the " device " humidity?" |
             "What is the " device " power consumption?" | "What is the " device " battery level?" |
             "What is the weather like today?" | "What is the forecast for tomorrow?" |
             "What is the time?" | "What is my schedule for today?" | "What tasks do I have?" |
             "What reminders do I have?"

device ::= "lights" | "thermostat" | "security system" | "door lock" | "camera" | "speaker" | "TV" |
           "music player" | "coffee machine" | "oven" | "refrigerator" | "washing machine" |
           "vacuum cleaner"

value ::= [0-9]+

media ::= "music" | "radio" | "podcast" | "audiobook" | "TV show" | "movie"

task ::= [a-zA-Z]+ (" " [a-zA-Z]+)?

time ::= [0-9] [0-9]? ("am" | "pm")?
)";

using namespace grammar_parser;

int main() {
    auto state = parse(src);
    auto rules1 = test_parse(src);

    if (state.rules.empty() != rules1.empty()) {
        fprintf(stderr, "Parsing success differs {%d} != {%d}\n", state.rules.empty(), rules1.empty());
        exit(1);
    }
    if (rules1.empty()) return 0;

    // traverse grammar, BFS comparison
    std::set<uint32_t> visited, visited1;
    std::queue<uint32_t> ref, ref1;
    ref.push(0);
    ref1.push(0);
    while(!ref.empty() || !ref1.empty()) {
        if (ref.size() != ref1.size()) {
            fprintf(stderr, "Current references differs {%d} != {%d}\n", (int)ref.size(), (int)ref1.size());
            exit(1);
        }
        uint32_t current = ref.front(); ref.pop();
        uint32_t current1 = ref1.front(); ref1.pop();
        bool check = visited.find(current) == visited.end();
        bool check1 = visited1.find(current1) == visited1.end();
        if (check != check1) {
            fprintf(stderr, "Current node status differs {%d} != {%d}\n", check, check1);
            exit(1);
        }
        if (check) {
            visited.insert(current);
            visited1.insert(current1);
            if (state.rules[current].size() != rules1[current1].size()) {
                fprintf(stderr, "Current rule size differs {%d, %d} != {%d, %d}\n", (int) current, (int)state.rules[current].size(), (int) current1, (int)rules1[current1].size());
                exit(1);
            }
            for (size_t i = 0; i < rules1[current1].size(); ++i) {
                if (state.rules[current][i].type != rules1[current1][i].type) {
                    fprintf(stderr, "Current %ith rule element type differs {%d, %d} != {%d, %d}\n",(int)i, (int)current, (int)state.rules[current][i].type, (int) current1, (int)rules1[current][i].type);
                    exit(1);
                }
                if (rules1[current1][i].type == WHISPER_GRETYPE_RULE_REF) {
                    ref.push(state.rules[current][i].value);
                    ref1.push(rules1[current1][i].value);
                    continue;
                }
                if (state.rules[current][i].value != rules1[current1][i].value) {
                    fprintf(stderr, "Current %ith rule element value differs {%d, %d} != {%d, %d}\n",(int)i, (int)current, (int)state.rules[current][i].value, (int)current1, (int)rules1[current][i].value);
                    exit(1);
                }
            }
            fprintf(stderr, "Rules {%d} and {%d} are identical. Sizes {%d} {%d}\n", (int) current, (int) current1, (int)state.rules[current].size(), (int)rules1[current1].size());
        }
        if (ref1.empty() && visited1.size() != rules1.size()) {
            for (uint32_t i = 0; i < state.rules.size(); ++i) {
                if (visited.find(i) == visited.end()) {
                    ref.push(i);
                    break;
                }
            }
            for (uint32_t i = 0; i < rules1.size(); ++i) {
                if (visited1.find(i) == visited1.end()) {
                    ref1.push(i);
                    break;
                }
            }
        }
    }
    fprintf(stderr, "Grammars are identical\n");
    return 0;
}