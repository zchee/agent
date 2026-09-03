#!/usr/bin/env python3
"""Generate a Claude Code marketplace manifest from a Codex one.

Reads <repo>/.agents/plugins/marketplace.json for the plugin list, each
plugin's own <repo>/plugins/<name>/.codex-plugin/plugin.json for its metadata,
and the official Claude Code marketplace for category assignments, then writes
<repo>/.claude-plugin/marketplace.json.
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path

OFFICIAL_SCHEMA = "https://anthropic.com/claude-code/marketplace.schema.json"

DEFAULT_OFFICIAL = (
    Path.home()
    / "src/github.com/anthropics/claude-plugins-official/.claude-plugin/marketplace.json"
)

# Codex title-case categories -> the lowercase set the official Claude Code
# marketplace uses. Consulted only when the official marketplace has no entry
# for the same plugin name.
CATEGORY_FALLBACK = {
    "Business & Operations": "productivity",
    "Communication": "productivity",
    "Creativity": "design",
    "Data & Analytics": "monitoring",
    "Developer Tools": "development",
    "Education & Research": "learning",
    "Finance": "productivity",
    "Productivity": "productivity",
    "Scientific Research": "learning",
    "Security": "security",
}

# Official descriptions top out at 665 characters and never contain newlines.
MAX_DESCRIPTION = 600


def normalize_description(raw: str) -> str:
    """First paragraph, whitespace-collapsed, trimmed at a sentence boundary."""
    parts = [re.sub(r"\s+", " ", p).strip() for p in raw.split("\n")]
    parts = [p for p in parts if p]
    text = parts[0] if parts else ""
    i = 1
    while len(text) < 60 and i < len(parts):
        text = f"{text} {parts[i]}"
        i += 1
    if len(text) > MAX_DESCRIPTION:
        cut = text.rfind(". ", 0, MAX_DESCRIPTION)
        text = text[: cut + 1] if cut > 200 else text[:MAX_DESCRIPTION].rstrip() + "..."
    return text


def author_of(manifest: dict) -> dict | None:
    author = manifest.get("author")
    if not isinstance(author, dict):
        return None
    return {k: author[k] for k in ("name", "email", "url") if author.get(k)} or None


def git(repo: Path, *args: str) -> str:
    result = subprocess.run(
        ["git", "-C", str(repo), *args], capture_output=True, text=True, check=False
    )
    return result.stdout.strip() if result.returncode == 0 else ""


def tree_url(repo: Path) -> str | None:
    """https://<host>/<owner>/<repo>/tree/<default branch>, from origin."""
    origin = git(repo, "remote", "get-url", "origin")
    if not origin:
        return None
    origin = origin.removesuffix(".git")
    if origin.startswith("git@"):
        origin = "https://" + origin[4:].replace(":", "/", 1)
    head = git(repo, "symbolic-ref", "--short", "refs/remotes/origin/HEAD")
    branch = head.split("/")[-1] if head else "main"
    return f"{origin}/tree/{branch}"


def build(repo: Path, official_path: Path | None) -> dict:
    src = json.loads((repo / ".agents/plugins/marketplace.json").read_text())
    official: dict[str, dict] = {}
    if official_path and official_path.is_file():
        official = {
            p["name"]: p for p in json.loads(official_path.read_text())["plugins"]
        }
    else:
        print(
            f"warning: official marketplace not read ({official_path}); "
            "categories come from CATEGORY_FALLBACK only",
            file=sys.stderr,
        )

    tree = tree_url(repo)
    plugins: list[dict] = []
    unknown_categories: set[str] = set()
    missing_metadata: list[str] = []

    for entry in src["plugins"]:
        name = entry["name"]
        source = entry["source"]
        local = source.get("source") == "local"
        upstream = official.get(name, {})

        manifest: dict = {}
        if local:
            path = source["path"].removeprefix("./")
            manifest = json.loads(
                (repo / path / ".codex-plugin/plugin.json").read_text()
            )

        out: dict = {"name": name}

        display = (manifest.get("interface") or {}).get("displayName") or (
            entry.get("interface") or {}
        ).get("displayName")
        if display:
            out["displayName"] = display

        # Remote entries carry no local manifest, so their prose comes from the
        # official marketplace entry for the same upstream repository.
        description = manifest.get("description") or upstream.get("description")
        if description:
            out["description"] = normalize_description(description)

        if manifest.get("version"):
            out["version"] = manifest["version"]

        author = author_of(manifest) or author_of(upstream)
        if author:
            out["author"] = author

        category = upstream.get("category") or CATEGORY_FALLBACK.get(entry["category"])
        if category:
            out["category"] = category
        else:
            unknown_categories.add(entry["category"])

        if local:
            out["source"] = source["path"]
            # These directories ship .codex-plugin/plugin.json, not
            # .claude-plugin/plugin.json. strict:false makes Claude Code take
            # the metadata from this entry and still auto-discover skills/,
            # commands/, agents/, hooks.json and .mcp.json at the plugin root.
            out["strict"] = False
        else:
            out["source"] = dict(source)

        homepage = manifest.get("homepage") or manifest.get("repository")
        if local and (not homepage or (tree and homepage == tree.split("/tree/")[0])):
            homepage = (
                f"{tree}/{source['path'].removeprefix('./')}" if tree else homepage
            )
        if not local:
            homepage = (
                homepage
                or upstream.get("homepage")
                or source["url"].removesuffix(".git")
            )
        if homepage:
            out["homepage"] = homepage

        if manifest.get("keywords"):
            out["keywords"] = manifest["keywords"]
        if manifest.get("license"):
            out["license"] = manifest["license"]

        if not out.get("description"):
            missing_metadata.append(name)
        plugins.append(out)

    if unknown_categories:
        raise SystemExit(
            "unmapped Codex categories, add them to CATEGORY_FALLBACK: "
            + ", ".join(sorted(unknown_categories))
        )
    if missing_metadata:
        raise SystemExit(
            "no description available (no local manifest and no official entry): "
            + ", ".join(missing_metadata)
        )

    # Entry order follows the official marketplace convention: name ascending.
    plugins.sort(key=lambda p: p["name"])

    marketplace = {
        "name": src["name"],
        "plugins": plugins,
    }
    if src.get("description"):
        marketplace["description"] = src["description"]
    return marketplace


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, default=Path.cwd(), help="repository root")
    parser.add_argument(
        "--official",
        type=Path,
        default=DEFAULT_OFFICIAL,
        help="path to the official Claude Code .claude-plugin/marketplace.json",
    )
    parser.add_argument("--out", type=Path, help="output path")
    parser.add_argument(
        "--schema",
        help="$schema URL (default: keep the one already in the output file)",
    )
    parser.add_argument(
        "--description", help="top-level marketplace description for the generated file"
    )
    parser.add_argument("--owner-name", help="top-level owner.name")
    parser.add_argument("--owner-url", help="top-level owner.url")
    args = parser.parse_args()

    repo = args.repo.resolve()
    marketplace = build(repo, args.official)
    out = args.out or repo / ".claude-plugin/marketplace.json"

    # The hand-written header ($schema, description, owner) survives
    # regeneration: whatever the output file already carries is kept unless the
    # matching flag overrides it. A schemastore mirror an editor validates
    # against therefore stays put.
    previous = json.loads(out.read_text()) if out.is_file() else {}
    marketplace["$schema"] = args.schema or previous.get("$schema") or OFFICIAL_SCHEMA
    description = (
        args.description
        or marketplace.get("description")
        or previous.get("description")
    )
    if description:
        marketplace["description"] = description
    owner = {k: v for k, v in (("name", args.owner_name), ("url", args.owner_url)) if v}
    if owner or previous.get("owner"):
        marketplace["owner"] = owner or previous["owner"]

    ordered = {
        k: marketplace[k]
        for k in ("$schema", "name", "description", "owner", "plugins")
        if k in marketplace
    }
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(ordered, indent=2, ensure_ascii=False) + "\n")
    print(f"wrote {out} ({len(ordered['plugins'])} plugins)")


if __name__ == "__main__":
    main()
