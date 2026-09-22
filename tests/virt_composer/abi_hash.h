#ifndef ABI_HASH_H
#define ABI_HASH_H

/*! @file
 * The ABI hash: what the files crossing between a host and a plugin come to, and how the value
 * carried in VIRT_COMPOSER_ABI is read and written.
 *
 * It is shared by two things that must never disagree. abi_hash_tool writes the value, as a build
 * step ahead of anything that compiles the header; 021-003-abi_hash checks that the value a
 * translation unit was *compiled* with is that one. The check is the point: the tool can only
 * promise what it wrote, and a stale build is exactly what the test is there to catch.
 *
 * Nothing here includes virt_composer.h, deliberately. The tool must be able to rewrite the
 * header without carrying a copy of the macro it is rewriting.
 *
 * @date 22-09-2026-12:40 */

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>

/* In this order, and the order is part of the answer. The files are the ones whose contents reach
a plugin: virt_object.h carries object_t, whose layout and vtable a plugin inherits;
virt_composer.h carries every inline function, macro and signature a plugin compiles against;
virt_composer_end.h closes its registrations; virt_composer.cpp is code a plugin calls rather than
compiles, so a change there moves the behaviour under it without moving a declaration. The
coroutine component is watched on both grounds: its header carries lua_coro_t's layout and
lua_await, a template a plugin compiles into itself, and its .cpp is behaviour a plugin calls.
22-09-2026-12:40 */
static const char *ABI_FILES[] = {
    "../../virt_object.h",
    "../../virt_composer.h",
    "../../virt_composer_end.h",
    "../../virt_composer.cpp",
    "../../virt_composer_coroutines.h",
    "../../virt_composer_coroutines.cpp",
};

/*! The file the value is written into and read back from. @date 22-09-2026-12:40 */
static const char *ABI_HEADER = "../../virt_composer.h";

/* Without the leading '#', so that a guarded `# define` with a space after the hash is skipped as
surely as a bare `#define`. Matching the stricter spelling let the hash line hash itself the moment
the macro was guarded, and no value could ever settle. `#ifndef VIRT_COMPOSER_ABI` does not contain
this and is hashed, which is right - guarding the macro is a change worth noticing.
2026-09-20 18:52 */
static const char *ABI_DEFINE = "define VIRT_COMPOSER_ABI";

/*! Folds one more byte into an FNV-1a hash. Short enough to read and needs nothing linked in: this
 * has to answer the same on every machine, not be hard to forge. @date 2026-09-20 18:30 */
inline uint32_t abi_fnv1a(uint32_t h, unsigned char c) {
    return (h ^ c) * 16777619u;
}

/*! Folds one file into the hash, skipping the line that carries the value itself. Carriage returns
 * are dropped so a checkout storing CRLF answers the same as one that does not; nothing else about
 * the bytes is normalised. Answers false having said which file it could not read.
 * @date 22-09-2026-12:40 */
inline bool abi_hash_file(const char *path, uint32_t& h) {
    std::ifstream f(path, std::ios::binary);
    if (!f.good()) {
        fprintf(stderr, "abi_hash: cannot read %s - this must run from tests/virt_composer\n",
                path);
        return false;
    }

    std::string line;
    while (std::getline(f, line)) {
        if (line.find(ABI_DEFINE) != std::string::npos)
            continue;
        for (char c : line)
            if (c != '\r')
                h = abi_fnv1a(h, (unsigned char)c);
        h = abi_fnv1a(h, '\n');
    }
    return true;
}

/*! Answers the eight hex characters the watched files come to. @date 22-09-2026-12:40 */
inline bool abi_computed_hash(std::string& out) {
    uint32_t h = 2166136261u;
    for (auto *path : ABI_FILES)
        if (!abi_hash_file(path, h))
            return false;

    char buf[16] = {0};
    snprintf(buf, sizeof(buf), "%08x", h);
    out = buf;
    return true;
}

/*! Answers what follows the last '-' of the quoted value on the VIRT_COMPOSER_ABI line, read out
 * of the file rather than out of a macro, and an empty string if the line is not shaped that way.
 * @date 22-09-2026-12:40 */
inline std::string abi_hash_in_header(std::string& whole_file) {
    std::ifstream in(ABI_HEADER, std::ios::binary);
    if (!in.good()) {
        fprintf(stderr, "abi_hash: cannot read %s\n", ABI_HEADER);
        return "";
    }
    whole_file.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());

    auto def = whole_file.find(ABI_DEFINE);
    if (def == std::string::npos)
        return "";
    auto q1 = whole_file.find('"', def);
    auto q2 = (q1 == std::string::npos) ? std::string::npos : whole_file.find('"', q1 + 1);
    if (q2 == std::string::npos)
        return "";
    auto dash = whole_file.rfind('-', q2);
    if (dash == std::string::npos || dash < q1)
        return "";
    return whole_file.substr(dash + 1, q2 - dash - 1);
}

/*! Writes `computed` into the hash field, leaving the version in front of it and every other byte
 * of the file as it was: the file is spliced rather than rewritten line by line, so line endings
 * survive whatever they are. @date 22-09-2026-12:40 */
inline bool abi_write_hash(const std::string& computed) {
    std::string text;
    std::string carried = abi_hash_in_header(text);
    if (carried.empty()) {
        fprintf(stderr, "abi_hash: the %s line is not a version and a hash\n", ABI_DEFINE);
        return false;
    }

    auto def  = text.find(ABI_DEFINE);
    auto q2   = text.find('"', text.find('"', def) + 1);
    auto dash = text.rfind('-', q2);
    text.replace(dash + 1, q2 - dash - 1, computed);

    std::ofstream out(ABI_HEADER, std::ios::binary | std::ios::trunc);
    if (!out.good()) {
        fprintf(stderr, "abi_hash: cannot write %s\n", ABI_HEADER);
        return false;
    }
    out << text;
    return out.good();
}

#endif /* ABI_HASH_H */
