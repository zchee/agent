---
name: gcloud
description: Use when Codex needs to inspect, manage, troubleshoot, or optimize Google Cloud resources with the Google Cloud CLI (`gcloud`). Apply it for Google Cloud tasks involving Compute Engine, Cloud SQL, GKE, Dataproc, Cloud DNS, Cloud Run, App Engine, authentication, configuration, or other project-scoped resource operations, especially when the work requires choosing between product-specific tools and raw `gcloud`, clarifying project or location identifiers, and using local gcloud defaults safely.
---

# Google Cloud CLI

Use this skill to operate on Google Cloud resources through `gcloud` without
guessing scope or reaching for generic shell wrappers too early.

## Workflow

1. Check whether `gcloud` is the right interface.
   - Prefer a more specific product tool or MCP integration when it can satisfy
     the request with safer behavior or more structured data.
   - Use `gcloud` when the request is explicitly about `gcloud` or when it is
     the native practical interface for the task.
2. Verify local prerequisites only when relevant.
   - Confirm `gcloud` is available if the environment is unknown.
   - Confirm the active account or project before relying on local defaults for
     anything state-changing.
3. Establish scope before execution.
   - Determine the product, `project_id`, resource name, and any required
     location fields such as region or zone.
   - If any required identifier is missing or ambiguous, ask instead of
     guessing.
   - If `project_id` is omitted, use the configured default and state that
     assumption once in the summary.
4. Prefer inspection before mutation.
   - Use `list` or `describe` commands to confirm the target resource when the
     action is destructive or high impact.
   - Echo the exact resource, project, and location back in the plan or summary
     for risky changes.
5. Discover exact command syntax on demand.
   - Use `gcloud <group> <command> --help` for current flags and examples.
   - Use the online reference when local help is insufficient or when you need
     to navigate unfamiliar command groups.
6. Keep output structured.
   - Prefer `--format=json`, `--format=value(...)`, or a targeted
     `--format='table(...)'` over wide unstructured output when the result will
     be parsed or summarized.
   - Use explicit `gcloud` flags instead of brittle shell post-processing when
     the CLI already supports the selection or formatting you need.
7. Execute narrowly.
   - Pass explicit `--project`, `--region`, or `--zone` flags whenever default
     drift could target the wrong resource.
   - Use `--quiet` only when the command is clearly safe to run
     non-interactively and scope is already confirmed.
8. Summarize assumptions and residual risk.
   - Record which project, location, account, configuration, and defaults were
     used.
   - Call out when results depend on local auth, active configuration, or
     enabled APIs.

## Command Selection Rules

- Prefer product-specific native tools over generic shell wrappers.
- Prefer `gcloud` over hand-written API calls when the CLI already covers the
  operation.
- Prefer dedicated tools over `kubectl` or arbitrary shell commands when they
  expose the same action more safely.
- Do not invent cluster names, service names, regions, zones, instance names,
  or project ids.
- Treat the active `gcloud` configuration as a default, not proof that it is
  the intended target.
- For destructive operations, require exact scope before execution.

## Useful Checks

- Inspect configuration: `gcloud config list`
- Inspect active project: `gcloud config get-value project`
- Inspect active account: `gcloud auth list`
- Inspect available configurations: `gcloud config configurations list`
- Inspect command help: `gcloud <group> <command> --help`
- Inspect command reference: `https://cloud.google.com/sdk/gcloud/reference`

## Good Triggers

Use this skill for requests like:

- `list Compute Engine instances in the current project`
- `describe this Cloud Run service with gcloud`
- `find the right gcloud command to update a Cloud SQL instance`
- `check which project my gcloud session is using`
- `troubleshoot why this GKE or Dataproc gcloud command fails`
- `deploy or inspect an App Engine service from the CLI`

## Scope Notes

- Keep this skill focused on operational guidance, not a full command catalog.
- Load online reference pages only when exact command details are needed.
- If the environment lacks `gcloud`, lacks credentials, or points at the wrong
  project, say so directly before suggesting or running commands.
