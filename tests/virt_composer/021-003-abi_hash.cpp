/* Test21 - Plugins: the hash VIRT_COMPOSER_ABI was *compiled* with still matches the sources.
 *
 * abi_hash_tool writes the value as a build step, ahead of anything that compiles the header, so
 * in an ordinary build this passes and says nothing. What it catches is the case the tool cannot:
 * a binary built before the tool ran, or built against a header somebody edited afterwards. The
 * tool can only promise what it wrote; this promises what was compiled, and those are different
 * facts.
 *
 * The mechanism - which files are watched, why that line is skipped, and how the value is read -
 * lives in abi_hash.h, shared with the tool so the two can never disagree about it.
 *
 * VIRT_COMPOSER_ABI is what a host and a plugin compare to decide they were built against the
 * same library, and forgetting to raise it is caught nowhere else.
 *
 * @date 22-09-2026-12:40 */

#include "abi_hash.h"

#include "tests_common.h"
#include "../../virt_composer_end.h"

/*! Answers the eight characters after the last '-' of VIRT_COMPOSER_ABI, which is where the hash
 * lives, or an empty string if it is not shaped that way. This one reads the *macro*, not the
 * file: that is the difference between what was compiled and what is written down.
 * @date 22-09-2026-12:40 */
static std::string abi_hash_field() {
    std::string abi = VIRT_COMPOSER_ABI;
    auto dash = abi.find_last_of('-');
    if (dash == std::string::npos)
        return "";
    return abi.substr(dash + 1);
}

static int test21_abi_hash_matches_the_sources() {
    std::string computed;
    ASSERT_FN(abi_computed_hash(computed) ? 0 : -1);

    std::string carried = abi_hash_field();
    if (carried != computed) {
        DBG("This binary was compiled with the hash '%s', the sources hash to '%s'.\n"
            "    Something crossing between a host and a plugin changed after this was built.\n"
            "    A plain rebuild fixes it: abi_hash_tool runs first and writes the new value.\n"
            "    Whether the version in front of it should also be raised is a judgement about\n"
            "    compatibility and stays yours: #define VIRT_COMPOSER_ABI  \"<version>-%s\"",
            carried.c_str(), computed.c_str(), computed.c_str());
        return -1;
    }
    return 0;
}

int main() {
    int ret = 0;

    ASSERT_FN(test21_abi_hash_matches_the_sources());

    print_test_result("021-003-abi_hash.cpp", ret >= 0);
    return ret;
}
