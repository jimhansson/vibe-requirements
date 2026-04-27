import re
import sys

with open("tests/test_entity.cpp", "r") as f:
    content = f.read()

# ── helpers ────────────────────────────────────────────────────────────────

def replace_all(text, old, new):
    return text.replace(old, new)

# ── Pattern 1: e.(int)XXX.size() → (int)e.XXX.<container>.size() ──────────

FIELD_MAP = {
    "acceptance_criteria": "criteria",
    "traceability":        "entries",
    "tags":                "tags",
    "sources":             "sources",
    "doc_membership":      "doc_ids",
    "applies_to":          "applies_to",
    "clause_collection":   "clauses",
    "attachment":          "attachments",
}

for field, container in FIELD_MAP.items():
    # e.(int)field.size() → (int)e.field.container.size()
    content = content.replace(
        f"e.(int){field}.size()",
        f"(int)e.{field}.{container}.size()"
    )

# src.(int)field.size() patterns in EntityCopyTest
for field, container in FIELD_MAP.items():
    content = content.replace(
        f"src.(int){field}.size()",
        f"(int)src.{field}.{container}.size()"
    )
    content = content.replace(
        f"dst.(int){field}.size()",
        f"(int)dst.{field}.{container}.size()"
    )

# test_procedure counts
content = content.replace("e.test_procedure.precondition_count", "(int)e.test_procedure.preconditions.size()")
content = content.replace("e.test_procedure.step_count",         "(int)e.test_procedure.steps.size()")
content = content.replace("e.composition_profile.order_count",   "(int)e.composition_profile.order.size()")
content = content.replace("src.test_procedure.precondition_count", "(int)src.test_procedure.preconditions.size()")
content = content.replace("dst.test_procedure.precondition_count", "(int)dst.test_procedure.preconditions.size()")

# ── Pattern 2: strstr on vector<string> fields → EXPECT_THAT / find ───────

VEC_STRING_FIELDS = [
    ("e.acceptance_criteria.criteria", "acceptance_criteria.criteria"),
    ("e.tags.tags",                    "tags.tags"),
    ("e.doc_membership.doc_ids",       "doc_membership.doc_ids"),
    ("e.applies_to.applies_to",        "applies_to.applies_to"),
    ("e.sources.sources",              "sources.sources"),
    ("e.composition_profile.order",    "composition_profile.order"),
    ("e.test_procedure.preconditions", "test_procedure.preconditions"),
]
VEC_PAIR_FIELDS = [
    ("e.traceability.entries",        "traceability.entries"),
    ("e.clause_collection.clauses",   "clause_collection.clauses"),
    ("e.attachment.attachments",      "attachment.attachments"),
    ("e.test_procedure.steps",        "test_procedure.steps"),
]
STD_STRING_FIELDS = [
    ("e.assumption.text",             "assumption.text"),
    ("e.constraint.text",             "constraint.text"),
    ("e.doc_body.body",               "doc_body.body"),
    ("e.test_procedure.expected_result", "test_procedure.expected_result"),
]

def make_strstr_pattern(field_expr, string_val):
    return f'strstr({field_expr}, "{string_val}")'

# Replace EXPECT_NE(strstr(..., "val"), nullptr) for vector<string>
for full_field, _ in VEC_STRING_FIELDS:
    # EXPECT_NE(strstr(field, "val"), nullptr)
    pattern = re.compile(
        r'EXPECT_NE\(strstr\(' + re.escape(full_field) + r', "([^"]+)"\), nullptr\)'
    )
    content = pattern.sub(
        lambda m: f'EXPECT_THAT({full_field}, testing::Contains("{m.group(1)}"))',
        content
    )

# Replace EXPECT_NE(strstr(..., "val"), nullptr) for vector<pair>
for full_field, _ in VEC_PAIR_FIELDS:
    pattern = re.compile(
        r'EXPECT_NE\(strstr\(' + re.escape(full_field) + r', "([^"]+)"\), nullptr\)'
    )
    content = pattern.sub(
        lambda m: f'EXPECT_TRUE(vec_pair_has({full_field}, "{m.group(1)}"))',
        content
    )

# Replace EXPECT_NE(strstr(e.assumption.text, "val"), nullptr)
#         EXPECT_NE(strstr(e.constraint.text, "val"), nullptr)
#         EXPECT_NE(strstr(e.doc_body.body, "val"), nullptr)
for full_field, _ in STD_STRING_FIELDS:
    pattern = re.compile(
        r'EXPECT_NE\(strstr\(' + re.escape(full_field) + r', "([^"]+)"\), nullptr\)'
    )
    content = pattern.sub(
        lambda m: f'EXPECT_NE({full_field}.find("{m.group(1)}"), std::string::npos)',
        content
    )

# ── Pattern 3: field[0] == '\0' / field[0] != '\0' → .empty() ─────────────

# These fields that are std::string now
ZERO_CHAR_FIELDS = [
    "e.assumption.text",
    "e.constraint.text",
    "e.doc_meta.doc_type",
    "e.doc_meta.client",
    "e.doc_membership.doc_ids",  # This is a vector, so [0] would be first element
    "e.traceability.entries",
    "e.sources.sources",
    "e.variant_profile.customer",
    "e.variant_profile.product",
    "e.render_profile.format",
    "e.composition_profile.order",
    "e.applies_to.applies_to",
]

for f in ZERO_CHAR_FIELDS:
    # EXPECT_EQ(field[0], '\0') → EXPECT_TRUE(field.empty())
    content = content.replace(f"EXPECT_EQ({f}[0], '\\0')", f"EXPECT_TRUE({f}.empty())")
    # EXPECT_NE(field[0], '\0') → EXPECT_FALSE(field.empty())
    content = content.replace(f"EXPECT_NE({f}[0], '\\0')", f"EXPECT_FALSE({f}.empty())")

# doc_membership.doc_ids is vector<string>; [0] check means "is empty"
# (already handled if listed above)

# ── Pattern 4: EXPECT_STREQ for std::string fields → EXPECT_EQ ─────────────

STREQ_STRING_FIELDS = [
    "e.epic_membership.epic_id",
    "e.variant_profile.customer",
    "e.variant_profile.product",
    "e.render_profile.format",
    "e.doc_body.body",
    "e.test_procedure.expected_result",
    "dst.doc_body.body",
    "dst.test_procedure.expected_result",
    "src.doc_body.body",
    "list[0].doc_body.body",
    "list[1].doc_body.body",
]

for f in STREQ_STRING_FIELDS:
    pattern = re.compile(r'EXPECT_STREQ\(' + re.escape(f) + r', "([^"]+)"\)')
    content = pattern.sub(lambda m: f'EXPECT_EQ({f}, "{m.group(1)}")', content)

# Generic EXPECT_STREQ(field, "value") → EXPECT_EQ(field, "value")
# for any field starting with e. that we know is now std::string
# Let's do the remaining ones that weren't caught above
# ... the remaining ones are done case by case in the memory section rewrite

# ── Pattern 5: strncpy on std::string fields → direct assignment ─────────

# strncpy(e.identity.id, "VAL", sizeof(e.identity.id) - 1)
content = re.sub(
    r'strncpy\(e\.identity\.id,\s*"([^"]+)",\s*sizeof\(e\.identity\.id\)\s*-\s*1\);',
    r'e.identity.id = "\1";',
    content
)
content = re.sub(
    r'strncpy\(e\.identity\.title,\s*"([^"]+)",\s*sizeof\(e\.identity\.title\)\s*-\s*1\);',
    r'e.identity.title = "\1";',
    content
)
# a.identity.id, b.identity.id
content = re.sub(
    r'strncpy\(a\.identity\.id,\s*"([^"]+)",\s*sizeof\(a\.identity\.id\)\s*-\s*1\);',
    r'a.identity.id = "\1";',
    content
)
content = re.sub(
    r'strncpy\(b\.identity\.id,\s*"([^"]+)",\s*sizeof\(b\.identity\.id\)\s*-\s*1\);',
    r'b.identity.id = "\1";',
    content
)
# e.user_story.role
content = re.sub(
    r'strncpy\(e\.user_story\.role,\s*"([^"]+)",\s*sizeof\(e\.user_story\.role\)\s*-\s*1\);',
    r'e.user_story.role = "\1";',
    content
)
# e.epic_membership.epic_id
content = re.sub(
    r'strncpy\(e\.epic_membership\.epic_id,\s*"([^"]+)",\s*sizeof\(e\.epic_membership\.epic_id\)\s*-\s*1\);',
    r'e.epic_membership.epic_id = "\1";',
    content
)
# e.assumption.text
content = re.sub(
    r'strncpy\(e\.assumption\.text,\s*"([^"]+)",\s*sizeof\(e\.assumption\.text\)\s*-\s*1\);',
    r'e.assumption.text = "\1";',
    content
)
# e.constraint.text
content = re.sub(
    r'strncpy\(e\.constraint\.text,\s*"([^"]+)",\s*sizeof\(e\.constraint\.text\)\s*-\s*1\);',
    r'e.constraint.text = "\1";',
    content
)
# e.doc_meta.doc_type
content = re.sub(
    r'strncpy\(e\.doc_meta\.doc_type,\s*"([^"]+)",\s*sizeof\(e\.doc_meta\.doc_type\)\s*-\s*1\);',
    r'e.doc_meta.doc_type = "\1";',
    content
)
# e.variant_profile.customer
content = re.sub(
    r'strncpy\(e\.variant_profile\.customer,\s*"([^"]+)",\s*sizeof\(e\.variant_profile\.customer\)\s*-\s*1\);',
    r'e.variant_profile.customer = "\1";',
    content
)
# e.render_profile.format
content = re.sub(
    r'strncpy\(e\.render_profile\.format,\s*"([^"]+)",\s*sizeof\(e\.render_profile\.format\)\s*-\s*1\);',
    r'e.render_profile.format = "\1";',
    content
)
# src.identity.id
content = re.sub(
    r'strncpy\(src\.identity\.id,\s*"([^"]+)",\s*sizeof\(src\.identity\.id\)\s*-\s*1\);',
    r'src.identity.id = "\1";',
    content
)
# src.identity.id  without  -1 variant
content = re.sub(
    r'strncpy\(src\.identity\.id,\s*"([^"]+)",\s*sizeof\(src\.identity\.id\)\);',
    r'src.identity.id = "\1";',
    content
)

# snprintf(e.identity.id, sizeof(e.identity.id), "ENT-%03d", i)
content = re.sub(
    r'snprintf\(e\.identity\.id,\s*sizeof\(e\.identity\.id\),\s*"ENT-%03d",\s*i\);',
    r'{ char _buf[32]; snprintf(_buf, sizeof(_buf), "ENT-%03d", i); e.identity.id = _buf; }',
    content
)

# e.identity.id = "ENT-xxx" on EntityCmpByIdTest
content = re.sub(
    r'strncpy\(e\.identity\.id,\s*"(REQ-\d+)",\s*sizeof\(e\.identity\.id\)\s*-\s*1\);',
    r'e.identity.id = "\1";',
    content
)

# EntityMembership strncpy
content = re.sub(
    r'strncpy\(e\.identity\.id,\s*"([^"]+)",\s*sizeof\(e\.identity\.id\)\s*-\s*1\);',
    r'e.identity.id = "\1";',
    content
)

# ── Pattern 7: EntityListTest fixes ───────────────────────────────────────

# list.capacity → list.capacity()
content = content.replace("EXPECT_EQ(list.capacity, 0)", "EXPECT_EQ(list.capacity(), 0u)")
# list.items → list.empty()
content = content.replace("EXPECT_EQ(list.items,    nullptr)", "EXPECT_TRUE(list.empty())")
# push_back returning 0
content = content.replace("EXPECT_EQ(list.push_back(e), 0)", "list.push_back(e); EXPECT_EQ(1,1) /* push_back is void */")
# Actually just replace with bare push_back
content = re.sub(
    r'EXPECT_EQ\(list\.push_back\((\w+)\),\s*0\);',
    r'list.push_back(\1);',
    content
)

# ── Pattern 8: doc_body null checks → .empty() ────────────────────────────

# ASSERT_NE(e.doc_body.body, nullptr) → EXPECT_FALSE(e.doc_body.body.empty())
content = content.replace("ASSERT_NE(e.doc_body.body,     nullptr)", "EXPECT_FALSE(e.doc_body.body.empty())")
content = content.replace("ASSERT_NE(e.doc_body.body, nullptr)", "EXPECT_FALSE(e.doc_body.body.empty())")
# EXPECT_EQ(e.doc_body.body, nullptr) → EXPECT_TRUE(e.doc_body.body.empty())
content = content.replace("EXPECT_EQ(e.doc_body.body, nullptr)", "EXPECT_TRUE(e.doc_body.body.empty())")
# strlen(e.doc_body.body) → e.doc_body.body.size()
content = content.replace("(int)strlen(e.doc_body.body)", "(int)e.doc_body.body.size()")
content = content.replace("strlen(e.doc_body.body)", "e.doc_body.body.size()")
# EXPECT_STREQ(e.doc_body.body, "xxx") → EXPECT_EQ(e.doc_body.body, "xxx")
content = re.sub(
    r'EXPECT_STREQ\(e\.doc_body\.body,\s+"([^"]+)"\)',
    r'EXPECT_EQ(e.doc_body.body, "\1")',
    content
)

# ── Pattern 9: test_procedure null checks ─────────────────────────────────

content = content.replace("ASSERT_NE(e.test_procedure.preconditions,       nullptr)", "EXPECT_FALSE(e.test_procedure.preconditions.empty())")
content = content.replace("ASSERT_NE(e.test_procedure.preconditions,      nullptr)", "EXPECT_FALSE(e.test_procedure.preconditions.empty())")
content = content.replace("ASSERT_NE(e.test_procedure.preconditions, nullptr)", "EXPECT_FALSE(e.test_procedure.preconditions.empty())")
content = content.replace("ASSERT_NE(e.test_procedure.steps,               nullptr)", "EXPECT_FALSE(e.test_procedure.steps.empty())")
content = content.replace("ASSERT_NE(e.test_procedure.steps,              nullptr)", "EXPECT_FALSE(e.test_procedure.steps.empty())")
content = content.replace("ASSERT_NE(e.test_procedure.steps, nullptr)", "EXPECT_FALSE(e.test_procedure.steps.empty())")
content = content.replace("ASSERT_NE(e.test_procedure.expected_result,     nullptr)", "EXPECT_FALSE(e.test_procedure.expected_result.empty())")
content = content.replace("ASSERT_NE(e.test_procedure.expected_result, nullptr)", "EXPECT_FALSE(e.test_procedure.expected_result.empty())")
content = content.replace("EXPECT_EQ(e.test_procedure.preconditions,        nullptr)", "EXPECT_TRUE(e.test_procedure.preconditions.empty())")
content = content.replace("EXPECT_EQ(e.test_procedure.preconditions,  nullptr)", "EXPECT_TRUE(e.test_procedure.preconditions.empty())")
content = content.replace("EXPECT_EQ(e.test_procedure.preconditions, nullptr)", "EXPECT_TRUE(e.test_procedure.preconditions.empty())")
content = content.replace("EXPECT_EQ(e.test_procedure.steps,                nullptr)", "EXPECT_TRUE(e.test_procedure.steps.empty())")
content = content.replace("EXPECT_EQ(e.test_procedure.steps,         nullptr)", "EXPECT_TRUE(e.test_procedure.steps.empty())")
content = content.replace("EXPECT_EQ(e.test_procedure.steps, nullptr)", "EXPECT_TRUE(e.test_procedure.steps.empty())")
content = content.replace("EXPECT_EQ(e.test_procedure.expected_result,      nullptr)", "EXPECT_TRUE(e.test_procedure.expected_result.empty())")
content = content.replace("EXPECT_EQ(e.test_procedure.expected_result, nullptr)", "EXPECT_TRUE(e.test_procedure.expected_result.empty())")
content = re.sub(
    r'EXPECT_STREQ\(e\.test_procedure\.expected_result,\s*"([^"]+)"\)',
    r'EXPECT_EQ(e.test_procedure.expected_result, "\1")',
    content
)
content = re.sub(
    r'EXPECT_STREQ\(e\.test_procedure\.expected_result,\s*\n\s*"([^"]+)"\)',
    r'EXPECT_EQ(e.test_procedure.expected_result, "\1")',
    content
)

# ── Pattern 10: clause_collection null checks ─────────────────────────────

content = content.replace("ASSERT_NE(e.clause_collection.clauses, nullptr)", "EXPECT_FALSE(e.clause_collection.clauses.empty())")
content = content.replace("EXPECT_EQ(e.clause_collection.clauses, nullptr)", "EXPECT_TRUE(e.clause_collection.clauses.empty())")

# ── Pattern: attachment null checks ──────────────────────────────────────

content = content.replace("ASSERT_NE(e.attachment.attachments, nullptr)", "EXPECT_FALSE(e.attachment.attachments.empty())")
content = content.replace("EXPECT_EQ(e.attachment.attachments, nullptr)", "EXPECT_TRUE(e.attachment.attachments.empty())")

# ── Pattern 11: CTripleList size → count ─────────────────────────────────

# ASSERT_EQ((int)list.size(), Xu)  where list is CTripleList
# We need to be careful here — EntityList.size() should remain as .size()
# We'll replace patterns around CTripleList variables explicitly:
# "by_obj1.size()", "by_obj2.size()", "by_doc.size()", "list.size()" in those contexts

# In the triplet contexts, the variable is "list", "by_obj1", "by_obj2", "by_doc"
# Let's replace them:
content = re.sub(
    r'ASSERT_EQ\(\(int\)list\.size\(\),\s*(\d+)u\)',
    r'ASSERT_EQ((int)list.count, \1u)',
    content
)
content = re.sub(
    r'EXPECT_GE\(\(int\)by_obj1\.size\(\),\s*(\d+)u\)',
    r'EXPECT_GE((int)by_obj1.count, \1u)',
    content
)
content = re.sub(
    r'EXPECT_GE\(\(int\)by_obj2\.size\(\),\s*(\d+)u\)',
    r'EXPECT_GE((int)by_obj2.count, \1u)',
    content
)
# for (size_t i = 0; i < (int)list.size(); i++) → for (size_t i = 0; i < list.count; i++)
content = re.sub(
    r'for \(size_t i = 0; i < \(int\)list\.size\(\); i\+\+\)',
    r'for (size_t i = 0; i < list.count; i++)',
    content
)
content = re.sub(
    r'for \(size_t i = 0; i < \(int\)by_doc\.size\(\); i\+\+\)',
    r'for (size_t i = 0; i < by_doc.count; i++)',
    content
)

# ── Pattern: entity_has_component for "acceptance_criteria" count setup ───

# e.(int)acceptance_criteria.size() = 1 → e.acceptance_criteria.criteria.push_back("test")
# These are already handled by pattern 1 changing to (int)e.acceptance_criteria.criteria.size()
# but then we have assignment. Let's look for the assignment patterns:
content = content.replace(
    "(int)e.acceptance_criteria.criteria.size() = 1",
    'e.acceptance_criteria.criteria.push_back("test")'
)
content = content.replace(
    "(int)e.traceability.entries.size() = 1",
    'e.traceability.entries.push_back({"REQ-001", "derived-from"})'
)
content = content.replace(
    "(int)e.sources.sources.size() = 1",
    'e.sources.sources.push_back("src")'
)
content = content.replace(
    "(int)e.tags.tags.size() = 2",
    'e.tags.tags = {"tag1", "tag2"}'
)
content = content.replace(
    "(int)e.doc_membership.doc_ids.size() = 1",
    'e.doc_membership.doc_ids.push_back("DOC-001")'
)
content = content.replace(
    "(int)e.clause_collection.clauses.size() = 2",
    'e.clause_collection.clauses = {{"id1","title1"}, {"id2","title2"}}'
)
content = content.replace(
    "(int)e.attachment.attachments.size() = 1",
    'e.attachment.attachments.push_back({"path", "desc"})'
)
content = content.replace(
    "(int)e.composition_profile.order.size() = 2",
    'e.composition_profile.order = {"SEC-1", "SEC-2"}'
)
content = content.replace(
    "(int)e.applies_to.applies_to.size() = 1",
    'e.applies_to.applies_to.push_back("target")'
)
# test_procedure step_count = 1
content = content.replace(
    "(int)e.test_procedure.steps.size() = 1",
    'e.test_procedure.steps.push_back({"action", "expected"})'
)

# ── qsort(list.items, ...) → qsort(list.data(), ...) ──────────────────────

content = content.replace(
    "qsort(list.items, (size_t)(int)list.size(), sizeof(Entity), entity_cmp_by_id)",
    "qsort(list.data(), (size_t)list.size(), sizeof(Entity), entity_cmp_by_id)"
)

# ── memset on Entity → Entity e{} ──────────────────────────────────────────
# memset(&a, 0, sizeof(a)); followed by strncpy → just Entity a{};
# We'll handle the specific cases

# ── entity_free → no-op comments (to be replaced manually in mem section) ─
# We'll handle in the memory section rewrite

print("Done with mechanical transformations", file=sys.stderr)

with open("tests/test_entity.cpp", "w") as f:
    f.write(content)

print("Written", file=sys.stderr)
