#ifndef WER_H
#define WER_H
#include <vector>
#include <string>

struct wer_result {
    size_t n_ref_words; // Number of words in the reference text.
    size_t n_act_words; // Number of words in the actual (transcribed) text.
    int    n_sub;       // Number of substitutions.
    int    n_del;       // Number of deletions.
    int    n_ins;       // Number of insertions.
    int    n_edits;     // Total number of edits.
    double wer;         // The word error rate.
};

wer_result calculate_wer(const std::string& reference_text, const std::string& actual_text);

#endif // WER_H
