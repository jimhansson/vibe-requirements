import re

with open("tests/test_entity.cpp", "r") as f:
    content = f.read()

# ── Add #include <algorithm> and vec_pair_has helper after the includes ────

old_includes = '#include "entity.h"\n#include "yaml_simple.h"'
new_includes = '''#include <algorithm>
#include "entity.h"
#include "yaml_simple.h"

static bool vec_pair_has(const std::vector<std::pair<std::string,std::string>> &vec,
                         const char *s)
{
    return std::any_of(vec.begin(), vec.end(),
        [s](const auto &p){ return p.first == s || p.second == s; });
}'''
content = content.replace(old_includes, new_includes)

# ── Fix remaining strstr on vector<string> fields ─────────────────────────
# These are cases the previous regex missed (different whitespace etc.)

def fix_strstr_vec_string(content, field_expr):
    pattern = re.compile(
        r'EXPECT_NE\(strstr\(' + re.escape(field_expr) + r',\s*"([^"]+)"\),\s*nullptr\)'
    )
    return pattern.sub(
        lambda m: f'EXPECT_THAT({field_expr}, testing::Contains("{m.group(1)}"))',
        content
    )

def fix_strstr_vec_pair(content, field_expr):
    pattern = re.compile(
        r'EXPECT_NE\(strstr\(' + re.escape(field_expr) + r',\s*"([^"]+)"\),\s*nullptr\)'
    )
    return pattern.sub(
        lambda m: f'EXPECT_TRUE(vec_pair_has({field_expr}, "{m.group(1)}"))',
        content
    )

def fix_strstr_std_string(content, field_expr):
    pattern = re.compile(
        r'EXPECT_NE\(strstr\(' + re.escape(field_expr) + r',\s*"([^"]+)"\),\s*nullptr\)'
    )
    return pattern.sub(
        lambda m: f'EXPECT_NE({field_expr}.find("{m.group(1)}"), std::string::npos)',
        content
    )

# vector<string> fields
for f in [
    "e.tags.tags",
    "e.sources.sources",
    "e.doc_membership.doc_ids",
    "e.applies_to.applies_to",
    "e.test_procedure.preconditions",
    "e.composition_profile.order",
    "e.acceptance_criteria.criteria",
]:
    content = fix_strstr_vec_string(content, f)

# vector<pair> fields
for f in [
    "e.traceability.entries",
    "e.clause_collection.clauses",
    "e.attachment.attachments",
    "e.test_procedure.steps",
]:
    content = fix_strstr_vec_pair(content, f)

# std::string fields
for f in [
    "e.assumption.text",
    "e.constraint.text",
    "e.doc_body.body",
]:
    content = fix_strstr_std_string(content, f)

# ── Remove entity_free(&e) calls outside memory management section ─────────
# Simply remove entity_free lines
# (they are no-ops with RAII)
content = re.sub(r'\n\s*entity_free\(&e\);', '', content)
content = re.sub(r'\n\s*entity_free\(nullptr\);', '', content)

# ── Fix EntityHasComponentTest::DocBodyAbsentAndPresent ──────────────────
# Replace strdup usage for doc_body.body
content = content.replace(
    'e.doc_body.body = strdup("Some body text");\n    ASSERT_NE(e.doc_body.body, nullptr);',
    'e.doc_body.body = "Some body text";'
)
# Fix remaining ASSERT_NE(e.doc_body.body, nullptr)
content = content.replace('ASSERT_NE(e.doc_body.body, nullptr)', 'EXPECT_FALSE(e.doc_body.body.empty())')
# Fix EXPECT_TRUE on list fields that were somehow missed
content = content.replace('EXPECT_EQ(list.capacity(), 0u)', 'EXPECT_EQ(list.capacity(), 0u)')  # Already fixed

# ── Add list[0].doc_body.body = "..." STREQ fix ───────────────────────────
content = re.sub(
    r'EXPECT_STREQ\(list\[(\d+)\]\.doc_body\.body,\s*"([^"]+)"\)',
    r'EXPECT_EQ(list[\1].doc_body.body, "\2")',
    content
)
# list[0].doc_body.body != nullptr
content = content.replace(
    'EXPECT_NE(list[0].doc_body.body, nullptr)',
    'EXPECT_FALSE(list[0].doc_body.body.empty())'
)
content = content.replace(
    'EXPECT_NE(list[1].doc_body.body, nullptr)',
    'EXPECT_FALSE(list[1].doc_body.body.empty())'
)

# ── Fix list[1].test_procedure.* null checks ─────────────────────────────
content = content.replace(
    'EXPECT_NE(list[1].test_procedure.preconditions, nullptr)',
    'EXPECT_FALSE(list[1].test_procedure.preconditions.empty())'
)
content = content.replace(
    'EXPECT_NE(list[1].test_procedure.expected_result, nullptr)',
    'EXPECT_FALSE(list[1].test_procedure.expected_result.empty())'
)

# ── Remaining EXPECT_EQ(e.doc_body.body, nullptr) ─────────────────────────
content = content.replace(
    'EXPECT_EQ(e.doc_body.body, nullptr)',
    'EXPECT_TRUE(e.doc_body.body.empty())'
)

# ── Now replace the entire memory management section ─────────────────────

MEM_SECTION_START = "/* =========================================================================\n * Tests — entity_free / entity_copy / memory management"
MEM_SECTION_END = "/* =========================================================================\n * Tests — ENTITY_KIND_DOCUMENT_SCHEMA / entity_kind_from_string"

NEW_MEM_SECTION = '''/* =========================================================================
 * Tests — RAII / value-semantics / copy semantics
 * ======================================================================= */

TEST(EntityFreeTest, DefaultEntityHasEmptyFields)
{
    /* Default-constructed Entity must have empty string/vector fields. */
    Entity e;
    EXPECT_TRUE(e.doc_body.body.empty());
    EXPECT_TRUE(e.test_procedure.preconditions.empty());
    EXPECT_TRUE(e.test_procedure.steps.empty());
    EXPECT_TRUE(e.test_procedure.expected_result.empty());
    EXPECT_TRUE(e.clause_collection.clauses.empty());
    EXPECT_TRUE(e.attachment.attachments.empty());
}

TEST(EntityFreeTest, ZeroInitEntityHasEmptyFields)
{
    /* Entity{} zero-initialises all fields — same as default-constructed. */
    Entity e{};
    EXPECT_TRUE(e.doc_body.body.empty());
    EXPECT_TRUE(e.test_procedure.preconditions.empty());
    EXPECT_TRUE(e.test_procedure.steps.empty());
    EXPECT_TRUE(e.test_procedure.expected_result.empty());
    EXPECT_TRUE(e.clause_collection.clauses.empty());
    EXPECT_TRUE(e.attachment.attachments.empty());
}

TEST(EntityFreeTest, FieldsCanBeSetAndRead)
{
    /* Verify that all "heap" fields are now plain std::string / std::vector. */
    Entity e{};
    e.doc_body.body = "body content";
    e.test_procedure.expected_result = "expected";
    e.test_procedure.preconditions.push_back("precond one");
    e.clause_collection.clauses.push_back({"cl-1", "title one"});
    e.attachment.attachments.push_back({"file.pdf", "desc"});

    EXPECT_EQ(e.doc_body.body, "body content");
    EXPECT_EQ(e.test_procedure.expected_result, "expected");
    EXPECT_FALSE(e.test_procedure.preconditions.empty());
    EXPECT_FALSE(e.clause_collection.clauses.empty());
    EXPECT_FALSE(e.attachment.attachments.empty());
}

TEST(EntityFreeTest, OutOfScopeEntityCleansUpAutomatically)
{
    /* Destroying an Entity with populated fields must not crash (RAII). */
    {
        Entity e{};
        e.doc_body.body = "hello";
        e.test_procedure.steps.push_back({"act", "exp"});
    }
    /* If we reach here, no crash and no leak (checked by sanitisers). */
    SUCCEED();
}

TEST(EntityCopyTest, CopyZeroEntityIsOk)
{
    Entity src;
    src.identity.id = "REQ-CPY-001";

    Entity dst = src;
    EXPECT_EQ(dst.identity.id, "REQ-CPY-001");
    /* No heap fields were set — all containers are empty. */
    EXPECT_TRUE(dst.doc_body.body.empty());
    EXPECT_TRUE(dst.test_procedure.expected_result.empty());
    EXPECT_TRUE(dst.clause_collection.clauses.empty());
    EXPECT_TRUE(dst.attachment.attachments.empty());
}

TEST(EntityCopyTest, CopyDeepCopiesAllFields)
{
    /* Build a source entity with all fields populated. */
    Entity src;
    src.identity.id = "DN-CPY-001";
    src.doc_body.body = "body text";
    src.test_procedure.expected_result = "result";
    src.test_procedure.preconditions.push_back("precond one");
    src.clause_collection.clauses.push_back({"cl-1", "title one"});
    src.attachment.attachments.push_back({"file.pdf", "desc"});

    Entity dst = src;

    EXPECT_EQ(dst.identity.id, "DN-CPY-001");
    EXPECT_EQ(dst.doc_body.body, "body text");
    EXPECT_EQ(dst.test_procedure.expected_result, "result");
    EXPECT_EQ((int)dst.test_procedure.preconditions.size(), 1);
    EXPECT_EQ((int)dst.clause_collection.clauses.size(), 1);
    EXPECT_EQ((int)dst.attachment.attachments.size(), 1);

    /* dst and src must be independent value copies. */
    EXPECT_NE(&dst.doc_body.body, &src.doc_body.body);
}

TEST(EntityCopyTest, MutatingCopyDoesNotAffectSource)
{
    /* Verify true independence: modifying dst does not affect src. */
    Entity src;
    src.doc_body.body = "original";

    Entity dst = src;
    dst.doc_body.body = "modified";

    EXPECT_EQ(src.doc_body.body, "original");
    EXPECT_EQ(dst.doc_body.body, "modified");
}

TEST(EntityListAddTest, ListAddCopiesEntity)
{
    /* std::vector push_back copies the entity. The source can go out of
     * scope without affecting the list item. */
    EntityList list;

    {
        Entity src;
        src.identity.id = "DN-LIST-001";
        src.doc_body.body = "list body";
        list.push_back(src);
        EXPECT_EQ((int)list.size(), 1);
    } /* src is destroyed here */

    /* List item must still have its own copy. */
    EXPECT_FALSE(list[0].doc_body.body.empty());
    EXPECT_EQ(list[0].doc_body.body, "list body");
}

TEST(EntityListFreeTest, FreeReleasesAllEntityHeapFields)
{
    /* Build a list from yaml_parse_entities, then destroy it via RAII. */
    const char *yaml =
        "id: DN-MEM-001\n"
        "title: Design note for memory test\n"
        "type: design-note\n"
        "body: heap allocated body text\n"
        "---\n"
        "id: TC-MEM-001\n"
        "title: Test case for memory test\n"
        "type: test-case\n"
        "preconditions:\n"
        "  - System is ready.\n"
        "expected-result: Test passes.\n";

    FILE *f = fopen("/tmp/mem_test.yaml", "w");
    ASSERT_NE(f, nullptr);
    fputs(yaml, f);
    fclose(f);

    int added;
    {
        EntityList list;
        added = yaml_parse_entities("/tmp/mem_test.yaml", &list);
        EXPECT_EQ(added, 2);
        EXPECT_EQ((int)list.size(), 2);

        /* Verify fields were set. */
        EXPECT_FALSE(list[0].doc_body.body.empty());
        EXPECT_FALSE(list[1].test_procedure.preconditions.empty());
        EXPECT_FALSE(list[1].test_procedure.expected_result.empty());
    } /* list destroyed here — RAII frees everything */
    EXPECT_EQ(added, 2);   /* sanity: added is still valid */
}

TEST(EntityMemoryTest, ParseEntityBodyIsStdString)
{
    /* Regression: doc_body.body is a std::string, no manual free needed. */
    const char *path = write_yaml("mem_parse_body.yaml",
        "id: DN-HEAP-001\n"
        "title: Heap body\n"
        "type: design-note\n"
        "body: some content\n");
    ASSERT_NE(path, nullptr);

    Entity e;
    int rc = yaml_parse_entity(path, &e);
    EXPECT_EQ(rc, 0);
    EXPECT_FALSE(e.doc_body.body.empty());
    EXPECT_EQ(e.doc_body.body, "some content");
    /* e is destroyed here — no manual cleanup needed */
}

TEST(EntityMemoryTest, ParsedEntityFieldsAreIndependentInVector)
{
    /* Parsing the same file twice and pushing to a vector must give two
     * independent copies. */
    const char *path = write_yaml("mem_two_copies.yaml",
        "id: REQ-DUP-001\n"
        "title: Duplicate test\n"
        "type: design-note\n"
        "body: shared body\n");
    ASSERT_NE(path, nullptr);

    EntityList list;

    Entity e1, e2;
    ASSERT_EQ(yaml_parse_entity(path, &e1), 0);
    ASSERT_EQ(yaml_parse_entity(path, &e2), 0);
    list.push_back(e1);
    list.push_back(e2);

    EXPECT_EQ((int)list.size(), 2);
    /* Both items have independent string values. */
    EXPECT_FALSE(list[0].doc_body.body.empty());
    EXPECT_FALSE(list[1].doc_body.body.empty());
    EXPECT_NE(&list[0].doc_body.body, &list[1].doc_body.body);
    EXPECT_EQ(list[0].doc_body.body, "shared body");
    EXPECT_EQ(list[1].doc_body.body, "shared body");
}

/* =========================================================================
 * Tests — ENTITY_KIND_DOCUMENT_SCHEMA / entity_kind_from_string'''

# Find and replace the memory section
start_idx = content.find(MEM_SECTION_START)
end_idx = content.find(MEM_SECTION_END)
if start_idx == -1 or end_idx == -1:
    print(f"ERROR: Could not find section markers! start={start_idx}, end={end_idx}")
    import sys
    sys.exit(1)

content = content[:start_idx] + NEW_MEM_SECTION + "\n * ======================================================================= */\n\n" + content[end_idx + len(MEM_SECTION_END) + len("\n * ======================================================================= */\n\n"):]

# ── Fix the EXPECT_EQ(list.capacity(), 0u) that may still be in wrong context
# Actually we need to keep it since it's referring to std::vector capacity,
# which is valid. But wait line 131 says:
# EXPECT_EQ(list.capacity(), 0u) — for a freshly created vector this should be 0
# In modern C++, vector may not have capacity 0 after creation -- let's just remove that line.

# Actually for InitAndFree test:
content = content.replace(
    '    EXPECT_EQ(list.capacity(), 0u)\n',
    ''
)
content = content.replace(
    '    EXPECT_EQ(list.capacity(), 0u);\n',
    ''
)

# ── Fix remaining EntityListFreeTest issues ───────────────────────────────
# The old test had:
#     EXPECT_EQ(list.items,    nullptr);
#     EXPECT_EQ((int)list.size(),    0);
#     EXPECT_EQ(list.capacity, 0);
# which were after the vector is still alive. Let me remove those.

# ── Fix memset on Entity in EntityCmpByIdTest ─────────────────────────────
# memset(&a, 0, sizeof(a)); + memset(&b, 0, sizeof(b))
content = content.replace(
    '    Entity a, b;\n    memset(&a, 0, sizeof(a));\n    memset(&b, 0, sizeof(b));',
    '    Entity a, b;'
)
# strncpy patterns we may have missed
content = re.sub(
    r'strncpy\(([ab])\.identity\.id,\s*"([^"]+)",\s*sizeof\(\1\.identity\.id\)\s*-\s*1\);',
    r'\1.identity.id = "\2";',
    content
)

print("Done", file=__import__('sys').stderr)

with open("tests/test_entity.cpp", "w") as f:
    f.write(content)
