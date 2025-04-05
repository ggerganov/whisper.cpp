#include "wer.h"

#include <cstdio>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>
#include <tuple>

std::vector<std::string> split_into_words(const std::string& text) {
    std::vector<std::string> words;
    std::stringstream ss(text);
    std::string word;

    while (ss >> word) {
        words.push_back(word);
    }

    return words;
}

std::tuple<int, int, int> count_edit_ops(const std::vector<std::string>& reference,
                                         const std::vector<std::string>& actual) {
    int m = reference.size();
    int n = actual.size();

    // Levenshtein matrix
    std::vector<std::vector<int>> l_matrix(m + 1, std::vector<int>(n + 1, 0));

    // Initialize the first row and column of the matrix.
    for (int i = 0; i <= m; i++) {
        l_matrix[i][0] = i;
    }

    for (int j = 0; j <= n; j++) {
        l_matrix[0][j] = j;
    }

    // Fill the matrix.
    for (int i = 1; i <= m; i++) {
        for (int j = 1; j <= n; j++) {
            if (reference[i-1] == actual[j-1]) {
                l_matrix[i][j] = l_matrix[i-1][j-1];
            } else {
                l_matrix[i][j] = 1 + std::min({
                    l_matrix[i-1][j],     // Deletion (top/above)
                    l_matrix[i][j-1],     // Insertion (left)
                    l_matrix[i-1][j-1]    // Substitution (diagonal)
                });
            }
        }
    }

    // Start backtracking from the bottom-right corner of the matrix.
    int i = m; // rows
    int j = n; // columns

    int substitutions = 0;
    int deletions = 0;
    int insertions = 0;

    // Backtrack to find the edit operations.
    while (i > 0 || j > 0) {
        if (i > 0 && j > 0 && reference[i-1] == actual[j-1]) {
            // Recalll that reference and actual are vectors, so this is just checking
            // the same position in both to see if they are equal. If they are equal
            // this means there was no edit operation, so we move diagonally.
            i--;
            j--;
        } else if (i > 0 && j > 0 && l_matrix[i][j] == l_matrix[i-1][j-1] + 1) {
            // Check the if the current cell is equal to the diagonal cell + 1
            // (for the operation cost), which means we have a substitution.
            substitutions++;
            i--;
            j--;
        } else if (i > 0 && l_matrix[i][j] == l_matrix[i-1][j] + 1) {
            // Check if the current cell is equal the top/above cell + 1
            // (for the operation cost) which means we have a deletion.
            deletions++;
            i--;
        } else {
            // If there there was no match for the diagonal cell or the top/above
            // cell, then we must be at the left cell, which means we have an insertion.
            insertions++;
            j--;
        }
    }

    return {substitutions, deletions, insertions};
}

wer_result calculate_wer(const std::string& reference_text, const std::string& actual_text) {
    std::vector<std::string> reference = split_into_words(reference_text);
    std::vector<std::string> actual = split_into_words(actual_text);

    auto [n_sub, n_del, n_ins] = count_edit_ops(reference, actual);
    int n_edits = n_sub + n_del + n_ins;

    double wer = 0.0;
    if (!reference.empty()) {
        wer = static_cast<double>(n_edits) / reference.size();
    }

    return wer_result{
        /* n_ref_words */ reference.size(),
        /* n_act_words */ actual.size(),
        /* n_sub       */ n_sub,
        /* n_del       */ n_del,
        /* n_ins       */ n_ins,
        /* n_edits     */ n_edits,
        /* wer         */ wer
    };
}
