# AI Integration — Research and Recommendations

## 1. Introduction

This document explores how **vibe-requirements** should support and interact with AI systems. Three concrete questions were raised in the original issue, and a fourth catch-all for anything else:

1. Should the tool expose a **Model Context Protocol (MCP) server** so that AI coding assistants and agents can use it as a tool?
2. Should the **GUI** include AI-assisted functions to help users write well-formed requirement text?
3. Should the project ship **ready-to-use agent skills / tool definitions** so that LLM agents can drive the tool without custom integration work?
4. Any other AI-related considerations?

---

## 2. Model Context Protocol (MCP) Server

### 2.1 What Is MCP?

The [Model Context Protocol](https://modelcontextprotocol.io/) is an open standard (initially released by Anthropic in late 2024) that lets LLM hosts — editors like VS Code with GitHub Copilot, Claude Desktop, and custom agent runtimes — discover and call structured tools exposed by a server. An MCP server advertises a set of named tools with typed input/output schemas; the LLM can then invoke them, read back the results, and reason over them without the user having to write any glue code.

### 2.2 Why MCP Is a Good Fit

- **vibe-requirements is already tool-shaped.** Every CLI command (`validate`, `trace`, `status`, `report`, `new`, …) is a discrete, well-scoped operation with clear inputs and outputs. Wrapping these as MCP tools requires little design work — the mapping is almost one-to-one.
- **Engineers already work inside AI-assisted editors.** Exposing the requirement graph to the editor's AI assistant lets developers ask natural-language questions ("Which requirements are unverified?", "What requirements does this function implement?") and get structured answers without leaving their editor.
- **No lock-in.** MCP is transport-agnostic (stdio or HTTP/SSE). The CLI binary can serve as its own MCP server with a `--mcp` flag, keeping the single-binary principle intact.

### 2.3 Suggested MCP Tool Definitions

| Tool name | Description | Key inputs | Key output |
|---|---|---|---|
| `list_requirements` | Return all requirements, optionally filtered by status, type, priority, or tag | `status?`, `type?`, `priority?`, `tags?` | Array of requirement summaries |
| `get_requirement` | Return the full content of one requirement | `id` | Requirement object |
| `trace_requirement` | Return the full traceability chain for a requirement | `id`, `direction?` (upstream/downstream/both) | Link graph |
| `validate` | Validate all (or selected) requirement files and return errors/warnings | `path?` | Validation report |
| `status_summary` | Return counts by status and priority | — | Status object |
| `create_requirement` | Create a new requirement file from structured input | Requirement fields | Created file path and ID |
| `search_requirements` | Full-text search over requirement fields | `query` | Matching requirement list |
| `get_coverage` | Return which requirements lack test cases or verification links | — | Coverage report |

### 2.4 Implementation Approach

The simplest path is to add a `vibe-req mcp` sub-command that starts an MCP server using stdio transport. This lets any MCP-compatible host (VS Code, Claude Desktop, etc.) spawn the binary as a subprocess and call its tools.

**Suggested implementation steps:**
1. Define the MCP tool schemas in a JSON file (or embedded in the binary) following the MCP spec.
2. Add a `mcp` sub-command that reads JSON-RPC messages from stdin and writes responses to stdout.
3. Internally, each tool call dispatches to the same core library functions used by the CLI.
4. Ship an example `mcp.json` configuration snippet (or `claude_desktop_config.json` entry) in the repository so users can enable the server with one copy-paste.

### 2.5 Suggested Requirements

| ID (suggested) | Requirement | Priority |
|---|---|---|
| REQ-090 | The tool shall provide a `mcp` sub-command that starts a Model Context Protocol server using the stdio transport | Should |
| REQ-091 | The MCP server shall advertise at minimum the tools: `list_requirements`, `get_requirement`, `trace_requirement`, `validate`, and `status_summary` | Should |
| REQ-092 | The MCP server shall accept and return JSON following the MCP specification | Must (if REQ-090 is implemented) |
| REQ-093 | The MCP server shall be included in the same single binary as the CLI with no additional runtime dependencies | Should |
| REQ-094 | The repository shall include example MCP host configuration files (e.g., for VS Code and Claude Desktop) | Should |

---

## 3. AI-Assisted Requirement Authoring in the GUI

### 3.1 The Problem

Writing good requirements is hard. Typical failure modes include:
- Ambiguous language ("The system should be fast").
- Missing rationale or verification method.
- Compound requirements that mix multiple conditions in one sentence.
- Inconsistent level of detail across the requirement set.

These are exactly the class of problem where a large language model can provide immediate, useful feedback.

### 3.2 Proposed GUI Features

#### 3.2.1 Inline AI Polish

A button or keyboard shortcut in the inline editor that sends the current `description` field to a configured LLM and suggests a rewritten version that:
- Uses shall/should/may correctly (ISO/IEC 29148 style).
- Splits compound requirements into separate bullets or fields.
- Removes ambiguous terms (e.g., "fast", "easy", "appropriate").
- Flags missing fields (rationale, verification method).

The user reviews the suggestion and accepts, rejects, or edits it before saving.

#### 3.2.2 Field Completion Suggestions

When the user starts typing in any free-text field, the GUI can suggest completions based on:
- The other fields already filled in for that requirement.
- Similar requirements already in the project.
- General requirement-writing best practices encoded in a system prompt.

#### 3.2.3 AI-Generated First Draft

Given a short natural-language description typed by the user ("The user must be able to log in with a username and password"), the GUI populates a full requirement skeleton: title, description (shall-form), rationale, suggested type/status/priority, and suggested tags — ready for the user to review and adjust.

#### 3.2.4 Consistency Checker

A panel that runs an AI-powered consistency check across the entire requirement set and flags:
- Requirements that contradict each other.
- Gaps in coverage relative to a selected standard or external source.
- Orphaned requirements (no test case, no implementation link).

#### 3.2.5 Design Considerations

- **Privacy first:** AI features must be opt-in and clearly labeled. The tool must never send requirement text to an external service without explicit user consent.
- **Configurable endpoint:** The AI backend should be configurable — the user can point it at a local model (Ollama, LM Studio), a self-hosted API, or a cloud provider. The tool must not hard-code any single provider.
- **Offline mode:** All non-AI features must work identically with no network access. AI features degrade gracefully when the configured endpoint is unavailable.
- **No mandatory dependency:** The AI layer should be an optional feature; the binary should build and run fully without it.

### 3.3 Suggested Requirements

| ID (suggested) | Requirement | Priority |
|---|---|---|
| REQ-100 | The GUI shall provide an opt-in AI-assisted authoring panel that can suggest improvements to requirement text | Could |
| REQ-101 | The AI authoring feature shall accept a configurable endpoint URL and API key so that users can point it at a local or cloud-hosted LLM | Should (if REQ-100 is implemented) |
| REQ-102 | The AI authoring feature shall never transmit requirement data to an external service without explicit user consent and configuration | Must (if REQ-100 is implemented) |
| REQ-103 | The GUI shall provide a "generate first draft" function that converts a short natural-language description into a structured requirement skeleton | Could |
| REQ-104 | The GUI shall provide an AI-powered consistency checker that flags contradictory or ambiguous requirements across the project | Could |
| REQ-105 | All GUI features that do not involve AI shall operate identically when no AI endpoint is configured | Must |

---

## 4. Ready-to-Use Agent Skills

### 4.1 What Are Agent Skills?

LLM-based coding agents (GitHub Copilot Workspace, Claude Code, GPT-4o with tools, custom AutoGen/LangChain agents) can invoke external tools via function-calling or tool-use APIs. A **skill** or **tool definition** is the structured metadata that tells the agent what the tool does, what parameters it accepts, and what it returns.

If vibe-requirements ships ready-to-use skill definitions, any agent can use the tool without the user having to write integration code.

### 4.2 Formats to Ship

| Format | Target audience | File to ship |
|---|---|---|
| **MCP tool definitions** | Any MCP-compatible host (VS Code Copilot, Claude Desktop) | Covered by Section 2; the `mcp` sub-command advertises these automatically |
| **OpenAPI / tool JSON** | OpenAI function-calling, custom agents | `skills/openapi.json` |
| **GitHub Copilot extension manifest** | VS Code users with GitHub Copilot | `skills/copilot-extension.json` |
| **LangChain / LlamaIndex tool wrappers** | Python-based agent frameworks | `skills/langchain_tool.py` (thin wrapper around CLI) |
| **System prompt fragment** | Any agent; describes the tool in prose for agents that use free-form system prompts | `skills/AGENT_SYSTEM_PROMPT.md` |

### 4.3 What the Skills Should Enable

An agent equipped with these skills should be able to, without any user-written glue code:

- Discover all requirements in the current project.
- Create new requirements from a description.
- Validate the requirement set and report errors.
- Trace a requirement forward (to tests) or backward (to sources).
- Search for requirements related to a code change.
- Generate a requirements report.
- Check which requirements are unverified or unimplemented.

### 4.4 System Prompt Fragment (Example)

The following prose description can be included in an agent's system prompt to give it a mental model of the tool even before it calls any MCP tool:

```
vibe-requirements is a "requirements as code" tool. Requirements, user stories,
test cases, and external normative sources are stored as YAML files inside the
project repository. The tool provides a CLI (vibe-req) and an optional GUI.

Key concepts:
- Every requirement has a unique ID (e.g., REQ-SW-001), a type, a status, a
  priority, and may be linked to other requirements, test cases, code artefacts,
  and external standards.
- The `validate` command checks all files for schema errors and broken links.
- The `trace` command shows the full traceability chain for a given requirement.
- The `status` command summarises counts by status and priority.
- The `report` command generates a Markdown or HTML report.

When helping the user with requirements, prefer creating structured YAML files
via `vibe-req new` rather than writing raw YAML manually. Always validate after
creating or modifying files.
```

### 4.5 Suggested Requirements

| ID (suggested) | Requirement | Priority |
|---|---|---|
| REQ-110 | The repository shall ship OpenAPI-compatible tool definitions for all major CLI commands in a `skills/` directory | Should |
| REQ-111 | The repository shall ship a ready-to-use system prompt fragment (`skills/AGENT_SYSTEM_PROMPT.md`) describing the tool for use in LLM agent configurations | Should |
| REQ-112 | The repository shall ship a thin LangChain-compatible Python tool wrapper in `skills/langchain_tool.py` | Could |
| REQ-113 | Tool definitions and the system prompt fragment shall be kept in sync with the CLI interface as part of the release process | Should |

---

## 5. Other AI-Related Considerations

### 5.1 AI-Assisted Traceability Linking

When a developer commits code, an agent could automatically suggest which requirements the changed code implements or affects, based on semantic similarity between the commit diff and requirement descriptions. This could be implemented as a Git pre-push hook or a CI step that calls an LLM with the diff and the requirement set and proposes new links.

### 5.2 Requirement Quality Scoring

An automated CI step (using a local or cloud LLM) could score each requirement on standard quality criteria (testability, unambiguity, completeness, atomicity) and produce a quality report alongside the standard validation report. This would not block the CI pipeline (non-zero exit) but would provide a quality trend over time.

### 5.3 AI-Driven Gap Analysis Against Standards

Given an external normative source document (e.g., EN ISO 13849), an LLM could suggest which clauses are not yet covered by derived requirements. This closes the loop on the external-source traceability feature and is particularly valuable for safety and compliance projects.

### 5.4 Privacy and Data Governance

For safety-critical and confidential projects, requirement text is often proprietary or regulated. The following controls should be available regardless of which AI feature is used:

- All AI features are opt-in and disabled by default.
- The user can restrict AI features to a local-only LLM endpoint.
- The tool must log (in its verbose output) any time it is about to transmit data to an external endpoint, including the endpoint URL and the nature of the data.
- An `ai.enabled: false` flag in `.vibe-req.yaml` must disable all AI features unconditionally, suitable for use on air-gapped systems.

### 5.5 Suggested Requirements

| ID (suggested) | Requirement | Priority |
|---|---|---|
| REQ-120 | All AI features shall be disabled by default and require explicit opt-in | Must |
| REQ-121 | The tool shall support a local-only AI mode where all LLM calls are routed to a user-configured local endpoint (e.g., Ollama) | Should |
| REQ-122 | The tool shall log every external AI API call in verbose output, including the endpoint URL and data category being transmitted | Must (if any cloud AI feature is implemented) |
| REQ-123 | The `.vibe-req.yaml` configuration file shall support an `ai.enabled: false` flag that unconditionally disables all AI features | Should |
| REQ-124 | The tool shall provide a CI step (script or GitHub Action) that runs an AI-powered requirement quality check and produces a non-blocking quality report | Could |

---

## 6. Summary and Prioritisation

| Theme | Recommendation | Priority |
|---|---|---|
| MCP server | Implement a `vibe-req mcp` stdio server; the CLI-to-MCP mapping is nearly one-to-one and high value for AI-assisted editors | **High** |
| Agent skills | Ship OpenAPI tool definitions and a system prompt fragment in a `skills/` directory; low effort, high reach | **High** |
| GUI AI authoring | Add opt-in AI polish and first-draft generation in the GUI; very useful but deferred until the GUI itself exists | **Medium** |
| Local LLM support | Ensure all AI features work against a local endpoint; critical for confidential and safety-critical projects | **High** (prerequisite for any AI feature) |
| AI traceability linking | Useful but complex; consider as a post-MVP stretch goal | **Low** |
| Quality scoring in CI | Valuable for long-term quality; straightforward to implement as an optional script | **Medium** |
| Gap analysis vs. standards | High value for compliance projects; requires a good external-source model first | **Low** (Phase 4+) |

The two highest-value items to pursue first are:
1. **MCP server** — maximises AI-assistant interoperability with minimal code beyond what the CLI already does.
2. **`skills/` directory with OpenAPI definitions and system prompt** — zero runtime cost, immediately usable by any agent.

GUI AI features should be designed once the GUI itself reaches a stable state (Phase 5 of the roadmap), and should follow the local-first, opt-in principles described in Section 5.4.
