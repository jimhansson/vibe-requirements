#!/usr/bin/env bash

set -euo pipefail

notice() {
  echo "::notice::$*"
}

warning() {
  echo "::warning::$*"
}

find_doc_id() {
  local wanted_doc_type="$1"
  local id_prefix

  case "${wanted_doc_type^^}" in
    SRS) id_prefix='SRS-' ;;
    SDD) id_prefix='SDD-' ;;
    *) return 1 ;;
  esac

  "$BIN_PATH" list --kind document "$SCAN_ROOT" 2>/dev/null \
    | grep -oE "${id_prefix}[A-Za-z0-9-]*" \
    | head -n 1
}

REPO_ROOT="$(git rev-parse --show-toplevel)"
BIN_PATH="${BIN_PATH:-$REPO_ROOT/bin/vibe-req}"
OUTPUT_DIR="${OUTPUT_DIR:-$REPO_ROOT/.published-docs}"
WIKI_DIR="${WIKI_DIR:-$REPO_ROOT/.wiki-publish}"
SCAN_ROOT="${SCAN_ROOT:-$REPO_ROOT}"

SRS_DOC_ID="${SRS_DOC_ID:-}"
SDD_DOC_ID="${SDD_DOC_ID:-}"

if [[ ! -x "$BIN_PATH" ]]; then
  echo "error: expected built binary at '$BIN_PATH'" >&2
  exit 1
fi

if [[ -z "$SRS_DOC_ID" ]]; then
  SRS_DOC_ID="$(find_doc_id SRS || true)"
fi

if [[ -z "$SDD_DOC_ID" ]]; then
  SDD_DOC_ID="$(find_doc_id SDD || true)"
fi

if [[ -z "$SRS_DOC_ID" || -z "$SDD_DOC_ID" ]]; then
  warning "Skipping wiki publish because SRS and SDD document entities were not both found (SRS='${SRS_DOC_ID:-}', SDD='${SDD_DOC_ID:-}')."
  exit 0
fi

if [[ -z "${GITHUB_REPOSITORY:-}" || -z "${GITHUB_TOKEN:-}" ]]; then
  warning "Skipping wiki publish because GITHUB_REPOSITORY or GITHUB_TOKEN is not set."
  exit 0
fi

rm -rf "$OUTPUT_DIR" "$WIKI_DIR"
mkdir -p "$OUTPUT_DIR"

notice "Rendering SRS document '$SRS_DOC_ID'."
"$BIN_PATH" doc "$SRS_DOC_ID" --output "$OUTPUT_DIR/SRS.md" "$SCAN_ROOT"

notice "Rendering SDD document '$SDD_DOC_ID'."
"$BIN_PATH" doc "$SDD_DOC_ID" --output "$OUTPUT_DIR/SDD.md" "$SCAN_ROOT"

cat > "$OUTPUT_DIR/Home.md" <<EOF
# ${GITHUB_REPOSITORY} Wiki

- [SRS](SRS)
- [SDD](SDD)
EOF

WIKI_REMOTE_URL="https://x-access-token:${GITHUB_TOKEN}@github.com/${GITHUB_REPOSITORY}.wiki.git"
if ! git clone "$WIKI_REMOTE_URL" "$WIKI_DIR"; then
  warning "Skipping wiki publish because the GitHub wiki repository is not available."
  exit 0
fi

cp "$OUTPUT_DIR/Home.md" "$WIKI_DIR/Home.md"
cp "$OUTPUT_DIR/SRS.md" "$WIKI_DIR/SRS.md"
cp "$OUTPUT_DIR/SDD.md" "$WIKI_DIR/SDD.md"

git -C "$WIKI_DIR" config user.name "github-actions[bot]"
git -C "$WIKI_DIR" config user.email "41898282+github-actions[bot]@users.noreply.github.com"
git -C "$WIKI_DIR" add Home.md SRS.md SDD.md

if git -C "$WIKI_DIR" diff --cached --quiet; then
  notice "Wiki content is already up to date."
  exit 0
fi

git -C "$WIKI_DIR" commit -m "docs: publish SRS and SDD"
git -C "$WIKI_DIR" push origin HEAD
<<<<<<< HEAD

=======
>>>>>>> a4ccbd9 (added main documents and links)
