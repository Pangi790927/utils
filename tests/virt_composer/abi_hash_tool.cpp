/*! @file
 * Brings VIRT_COMPOSER_ABI's hash up to date with the files it stands for.
 *
 * It runs as a build step, ahead of anything that compiles virt_composer.h, so that whatever it
 * writes is what the binaries are built with. That ordering is the whole reason it exists rather
 * than the test doing its own fixing: the test reads its value as a compile-time macro, so a value
 * written after compilation is a value the test cannot see, and it would fail on the very run that
 * repaired it.
 *
 * It never touches the version in front of the hash. Whether a change deserves a new version is a
 * judgement about plugin compatibility, and nothing here can make it.
 *
 * @date 22-09-2026-12:40 */

#include "abi_hash.h"

int main() {
    std::string computed;
    if (!abi_computed_hash(computed))
        return 1;

    std::string text;
    if (abi_hash_in_header(text) == computed)
        return 0;                           /* already right: say nothing, this runs every build */

    if (!abi_write_hash(computed))
        return 1;

    printf("abi_hash: VIRT_COMPOSER_ABI hash refreshed to '%s'. The version in front of it was "
           "not touched -\n           raising it is a judgement about plugin compatibility.\n",
           computed.c_str());
    return 0;
}
