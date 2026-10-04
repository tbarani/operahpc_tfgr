#include "tfgr/Validator.hxx"

#include <cstdio>
#include <cstdlib>

/**
 * @file test_validator.cxx
 * @brief Standalone harness for tfgr::validateAgainstJerkvist().
 *
 * Usage: test_validator <our_csv> <ref_csv> <threshold>
 *
 * Exits EXIT_SUCCESS when the validation passes, EXIT_FAILURE otherwise.
 */

int main(int argc, char* argv[])
{
    if (argc != 4) {
        std::fprintf(stderr,
                     "Usage: %s <our_csv> <ref_csv> <threshold>\n",
                     argv[0]);
        return EXIT_FAILURE;
    }

    const double threshold = std::atof(argv[3]);

    const tfgr::ValidationReport report =
        tfgr::validateAgainstJerkvist(argv[1], argv[2], threshold);

    std::printf("=== Validation report ===\n");
    std::printf("  our rows       : %zu\n", report.n_ours);
    std::printf("  ref rows       : %zu\n", report.n_ref);
    std::printf("  max_abs_error  : %.6e\n", report.max_abs_error);
    std::printf("  mean_abs_error : %.6e\n", report.mean_abs_error);
    std::printf("  final_ours     : %.6e\n", report.final_ours);
    std::printf("  final_ref      : %.6e\n", report.final_ref);
    std::printf("  threshold      : %.6e\n", threshold);
    std::printf("  passed         : %s\n", report.passed ? "yes" : "no");

    return report.passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
