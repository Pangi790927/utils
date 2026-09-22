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
 * The coroutine component is watched on the same two grounds: virt_composer_coroutines.h carries
 * lua_coro_t's layout and lua_await, a template a plugin compiles into itself, and
 * virt_composer_coroutines.cpp is behaviour a plugin calls. It is watched ahead of being reachable
 * -- its #error guard means only virt_composer.h may include it, and virt_composer.h does not yet
 * -- because the alternative is remembering to add it on the day that changes, which is the kind
 * of thing this test exists to stop depending on. 2026-09-22 05:10
 *
 * THE LINE DEFINING VIRT_COMPOSER_ABI IS SKIPPED, and it has to be: the hash lives inside one of
 * the files it covers, so hashing it whole would mean writing a new value changed the value, and
 * no number could ever be right. Skipping that one line makes it a fixed point.
 *
 * Carriage returns are dropped so a checkout that stores CRLF answers the same as one that does
 * not. Nothing else about the bytes is normalised.
 *
 * 2026-09-20 18:30 */

#include <iterator>

#include "tests_common.h"
#include "../../virt_composer_end.h"

/* In this order, and the order is part of the answer. 2026-09-20 18:30 */
static const char *ABI_FILES[] = {
    "../../virt_object.h",
    "../../virt_composer.h",
    "../../virt_composer_end.h",
    "../../virt_composer.cpp",
    "../../virt_composer_coroutines.h",
    "../../virt_composer_coroutines.cpp",
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

/*! Writes `computed` into the hash field of the VIRT_COMPOSER_ABI line, leaving the version in
 * front of it and every other byte of the file exactly as it was. The file is spliced rather than
 * rewritten line by line, so line endings survive whatever they are. Answers -1, having said
 * which shape it did not recognise, rather than writing something it is unsure of.
 * 2026-09-22 05:10 */
static int rewrite_abi_hash(const std::string& computed) {
    const char *path = "../../virt_composer.h";

    std::ifstream in(path, std::ios::binary);
    if (!in.good()) {
        DBG("cannot read %s - this must run from tests/virt_composer", path);
        return -1;
    }
    std::string text{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    in.close();

    auto def = text.find(ABI_DEFINE);
    if (def == std::string::npos) {
        DBG("no '%s' line in %s", ABI_DEFINE, path);
        return -1;
    }
    auto q1 = text.find('"', def);
    auto q2 = (q1 == std::string::npos) ? std::string::npos : text.find('"', q1 + 1);
    if (q2 == std::string::npos) {
        DBG("the '%s' line carries no quoted value", ABI_DEFINE);
        return -1;
    }
    /* The hash is what follows the last '-' of the value, which is where abi_hash_field() reads
    it from, so the version in front of it is never touched. 2026-09-22 05:10 */
    auto dash = text.rfind('-', q2);
    if (dash == std::string::npos || dash < q1) {
        DBG("the value has no '-' to write after: a version and a hash were expected");
        return -1;
    }
    text.replace(dash + 1, q2 - dash - 1, computed);

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out.good()) {
        DBG("cannot write %s", path);
        return -1;
    }
    out << text;
    return out.good() ? 0 : -1;
}

/*! Answers the hash the sources come to, or -1 having said which file it could not read.
 * 2026-09-22 05:10 */
static int computed_hash(std::string& out) {
    uint32_t h = 2166136261u;
    for (auto *path : ABI_FILES)
        ASSERT_FN(hash_file(path, h));

    char buf[16] = {0};
    snprintf(buf, sizeof(buf), "%08x", h);
    out = buf;
    return 0;
}

static int test21_abi_hash_matches_the_sources() {
    std::string computed;
    ASSERT_FN(computed_hash(computed));

    std::string carried = abi_hash_field();
    if (carried != computed) {
        DBG("VIRT_COMPOSER_ABI carries the hash '%s', the sources hash to '%s'.\n"
            "    Something crossing between a host and a plugin changed. Run this binary again\n"
            "    with --fix to write the new hash, which run_tests.py does for you when nothing\n"
            "    else failed. Whether the version in front of it should also be raised is a\n"
            "    judgement about compatibility and stays yours:\n"
            "        #define VIRT_COMPOSER_ABI  \"<version>-%s\"",
            carried.c_str(), computed.c_str(), computed.c_str());
        return -1;
    }
    return 0;
}

/*! Writes the hash the sources come to, and says what it did. Answers 0 when there was nothing to
 * do, so a caller can run it blind. 2026-09-22 05:10 */
static int fix_abi_hash() {
    std::string computed;
    ASSERT_FN(computed_hash(computed));

    if (abi_hash_field() == computed) {
        DBG("VIRT_COMPOSER_ABI already carries '%s', nothing written", computed.c_str());
        return 0;
    }
    ASSERT_FN(rewrite_abi_hash(computed));
    DBG("VIRT_COMPOSER_ABI hash refreshed to '%s'. The version in front of it was not touched - "
        "raising it is a judgement about plugin compatibility.", computed.c_str());
    return 0;
}

/* `--fix` writes the hash instead of failing over it. Without it this is the gate it always was,
so a plain run still tells the truth about a stale value. 2026-09-22 05:10 */
int main(int argc, char **argv) {
    bool fix = (argc > 1 && std::string(argv[1]) == "--fix");
    int  ret = 0;

    if (fix) {
        ASSERT_FN(fix_abi_hash());
        print_test_result("021-003-abi_hash.cpp --fix", ret >= 0);
        return ret;
    }

    ASSERT_FN(test21_abi_hash_matches_the_sources());

    print_test_result("021-003-abi_hash.cpp", ret >= 0);
    return ret;
}
