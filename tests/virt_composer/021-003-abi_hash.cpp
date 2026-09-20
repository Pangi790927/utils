/* Test21 - Plugins: the hash in VIRT_COMPOSER_ABI still matches the sources it stands for.
 *
 * VIRT_COMPOSER_ABI is what a host and a plugin compare to decide they were built against the same
 * library, and it is written by hand. Its own doc comment says forgetting to raise it is caught
 * nowhere - this test is what catches it. Edit anything that crosses between a host and a plugin
 * and this fails, saying what the value should now be.
 *
 * The files are the ones whose contents reach a plugin: virt_object.h carries object_t, whose
 * layout and vtable a plugin inherits; virt_composer.h carries every inline function, macro and
 * signature a plugin compiles against; virt_composer_end.h closes its registrations;
 * virt_composer.cpp is the code a plugin calls rather than compiles, so a change there moves the
 * behaviour under it without moving a single declaration.
 *
 * THE LINE DEFINING VIRT_COMPOSER_ABI IS SKIPPED, and it has to be: the hash lives inside one of
 * the files it covers, so hashing it whole would mean writing a new value changed the value, and
 * no number could ever be right. Skipping that one line makes it a fixed point.
 *
 * Carriage returns are dropped so a checkout that stores CRLF answers the same as one that does
 * not. Nothing else about the bytes is normalised.
 *
 * 2026-09-20 18:30 */

#include "tests_common.h"
#include "../../virt_composer_end.h"

/* In this order, and the order is part of the answer. 2026-09-20 18:30 */
static const char *ABI_FILES[] = {
    "../../virt_object.h",
    "../../virt_composer.h",
    "../../virt_composer_end.h",
    "../../virt_composer.cpp",
};

/* Without the leading '#', so that a guarded `# define` with a space after the hash is skipped as
surely as a bare `#define`. Matching the stricter spelling let the hash line hash itself the moment
the macro was guarded, and no value could ever settle. `#ifndef VIRT_COMPOSER_ABI` does not contain
this and is hashed, which is right - guarding the macro is a change worth noticing.
2026-09-20 18:52 */
static const char *ABI_DEFINE = "define VIRT_COMPOSER_ABI";

/*! Folds one more byte into an FNV-1a hash. Chosen because it is short enough to read and needs
 * nothing linked in - this has to answer the same on every machine, not be hard to forge.
 * 2026-09-20 18:30 */
static uint32_t fnv1a(uint32_t h, unsigned char c) {
    return (h ^ c) * 16777619u;
}

/*! Folds one file into the hash, or answers -1 having said which file it could not read.
 * 2026-09-20 18:30 */
static int hash_file(const char *path, uint32_t& h) {
    std::ifstream f(path, std::ios::binary);
    if (!f.good()) {
        DBG("cannot read %s - this test must run from tests/virt_composer", path);
        return -1;
    }

    std::string line;
    while (std::getline(f, line)) {
        if (line.find(ABI_DEFINE) != std::string::npos)
            continue;
        for (char c : line)
            if (c != '\r')
                h = fnv1a(h, (unsigned char)c);
        h = fnv1a(h, '\n');
    }
    return 0;
}

/*! Answers the eight characters after the last '-' of VIRT_COMPOSER_ABI, which is where the hash
 * lives, or an empty string if it is not shaped that way. 2026-09-20 18:30 */
static std::string abi_hash_field() {
    std::string abi = VIRT_COMPOSER_ABI;
    auto dash = abi.find_last_of('-');
    if (dash == std::string::npos)
        return "";
    return abi.substr(dash + 1);
}

static int test21_abi_hash_matches_the_sources() {
    uint32_t h = 2166136261u;
    for (auto *path : ABI_FILES)
        ASSERT_FN(hash_file(path, h));

    char computed[16] = {0};
    snprintf(computed, sizeof(computed), "%08x", h);

    std::string carried = abi_hash_field();
    if (carried != computed) {
        DBG("VIRT_COMPOSER_ABI carries the hash '%s', the sources hash to '%s'.\n"
            "    Something crossing between a host and a plugin changed. Raise the version and\n"
            "    write the new hash: #define VIRT_COMPOSER_ABI  \"<version>-%s\"",
            carried.c_str(), computed, computed);
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
