#include "wer.h"

#include <cassert>
#include <cstdio>

int main() {
    std::string reference = "the cat sat on the mat";
    std::string actual = "the cat sat mat";

    wer_result result = calculate_wer(reference, actual);
    assert(result.n_ref_words == 6);
    assert(result.n_act_words == 4);
    assert(result.n_sub == 0);
    assert(result.n_del == 2);
    assert(result.n_ins == 0);
    assert(result.n_edits == 2);
    assert(std::abs(result.wer - 0.333333) < 0.0001);

    return 0;
}
