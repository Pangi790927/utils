#include "ap_storage.h"
#include "ap_vector.h"
#include "ap_string.h"
#include "debug.h"
#include "misc_utils.h"
#include "sys_utils.h"
#include "ap_ptr.h"
#include "time_utils.h"

#include <sys/wait.h>
#include <unistd.h>
#include <map>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCK_PATH "./ap_storage_test_comm"

struct test_struct_t {
    ap_vector_t<int> vec;
    ap_string_t str;
    ap_ptr_t<ap_string_t> pstr1;
    ap_ptr_t<ap_string_t> pstr2;
    ap_ptr_t<ap_string_t> pstr3;
};

struct test_ap_ptr_t {
    ap_ptr_t<test_struct_t> a1;
    ap_ptr_t<test_struct_t> a2;
    ap_ptr_t<test_struct_t> a3;
};

struct test_ap_str_t {
    ap_vector_t<ap_string_t> strings;
};

struct test_ap_malloc_t {
    ap_off_t ptrs[8192] = {0};
    uint64_t sz[8192] = {0};
};

struct test_ap_vector_t {};
struct test_ap_map_t {};
struct test_ap_hashmap_t {};

static bool full_tests = false;

void ap_storage_except_cbk(void *ctx, const char *errmsg, ap_except_info_t *ei) {
    DBG("[CRITICAL] %s\n%s", errmsg, ei->bt.c_str());
    exit(1);
}

static int run_program(const char *prog_name, const char *param) {
    int pid;
    ASSERT_FN(pid = fork());
    if (pid == 0) {
        DBG("Will exec: %s %s", prog_name, param);
        std::vector<const char *> argv = {prog_name, param, NULL};
        ASSERT_FN(execv(prog_name, (char* const*)argv.data()));
    }
    else {
        int status = 0;
        ASSERT_FN(waitpid(pid, &status, 0));
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            DBG("Guest program failed");
            return -1;
        }
    }
    return 0;
}

static void clear_tests() {
    unlink("data/storage");
    unlink("data/storage_0.data");
    unlink("data/storage_1.data");
}

static void alloc_slot(test_ap_malloc_t *tam, uint32_t slot, uint32_t sz) {
    uint8_t val = rand() % 256;
    tam->ptrs[slot] = ap_malloc_alloc(ap_static_ctx, sz);
    tam->sz[slot] = sz;
    uint8_t *ptr = (uint8_t *)ap_malloc_ptr(ap_static_ctx, tam->ptrs[slot]);
    memset(ptr, val, sz);
}

static void free_slot(test_ap_malloc_t *tam, uint32_t slot) {
    uint8_t *ptr = (uint8_t *)ap_malloc_ptr(ap_static_ctx, tam->ptrs[slot]);
    memset(ptr, 0, tam->sz[slot]);
    ap_malloc_free(ap_static_ctx, tam->ptrs[slot]);
    tam->ptrs[slot] = 0;
    tam->sz[slot] = 0;
}

static uint64_t hash_slots(test_ap_malloc_t *tam) {
    uint64_t ret = 0;
    for (int i = 0; i < 8192; i++) {
        uint64_t val = tam->ptrs[i];
        uint64_t h = std::hash<uint64_t>{}(val);
        ret = ret ^ (h << 1);
        val = tam->sz[i];
        h = std::hash<uint64_t>{}(val);
        ret = ret ^ (h << 1);
        if (tam->ptrs[i] && tam->sz[i]) {
            uint8_t *ptr = (uint8_t *)ap_malloc_ptr(ap_static_ctx, tam->ptrs[i]);
            val = ptr[0];
            h = std::hash<uint64_t>{}(val);
            std::vector<uint8_t> cpy(tam->sz[i]);
            memcpy(cpy.data(), ptr, tam->sz[i]); // this is here so we validate data access
            ret = ret ^ (h << 1);
        }
    }
    return ret;
}

static int do_host_stuff(const char *prog_name) {
    clear_tests();

    DBG("######################### init_exit:");
    ASSERT_FN(run_program(prog_name, "init_exit"));
    ASSERT_FN(run_program(prog_name, "init_exit"));
    clear_tests();

    /* TODO: test every feature */
    if (full_tests) {
        DBG("######################### ap_malloc_test:");
        ASSERT_FN(run_program(prog_name, "ap_malloc_long_test"));
        ASSERT_FN(run_program(prog_name, "ap_malloc_read_test"));
        clear_tests();
    }

    DBG("######################### base_test:");
    ASSERT_FN(run_program(prog_name, "base_test"));
    ASSERT_FN(run_program(prog_name, "read_test"));
    clear_tests();

    DBG("######################### ap_ptr_test:");
    ASSERT_FN(run_program(prog_name, "ap_ptr_base_test"));
    ASSERT_FN(run_program(prog_name, "ap_ptr_read_test"));
    clear_tests();

    DBG("######################### ap_str_test:");
    ASSERT_FN(run_program(prog_name, "ap_str_base_test"));
    ASSERT_FN(run_program(prog_name, "ap_str_read_test"));
    clear_tests();

    /* TODO */
    DBG("######################### ap_vector_test:");
    ASSERT_FN(run_program(prog_name, "ap_vector_base_test"));
    ASSERT_FN(run_program(prog_name, "ap_vector_read_test"));
    clear_tests();

    /* TODO */
    DBG("######################### ap_map_test:");
    ASSERT_FN(run_program(prog_name, "ap_map_base_test"));
    ASSERT_FN(run_program(prog_name, "ap_map_read_test"));
    clear_tests();

    /* TODO */
    DBG("######################### ap_hashmap_test:");
    ASSERT_FN(run_program(prog_name, "ap_hashmap_base_test"));
    ASSERT_FN(run_program(prog_name, "ap_hashmap_read_test"));
    clear_tests();

    DBG("UNINIT");

    return 0;
}

static int do_guest_stuff(const char *_param) {
    std::string param = _param;
    uint64_t last_malloc_test_hash = 0;
    if (param == "init_exit") {
        DBG("Start INIT_EXIT");
        ASSERT_FN(ap_storage_init("data/storage", ap_storage_except_cbk, NULL));
        ASSERT_FN(ap_storage_do_changes(AP_STORAGE_COMMIT_CHANGES));
        ap_storage_uninit();
        DBG("Done INIT_EXIT");
    }
    else if (param == "ap_malloc_long_test") {
        DBG("Start ap_malloc_long_test");
        ASSERT_FN(ap_storage_init("data/storage", ap_storage_except_cbk, NULL));
        auto [off, ptr] = ap_storage_construct<test_ap_malloc_t>();
        ap_malloc_set_usr(ap_static_ctx, off);

        memset(ptr->ptrs, 0, sizeof(ap_off_t) * 8192);
        memset(ptr->sz, 0, sizeof(uint64_t) * 8192);
        
        ASSERT_FN(ap_storage_do_changes(AP_STORAGE_COMMIT_CHANGES));

        for (int i = 0; i < 100000; i++) {
            bool do_commit = rand() % 1000 == 0;
            bool do_revert = rand() % 1000 == 0;
            uint32_t slot = rand() % 8192;
            // uint32_t sz = rand() % 8192 + 1;
            // uint32_t sz = (i % 16) + 1;
            uint32_t sz = (i % 200) + 1;
            // DBG("i: %d slot: %d sz: %d com: %d rev: %d", i, slot, sz, do_commit, do_revert);

            if (do_commit) {
                uint64_t hash = hash_slots(ptr);
                ASSERT_FN(ap_storage_do_changes(AP_STORAGE_COMMIT_CHANGES));
                if (hash != hash_slots(ptr)) {
                    DBG("Failed because hashes differ after commit");
                    return -1;
                }
                last_malloc_test_hash = hash;
                DBG("[%5d]Commit: %lx", i, hash);
            }
            else if (do_revert) {
                ASSERT_FN(ap_storage_do_changes(AP_STORAGE_REVERT_CHANGES));
        
                if (hash_slots(ptr) != last_malloc_test_hash) {
                    DBG("Failed because hashes differ after revert");
                    return -1;
                }
                DBG("[%5d]Revert: %lx", i, last_malloc_test_hash);
            }
            else if (!ptr->ptrs[slot]) {
                alloc_slot(ptr, slot, sz);
            }
            else {
                free_slot(ptr, slot);
            }
        }
        DBG("last_malloc_test_hash %lx", last_malloc_test_hash);

        ASSERT_FN(ap_storage_do_changes(AP_STORAGE_COMMIT_CHANGES));
        ap_storage_uninit();
    }
    else if (param == "ap_malloc_read_test") {
        DBG("Start ap_malloc_read_test");
        ASSERT_FN(ap_storage_init("data/storage", ap_storage_except_cbk, NULL));

        ap_off_t off = ap_malloc_get_usr(ap_static_ctx);
        test_ap_malloc_t *ptr = (test_ap_malloc_t *)ap_malloc_ptr(ap_static_ctx, off);

        /* must be the same as last_malloc_test_hash (the last commit) */
        DBG("Restart hash: %lx", hash_slots(ptr));

        ap_storage_uninit();
    }
    else if (param == "ap_str_base_test") {
        DBG("Start ap_str_base_test");
        ASSERT_FN(ap_storage_init("data/storage", ap_storage_except_cbk, NULL));
        auto [off, ptr] = ap_storage_construct<test_ap_str_t>();
        ap_malloc_set_usr(ap_static_ctx, off);

        const char *strings[] = {"a", "b", "c", "Ana are mere", "Bob", "Ciuperca", "Bazin",
                "Algebra", "Papuc", "Zig-Zag", "Mai", "Alea merg mai mult", "Ceva"};

        for (auto s : strings) {
            ptr->strings.push_back(std::string(s));
        }

        {            
            /* test ap_string_t */
            ap_string_t str1;
            str1 = "This is a string";

            ap_string_t str2(str1);
            DBG("str1: [%s] str2: [%s]", str1.c_str(), str2.c_str());
            ptr->strings.push_back(str1);
            ptr->strings.push_back(str2);

            /* test append/size */
            std::string append = "_the append";
            str1.append(append);
            str1.append("_");
            str1.append(str2);
            DBG("str1 after append: [%s] size: %ld", str1.c_str(), str1.size());
            ptr->strings.push_back(str1);

            /* test clear */
            str1.clear();
            DBG("str1 after clear: [%s] size: %ld", str1.c_str(), str1.size());
            ptr->strings.push_back(str1);

            /* test begin/end/cbegin/cend */
            std::string cppstr = "";
            for (auto &c : str2)
                cppstr += c;
            DBG("begin/end: [%s]", cppstr.c_str());
            ptr->strings.push_back(str2);

            for (const auto &c : str2)
                cppstr += c;
            DBG("cbegin/cend: [%s]", cppstr.c_str());
            ptr->strings.push_back(str2);

            /* test operator = */
            str1 = str2;
            DBG("str1 ap_str: [%s]", str1.c_str());

            str1 = cppstr;
            DBG("str1 cpp_str: [%s]", str1.c_str());

            str1 = "The new string";
            DBG("str1 c_str: [%s]", str1.c_str());
            ptr->strings.push_back(str1);

            /* test operator += */
            str1.clear();
            str1 += str2;
            str1 += "___";
            str1 += cppstr;
            DBG("str1 +=: [%s]", str1.c_str());
            ptr->strings.push_back(str1);

            /* test operator + */
            str1.clear();
            str1 = (str1 + str2) + "~~~" + (str2 + str2) + "~~~" + std::string("Bob");
            DBG("str1 +: [%s]", str1.c_str());
            ptr->strings.push_back(str1);
        }

        ASSERT_FN(ap_storage_do_changes(AP_STORAGE_COMMIT_CHANGES));
        ap_storage_uninit();
    }
    else if (param == "ap_str_read_test") {
        DBG("Start ap_str_read_test");
        ASSERT_FN(ap_storage_init("data/storage", ap_storage_except_cbk, NULL));

        ap_off_t off = ap_malloc_get_usr(ap_static_ctx);
        test_ap_str_t *ptr = (test_ap_str_t *)ap_malloc_ptr(ap_static_ctx, off);

        for (auto &str : ptr->strings) {
            DBG("Str: %s", str.c_str());
        }

        ap_storage_uninit();
    }
    else if (param == "ap_ptr_base_test") {
        DBG("Start ap_ptr_base_test");
        ASSERT_FN(ap_storage_init("data/storage", ap_storage_except_cbk, NULL));
        auto [off, ptr] = ap_storage_construct<test_ap_ptr_t>();
        ap_malloc_set_usr(ap_static_ctx, off);

        ptr->a1 = ap_ptr_t<test_struct_t>::mkptr();
        ptr->a2 = ap_ptr_t<test_struct_t>::mkptr();
        ptr->a3 = ap_ptr_t<test_struct_t>::mkptr();

        auto &ptest = ptr->a1;

        /* test ap_ptr_t::mkptr */
        ptest->pstr1 = ap_ptr_t<ap_string_t>::mkptr("This is a string");

        /* test ap_ptr_t::operator-> */
        DBG("String (->): %s", ptest->pstr1->c_str());

        /* test ap_ptr_t::operator* */
        DBG("String (*): %s", (*ptest->pstr1).c_str());

        /* test ap_ptr_t::operator bool */
        if (ptest->pstr1) {
            DBG("String pointer valid");
        }
        if (!ptest->pstr2) {
            DBG("String pointer not valid (OK)");
        }

        /* test ap_ptr_t::ap_ptr_t */
        ap_ptr_t<ap_string_t> pref(ptest->pstr1);
        ap_ptr_t<ap_string_t> pref2(ap_ptr_t<ap_string_t>::mkptr("This is another string"));

        DBG("String ref1: %s", pref->c_str());
        DBG("String ref2: %s", pref2->c_str());

        /* test ap_ptr_t::operator = */
        pref = ap_ptr_t<ap_string_t>::mkptr("This is yet another string");
        pref2 = pref;

        DBG("String ref1: %s", pref->c_str());
        DBG("String ref2: %s", pref2->c_str());

        /* pointers need to be reseted before ap_storage_uninit(), also test reset */
        pref.reset();
        pref2.reset();

        if (!pref && !pref2 && ptest->pstr1) {
            DBG("Only ptest->pstr1 is valid now (OK)");
        }

        ptr->a1->vec.push_back(1);
        ptr->a1->vec.push_back(2);
        ptr->a1->vec.push_back(3);
        ptr->a2->vec.push_back(11);
        ptr->a2->vec.push_back(12);
        ptr->a3->vec.push_back(0);

        ptr->a1.reset();

        ASSERT_FN(ap_storage_do_changes(AP_STORAGE_COMMIT_CHANGES));
        ap_storage_uninit();
    }
    else if (param == "ap_ptr_read_test") {
        DBG("Start ap_ptr_read_test");
        ASSERT_FN(ap_storage_init("data/storage", ap_storage_except_cbk, NULL));

        ap_off_t off = ap_malloc_get_usr(ap_static_ctx);
        test_ap_ptr_t *ptr = (test_ap_ptr_t *)ap_malloc_ptr(ap_static_ctx, off);

        if (ptr->a1)
            for (auto i : ptr->a1->vec)
                DBG("a1: %d", i);
        if (ptr->a2)
            for (auto i : ptr->a2->vec)
                DBG("a2: %d", i);
        if (ptr->a3)
            for (auto i : ptr->a3->vec)
                DBG("a3: %d", i);

        ap_storage_uninit();
    }
    else if (param == "base_test") {
        DBG("Start BASE_TEST");
        ASSERT_FN(ap_storage_init("data/storage", ap_storage_except_cbk, NULL));

        {
            /* OBS: Don't use ap_vector_t before/after initing/uniniting the ap_storage, because you
            will get a strange segfault */
            ap_vector_t<int> vec;

            for (int i = 0; i < 4096; i++) {
                ASSERT_FN(vec.push_back(0xf1f1f1f1));
            }
        }

        auto [off, test_struct] = ap_storage_construct<test_struct_t>();
        ap_malloc_set_usr(ap_static_ctx, off);

        test_struct->vec.push_back(1);
        test_struct->vec.push_back(2);
        test_struct->vec.push_back(4);
        test_struct->vec.push_back(8);
        test_struct->vec.push_back(16);
        test_struct->vec.push_back(32);
        test_struct->vec.push_back(64);
        test_struct->vec.push_back(128);

        test_struct->str = "Ana are mere";

        ASSERT_FN(ap_storage_do_changes(AP_STORAGE_COMMIT_CHANGES));
        test_struct->vec.push_back(256);
        test_struct->vec.push_back(512);
        ASSERT_FN(ap_storage_do_changes(AP_STORAGE_COMMIT_CHANGES)); /* TODO: fix, second commit does nothing */
        test_struct->vec.push_back(1023);
        test_struct->vec.push_back(2047);
        ASSERT_FN(ap_storage_do_changes(AP_STORAGE_REVERT_CHANGES));
        test_struct->vec.push_back(1024);
        test_struct->vec.push_back(2048);
        ASSERT_FN(ap_storage_do_changes(AP_STORAGE_COMMIT_CHANGES));
        test_struct->vec.push_back(777);
        test_struct->vec.push_back(888);

        ap_storage_uninit();
        DBG("Done BASE_TEST");
    }
    else if (param == "read_test") {
        DBG("Start READ_TEST");
        ASSERT_FN(ap_storage_init("data/storage", ap_storage_except_cbk, NULL));

        {
            ap_off_t off = ap_malloc_get_usr(ap_static_ctx);
            test_struct_t *test_struct = (test_struct_t *)ap_malloc_ptr(ap_static_ctx, off);

            DBG("str: %s", test_struct->str.c_str());
            for (auto i : test_struct->vec)
                DBG("%d", i);
        }

        ASSERT_FN(ap_storage_do_changes(AP_STORAGE_COMMIT_CHANGES));
        ap_storage_uninit();
        DBG("Done READ_TEST");
    }
    else if (param == "ap_vector_base_test") {
        DBG("Start ap_vector_test");
        ASSERT_FN(ap_storage_init("data/storage", ap_storage_except_cbk, NULL));
        auto [off, ptr] = ap_storage_construct<test_ap_vector_t>();
        ap_malloc_set_usr(ap_static_ctx, off);

        {
            auto vec2str = [](auto &vec) {
                std::string res = "{";
                for (auto &e : vec) {
                    res += std::to_string(e);
                    if (&e != &vec.back())
                        res += ", ";
                }
                res += "}";
                return res;
            };

            ap_vector_t<int> vec0;
            ap_vector_t<int> vec1({1, 2, 3, 4});
            ap_vector_t<int> vec2(vec1);
            ap_vector_t<int> vec3(vec2.begin(), vec2.end());
            ap_vector_t<int> vec4(std::vector<long>{10, 20, 30, 40});
            ap_vector_t<int> vec5(ap_vector_t<int>{1, 2, 3, 4});

            DBG("vec0: %s vec1: %s vec2: %s vec3: %s vec4: %s vec5: %s", vec2str(vec0).c_str(),
                    vec2str(vec1).c_str(), vec2str(vec2).c_str(), vec2str(vec3).c_str(),
                    vec2str(vec4).c_str(), vec2str(vec5).c_str());

            vec5.clear();
            DBG("vec5 clear: %s", vec2str(vec5).c_str());

            vec3 = vec4;
            DBG("vec3 after vec3 = vec4: %s", vec2str(vec3).c_str());

            vec4 = ap_vector_t<int>{6, 7};
            DBG("vec4 after vec4 = vec{6, 7}: %s", vec2str(vec4).c_str());

            vec4.resize(10, 3);
            DBG("vec4 resize 10, 3: %s", vec2str(vec4).c_str());

            std::string vecstr = "";
            for (const auto &e : vec2)
                vecstr += std::to_string(e) + ", ";
            DBG("cbegin/cend vec2: %s", vecstr.c_str());

            vec4[7] = 2;
            vec4.erase(vec4.begin() + 1, vec4.begin() + 7);
            DBG("vec4[7] = 2; vec4 erase 1..7: %s", vec2str(vec4).c_str());

            std::vector<int> to_insert{-1, -2, -3, -4};
            vec4.insert(vec4.begin() + 1, to_insert.begin(), to_insert.end());
            DBG("vec4.insert vec{-1, -2, -3, -4} at 1: %s", vec2str(vec4).c_str());

            vec4.insert(vec4.begin() + 2, 10);
            DBG("vec4.insert 10 at 2: %s", vec2str(vec4).c_str());

            vec4.insert(vec4.begin() + 1, {1, 2, 3, 4});
            DBG("vec4.insert {1, 2, 3, 4} at 1: %s", vec2str(vec4).c_str());

            vec4.push_back(50);
            DBG("vec4.push_back 50: %s", vec2str(vec4).c_str());
            DBG("vec4.back: %d vec4.front: %d", vec4.back(), vec4.front());

            DBG("hexdump(vec4.data(), vec4.size() * sizeof(int)):");
            hexdump(vec4.data(), vec4.size() * sizeof(int));

            DBG("vec0.size(): %ld", vec0.size());

            vec4.pop_back();
            DBG("vec4.pop_back(): %s", vec2str(vec4).c_str());

            DBG("vec4[0]: %d vec4[1]: %d vec4[3]: %d vec4[-1]: %d vec4[-2]: %d vec4[-3]: %d",
                    vec4[0], vec4[1], vec4[3], vec4[-1], vec4[-2], vec4[-3]);
        }

        /* TODO: some tests that test commits and reads afterwards */

        ASSERT_FN(ap_storage_do_changes(AP_STORAGE_COMMIT_CHANGES));
        ap_storage_uninit();
    }
    else if (param == "ap_vector_read_test") {
        DBG("Start ap_vector_read_test");
        ASSERT_FN(ap_storage_init("data/storage", ap_storage_except_cbk, NULL));

        ap_off_t off = ap_malloc_get_usr(ap_static_ctx);
        test_ap_vector_t *ptr = (test_ap_vector_t *)ap_malloc_ptr(ap_static_ctx, off);

        /* TODO: read */

        ap_storage_uninit();
    }
    else if (param == "ap_map_base_test") {
        DBG("Start ap_map_test");
        ASSERT_FN(ap_storage_init("data/storage", ap_storage_except_cbk, NULL));
        auto [off, ptr] = ap_storage_construct<test_ap_map_t>();
        ap_malloc_set_usr(ap_static_ctx, off);

        {
            /* TODO: tests */
        }

        ASSERT_FN(ap_storage_do_changes(AP_STORAGE_COMMIT_CHANGES));
        ap_storage_uninit();
    }
    else if (param == "ap_map_read_test") {
        DBG("Start ap_map_read_test");
        ASSERT_FN(ap_storage_init("data/storage", ap_storage_except_cbk, NULL));

        ap_off_t off = ap_malloc_get_usr(ap_static_ctx);
        test_ap_map_t *ptr = (test_ap_map_t *)ap_malloc_ptr(ap_static_ctx, off);

        /* TODO: read */

        ap_storage_uninit();
    }
    else if (param == "ap_hashmap_base_test") {
        DBG("Start ap_hashmap_test");
        ASSERT_FN(ap_storage_init("data/storage", ap_storage_except_cbk, NULL));
        auto [off, ptr] = ap_storage_construct<test_ap_hashmap_t>();
        ap_malloc_set_usr(ap_static_ctx, off);

        {
            /* TODO: tests */
        }

        ASSERT_FN(ap_storage_do_changes(AP_STORAGE_COMMIT_CHANGES));
        ap_storage_uninit();
    }
    else if (param == "ap_hashmap_read_test") {
        DBG("Start ap_hashmap_read_test");
        ASSERT_FN(ap_storage_init("data/storage", ap_storage_except_cbk, NULL));

        ap_off_t off = ap_malloc_get_usr(ap_static_ctx);
        test_ap_hashmap_t *ptr = (test_ap_hashmap_t *)ap_malloc_ptr(ap_static_ctx, off);

        /* TODO: read */

        ap_storage_uninit();
    }
    return 0;
}

int main(int argc, char const *argv[]) {
    DBG_SCOPE();

    /* ap storage should be able to exit at all times and the data should be always consistent.
    Changes need to be saved using ap_storage_submit_changes */

    /* This db, if I can call it a db, works as follows: it creates a header file of 4Kb and two
    copies of the memory. The copies are filled in turn such that at least one of them is valid
    at any point in time. The header remembers the last valid copy such that it can recover from
    it in case of a malfunction of any kind. Both copies are mapped in virtual memory, the base
    one is always mapped and the backup is only mapped during a submit. */

    /* To test the storage we need to cover the following cases:
        1. creation of storage if no storage exists
        2. loading of storage if storage already exists
        3. having more than one page allocated
        4. having sparse pages modified
        5. data integrity should be checked

        Those tests should be also done with the two init modes of vec, map, hmap, string
        a. manual init
        b. auto init

        Finally the tests should also be done using the different methods of error passing
    */

    /* On data integrity checking: Data is required to be in order, not necessarily complete,
    meaning that data between save points should be valid, not necessarily all data that was
    inserted. To test that we will mimic the behavior of ap_* objects with srd::* objects and
    remember the state at the beginning of each submit. We will denote the current submit by
    curr_sub and the submit before that as last_sub. We have the following cases:
        1. reload == curr_sub, means that the submit was successful and everything is ok
        2. reload == last_sub, means that the submit was not successful, but that is expected, so ok
        3. if the reload matches neither of the two then the program is known to be broken
    All tests will be executed without fault signals to ensure that on a normal run the program
    would behave properly.
    To avoid loosing a lot of time designing exact tests for each possible break point the guest
    program will auto send the signal based on the different parameters received and send the
    curr_sub and last_sub via unix_sockets to the host program.
    */

    /* TODO: add all the needed tests */

    if (argc != 1 && argc != 2) {
        DBG("This program must be manually executed without parameters, "
                "the parameters are for internal use");
        return -1;
    }
    if (argc == 1) {
        ASSERT_FN(do_host_stuff(argv[0]));
    }
    else {
        ASSERT_FN(do_guest_stuff(argv[1]));
    }

    return 0;
}
