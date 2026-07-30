# Claude Code Environment Variables (v2.1.220)

Reverse-engineered from `cli.unpack.js` at tag `2.1.220` (last updated 2026-07-29). This catalog covers first-party Claude Code knobs plus the ambient and bundled-dependency environment variables that are read somewhere in the shipped runtime.

## How this was derived

At 2.1.220 the bundle is a Bun-compiled artifact (`package.json` `bin` is `bin/claude.exe`) and most first-party variables are no longer read through bare `process.env.NAME`. They are declared in a typed schema and reached through an accessor namespace, so a `process.env` grep alone under-reports the surface by roughly a third. The list below is the union of four access patterns:

1. **Typed schema declarations** — a builder (`Pe.str()`, `Pe.bool()`, `Pe.triBool()`, `Pe.int()`, `Pe.enum([...])`) defines 779 variables, exported as `NAME: () => symbol` maps and consumed at call sites as `Z.NAME`. This is the authoritative first-party surface and the source of the **Type** column.
2. **Direct `process.env.NAME` reads** — 523 names, mostly platform/ambient detection and bundled SDKs.
3. **Bundled-SDK helpers** — `getStringFromEnv(...)` / `getNumberFromEnv(...)` in the OpenTelemetry SDK.
4. **Name registries** — literal allowlists such as the settings-injectable set (`axh`, ~185 names, around line 77978), the provider-sensitive set, the proxy set, and the subprocess-forwarding sets. Variables that appear only here are still live: they are injected into `process.env` from `settings.json` `env` and then read indirectly.

**1014 variables** are documented. Rows marked *Settings-injectable* may also be set through the `env` block of `settings.json`.

## Reading the Type column

| Type | Meaning |
|---|---|
| `bool` | Truthy string (`1`, `true`) enables; unset or falsy disables. |
| `tri-bool` | Three-state. Unset defers to the default or a server-side experiment flag; `1`/`true` forces on; `0`/`false` forces off. This distinction matters — an explicit `0` is not the same as leaving it unset. |
| `int` | Integer. Unparseable values fall back to the built-in default. |
| `string` | Free-form string. |
| `enum a \| b` | Only the listed values are accepted; anything else is discarded and treated as unset. |
| `—` | Not in the typed schema — read directly from `process.env`, by a bundled dependency, or only via a name registry. |

## Changes since v2.1.154

This revision is a full re-derivation rather than an incremental patch, because the access pattern changed. Notable results:

- **294 variables newly documented**, including the Fable model family (`ANTHROPIC_DEFAULT_FABLE_MODEL*`, `DISABLE_PROMPT_CACHING_FABLE`, `VERTEX_REGION_CLAUDE_FABLE_5`), the Anthropic-on-Google-Cloud provider (`ANTHROPIC_GOOGLE_CLOUD_*`, `CLAUDE_CODE_USE_ANTHROPIC_GOOGLE_CLOUD`), the cloud gateway (`CLAUDE_CODE_USE_GATEWAY`, `CLAUDE_GATEWAY_*`), the agent egress proxy (`CCR_AGENT_PROXY_*`, `CLAUDE_CODE_AGENT_PROXY_*`), plugin sync (`CLAUDE_CODE_SYNC_PLUGINS*`), and 14 codename experiment gates now grouped under *Experiment & Feature Gates*.
- **A Type column** derived from the typed schema, including the `tri-bool` distinction that was previously invisible.
- **33 variables removed** — verified absent from the bundle (zero occurrences), listed under *Removed / Legacy*.
- **60 variables are declared but never consumed** — they have a typed-registry entry and a schema line, but zero reads anywhere else in the bundle (checked by both string-literal and minified-symbol search). They are marked as reserved/dead rather than described speculatively. `CLAUDE_INTERNAL_FC_OVERRIDES` is a separate case: it has a real reader, but two unconditional `return` statements sit above the `process.env` access, so that code is unreachable.
- **Three entries are not environment variables at all** and are annotated as such rather than removed, since the names look env-var-shaped and turn up in greps: `CLAUDE_SDK_CAN_USE_TOOL_SHADOWED` (a `process.emitWarning` code), `MODE` (an esbuild `define` key), and `DATABASE_URL` (an example row in an embedded doc template).
- Entries carried forward from earlier revisions retain their prior descriptions; their type and presence were re-verified against this tag.

## Authentication & API

| Variable | Type | Default | Description |
|---|---|---|---|
| `ANTHROPIC_API_KEY` | string | — | Primary API key for Anthropic API |
| `ANTHROPIC_AUTH_TOKEN` | string | — | Alternative authentication token (OAuth) |
| `ANTHROPIC_BASE_URL` | string | `https://api.anthropic.com` | Custom Anthropic API endpoint |
| `ANTHROPIC_BETAS` | string | — | Comma-separated beta feature flags to send to API |
| `ANTHROPIC_CONFIG_DIR` | string | — | Overrides the directory used to read/write the Anthropic SDK profile config |
| `ANTHROPIC_CUSTOM_HEADERS` | string | — | Custom HTTP headers for API requests |
| `ANTHROPIC_DEFAULT_FABLE_MODEL` | string | — | Overrides the model id used for the Fable tier. Setting it (like the other ANTHROPIC_DEFAULT_*_MODEL vars) marks the install as using custom model defaults. |
| `ANTHROPIC_DEFAULT_FABLE_MODEL_DESCRIPTION` | string | `Custom Fable model` | Description shown for the custom Fable model in the model picker. |
| `ANTHROPIC_DEFAULT_FABLE_MODEL_NAME` | string | value of `ANTHROPIC_DEFAULT_FABLE_MODEL` | Display label for the custom Fable model; falls back to the raw model id. |
| `ANTHROPIC_DEFAULT_FABLE_MODEL_SUPPORTED_CAPABILITIES` | — | — | Comma-separated capability list advertised for the custom Fable model. |
| `ANTHROPIC_ENVIRONMENT_ID` | — | — | Self-hosted agent environment ID read by the agent-environment worker runtime (`config:{type:self_hosted}`) (v2.1.145) |
| `ANTHROPIC_ENVIRONMENT_KEY` | — | — | Self-hosted agent environment key paired with `ANTHROPIC_ENVIRONMENT_ID` for `EnvironmentWorker.run` / `ant beta:worker poll` flows (v2.1.145) |
| `ANTHROPIC_FEDERATION_RULE_ID` | string | — | OIDC federation rule ID for workload-identity auth; required for `oidc_federation` configs unless set in the profile |
| `ANTHROPIC_FOUNDRY_AUTH_TOKEN` | string | — | Bearer token for Azure AI Foundry. When set it is used directly as the auth value instead of running the normal Foundry auth flow. |
| `ANTHROPIC_GOOGLE_CLOUD_BASE_URL` | string | `https://claude.googleapis.com` | Base URL for the Anthropic-on-Google-Cloud provider. |
| `ANTHROPIC_GOOGLE_CLOUD_LOCATION` | string | `global` | Google Cloud location/region for the Anthropic-on-Google-Cloud provider. |
| `ANTHROPIC_GOOGLE_CLOUD_PROJECT` | string | value of `GOOGLE_CLOUD_PROJECT` | Google Cloud project id for the Anthropic-on-Google-Cloud provider; falls back to GOOGLE_CLOUD_PROJECT. |
| `ANTHROPIC_GOOGLE_CLOUD_WORKSPACE_ID` | string | — | Google Cloud Workspace ID for the Anthropic-on-Google-Cloud client; the client throws when it is required and missing. |
| `ANTHROPIC_IDENTITY_TOKEN` | — | — | Raw OIDC identity token used for `oidc_federation` authentication when no identity-token file is configured. |
| `ANTHROPIC_IDENTITY_TOKEN_FILE` | — | — | Path to a file holding the OIDC identity token for `oidc_federation` authentication; takes precedence over `ANTHROPIC_IDENTITY_TOKEN`. |
| `ANTHROPIC_MODEL` | string | — | Override the default model used |
| `ANTHROPIC_ORGANIZATION_ID` | string | — | Anthropic organization ID for workload-identity / federation authentication |
| `ANTHROPIC_PROFILE` | string | `default` | Selects which named Anthropic SDK profile to load from the config file |
| `ANTHROPIC_SERVICE_ACCOUNT_ID` | — | — | Service-account ID supplied in the `oidc_federation` authentication config. |
| `ANTHROPIC_UNIX_SOCKET` | string | — | Unix socket path for Anthropic API (Bun runtime) |
| `ANTHROPIC_WORKSPACE_ID` | string | — | Workspace ID used for workload-identity authentication when a federation rule is scoped to multiple workspaces |
| `CLAUDE_CODE_ACCOUNT_UUID` | string | — | Account UUID for telemetry |
| `CLAUDE_CODE_API_BASE_URL` | string | — | Alternative base API URL |
| `CLAUDE_CODE_API_KEY_FILE_DESCRIPTOR` | string | — | File descriptor to read API key from |
| `CLAUDE_CODE_API_KEY_HELPER_TTL_MS` | int | — | TTL in ms for API key helper cache |
| `CLAUDE_CODE_CUSTOM_OAUTH_URL` | string | — | Custom OAuth endpoint URL |
| `CLAUDE_CODE_HOST_AUTH_ENV_VAR` | string | `ANTHROPIC_AUTH_TOKEN` | Names which environment variable the host process exposes the auth token in; read when the host manages authentication (v2.1.145) |
| `CLAUDE_CODE_HOST_AUTH_REFRESH_TIMEOUT_MS` | — | — | Timeout in ms to wait for the host to refresh the auth token before giving up; coerced via `Number()` (v2.1.145) |
| `CLAUDE_CODE_OAUTH_CLIENT_ID` | string | — | Custom OAuth client ID |
| `CLAUDE_CODE_OAUTH_REFRESH_TOKEN` | string | — | OAuth refresh token |
| `CLAUDE_CODE_OAUTH_SCOPES` | string | — | OAuth scopes |
| `CLAUDE_CODE_OAUTH_TOKEN` | string | — | OAuth token for authentication |
| `CLAUDE_CODE_OAUTH_TOKEN_FILE_DESCRIPTOR` | string | — | File descriptor to read OAuth token from |
| `CLAUDE_CODE_ORGANIZATION_UUID` | string | — | Organization UUID |
| `CLAUDE_CODE_RATE_LIMIT_TIER` | string | — | Rate-limit tier label reported with each request; populated from the auth snapshot |
| `CLAUDE_CODE_SDK_HAS_HOST_AUTH_REFRESH` | bool | — | Set to `"1"` by the runtime to signal that the SDK host can refresh host-managed auth tokens on behalf of Claude Code (v2.1.145) |
| `CLAUDE_CODE_SESSION_ACCESS_TOKEN` | string | — | Session access token |
| `CLAUDE_CODE_SUBSCRIPTION_TYPE` | string | — | Subscription type reported with each request; populated from the auth snapshot |
| `CLAUDE_CODE_USER_EMAIL` | string | — | User email for telemetry |
| `CLAUDE_TRUSTED_DEVICE_TOKEN` | string | — | Trusted device token for authentication |

## Model Configuration

| Variable | Type | Default | Description |
|---|---|---|---|
| `ANTHROPIC_CUSTOM_MODEL_OPTION` | string | — | Add a custom model ID to the model picker and model-validation path |
| `ANTHROPIC_CUSTOM_MODEL_OPTION_DESCRIPTION` | string | `Custom model (<id>)` | Display description for `ANTHROPIC_CUSTOM_MODEL_OPTION` |
| `ANTHROPIC_CUSTOM_MODEL_OPTION_NAME` | string | value of `ANTHROPIC_CUSTOM_MODEL_OPTION` | Display label for `ANTHROPIC_CUSTOM_MODEL_OPTION` |
| `ANTHROPIC_CUSTOM_MODEL_OPTION_SUPPORTED_CAPABILITIES` | — | — | Comma-separated capability list advertised for `ANTHROPIC_CUSTOM_MODEL_OPTION` |
| `ANTHROPIC_DEFAULT_HAIKU_MODEL` | string | — | Override the default Haiku model ID |
| `ANTHROPIC_DEFAULT_HAIKU_MODEL_DESCRIPTION` | string | `Custom Haiku model` | Display description for custom Haiku model |
| `ANTHROPIC_DEFAULT_HAIKU_MODEL_NAME` | string | value of `ANTHROPIC_DEFAULT_HAIKU_MODEL` | Display label for custom Haiku model |
| `ANTHROPIC_DEFAULT_HAIKU_MODEL_SUPPORTED_CAPABILITIES` | — | — | Comma-separated capabilities for custom Haiku model |
| `ANTHROPIC_DEFAULT_OPUS_MODEL` | string | — | Override the default Opus model ID |
| `ANTHROPIC_DEFAULT_OPUS_MODEL_DESCRIPTION` | string | `Custom Opus model` | Display description for custom Opus model |
| `ANTHROPIC_DEFAULT_OPUS_MODEL_NAME` | string | value of `ANTHROPIC_DEFAULT_OPUS_MODEL` | Display label for custom Opus model |
| `ANTHROPIC_DEFAULT_OPUS_MODEL_SUPPORTED_CAPABILITIES` | — | — | Comma-separated capabilities for custom Opus model |
| `ANTHROPIC_DEFAULT_SONNET_MODEL` | string | — | Override the default Sonnet model ID |
| `ANTHROPIC_DEFAULT_SONNET_MODEL_DESCRIPTION` | string | `Custom Sonnet model` | Display description for custom Sonnet model |
| `ANTHROPIC_DEFAULT_SONNET_MODEL_NAME` | string | value of `ANTHROPIC_DEFAULT_SONNET_MODEL` | Display label for custom Sonnet model |
| `ANTHROPIC_DEFAULT_SONNET_MODEL_SUPPORTED_CAPABILITIES` | — | — | Comma-separated capabilities for custom Sonnet model |
| `ANTHROPIC_SMALL_FAST_MODEL` | string | — | Override the small/fast model used internally |
| `ANTHROPIC_SMALL_FAST_MODEL_AWS_REGION` | string | — | AWS region for the small/fast model |
| `API_TIMEOUT_MS` | int | `600000` | API request timeout in milliseconds |
| `CLAUDE_CODE_ALWAYS_ENABLE_EFFORT` | bool | `false` | Force enable effort/thinking for all models |
| `CLAUDE_CODE_EFFORT_LEVEL` | string | — | Effort level: `low`, `medium`, `high`, `max`, or `unset`/`auto` |
| `CLAUDE_CODE_MAX_CONTEXT_TOKENS` | int | — | Override the per-model context window limit (only honored when `DISABLE_COMPACT` is set) |
| `CLAUDE_CODE_MAX_OUTPUT_TOKENS` | int | model-dependent | Maximum output tokens per response |
| `CLAUDE_CODE_MAX_RETRIES` | int | — | Maximum API retry count |
| `CLAUDE_CODE_MAX_TURNS` | string | — | Hard cap on conversation turns; must be a positive integer or startup fails |
| `CLAUDE_CODE_SUBAGENT_MODEL` | string | — | Override model for subagents |
| `MAX_THINKING_TOKENS` | int | — | Maximum thinking tokens (set >0 to enable thinking) |

## AWS Bedrock

| Variable | Type | Default | Description |
|---|---|---|---|
| `ANTHROPIC_BEDROCK_BASE_URL` | string | `https://bedrock-runtime.{region}.amazonaws.com` | Custom Bedrock endpoint |
| `ANTHROPIC_BEDROCK_SERVICE_TIER` | string | — | Value sent as the `X-Amzn-Bedrock-Service-Tier` header when calling Bedrock-hosted Claude models |
| `AWS_ACCESS_KEY_ID` | string | — | AWS access key |
| `AWS_BEARER_TOKEN_BEDROCK` | string | — | Bearer token for Bedrock authentication |
| `AWS_DEFAULT_REGION` | string | `us-east-1` | Fallback AWS region |
| `AWS_LOGIN_CACHE_DIRECTORY` | — | — | Directory for AWS login cache |
| `AWS_PROFILE` | string | — | Named AWS profile |
| `AWS_REGION` | string | `us-east-1` | AWS region |
| `AWS_SECRET_ACCESS_KEY` | string | — | AWS secret key |
| `AWS_SESSION_TOKEN` | string | — | Temporary AWS session token |
| `CLAUDE_CODE_SKIP_BEDROCK_AUTH` | bool | `false` | Skip Bedrock authentication setup |
| `CLAUDE_CODE_USE_BEDROCK` | bool | `false` | Enable AWS Bedrock as the API provider |
| `CLAUDE_ENABLE_BYTE_WATCHDOG_BEDROCK` | bool | — | Truthy enables the byte-level stream watchdog for Bedrock `vnd.amazon.eventstream` responses (v2.1.145) |

## AWS SDK Credential Chain

| Variable | Type | Default | Description |
|---|---|---|---|
| `AWS_ACCOUNT_ID` | — | ambient | Account ID paired with env-provided AWS credentials |
| `AWS_AUTH_SCHEME_PREFERENCE` | — | ambient | Comma-separated preferred AWS auth schemes for bundled SDK auth resolution |
| `AWS_CA_BUNDLE` | — | — | Custom CA bundle path for the AWS SDK. Part of the CA-bundle env set propagated to sandboxed/proxied child processes. |
| `AWS_CONFIG_FILE` | string | ambient | Override the path to the AWS shared config file |
| `AWS_CONTAINER_AUTHORIZATION_TOKEN` | — | ambient | Authorization token for container-credentials endpoint requests |
| `AWS_CONTAINER_AUTHORIZATION_TOKEN_FILE` | — | ambient | File containing the authorization token for container-credentials endpoint requests |
| `AWS_CONTAINER_CREDENTIALS_FULL_URI` | string | ambient | Full container-credentials endpoint URL |
| `AWS_CONTAINER_CREDENTIALS_RELATIVE_URI` | string | ambient | Relative ECS container-credentials endpoint path |
| `AWS_CREDENTIAL_EXPIRATION` | — | ambient | Expiration timestamp paired with env-provided AWS credentials |
| `AWS_CREDENTIAL_SCOPE` | — | ambient | Credential scope paired with env-provided AWS credentials |
| `AWS_DEFAULTS_MODE` | — | `legacy` | AWS SDK defaults mode (`auto`, `in-region`, `cross-region`, `mobile`, `standard`, `legacy`) |
| `AWS_EC2_METADATA_DISABLED` | — | ambient | Disable EC2 instance-metadata credential and region resolution in bundled AWS SDK providers |
| `AWS_EC2_METADATA_SERVICE_ENDPOINT` | — | ambient | Override the EC2 instance metadata service endpoint |
| `AWS_EC2_METADATA_SERVICE_ENDPOINT_MODE` | — | ambient | Select IPv4 or IPv6 EC2 metadata endpoint mode |
| `AWS_EC2_METADATA_V1_DISABLED` | — | ambient | Disable IMDSv1 fallback in bundled AWS SDK providers |
| `AWS_ENDPOINT_URL` | — | ambient | Override the AWS service endpoint base URL; service-specific endpoint suffixes are also supported by bundled AWS SDK components |
| `AWS_MAX_ATTEMPTS` | — | ambient | Override the AWS SDK retry attempt count |
| `AWS_RETRY_MODE` | — | ambient | Override the AWS SDK retry strategy selection |
| `AWS_ROLE_ARN` | string | ambient | IAM role ARN to assume when web-identity credentials are used |
| `AWS_ROLE_SESSION_NAME` | — | ambient | Session name to use when assuming `AWS_ROLE_ARN` |
| `AWS_SDK_UA_APP_ID` | — | ambient | Attach an application ID to bundled AWS SDK user-agent strings |
| `AWS_SHARED_CREDENTIALS_FILE` | string | ambient | Override the path to the AWS shared credentials file |
| `AWS_USE_DUALSTACK_ENDPOINT` | — | ambient | Prefer dual-stack AWS service endpoints when supported |
| `AWS_USE_FIPS_ENDPOINT` | — | ambient | Prefer FIPS AWS service endpoints when supported |
| `AWS_WEB_IDENTITY_TOKEN_FILE` | string | ambient | Path to the web-identity token file used for STS role assumption |

## Google Vertex AI

| Variable | Type | Default | Description |
|---|---|---|---|
| `ANTHROPIC_VERTEX_BASE_URL` | string | — | Custom Vertex AI endpoint |
| `ANTHROPIC_VERTEX_PROJECT_ID` | string | — | Google Cloud project ID for Vertex AI |
| `CLAUDE_CODE_SKIP_VERTEX_AUTH` | bool | `false` | Skip Vertex AI authentication setup |
| `CLAUDE_CODE_USE_VERTEX` | bool | `false` | Enable Google Vertex AI as the API provider |
| `CLOUD_ML_REGION` | string | `us-east5` | Google Cloud region for Vertex AI |
| `GCLOUD_PROJECT` | string | — | Alternative Google Cloud project ID |
| `GOOGLE_APPLICATION_CREDENTIALS` | string | — | Path to Google Cloud service account credentials JSON |
| `GOOGLE_CLOUD_QUOTA_PROJECT` | — | — | Google Cloud quota project ID |
| `VERTEX_REGION_CLAUDE_3_5_HAIKU` | — | falls back to `CLOUD_ML_REGION` / `us-east5` | Per-model Vertex region override for `claude-3-5-haiku` |
| `VERTEX_REGION_CLAUDE_3_5_SONNET` | — | falls back to `CLOUD_ML_REGION` / `us-east5` | Per-model Vertex region override for `claude-3-5-sonnet` |
| `VERTEX_REGION_CLAUDE_3_7_SONNET` | — | falls back to `CLOUD_ML_REGION` / `us-east5` | Per-model Vertex region override for `claude-3-7-sonnet` |
| `VERTEX_REGION_CLAUDE_4_0_OPUS` | — | falls back to `CLOUD_ML_REGION` / `us-east5` | Per-model Vertex region override for `claude-opus-4` |
| `VERTEX_REGION_CLAUDE_4_0_SONNET` | — | falls back to `CLOUD_ML_REGION` / `us-east5` | Per-model Vertex region override for `claude-sonnet-4` |
| `VERTEX_REGION_CLAUDE_4_1_OPUS` | — | falls back to `CLOUD_ML_REGION` / `us-east5` | Per-model Vertex region override for `claude-opus-4-1` |
| `VERTEX_REGION_CLAUDE_4_5_OPUS` | — | falls back to `CLOUD_ML_REGION` / `us-east5` | Per-model Vertex region override for `claude-opus-4-5` |
| `VERTEX_REGION_CLAUDE_4_5_SONNET` | — | falls back to `CLOUD_ML_REGION` / `us-east5` | Per-model Vertex region override for `claude-sonnet-4-5` |
| `VERTEX_REGION_CLAUDE_4_6_OPUS` | — | falls back to `CLOUD_ML_REGION` / `us-east5` | Per-model Vertex region override for `claude-opus-4-6` |
| `VERTEX_REGION_CLAUDE_4_6_SONNET` | — | falls back to `CLOUD_ML_REGION` / `us-east5` | Per-model Vertex region override for `claude-sonnet-4-6` |
| `VERTEX_REGION_CLAUDE_4_7_OPUS` | — | falls back to `CLOUD_ML_REGION` / `us-east5` | Per-model Vertex region override for `claude-opus-4-7` |
| `VERTEX_REGION_CLAUDE_4_8_OPUS` | — | — | Vertex AI region for Claude Opus 4.8. Settings-injectable. |
| `VERTEX_REGION_CLAUDE_5_OPUS` | — | — | Vertex AI region for Claude Opus 5. Settings-injectable. |
| `VERTEX_REGION_CLAUDE_5_SONNET` | — | — | Vertex AI region for Claude Sonnet 5. Settings-injectable. |
| `VERTEX_REGION_CLAUDE_FABLE_5` | — | — | Vertex AI region for Claude Fable 5. Settings-injectable. |
| `VERTEX_REGION_CLAUDE_HAIKU_4_5` | — | falls back to `CLOUD_ML_REGION` / `us-east5` | Per-model Vertex region override for `claude-haiku-4-5` |

## Anthropic Foundry

| Variable | Type | Default | Description |
|---|---|---|---|
| `ANTHROPIC_FOUNDRY_API_KEY` | string | — | Foundry API key |
| `ANTHROPIC_FOUNDRY_BASE_URL` | string | — | Foundry API endpoint |
| `ANTHROPIC_FOUNDRY_RESOURCE` | string | — | Foundry resource identifier |
| `CLAUDE_CODE_SKIP_FOUNDRY_AUTH` | bool | `false` | Skip Foundry authentication setup |
| `CLAUDE_CODE_USE_FOUNDRY` | bool | `false` | Enable Anthropic Foundry as the API provider |

## Anthropic AWS

| Variable | Type | Default | Description |
|---|---|---|---|
| `ANTHROPIC_AWS_API_KEY` | string | — | Anthropic AWS API key |
| `ANTHROPIC_AWS_BASE_URL` | string | — | Anthropic AWS API endpoint |
| `ANTHROPIC_AWS_WORKSPACE_ID` | string | — | Anthropic AWS workspace identifier |
| `CLAUDE_CODE_SKIP_ANTHROPIC_AWS_AUTH` | bool | `false` | Skip Anthropic AWS authentication setup |
| `CLAUDE_CODE_USE_ANTHROPIC_AWS` | bool | `false` | Enable Anthropic AWS as the API provider |

## Bedrock Mantle

| Variable | Type | Default | Description |
|---|---|---|---|
| `ANTHROPIC_BEDROCK_MANTLE_BASE_URL` | string | computed from region | Override the Bedrock Mantle base URL |
| `CLAUDE_CODE_SKIP_MANTLE_AUTH` | bool | `false` | Skip Bedrock Mantle authentication setup |
| `CLAUDE_CODE_USE_MANTLE` | bool | `false` | Enable the Bedrock Mantle route as the API provider |

## Configuration & Directories

| Variable | Type | Default | Description |
|---|---|---|---|
| `CLAUDE_CODE_GIT_BASH_PATH` | string | — | Path to Git Bash on Windows |
| `CLAUDE_CODE_SHELL` | string | — | Override shell used for Bash tool |
| `CLAUDE_CODE_SHELL_PREFIX` | string | — | Prefix command for shell executions |
| `CLAUDE_CODE_TMPDIR` | string | `/tmp` | Temporary directory for Claude Code |
| `CLAUDE_CONFIG_DIR` | string | `~/.claude` | Claude configuration directory |
| `CLAUDE_ENV_FILE` | string | — | Path to environment file to load at session start |
| `CLAUDE_SECURESTORAGE_CONFIG_DIR` | string | — | Override the directory used by the secure-storage backend for its config (v2.1.145) |
| `CLAUDE_TMPDIR` | string | `/tmp/claude` | Temporary directory used in sandbox |

## Tool Configuration

| Variable | Type | Default | Description |
|---|---|---|---|
| `BASH_DEFAULT_TIMEOUT_MS` | — | `120000` | Default Bash tool timeout in milliseconds before max-timeout clamping is applied |
| `BASH_MAX_OUTPUT_LENGTH` | int | `30000` (upper: `150000`) | Maximum Bash tool output length in characters |
| `BASH_MAX_TIMEOUT_MS` | — | `600000` | Maximum Bash tool timeout in milliseconds |
| `CLAUDE_CODE_FILE_READ_MAX_OUTPUT_TOKENS` | int | — | Maximum output tokens for file reads |
| `CLAUDE_CODE_GLOB_HIDDEN` | bool | `true` | Whether glob includes hidden files |
| `CLAUDE_CODE_GLOB_NO_IGNORE` | bool | `true` | Whether glob ignores gitignore patterns |
| `CLAUDE_CODE_GLOB_TIMEOUT_SECONDS` | — | `0` (unlimited) | Glob tool timeout in seconds |
| `CLAUDE_CODE_MAX_TOOL_USE_CONCURRENCY` | — | `10` | Maximum concurrent tool executions |
| `CLAUDE_CODE_PWSH_PARSE_TIMEOUT_MS` | int | — | Timeout for PowerShell command parsing (ms) |
| `CLAUDE_CODE_USE_NATIVE_FILE_SEARCH` | bool | `false` | Use native file search |
| `CLAUDE_CODE_USE_POWERSHELL_TOOL` | bool | `false` | Allow the Bash/input-box execution path to run via PowerShell when `defaultShell` is `powershell` and sandbox policy permits it |
| `SLASH_COMMAND_TOOL_CHAR_BUDGET` | — | — | Character budget for slash command tools |
| `TASK_MAX_OUTPUT_LENGTH` | int | — | Maximum TaskOutput length in characters |
| `USE_BUILTIN_RIPGREP` | string | `false` | Use built-in ripgrep instead of system |

## MCP (Model Context Protocol)

| Variable | Type | Default | Description |
|---|---|---|---|
| `ALLOW_ANT_COMPUTER_USE_MCP` | bool | — | Declared in the typed env registry but with no consumer anywhere else in the bundle (verified by both string-literal and minified-symbol search) — reserved or dead at this tag; setting it has no effect. |
| `ENABLE_CLAUDEAI_MCP_SERVERS` | bool | `false` | Enable claude.ai MCP proxy servers |
| `ENABLE_MCP_LARGE_OUTPUT_FILES` | tri-bool | `false` | Enable large file output for MCP |
| `MAX_MCP_OUTPUT_TOKENS` | int | `25000` | Maximum tokens for MCP output |
| `MCP_CLIENT_SECRET` | string | — | OAuth client secret for MCP servers |
| `MCP_CONNECTION_NONBLOCKING` | bool | `false` | Make MCP server connections non-blocking |
| `MCP_CONNECT_TIMEOUT_MS` | int | — | Override (ms) for the MCP server connection timeout; parsed as an integer |
| `MCP_DISCOVERY_CACHE` | tri-bool | — | Declared in the typed env registry but with no consumer anywhere else in the bundle (verified by both string-literal and minified-symbol search) — reserved or dead at this tag; setting it has no effect. |
| `MCP_OAUTH_CALLBACK_PORT` | — | — | Port for MCP OAuth callbacks |
| `MCP_OAUTH_CLIENT_METADATA_URL` | string | — | Override the client metadata URL advertised during MCP OAuth registration flows |
| `MCP_REMOTE_SERVER_CONNECTION_BATCH_SIZE` | — | `20` | Remote MCP server connection batch size |
| `MCP_SDK_GENERATION` | string | — | Forces the MCP runtime to SDK generation `v1` or `v2`. Invalid values are ignored with a warning, after which a GrowthBook flag decides. |
| `MCP_SERVER_CONNECTION_BATCH_SIZE` | — | `3` | Local MCP server connection batch size |
| `MCP_TIMEOUT` | int | `30000` | Default timeout for MCP operations (ms) |
| `MCP_TOOL_TIMEOUT` | — | — | Timeout for individual MCP tool calls (ms) |
| `MCP_TRUNCATION_PROMPT_OVERRIDE` | string | — | Override the truncation prompt used for MCP output |
| `MCP_XAA_IDP_CLIENT_SECRET` | string | — | OAuth client secret for MCP XAA IdP connections |

## Feature Disable Flags

| Variable | Type | Default | Description |
|---|---|---|---|
| `CLAUDE_CODE_DISABLE_1M_CONTEXT` | bool | `false` | Disable 1M token context window |
| `CLAUDE_CODE_DISABLE_ADAPTIVE_THINKING` | bool | `false` | Disable adaptive thinking |
| `CLAUDE_CODE_DISABLE_ADVISOR_TOOL` | bool | `false` | Disable the advisor tool |
| `CLAUDE_CODE_DISABLE_AGENT_VIEW` | bool | — | Truthy disables the agent view (`claude agents`, `--bg`, `/background`, on-demand daemon); mirrors the `disableAgentView` setting |
| `CLAUDE_CODE_DISABLE_ALTERNATE_SCREEN` | bool | — | Truthy disables the terminal alternate-screen buffer (same effect as `CLAUDE_CODE_NO_FLICKER`) |
| `CLAUDE_CODE_DISABLE_ARTIFACT` | bool | — | Disables the Artifact tool. Equivalent to setting disableArtifact in settings; either source disables it. |
| `CLAUDE_CODE_DISABLE_ATTACHMENTS` | bool | `false` | Disable attachments feature |
| `CLAUDE_CODE_DISABLE_AUTO_MEMORY` | bool | `false` | Disable automatic memory saving |
| `CLAUDE_CODE_DISABLE_BACKGROUND_TASKS` | bool | `false` | Disable background task execution |
| `CLAUDE_CODE_DISABLE_BEDROCK_CONTENT_TYPE_GUARD` | bool | — | Disables the guard that rejects Bedrock streaming responses whose content type is not vnd.amazon.eventstream. Set it to tolerate proxies that rewrite the content type. |
| `CLAUDE_CODE_DISABLE_BG_EXIT_HANDOFF` | bool | — | Disables handing a background session off to its exit-cleanup path. |
| `CLAUDE_CODE_DISABLE_BG_SHELL_PRESSURE_REAP` | bool | — | Disables reaping background shells under memory pressure. |
| `CLAUDE_CODE_DISABLE_BUNDLED_SKILLS` | bool | — | Removes the skills and workflows that ship with Claude Code. Equivalent to disableBundledSkills in settings; plugin and project skills are unaffected. |
| `CLAUDE_CODE_DISABLE_CLAUDE_API_SKILL` | bool | `false` | Disable the Claude API skill |
| `CLAUDE_CODE_DISABLE_CLAUDE_CODE_SKILL` | bool | `false` | Disable loading of the built-in Claude Code skill (gated by `!xH(process.env.CLAUDE_CODE_DISABLE_CLAUDE_CODE_SKILL)`) (v2.1.154) |
| `CLAUDE_CODE_DISABLE_CLAUDE_MDS` | bool | `false` | Disable CLAUDE.md file scanning |
| `CLAUDE_CODE_DISABLE_CRON` | bool | `false` | Disable cron/scheduled tasks |
| `CLAUDE_CODE_DISABLE_EXPERIMENTAL_BETAS` | bool | `false` | Disable experimental beta features |
| `CLAUDE_CODE_DISABLE_EXPLORE_INHERIT_CAP` | bool | — | Forces built-in Explore subagents to always inherit the parent session's model, skipping the normal cap/downgrade that would otherwise pick a cheaper one. Despite the name it governs model selection, not context. |
| `CLAUDE_CODE_DISABLE_EXPLORE_PLAN_AGENTS` | bool | — | Removes the built-in Explore and Plan agents from the agent list. |
| `CLAUDE_CODE_DISABLE_FAST_MODE` | bool | `false` | Disable fast mode |
| `CLAUDE_CODE_DISABLE_FEEDBACK_SURVEY` | bool | `false` | Disable feedback survey |
| `CLAUDE_CODE_DISABLE_FILE_CHECKPOINTING` | bool | `false` | Disable file checkpointing (undo) |
| `CLAUDE_CODE_DISABLE_GIT_INSTRUCTIONS` | bool | `false` | Disable git instructions in system prompt |
| `CLAUDE_CODE_DISABLE_LAUNCH_COMPOSER` | bool | — | Declared in the typed env registry but with no consumer anywhere else in the bundle — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_CODE_DISABLE_LEGACY_MODEL_REMAP` | bool | `false` | Disable legacy model name remapping |
| `CLAUDE_CODE_DISABLE_MEMORY_BULK_INFLATE` | bool | — | Disables bulk inflation of memory entries, forcing them to be loaded individually. |
| `CLAUDE_CODE_DISABLE_MEMORY_MASS_DELETE_HOLD` | bool | — | Disables the safety hold that pauses a mass memory deletion before it is pushed. |
| `CLAUDE_CODE_DISABLE_MEMORY_PERIODIC_RESYNC` | bool | — | Disables the periodic background resync of memory state. |
| `CLAUDE_CODE_DISABLE_MEMORY_STREAM_LIST` | bool | — | Disables streaming (paginated) listing of memory entries during export, falling back to the non-streaming path; the `tengu_memory_stream_list` flag (default on) otherwise controls it. |
| `CLAUDE_CODE_DISABLE_MOUSE_CLICKS` | tri-bool | — | Tri-state mouse control. Truthy puts the terminal in scroll-only mode (click reporting off, wheel still works); an explicit falsy value forces full mouse mode; unset defers to autodetection. |
| `CLAUDE_CODE_DISABLE_NESTED_CHAIN_IDLE` | bool | — | Disables the blocked/idle notification counter for nested subagent chains waiting on user input. |
| `CLAUDE_CODE_DISABLE_NONESSENTIAL_TRAFFIC` | bool | `false` | Disable all non-essential network traffic (telemetry, updates, etc.) |
| `CLAUDE_CODE_DISABLE_NONSTREAMING_FALLBACK` | bool | `false` | Disable fallback from streaming to non-streaming API calls |
| `CLAUDE_CODE_DISABLE_NOTIFICATION_PRESENCE_CHECK` | bool | — | Sends notifications without first checking whether a client is present; useful when presence detection misfires. |
| `CLAUDE_CODE_DISABLE_OFFICIAL_MARKETPLACE_AUTOINSTALL` | bool | `false` | Disable auto-install of official marketplace items |
| `CLAUDE_CODE_DISABLE_ORG_MEMORY` | bool | — | Disables organization-scoped memory entirely, so only personal and project memory are loaded. |
| `CLAUDE_CODE_DISABLE_POLICY_SKILLS` | bool | `false` | Disable policy-driven skills loading |
| `CLAUDE_CODE_DISABLE_PRECOMPACT_SKIP` | bool | `false` | Disable pre-compaction skip optimization |
| `CLAUDE_CODE_DISABLE_REFUSAL_FALLBACK` | bool | — | Disables the automatic fallback retry that normally runs when the model ends a turn with a refusal. |
| `CLAUDE_CODE_DISABLE_TERMINAL_TITLE` | bool | `false` | Don't update terminal title |
| `CLAUDE_CODE_DISABLE_THINKING` | bool | `false` | Disable extended thinking |
| `CLAUDE_CODE_DISABLE_VIRTUAL_SCROLL` | bool | `false` | Disable virtual scroll in UI |
| `CLAUDE_CODE_DISABLE_WORKFLOWS` | bool | `false` | Disable the workflows feature; truthy disables it, equivalent to `settings.disableWorkflows === true` (v2.1.154) |
| `CLAUDE_CODE_DISABLE_WORKING_SYNC` | bool | — | Disables syncing "working" session state to the SDK/remote endpoint. Only consulted for remote sessions with an SDK URL and no environment kind set. |
| `DISABLE_AUTOUPDATER` | bool | `false` | Disable auto-updater |
| `DISABLE_AUTO_COMPACT` | bool | `false` | Disable automatic context compaction |
| `DISABLE_BRIEF_MODE_STOP_HOOK` | bool | — | Truthy suppresses the Brief-mode stop hook in SDK/REPL sessions that have the Brief tool |
| `DISABLE_BUG_COMMAND` | bool | `false` | Disable the `/bug` command |
| `DISABLE_COMPACT` | bool | `false` | Disable context compaction entirely |
| `DISABLE_COST_WARNINGS` | bool | `false` | Disable cost warning notifications |
| `DISABLE_DOCTOR_COMMAND` | bool | `false` | Disable the `/doctor` command |
| `DISABLE_ERROR_REPORTING` | bool | `false` | Disable error reporting |
| `DISABLE_EXTRA_USAGE_COMMAND` | bool | `false` | Disable extra usage display |
| `DISABLE_FEEDBACK_COMMAND` | bool | `false` | Disable the `/feedback` command |
| `DISABLE_GROWTHBOOK` | bool | — | Truthy disables GrowthBook feature-flag lookups |
| `DISABLE_INSTALLATION_CHECKS` | bool | `false` | Disable installation health checks |
| `DISABLE_INSTALL_GITHUB_APP_COMMAND` | bool | `false` | Disable the GitHub App install command |
| `DISABLE_INTERLEAVED_THINKING` | bool | `false` | Disable interleaved thinking |
| `DISABLE_LOGIN_COMMAND` | bool | `false` | Disable the `/login` command |
| `DISABLE_LOGOUT_COMMAND` | bool | `false` | Disable the `/logout` command |
| `DISABLE_PROMPT_CACHING` | bool | `false` | Disable prompt caching for all models |
| `DISABLE_PROMPT_CACHING_FABLE` | bool | — | Disables prompt caching for Fable-class models specifically. Settings-injectable. |
| `DISABLE_PROMPT_CACHING_HAIKU` | bool | `false` | Disable prompt caching for Haiku |
| `DISABLE_PROMPT_CACHING_MYTHOS` | bool | — | Disables prompt caching for Mythos-class models specifically. |
| `DISABLE_PROMPT_CACHING_OPUS` | bool | `false` | Disable prompt caching for Opus |
| `DISABLE_PROMPT_CACHING_SONNET` | bool | `false` | Disable prompt caching for Sonnet |
| `DISABLE_TELEMETRY` | bool | `false` | Disable all telemetry |
| `DISABLE_UPDATES` | bool | — | Truthy disables auto-update checks and installs |
| `DISABLE_UPGRADE_COMMAND` | bool | `false` | Disable the `/upgrade` command |

## Feature Enable Flags

| Variable | Type | Default | Description |
|---|---|---|---|
| `CLAUDE_CODE_ENABLE_AUTO_MODE` | — | — | Opt-in required to use Auto mode with non-first-party providers. Without it set, Auto mode reports itself unavailable for that provider. Settings-injectable. |
| `CLAUDE_CODE_ENABLE_AWAY_SUMMARY` | bool | — | Enable away summary feature |
| `CLAUDE_CODE_ENABLE_BACKGROUND_PLUGIN_REFRESH` | bool | `false` | Enable background refresh for plugins |
| `CLAUDE_CODE_ENABLE_CFC` | tri-bool | — | Enable CFC feature |
| `CLAUDE_CODE_ENABLE_DESIGN_SYNC` | bool | — | Enables the design-sync integration. Settings-injectable. |
| `CLAUDE_CODE_ENABLE_EXPERIMENTAL_ADVISOR_TOOL` | bool | `false` | Enable the experimental advisor tool |
| `CLAUDE_CODE_ENABLE_FEEDBACK_SURVEY_FOR_OTEL` | bool | — | Truthy enables the in-product feedback survey for OTEL-instrumented installations |
| `CLAUDE_CODE_ENABLE_FINE_GRAINED_TOOL_STREAMING` | bool | `false` | Enable fine-grained tool streaming |
| `CLAUDE_CODE_ENABLE_GATEWAY_MODEL_DISCOVERY` | bool | — | Truthy enables gateway-side model discovery (first-party provider only) |
| `CLAUDE_CODE_ENABLE_LAUNCH_COMPOSER` | bool | — | Declared in the typed env registry but with no consumer anywhere else in the bundle — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_CODE_ENABLE_MENU_KIND_LANES` | bool | — | Forces on the `tengu_mint_lanes` experiment, grouping the slash-command menu into kind-based lanes. |
| `CLAUDE_CODE_ENABLE_PROMPT_SUGGESTION` | tri-bool | — | Enable/disable prompt suggestions (`false` to disable) |
| `CLAUDE_CODE_ENABLE_REFRESH_MCP_TOOLS` | bool | — | Enables the tool that refreshes MCP server tool lists mid-session. |
| `CLAUDE_CODE_ENABLE_REMOTE_RECAP` | tri-bool | — | Tri-state override for the `tengu_harbor_moth` experiment controlling the away-summary / remote recap feature (default off). |
| `CLAUDE_CODE_ENABLE_SDK_FILE_CHECKPOINTING` | bool | `false` | Enable file checkpointing in SDK mode |
| `CLAUDE_CODE_ENABLE_TASKS` | tri-bool | `false` | Enable tasks feature |
| `CLAUDE_CODE_ENABLE_TELEMETRY` | — | `false` | Enable OpenTelemetry telemetry |
| `CLAUDE_CODE_ENABLE_TOKEN_USAGE_ATTACHMENT` | bool | `false` | Enable token usage attachment |
| `CLAUDE_CODE_ENABLE_XAA` | bool | — | Enable XAA IdP authentication feature |
| `CLAUDE_CODE_FORCE_FULL_LOGO` | bool | `false` | Force display of full logo |
| `CLAUDE_CODE_WORKFLOWS` | tri-bool | — | Truthy gates the workflows feature; combined with the `tengu_workflows_enabled` Statsig gate to decide whether workflows are available (v2.1.146) |
| `EMBEDDED_SEARCH_TOOLS` | bool | `false` | Enable embedded search tools |
| `ENABLE_BETA_TRACING_DETAILED` | bool | `false` | Enable detailed beta tracing |
| `ENABLE_ENHANCED_TELEMETRY_BETA` | bool | `false` | Enable enhanced telemetry beta |
| `ENABLE_LOCKLESS_UPDATES` | bool | — | Declared in the typed env registry but with no consumer anywhere else in the bundle (verified by both string-literal and minified-symbol search) — reserved or dead at this tag; setting it has no effect. |
| `ENABLE_LSP_TOOL` | bool | — | Declared in the typed env registry but with no consumer anywhere else in the bundle (verified by both string-literal and minified-symbol search) — reserved or dead at this tag; setting it has no effect. |
| `ENABLE_PID_BASED_VERSION_LOCKING` | tri-bool | — | Declared in the typed env registry but with no consumer anywhere else in the bundle (verified by both string-literal and minified-symbol search) — reserved or dead at this tag; setting it has no effect. |
| `ENABLE_PROMPT_CACHING_1H` | bool | `false` | Enable 1-hour prompt caching |
| `ENABLE_PROMPT_CACHING_1H_BEDROCK` | bool | `false` | Enable 1-hour prompt caching on Bedrock |
| `ENABLE_SESSION_BACKGROUNDING` | bool | — | Declared in the typed env registry but with no consumer anywhere else in the bundle (verified by both string-literal and minified-symbol search) — reserved or dead at this tag; setting it has no effect. |
| `ENABLE_SESSION_PERSISTENCE` | bool | — | Declared in the typed env registry but with no consumer anywhere else in the bundle (verified by both string-literal and minified-symbol search) — reserved or dead at this tag; setting it has no effect. |
| `ENABLE_TOOL_SEARCH` | string | — | Enable tool search/deferred tools (`true`, `auto`, `auto:N`) |
| `FORCE_PROMPT_CACHING_5M` | bool | `false` | Force 5-minute prompt caching |
| `USE_API_CONTEXT_MANAGEMENT` | bool | `false` | Use API context management |

## Compaction & Context

| Variable | Type | Default | Description |
|---|---|---|---|
| `CLAUDE_AUTOCOMPACT_PCT_OVERRIDE` | string | — | Override auto-compact percentage threshold |
| `CLAUDE_CODE_AUTO_COMPACT_WINDOW` | string | — | Auto-compact window size |
| `CLAUDE_CODE_BLOCKING_LIMIT_OVERRIDE` | string | — | Override blocking limit for compaction |
| `CLAUDE_CODE_CLASSIFIER_SUMMARY` | string | — | Overrides summary classifier mode: truthy = `llm`, falsy = `heuristic`; unset falls back to the default selector |
| `CLAUDE_CODE_COLD_COMPACT` | bool | — | Truthy forces the "cold compact" conversation-compaction path |

## Debug & Logging

| Variable | Type | Default | Description |
|---|---|---|---|
| `ANTHROPIC_LOG` | — | library default | Anthropic SDK log-level override used by the bundled client logger |
| `AUTOMODE_DECISION_LOG` | — | — | Set to `1` to append auto-mode decisions to `.automode_decisions.jsonl` in the working directory. Any other value disables the log. |
| `CLAUDE_CODE_BENCH_LIVE_COUNTS` | bool | — | Truthy enables live Yoga/DOM/Fiber node-count sampling in the renderer for benchmark instrumentation |
| `CLAUDE_CODE_BYOC_ENABLE_DATADOG` | bool | — | When `CLAUDE_CODE_ENVIRONMENT_KIND=byoc`, truthy re-enables Datadog telemetry (off by default in BYOC) |
| `CLAUDE_CODE_COMMIT_LOG` | string | — | Path to commit log file |
| `CLAUDE_CODE_DATADOG_FLUSH_INTERVAL_MS` | — | — | Datadog flush interval |
| `CLAUDE_CODE_DEBUG_LOGS_DIR` | string | `~/.claude/debug/{timestamp}.txt` | Debug log file directory |
| `CLAUDE_CODE_DEBUG_LOG_LEVEL` | string | `debug` | Log level: `debug`, `info`, `warn`, `error` |
| `CLAUDE_CODE_DEBUG_REPAINTS` | bool | `false` | Enable debug logging for UI repaints |
| `CLAUDE_CODE_DEV_RAW_CHANGELOG_URL` | string | — | Declared in the typed env registry but with no consumer anywhere else in the bundle — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_CODE_DIAGNOSTICS_FILE` | string | — | Path to diagnostics output file |
| `CLAUDE_CODE_FABLE_BRIDGE_DIALOG_TIMEOUT_MS` | int | `60000` | Milliseconds to wait for the Fable bridge dialog before abandoning it. |
| `CLAUDE_CODE_FRAME_TIMING_LOG` | string | — | Path to frame timing log |
| `CLAUDE_CODE_PERFETTO_TRACE` | string | — | Path for Perfetto trace output |
| `CLAUDE_CODE_PROFILE_STARTUP` | bool | — | Set to `1` to enable startup profiling |
| `CLAUDE_CODE_RETRY_WATCHDOG` | bool | — | Truthy enables the API retry watchdog that surfaces stalled retry loops |
| `CLAUDE_CODE_SLOW_OPERATION_THRESHOLD_MS` | int | — | Threshold for logging slow operations (ms) |
| `CLAUDE_CODE_TEE_SDK_STDOUT` | bool | — | Truthy tees SDK stdout to the parent process for debugging |
| `CLAUDE_CODE_USER_DIALOG_TIMEOUT_MS` | int | `300000` | Milliseconds to wait for a response to an interactive user dialog before giving up. |
| `CLAUDE_DEBUG` | bool | `false` | Enable Claude debug logging |
| `CLAUDE_GATEWAY_LOG_LEVEL` | string | `info` | Cloud gateway log verbosity: `debug`, `info`, `warn` or `error`. The value is lower-cased before comparison. |
| `CLAUDE_PTY_RECORD` | string | — | Path to write a PTY session recording for the current terminal session |
| `DEBUG` | string | — | npm debug module namespace patterns |
| `DEBUG_AUTH` | — | — | Enable authentication debug logging |
| `DEBUG_CLAUDE_AGENT_SDK` | bool | — | Truthy enables verbose debug logging and pipes stderr from the Claude Agent SDK helper process |
| `DEBUG_SDK` | bool | `false` | Enable SDK debug logging |

## OpenTelemetry

| Variable | Type | Default | Description |
|---|---|---|---|
| `ANT_CLAUDE_CODE_METRICS_ENDPOINT` | string | — | Declared in the typed env registry but with no consumer anywhere else in the bundle (verified by both string-literal and minified-symbol search) — reserved or dead at this tag; setting it has no effect. |
| `ANT_OTEL_EXPORTER_OTLP_ENDPOINT` | string | — | Declared in the typed env registry but with no consumer anywhere else in the bundle (verified by both string-literal and minified-symbol search) — reserved or dead at this tag; setting it has no effect. |
| `ANT_OTEL_EXPORTER_OTLP_HEADERS` | string | — | Declared in the typed env registry but with no consumer anywhere else in the bundle (verified by both string-literal and minified-symbol search) — reserved or dead at this tag; setting it has no effect. |
| `ANT_OTEL_EXPORTER_OTLP_PROTOCOL` | string | — | Declared in the typed env registry but with no consumer anywhere else in the bundle (verified by both string-literal and minified-symbol search) — reserved or dead at this tag; setting it has no effect. |
| `ANT_OTEL_LOGS_EXPORTER` | string | — | Declared in the typed env registry but with no consumer anywhere else in the bundle (verified by both string-literal and minified-symbol search) — reserved or dead at this tag; setting it has no effect. |
| `ANT_OTEL_METRICS_EXPORTER` | string | — | Declared in the typed env registry but with no consumer anywhere else in the bundle (verified by both string-literal and minified-symbol search) — reserved or dead at this tag; setting it has no effect. |
| `ANT_OTEL_RESOURCE_ATTRIBUTES` | string | — | Declared in the typed env registry but with no consumer anywhere else in the bundle (verified by both string-literal and minified-symbol search) — reserved or dead at this tag; setting it has no effect. |
| `ANT_OTEL_TRACES_EXPORTER` | string | — | Declared in the typed env registry but with no consumer anywhere else in the bundle (verified by both string-literal and minified-symbol search) — reserved or dead at this tag; setting it has no effect. |
| `BETA_TRACING_ENDPOINT` | string | — | Beta tracing endpoint URL |
| `CLAUDE_CODE_OTEL_FLUSH_TIMEOUT_MS` | int | `5000` | OTEL flush timeout |
| `CLAUDE_CODE_OTEL_HEADERS_HELPER_DEBOUNCE_MS` | int | — | OTEL headers helper debounce |
| `CLAUDE_CODE_OTEL_SHUTDOWN_TIMEOUT_MS` | int | `2000` | OTEL shutdown timeout |
| `CLAUDE_CODE_PROPAGATE_TRACEPARENT` | bool | — | Truthy forces W3C `traceparent` propagation to spawned/child requests even outside the default conditions (v2.1.154) |
| `OTEL_ATTRIBUTE_COUNT_LIMIT` | — | `128` | Global OpenTelemetry attribute-count limit |
| `OTEL_ATTRIBUTE_VALUE_LENGTH_LIMIT` | — | `Infinity` | Global OpenTelemetry attribute-value length limit |
| `OTEL_BLRP_EXPORT_TIMEOUT` | — | `30000` | Export timeout in milliseconds for the OpenTelemetry log-record batch processor |
| `OTEL_BLRP_MAX_EXPORT_BATCH_SIZE` | — | `512` | Maximum batch size for the OpenTelemetry log-record batch processor |
| `OTEL_BLRP_MAX_QUEUE_SIZE` | — | `2048` | Maximum queue size for the OpenTelemetry log-record batch processor |
| `OTEL_BLRP_SCHEDULE_DELAY` | — | `5000` | Scheduled delay in milliseconds for the OpenTelemetry log-record batch processor |
| `OTEL_BSP_EXPORT_TIMEOUT` | — | `30000` | Export timeout in milliseconds for the OpenTelemetry span batch processor |
| `OTEL_BSP_MAX_EXPORT_BATCH_SIZE` | — | `512` | Maximum batch size for the OpenTelemetry span batch processor |
| `OTEL_BSP_MAX_QUEUE_SIZE` | — | `2048` | Maximum queue size for the OpenTelemetry span batch processor |
| `OTEL_BSP_SCHEDULE_DELAY` | — | `5000` | Scheduled delay in milliseconds for the OpenTelemetry span batch processor |
| `OTEL_EXPORTER_OTLP_CERTIFICATE` | — | — | Path to a custom root certificate bundle for OTLP exporters; signal-specific `OTEL_EXPORTER_OTLP_{TRACES |
| `OTEL_EXPORTER_OTLP_CLIENT_CERTIFICATE` | — | — | Path to a client certificate chain for OTLP mTLS; signal-specific `OTEL_EXPORTER_OTLP_{TRACES |
| `OTEL_EXPORTER_OTLP_CLIENT_KEY` | — | — | Path to a client private key for OTLP mTLS; signal-specific `OTEL_EXPORTER_OTLP_{TRACES |
| `OTEL_EXPORTER_OTLP_COMPRESSION` | — | — | Global OTLP compression mode (`none` or `gzip`); signal-specific `OTEL_EXPORTER_OTLP_{TRACES |
| `OTEL_EXPORTER_OTLP_ENDPOINT` | string | — | OTLP collector endpoint URL |
| `OTEL_EXPORTER_OTLP_HEADERS` | string | — | Custom OTLP headers |
| `OTEL_EXPORTER_OTLP_INSECURE` | — | — | Skip TLS verification |
| `OTEL_EXPORTER_OTLP_LOGS_ENDPOINT` | string | — | Per-signal OTLP endpoint for logs, overriding OTEL_EXPORTER_OTLP_ENDPOINT. |
| `OTEL_EXPORTER_OTLP_LOGS_HEADERS` | — | — | Logs-specific OTLP headers |
| `OTEL_EXPORTER_OTLP_LOGS_PROTOCOL` | string | — | Logs-specific OTLP protocol |
| `OTEL_EXPORTER_OTLP_METRICS_ENDPOINT` | string | — | Per-signal OTLP endpoint for metrics, overriding OTEL_EXPORTER_OTLP_ENDPOINT. |
| `OTEL_EXPORTER_OTLP_METRICS_HEADERS` | — | — | Metrics-specific OTLP headers |
| `OTEL_EXPORTER_OTLP_METRICS_PROTOCOL` | string | — | Metrics-specific OTLP protocol |
| `OTEL_EXPORTER_OTLP_METRICS_TEMPORALITY_PREFERENCE` | string | `delta` | Metrics temporality preference |
| `OTEL_EXPORTER_OTLP_PROTOCOL` | string | — | OTLP protocol (`grpc`/`http`) |
| `OTEL_EXPORTER_OTLP_TIMEOUT` | — | — | Global OTLP export timeout in milliseconds; signal-specific `OTEL_EXPORTER_OTLP_{TRACES |
| `OTEL_EXPORTER_OTLP_TRACES_ENDPOINT` | string | — | Per-signal OTLP endpoint for traces, overriding OTEL_EXPORTER_OTLP_ENDPOINT. |
| `OTEL_EXPORTER_OTLP_TRACES_HEADERS` | — | — | Trace-specific OTLP headers |
| `OTEL_EXPORTER_OTLP_TRACES_PROTOCOL` | string | — | Traces-specific OTLP protocol |
| `OTEL_EXPORTER_PROMETHEUS_HOST` | — | `localhost` | Prometheus exporter host |
| `OTEL_EXPORTER_PROMETHEUS_PORT` | — | `9464` | Prometheus exporter port |
| `OTEL_LOGRECORD_ATTRIBUTE_COUNT_LIMIT` | — | falls back to `OTEL_ATTRIBUTE_COUNT_LIMIT` / `128` | Log-record attribute-count limit |
| `OTEL_LOGRECORD_ATTRIBUTE_VALUE_LENGTH_LIMIT` | — | falls back to `OTEL_ATTRIBUTE_VALUE_LENGTH_LIMIT` / `Infinity` | Log-record attribute-value length limit |
| `OTEL_LOGS_EXPORTER` | string | — | Logs exporter type |
| `OTEL_LOGS_EXPORT_INTERVAL` | int | — | Logs export interval (ms) |
| `OTEL_LOG_ASSISTANT_RESPONSES` | tri-bool | value of `OTEL_LOG_USER_PROMPTS` | Whether assistant responses are included in telemetry logs. Falls back to OTEL_LOG_USER_PROMPTS, so enabling prompt logging enables response logging unless this is set separately. Settings-injectable. |
| `OTEL_LOG_RAW_API_BODIES` | bool | `false` | Include raw API request/response bodies in OTEL spans (truncated by an internal size cap); verbose and sensitive — intended for debugging only |
| `OTEL_LOG_TOOL_CONTENT` | bool | `false` | Log tool content in OTEL spans |
| `OTEL_LOG_TOOL_DETAILS` | bool | `false` | Log tool details in OTEL spans |
| `OTEL_LOG_USER_PROMPTS` | bool | `false` | Log user prompts in OTEL spans |
| `OTEL_METRICS_EXPORTER` | string | — | Metrics exporter type |
| `OTEL_METRICS_INCLUDE_ACCOUNT_UUID` | — | `true` | Include the tagged account identifier in emitted OTEL metric attributes |
| `OTEL_METRICS_INCLUDE_ENTRYPOINT` | — | `false` | Attaches the `app.entrypoint` attribute to emitted metrics. Settings-injectable. |
| `OTEL_METRICS_INCLUDE_RESOURCE_ATTRIBUTES` | — | `true` | Attaches `OTEL_RESOURCE_ATTRIBUTES`-derived key/value pairs to emitted metrics. Settings-injectable. |
| `OTEL_METRICS_INCLUDE_SESSION_ID` | — | `true` | Include the Claude Code session ID in emitted OTEL metric attributes |
| `OTEL_METRICS_INCLUDE_VERSION` | — | `false` | Include the Claude Code version in emitted OTEL metric attributes |
| `OTEL_METRIC_EXPORT_INTERVAL` | int | — | Metrics export interval (ms) |
| `OTEL_RESOURCE_ATTRIBUTES` | string | — | Additional OTEL resource attributes |
| `OTEL_SERVICE_NAME` | — | — | Override the OTEL service name resource attribute |
| `OTEL_SPAN_ATTRIBUTE_COUNT_LIMIT` | — | falls back to `OTEL_ATTRIBUTE_COUNT_LIMIT` / SDK default | Span attribute-count limit |
| `OTEL_SPAN_ATTRIBUTE_PER_EVENT_COUNT_LIMIT` | — | `128` | Maximum number of attributes recorded per span event |
| `OTEL_SPAN_ATTRIBUTE_PER_LINK_COUNT_LIMIT` | — | `128` | Maximum number of attributes recorded per span link |
| `OTEL_SPAN_ATTRIBUTE_VALUE_LENGTH_LIMIT` | — | falls back to `OTEL_ATTRIBUTE_VALUE_LENGTH_LIMIT` / SDK default | Span attribute-value length limit |
| `OTEL_SPAN_EVENT_COUNT_LIMIT` | — | `128` | Maximum number of events recorded per span |
| `OTEL_SPAN_LINK_COUNT_LIMIT` | — | `128` | Maximum number of links recorded per span |
| `OTEL_TRACES_EXPORTER` | string | — | Traces exporter type |
| `OTEL_TRACES_EXPORT_INTERVAL` | int | — | Traces export interval (ms) |
| `OTEL_TRACES_SAMPLER` | — | `parentbased_always_on` | OpenTelemetry trace sampler selection |
| `OTEL_TRACES_SAMPLER_ARG` | — | sampler-dependent | Optional numeric argument for the selected OpenTelemetry trace sampler |
| `TRACEPARENT` | string | — | W3C trace context propagation: trace parent header (used in SDK mode) |
| `TRACESTATE` | string | — | W3C trace context propagation: trace state header (used in SDK mode) |

## Network & Proxy

| Variable | Type | Default | Description |
|---|---|---|---|
| `AGENT_PROXY_AUTH_TOKEN` | string | — | Bearer token paired with `AGENT_PROXY_URL` for the remote agent egress proxy; consumed and deleted at startup |
| `AGENT_PROXY_URL` | string | — | Upstream proxy URL used by the remote agent egress gateway; consumed and deleted at startup |
| `ALL_PROXY` | string | — | Generic proxy URL for all protocols; part of the proxy env set forwarded to child processes and consulted by bundled HTTP clients. |
| `API_FORCE_IDLE_TIMEOUT` | int | — | Truthy keeps the default idle timeout on Anthropic API requests; otherwise the request timeout is disabled |
| `CCR_AGENT_PROXY_ENABLED` | bool | — | Enables the remote agent egress proxy. When unset the proxy is not started. |
| `CCR_AGENT_PROXY_INCLUDE_HOSTS` | string | — | Comma-separated host allowlist relayed through the remote agent proxy when `CCR_AGENT_PROXY_RELAY_MODE` is `selective`; unlisted hosts use normal networking. |
| `CCR_AGENT_PROXY_RELAY_MODE` | string | — | Relay mode for the remote agent proxy. `selective` tunnels only the hosts in `CCR_AGENT_PROXY_INCLUDE_HOSTS`; any other value tunnels all traffic, and an empty include-list fails closed to tunnel-all. |
| `CLAUDE_CODE_ADDITIONAL_PROTECTION` | bool | `false` | Enable additional API protection headers |
| `CLAUDE_CODE_AGENT_PROXY_GH_SHIM` | bool | — | Installs the `gh` CLI shim that routes GitHub CLI traffic through the agent proxy. |
| `CLAUDE_CODE_AGENT_PROXY_GIT_CONFIG` | bool | — | Rewrites git configuration so git traffic goes through the agent proxy. Either this or the gh shim being set enables agent-proxy git integration. |
| `CLAUDE_CODE_CERT_STORE` | string | — | Select the CA certificate store strategy (`system`, `bundled`, or a custom path); when unset, Node defaults are used |
| `CLAUDE_CODE_CLIENT_CERT` | string | — | Path to client TLS certificate |
| `CLAUDE_CODE_CLIENT_KEY` | string | — | Path to client TLS private key |
| `CLAUDE_CODE_CLIENT_KEY_PASSPHRASE` | string | — | Passphrase for client TLS key |
| `CLAUDE_CODE_ENABLE_PROXY_AUTH_HELPER` | bool | `false` | Enable the proxy-auth helper (`proxyAuthHelper` settings entry) that supplies dynamic `Proxy-Authorization` headers; must be `1` to activate |
| `CLAUDE_CODE_PROXY_AUTH_HELPER_TTL_MS` | int | helper default | Cache TTL (ms) for credentials produced by the proxy-auth helper |
| `CLAUDE_CODE_PROXY_RESOLVES_HOSTS` | bool | `false` | Let proxy resolve hostnames |
| `CLAUDE_CODE_SIMULATE_PROXY_USAGE` | bool | `false` | Simulate proxy usage for testing |
| `CLAUDE_CODE_WEBFETCH_USE_CCR_PROXY` | bool | — | Routes WebFetch requests through the coordinator (CCR) proxy when a coordinator session is active. |
| `CLAUDE_CODE_WEBSEARCH_USE_CCR_PROXY` | bool | — | Routes WebSearch requests through the coordinator (CCR) proxy when a coordinator session is active. |
| `CLAUDE_ENABLE_BYTE_WATCHDOG` | tri-bool | — | Enable byte-level stream watchdog |
| `CLAUDE_SLOW_FIRST_BYTE_MS` | — | `30000` | Timeout threshold for first byte from API (ms) |
| `CURL_CA_BUNDLE` | — | — | Path to CA bundle file (forwarded to subprocesses for curl compatibility) |
| `DOCKER_HTTPS_PROXY` | — | — | HTTPS proxy propagated to Docker-based child tooling when Claude Code configures a proxy. |
| `DOCKER_HTTP_PROXY` | — | — | HTTP proxy propagated to Docker-based child tooling when Claude Code configures a proxy. |
| `ELECTRON_GET_USE_PROXY` | — | `1` when a proxy is detected | Set to `1` by the proxy-propagation logic so the `@electron/get` package honors the detected system proxy. |
| `GLOBAL_AGENT_HTTPS_PROXY` | — | — | HTTPS proxy propagated for the global-agent proxy library used by child tooling. |
| `GLOBAL_AGENT_HTTP_PROXY` | — | — | HTTP proxy propagated for the global-agent proxy library used by child tooling. |
| `GLOBAL_AGENT_NO_PROXY` | — | — | No-proxy list propagated for the global-agent proxy library used by child tooling. |
| `HTTPS_PROXY` | string | — | HTTPS proxy URL. Its presence (with the lowercase form, HTTP_PROXY, a unix socket, or a client cert) puts the HTTP stack on the custom-transport path. Settings-injectable. |
| `HTTP_PROXY` | string | — | HTTP proxy URL, checked alongside the lowercase form. Settings-injectable. |
| `NODE_EXTRA_CA_CERTS` | string | — | Path to additional CA certificates |
| `NO_PROXY` | string | — | Comma-separated no-proxy host list, checked alongside the lowercase form. Settings-injectable. |
| `REQUESTS_CA_BUNDLE` | — | — | Path to CA bundle file (forwarded to subprocesses for Python compatibility) |
| `SSL_CERT_FILE` | string | — | Path to SSL certificate file (used with proxy config) |
| `YARN_HTTPS_PROXY` | — | — | HTTPS proxy propagated to Yarn in child tooling when Claude Code configures a proxy. |
| `YARN_HTTP_PROXY` | — | — | HTTP proxy propagated to Yarn in child tooling when Claude Code configures a proxy. |
| `http_proxy` | string | — | Lowercase HTTP proxy URL, checked alongside HTTP_PROXY. |
| `https_proxy` | string | — | Lowercase HTTPS proxy URL, checked alongside HTTPS_PROXY. |
| `no_proxy` | string | — | Lowercase no-proxy host list, checked alongside NO_PROXY. |

## Remote / Headless Mode

| Variable | Type | Default | Description |
|---|---|---|---|
| `CCR_ENABLE_BUNDLE` | bool | `false` | Enable the CCR bundle path used by remote/background task bootstrap logic |
| `CCR_FORCE_BUNDLE` | bool | `false` | Force the CCR bundle path even when normal preflight heuristics would not select it |
| `CCR_OAUTH_TOKEN_FILE` | — | — | Marks the active OAuth token as one injected by the remote (CCR) host session rather than a local auth source. |
| `CCR_ON_BRANCH_DEFAULT_GUARD` | enum enforce \| observe \| off | `enforce` | Guard policy for committing on the default branch in remote sessions. "enforce" blocks it, "observe" only records the event, "off" disables the guard. Falls back to "observe" when the tengu_on_branch_default_guard_observe gate is on, otherwise "enforce". |
| `CCR_SPAWN_TIMESTAMP_MS` | — | — | Unix spawn timestamp (ms) injected by the Claude Code Remote launcher; used to compute `spawn_to_first_checkpoint_ms` / `spawn_to_exec_ms` startup-performance telemetry (v2.1.145) |
| `CLAUDE_BRIDGE_BASE_URL` | string | — | Declared in the typed env registry but with no consumer anywhere else in the bundle (verified by both string-literal and minified-symbol search) — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_BRIDGE_OAUTH_TOKEN` | string | — | Declared in the typed env registry but with no consumer anywhere else in the bundle (verified by both string-literal and minified-symbol search) — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_BRIDGE_REATTACH_GROUPING` | string | — | Grouping key used when the bridge reattaches sessions. Deleted from the environment after being read so it does not leak into children. |
| `CLAUDE_BRIDGE_REATTACH_SEQ` | int | — | Sequence number passed when the TUI bridge reattaches to an existing session; consumed and deleted on read |
| `CLAUDE_BRIDGE_REATTACH_SESSION` | string | — | Session ID used to reattach the TUI bridge to an existing session; consumed and deleted on read |
| `CLAUDE_BRIDGE_SESSION_INGRESS_URL` | string | — | Declared in the typed env registry but with no consumer anywhere else in the bundle (verified by both string-literal and minified-symbol search) — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_CODE_CONTAINER_ID` | string | — | Container ID for remote environments |
| `CLAUDE_CODE_ENVIRONMENT_RUNNER_VERSION` | string | — | Attach an environment-runner version header in remote bridge mode |
| `CLAUDE_CODE_POST_FOR_SESSION_INGRESS_V2` | bool | `false` | Use POST transport for session-ingress v2 websocket URLs |
| `CLAUDE_CODE_REMOTE` | bool | `false` | Running in remote/headless mode |
| `CLAUDE_CODE_REMOTE_ENVIRONMENT_TYPE` | string | — | Remote environment type label |
| `CLAUDE_CODE_REMOTE_HERMETIC_MODE` | bool | — | Runs a remote session in hermetic mode. Only takes effect when CLAUDE_CODE_REMOTE is also set. |
| `CLAUDE_CODE_REMOTE_MEMORY_DIR` | string | — | Memory directory for remote mode |
| `CLAUDE_CODE_REMOTE_RAW_EVENTS_FILE` | string | — | Declared in the typed env registry but with no consumer anywhere else in the bundle — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_CODE_REMOTE_SEND_KEEPALIVES` | bool | `false` | Send keepalive pings in remote mode |
| `CLAUDE_CODE_REMOTE_SESSION_ID` | string | — | Remote session identifier |
| `CLAUDE_CODE_REMOTE_SESSION_ORIGIN` | string | — | Origin that started the remote session. The value `review` changes the default handling of the cloud-agent session origin. |
| `CLAUDE_CODE_REMOTE_SETTINGS_PATH` | string | — | Path to a local file that overrides remote settings, skipping the settings API fetch entirely. |
| `CLAUDE_CODE_REMOTE_SETTINGS_POLL_MS` | int | — | Declared in the typed env registry but with no consumer anywhere else in the bundle — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_CODE_RESUME_FROM_SESSION` | string | — | Resume from a specific session ID |
| `CLAUDE_CODE_SYSTEM_PROMPT_GB_FEATURE` | string | — | When running in remote mode, selects a Growthbook/feature-flag key whose evaluated string value is used to override the Agent SDK `systemPrompt` option |
| `CLAUDE_CODE_WEBSOCKET_AUTH_FILE_DESCRIPTOR` | string | — | WebSocket auth file descriptor |
| `CLAUDE_ENABLE_STREAM_WATCHDOG` | tri-bool | `false` | Enable stream watchdog |
| `CLAUDE_REMOTE_CONTROL_SESSION_NAME_PREFIX` | string | — | Prefix for remote control session names |
| `CLAUDE_REMOTE_WORKFLOW_ARGS` | string | — | JSON-encoded arguments for the workflow named by `CLAUDE_REMOTE_WORKFLOW_SCRIPT` in a remote session; rejected if it exceeds a size cap. |
| `CLAUDE_REMOTE_WORKFLOW_SCRIPT` | string | — | Names the Workflow-tool script that is the deterministic entry point for a remote session launched with an environment-delivered workflow. |
| `CLAUDE_SESSION_INGRESS_TOKEN_FILE` | string | — | Path to session ingress token file |
| `CLAUDE_STREAM_IDLE_TIMEOUT_MS` | — | `90000` | Stream idle timeout before disconnect (ms) |
| `SESSION_INGRESS_URL` | string | — | Session ingress URL for remote |

## IDE Integration

| Variable | Type | Default | Description |
|---|---|---|---|
| `CLAUDE_CODE_AUTO_CONNECT_IDE` | tri-bool | — | Auto-connect to IDE |
| `CLAUDE_CODE_IDE_HOST_OVERRIDE` | string | — | Override IDE host detection |
| `CLAUDE_CODE_IDE_SKIP_AUTO_INSTALL` | bool | `false` | Skip IDE auto-install |
| `CLAUDE_CODE_IDE_SKIP_VALID_CHECK` | bool | `false` | Skip IDE validation check |
| `CLAUDE_CODE_SSE_PORT` | int | — | SSE port for IDE integration |
| `CURSOR_TRACE_ID` | string | ambient | Detect Cursor editor environment |
| `FORCE_CODE_TERMINAL` | bool | `false` | Force code terminal mode |
| `INTELLIJ_TERMINAL_COMMAND_BLOCKS` | string | — | Presence (any value) indicates the JetBrains IntelliJ terminal command-blocks integration is active |
| `INTELLIJ_TERMINAL_COMMAND_BLOCKS_REWORKED` | string | — | Presence (any value) indicates the reworked JetBrains terminal command-blocks integration is active |
| `VSCODE_GIT_ASKPASS_MAIN` | string | ambient | Detect VS Code terminal environment |
| `VisualStudioVersion` | — | ambient | Detect Visual Studio environment |

## UI & Display

| Variable | Type | Default | Description |
|---|---|---|---|
| `BAT_THEME` | string | — | Fallback syntax highlight theme (bat/batcat theme name) |
| `CLAUDE_CODE_ACCESSIBILITY` | bool | `false` | Enable accessibility mode |
| `CLAUDE_CODE_ALT_SCREEN_FULL_REPAINT` | bool | — | Truthy forces a full repaint of the alternate screen buffer on each render instead of incremental updates (v2.1.145) |
| `CLAUDE_CODE_BRIEF` | bool | `false` | Brief output mode |
| `CLAUDE_CODE_BRIEF_UPLOAD` | bool | `false` | Brief mode for uploads |
| `CLAUDE_CODE_DECSTBM` | string | — | When truthy, force-enable DECSTBM (top/bottom-margin) scroll-region rendering; bypasses the `tengu_marlin_porch` feature-flag gate |
| `CLAUDE_CODE_DISABLE_MOUSE` | tri-bool | `false` | Disable mouse input entirely |
| `CLAUDE_CODE_EMIT_TOOL_USE_SUMMARIES` | bool | `false` | Emit tool use summaries |
| `CLAUDE_CODE_FORCE_FULLSCREEN_UPSELL` | bool | `false` | Force fullscreen upsell display |
| `CLAUDE_CODE_FORCE_SYNC_OUTPUT` | bool | — | Truthy forces synchronous TTY output, bypassing terminal-based auto-detection |
| `CLAUDE_CODE_HIDE_CWD` | bool | — | Truthy hides the current working directory from the TUI footer/status line |
| `CLAUDE_CODE_NATIVE_CURSOR` | bool | — | Truthy enables native terminal cursor rendering instead of the simulated one |
| `CLAUDE_CODE_NO_FLICKER` | tri-bool | — | Control flicker reduction (`true` to enable, `false` to disable) |
| `CLAUDE_CODE_SCROLL_SPEED` | string | — | Override scroll speed |
| `CLAUDE_CODE_SIMPLE` | bool | `false` | Simplified mode (disables CLAUDE.md, attachments, etc.) |
| `CLAUDE_CODE_SYNTAX_HIGHLIGHT` | string | — | Control syntax highlighting (`false` to disable); falls back to `BAT_THEME` |
| `CLAUDE_CODE_TUI_JUST_SWITCHED` | string | — | Internal: track TUI mode switch state (set/dropped across process restarts) |
| `CLI_WIDTH` | — | ambient | Override terminal width detection |

## Sandbox

| Variable | Type | Default | Description |
|---|---|---|---|
| `CLAUDE_BG_TCC_DISCLAIMED` | string | — | Set by the parent and consumed (then deleted from env) to signal that a background process has been disclaimed from the macOS TCC privacy prompt (v2.1.145) |
| `CLAUDE_CODE_BASH_SANDBOX_SHOW_INDICATOR` | bool | `false` | Show sandbox indicator in bash |
| `CLAUDE_CODE_BUBBLEWRAP` | bool | — | Set to `1` when running inside bubblewrap sandbox |
| `CLAUDE_CODE_HOST_HTTP_PROXY_PORT` | — | — | Host HTTP proxy port for sandbox |
| `CLAUDE_CODE_HOST_PLATFORM` | string | — | Host platform when running in sandbox |
| `CLAUDE_CODE_HOST_SOCKS_PROXY_PORT` | — | — | Host SOCKS proxy port for sandbox |
| `CLAUDE_CODE_MCP_ALLOWLIST_ENV` | string | — | Override the sandbox environment allowlist forwarded to MCP server subprocesses |
| `CLAUDE_CODE_SANDBOXED` | bool | — | Short-circuit trust check; when truthy, the project is always treated as trusted |
| `CLAUDE_CODE_SCRIPT_CAPS` | int | — | Override the sandbox script capability set for bash-tool executions |
| `IS_SANDBOX` | string | — | Set to `1` inside sandbox |

## Agent SDK

| Variable | Type | Default | Description |
|---|---|---|---|
| `AI_AGENT` | bool | `claude-code/harness` | Identifier of the AI agent harness running the CLI; auto-set if missing or already a `claude-code` value, and re-set to `claude-code/agent` inside spawned subagents |
| `CLAUDE_AGENTS_SELECT` | string | — | Pre-selected agent name(s) passed through to a spawned `claude agents` invocation; consumed and re-injected per child |
| `CLAUDE_AGENT_SDK_CLIENT_APP` | string | — | Agent SDK client app identifier |
| `CLAUDE_AGENT_SDK_DISABLE_BUILTIN_AGENTS` | bool | `false` | Disable built-in agent types |
| `CLAUDE_AGENT_SDK_MCP_NO_PREFIX` | bool | `false` | Don't prefix MCP tool names in SDK mode |
| `CLAUDE_AGENT_SDK_VERSION` | string | — | Agent SDK version string |
| `CLAUDE_CODE_AGENT` | string | — | Agent name recorded on the session metadata (alongside `CLAUDE_CODE_SESSION_NAME`/`_LOG`) |
| `CLAUDE_CODE_EMIT_SESSION_STATE_EVENTS` | bool | `false` | Emit session state change events in SDK mode |
| `CLAUDE_CODE_ENABLE_APPEND_SUBAGENT_PROMPT` | bool | `false` | Gate for opting-in to appending `appendSubagentSystemPrompt` (from the Agent SDK options) onto every Task-tool subagent's system prompt (propagates to nested subagents) |
| `CLAUDE_CODE_ENTRYPOINT` | string | `cli` | Entry point identifier (`cli`, `sdk-ts`, `sdk-py`, `sdk-cli`, `local-agent`, `claude-desktop`, `remote`, `mcp`, `claude-vscode`, `claude-code-github-action`) |
| `CLAUDE_CODE_FORK_SUBAGENT` | tri-bool | — | Truthy forces the fork-based subagent execution path (otherwise governed by the `tengu_copper_fox` flag) |
| `CLAUDE_CODE_INCLUDE_PARTIAL_MESSAGES` | bool | `false` | Include partial stream events/messages in SDK output |
| `CLAUDE_CODE_SDK_HAS_OAUTH_REFRESH` | bool | `false` | Signal that the SDK host can refresh OAuth tokens on behalf of Claude Code (gated to specific entry points) |
| `CLAUDE_SUBAGENT_BG_SHELL_MAX_MS` | — | — | Max milliseconds a subagent background shell may run; falls back to the built-in `o53` default |
| `SDK_NATIVE_BIN` | string | `claude` | Path/name of the native Claude binary the Agent SDK spawns |

## Teams / Teammates

| Variable | Type | Default | Description |
|---|---|---|---|
| `CLAUDE_CODE_COORDINATOR_EXTRA_TOOLS` | — | `""` | Comma-separated MCP tool names always kept available to a coordinator agent beyond its default filtered tool set; blank entries ignored. |
| `CLAUDE_CODE_COORDINATOR_MODE` | — | — | Marks the current process as a team coordinator; set to `"1"` on the coordinator and inherited by child processes (v2.1.154) |
| `CLAUDE_CODE_EXPERIMENTAL_AGENT_TEAMS` | bool | `false` | Enable experimental agent teams |
| `CLAUDE_CODE_PLAN_MODE_REQUIRED` | bool | `false` | Force plan mode |
| `CLAUDE_CODE_PLAN_V2_AGENT_COUNT` | int | — | Number of agents in plan v2 |
| `CLAUDE_CODE_PLAN_V2_EXPLORE_AGENT_COUNT` | int | — | Number of explore agents in plan v2 |
| `CLAUDE_CODE_SYNC_SKILLS` | bool | — | Truthy enables synchronous skill syncing for teammate/agent spawns (v2.1.154) |
| `CLAUDE_CODE_SYNC_SKILLS_WAIT_TIMEOUT_MS` | int | — | Timeout in ms to wait for skill sync to finish; parsed via `parseInt(..., 10)` (v2.1.154) |
| `CLAUDE_CODE_TEAMMATE_COMMAND` | — | — | Command to spawn teammate processes |
| `CLAUDE_CODE_TMUX_PREFIX` | string | — | Tmux prefix key |
| `CLAUDE_CODE_TMUX_PREFIX_CONFLICTS` | bool | — | Whether tmux prefix conflicts exist |
| `CLAUDE_CODE_TMUX_SESSION` | string | — | Tmux session name for teams |
| `CLAUDE_CODE_TMUX_TRUECOLOR` | bool | — | Override truecolor detection for tmux sessions |
| `CLAUDE_INTERNAL_ASSISTANT_TEAM_NAME` | string | — | Anthropic-internal assistant team name. Deleted from the environment immediately after being read so it is not inherited. |

## Plugins / Cowork

| Variable | Type | Default | Description |
|---|---|---|---|
| `CLAUDE_CODE_PLUGIN_BINARY_ASSETS` | bool | — | Forces on the `tengu_plugin_binary_assets` experiment, enabling platform-specific plugin binary assets (darwin and linux, arm64/x64, musl). |
| `CLAUDE_CODE_PLUGIN_CACHE_DIR` | string | — | Plugin cache directory |
| `CLAUDE_CODE_PLUGIN_GIT_TIMEOUT_MS` | int | — | Git timeout for plugin operations (ms) |
| `CLAUDE_CODE_PLUGIN_KEEP_MARKETPLACE_ON_FAILURE` | bool | `false` | Keep marketplace state on plugin failure |
| `CLAUDE_CODE_PLUGIN_PREFER_HTTPS` | bool | — | Truthy makes plugin git operations prefer HTTPS over SSH (also implied by `CLAUDE_CODE_REMOTE`) |
| `CLAUDE_CODE_PLUGIN_SEED_DIR` | string | — | Plugin seed directory |
| `CLAUDE_CODE_PLUGIN_USE_ZIP_CACHE` | bool | `false` | Use zip cache for plugins |
| `CLAUDE_CODE_SKIP_PLUGIN_MCP_SERVERS` | bool | — | Skips MCP server discovery for plugins entirely, except plugins named in `CLAUDE_CODE_SKIP_PLUGIN_MCP_SERVERS_EXCEPT`. |
| `CLAUDE_CODE_SKIP_PLUGIN_MCP_SERVERS_EXCEPT` | string | — | Comma-separated plugin names exempted from `CLAUDE_CODE_SKIP_PLUGIN_MCP_SERVERS`. |
| `CLAUDE_CODE_SYNC_PLUGINS` | bool | — | Enables syncing installed plugins from the remote plugin registry; also auto-enabled when session-ref syncing is active. Forwarded to child sessions. |
| `CLAUDE_CODE_SYNC_PLUGINS_BUFFERED_DOWNLOAD` | bool | — | Buffers plugin downloads fully in memory before writing, instead of streaming to disk. |
| `CLAUDE_CODE_SYNC_PLUGINS_INSTALL_TIMEOUT_MS` | — | `30000` | Timeout in milliseconds for installing a plugin during sync. |
| `CLAUDE_CODE_SYNC_PLUGINS_MCP_TIMEOUT_MS` | — | `10000` | Timeout in milliseconds for starting a synced plugin's MCP server. |
| `CLAUDE_CODE_SYNC_PLUGIN_INSTALL` | bool | `false` | Synchronous plugin installation |
| `CLAUDE_CODE_SYNC_PLUGIN_INSTALL_TIMEOUT_MS` | — | — | Timeout for synchronous plugin installation |
| `CLAUDE_CODE_USE_COWORK_PLUGINS` | bool | `false` | Enable cowork plugins |
| `CLAUDE_PLUGIN_DATA` | — | injected for plugin execution | Per-plugin data directory, exposed to plugin commands and templated config expansion |
| `CLAUDE_PLUGIN_ROOT` | — | injected for plugin execution | Absolute path to the active plugin root, exposed to plugin commands and templated config expansion |
| `FORCE_AUTOUPDATE_PLUGINS` | bool | `false` | Force auto-update plugins |

## Memory & Prompt History

| Variable | Type | Default | Description |
|---|---|---|---|
| `CLAUDE_CODE_ADDITIONAL_DIRECTORIES_CLAUDE_MD` | string | `false` | Scan additional directories for CLAUDE.md |
| `CLAUDE_CODE_FORCE_EVALUATE_MEMORY` | bool | — | Declared in the typed env registry but with no consumer anywhere else in the bundle — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_CODE_FORCE_MEMORY_SURVEY` | bool | — | Declared in the typed env registry but with no consumer anywhere else in the bundle — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_CODE_MEMORY_PUSH_DELETE_MODE` | enum corroborate \| immediate \| never | `corroborate` | How memory deletions are pushed upstream: "immediate" pushes at once, "corroborate" waits for confirmation from another source first, "never" suppresses deletion pushes. When unset the `tengu_mem_push_delete_mode` flag decides, itself defaulting to `corroborate`. |
| `CLAUDE_CODE_SKIP_PROMPT_HISTORY` | bool | `false` | Skip saving prompt history |
| `CLAUDE_COWORK_MEMORY_EXTRA_GUIDELINES` | string | — | Extra guidelines for cowork memory |
| `CLAUDE_COWORK_MEMORY_GUIDELINES` | string | — | Extra guideline text inserted into the cowork memory system prompt |
| `CLAUDE_COWORK_MEMORY_INDEX_CONTENT` | string | — | Override content for the cowork auto-memory index; empty string disables the index, otherwise parsed in place of the file |
| `CLAUDE_COWORK_MEMORY_PATH_OVERRIDE` | string | — | Override cowork memory path |
| `CLAUDE_MEMORY_STORES` | string | — | JSON-encoded memory-store configuration; parsed at load time and rejected with an error if it is not valid JSON (v2.1.145) |

## Idle, Resume & Background

| Variable | Type | Default | Description |
|---|---|---|---|
| `CLAUDE_AFK_COUNTDOWN_MS` | int | `20000` | Milliseconds of visible countdown shown before the AFK timeout fires; clamped to at most `CLAUDE_AFK_TIMEOUT_MS`. |
| `CLAUDE_AFK_TIMEOUT_MS` | int | `60000` | Idle milliseconds before an interactive prompt auto-resolves via the away-from-keyboard handler. Setting it also forces the AFK machinery on. |
| `CLAUDE_ASYNC_AGENT_STALL_TIMEOUT_MS` | int | `600000` | Stall timeout (ms) for async-agent turns; turns with no progress for this long are considered stalled |
| `CLAUDE_AUTO_BACKGROUND_TASKS` | bool | `false` | Auto-spawn background tasks |
| `CLAUDE_BG_AUTH_SNAPSHOT_PATH` | string | — | Path to an auth snapshot file used to seed a background job's credentials; consumed and deleted on read |
| `CLAUDE_BG_BACKEND` | string | — | Set to `daemon` to run background tasks via the daemon backend (ignores SIGHUP so the daemon survives terminal disconnects) |
| `CLAUDE_BG_CLAIM_AUTH` | string | — | One-shot auth secret passed to a background session so it can claim its slot. Deleted from the environment immediately after being read. |
| `CLAUDE_BG_ISOLATION` | string | — | Background-session isolation mode; `worktree` instructs the subagent to call `EnterWorktree` before any file or command action |
| `CLAUDE_BG_MEMORY_TOGGLED_OFF` | string | — | Presence (`!== undefined`) signals that auto-memory was toggled off for this background session; consumed and deleted from `process.env` on read (v2.1.154) |
| `CLAUDE_BG_POST_CLEAR_RESPAWN` | bool | — | Marks a background-daemon respawn that follows a reset to a new session id (as opposed to an interrupted-turn resume), suppressing the agent-not-found check on that respawn. |
| `CLAUDE_BG_PTY_AUTH` | string | — | One-shot auth secret for the background PTY channel. Deleted from the environment immediately after being read. |
| `CLAUDE_BG_RENDEZVOUS_SOCK` | string | — | Unix socket path used by background jobs to rendezvous with the parent daemon; consumed and deleted on read |
| `CLAUDE_BG_RV_AUTH` | string | — | One-shot auth secret for the background rendezvous socket. Deleted from the environment immediately after being read. |
| `CLAUDE_BG_SESSION_PERMISSION_RULES` | string | — | JSON-encoded permission rule overrides applied to a background session (`CLAUDE_CODE_SESSION_KIND=bg`); the parent injects it via `JSON.stringify(...)` and the worker consumes and deletes it from `process.env` on read (v2.1.146) |
| `CLAUDE_BG_SOCKET_TOKENS_PATH` | string | — | Path to the file holding background-session socket tokens. Deleted from the environment immediately after being read. |
| `CLAUDE_BG_SOURCE` | string | `shell` | Origin label for a background session (e.g. `shell`); stripped before passing env to children |
| `CLAUDE_BG_STARTUP_WEDGE_MS` | int | `45000` | Milliseconds to delay (`unref`'d timer) before the background job's startup wedge fires |
| `CLAUDE_CODE_DAEMON_COLD_START` | bool | — | Overrides daemon cold-start behavior; accepts `transient` (spawn for this login) or `ask` (prompt to install persistently) |
| `CLAUDE_CODE_IDLE_THRESHOLD_MINUTES` | int | `75` | Minutes of idle time before idle behavior triggers |
| `CLAUDE_CODE_IDLE_TOKEN_THRESHOLD` | int | `100000` | Token threshold for idle detection |
| `CLAUDE_CODE_LOOP_PERSISTENT` | bool | — | Truthy enables persistent loop behavior in long-running session loops |
| `CLAUDE_CODE_RESUME_INTERRUPTED_TURN` | bool | — | Resume an interrupted turn/session by identifier or token |
| `CLAUDE_CODE_RESUME_INTERRUPTED_TURN_MAX_AGE_MS` | string | `3600000` | Maximum age in milliseconds of an interrupted turn that is still eligible for auto-resume; older turns are discarded. |
| `CLAUDE_CODE_RESUME_PROMPT` | string | `Continue from where you left off.` | Prompt text injected when a session is resumed via `--resume` |
| `CLAUDE_CODE_RESUME_SOURCE_ALIVE` | string | — | Descriptor telling a resumed session whether its source process is still alive, so it can decide between takeover and cooperation. |
| `CLAUDE_CODE_RESUME_THRESHOLD_MINUTES` | int | `70` | Minutes threshold for resume-interrupted-turn eligibility |
| `CLAUDE_CODE_RESUME_TOKEN_THRESHOLD` | int | `100000` | Token threshold for resume-interrupted-turn eligibility |
| `CLAUDE_CODE_SESSION_ID` | string | — | Current session UUID; re-randomized on `/clear`-style resets and propagated to child processes |
| `CLAUDE_CODE_SESSION_KIND` | string | — | Session classification; `bg` marks a background-agent session and forces full-screen / bg-specific code paths |
| `CLAUDE_CODE_SESSION_LOG` | string | — | Path to write the session log file for the current session |
| `CLAUDE_CODE_SESSION_NAME` | string | — | Human-readable session label recorded on session metadata |
| `CLAUDE_CODE_SPAWN_TIMESTAMP_MS` | int | — | Spawn timestamp (ms) propagated to a child/background process; parsed via `parseInt(..., 10)` and used as a fallback spawn time (v2.1.154) |
| `CLAUDE_JOB_DIR` | string | — | Directory representing the current background job; basename is used as the `jobId` in session metadata |

## Miscellaneous

| Variable | Type | Default | Description |
|---|---|---|---|
| `AWS_LAMBDA_BENCHMARK_MODE` | — | `false` | AWS SDK benchmark/testing flag observed in the bundled dependencies |
| `AWS_LAMBDA_NODEJS_NO_GLOBAL_AWSLAMBDA` | — | `false` | AWS Lambda Node.js runtime flag; when `"true"`/`"1"` suppresses the `awslambda` global that the bundled Lambda runtime adapter would otherwise install |
| `BUF_BIGINT_DISABLE` | — | — | Read by the bundled `@bufbuild/protobuf` runtime; set to `1` to disable the native BigInt DataView codec path for 64-bit protobuf fields. |
| `BUN_CHROME_PATH` | string | — | Declared in the typed env registry but with no consumer anywhere else in the bundle (verified by both string-literal and minified-symbol search) — reserved or dead at this tag; setting it has no effect. |
| `CI` | string | — | Checked alongside vendor markers (`GITHUB_ACTIONS`, `CIRCLECI`, …) to pick the ANSI color level assumed for CI output. |
| `CLAUBBIT` | bool | `false` | Running in Claubbit mode |
| `CLAUDECODE` | bool | — | Set to `1` in spawned processes |
| `CLAUDE_AFTER_LAST_COMPACT` | string | `false` | Flag set after last compaction |
| `CLAUDE_AGENTS_AUTO_RELAUNCHED_AT` | — | — | Unix-ms timestamp of the last auto-relaunch into the agents launcher after an update; throttles repeat auto-relaunches to at most once per 6 hours. |
| `CLAUDE_AX_SCREEN_READER` | tri-bool | — | Tri-state accessibility override forcing screen-reader mode on or off instead of relying on autodetection. |
| `CLAUDE_BASH_MAINTAIN_PROJECT_WORKING_DIR` | bool | `false` | Maintain working directory between bash calls |
| `CLAUDE_BRIDGE_REATTACH_OUTBOUND_ONLY` | bool | — | Set to `"1"` by the parent to instruct a re-attaching Chrome-bridge connection to operate outbound-only; consumed and deleted from `process.env` on read (v2.1.154) |
| `CLAUDE_BYTE_STREAM_IDLE_TIMEOUT_MS` | — | — | Milliseconds a response byte stream may stall before the byte watchdog aborts it. |
| `CLAUDE_CHROME_CLASSIFIER_FLOOR` | tri-bool | — | Tri-state override for the Chrome automode classifier floor; unset defers to the tengu_cowork_chrome_automode_default flag. |
| `CLAUDE_CHROME_PERMISSION_MODE` | string | — | Permission mode for Chrome integration |
| `CLAUDE_CLIENT_PRESENCE_FILE` | string | — | Path to the presence file used to detect whether a client UI is currently attached. |
| `CLAUDE_CODE_3P_PROBE_WROTE_OPUS_DEFAULT` | string | — | Records the Opus model id a third-party provider probe wrote as the default, so a later change to `ANTHROPIC_DEFAULT_OPUS_MODEL` can be told apart from the probe's own value. |
| `CLAUDE_CODE_3P_PROBE_WROTE_SONNET_DEFAULT` | string | — | Records the Sonnet model id a third-party provider probe wrote as the default, used the same way as the Opus counterpart. |
| `CLAUDE_CODE_ACCOUNT_TAGGED_ID` | string | — | Override the tagged account identifier emitted in OpenTelemetry metric attributes |
| `CLAUDE_CODE_ACTION` | string | `false` | Running as a GitHub Action |
| `CLAUDE_CODE_ACT_DONT_REDERIVE` | tri-bool | `true` | Tri-state override for the `tengu_cedar_lantern` experiment controlling the "act, don't re-derive" system-prompt section. Enabled by default. |
| `CLAUDE_CODE_AGENT_VIEW_RELAUNCH` | — | — | One-shot internal marker set to `1` when the CLI relaunches itself into the agents view; read once then deleted from the environment. |
| `CLAUDE_CODE_ARTIFACT` | string | — | Tri-state override for the Artifact tool: truthy force-enables it, an explicitly false value disables it, unset defers to defaults and settings. |
| `CLAUDE_CODE_ARTIFACTS_API_BASE_URL` | string | — | Base URL override for the Artifacts API; part of the recognized base-URL set. |
| `CLAUDE_CODE_ARTIFACT_AUTO_OPEN` | string | — | Controls whether a created artifact opens automatically. Auto-open is skipped regardless for background, teammate, remote, desktop-pane and VS Code sessions. |
| `CLAUDE_CODE_ARTIFACT_DIRECT_UPLOAD` | bool | — | Enables direct upload of artifact content instead of the default proxied path; otherwise the `tengu_cobalt_plinth_direct` experiment decides (default on). |
| `CLAUDE_CODE_ATTRIBUTION_HEADER` | string | — | Custom attribution header |
| `CLAUDE_CODE_AUTO_MODE_EXTERNAL_PERMISSIONS` | bool | — | Declared in the typed env registry but with no consumer anywhere else in the bundle — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_CODE_AUTO_MODE_MODEL` | string | — | Declared in the typed env registry but with no consumer anywhere else in the bundle — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_CODE_BASE_REF` | string | — | Override base git ref for diffs |
| `CLAUDE_CODE_BASE_REFS` | string | — | Override base git refs (comma-separated list) |
| `CLAUDE_CODE_BG_CLASSIFIER_MODEL` | string | — | Declared in the typed env registry but with no consumer anywhere else in the bundle — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_CODE_BG_TASKS_REPORT_RUNNING` | bool | — | Controls whether active teammates, running background tasks or pending notifications keep the session reported as running rather than idle. |
| `CLAUDE_CODE_BRIDGE_SESSION_ID` | string | — | Session id for a bridge-attached session. Written by the parent and removed when the bridge detaches. |
| `CLAUDE_CODE_CHILD_SESSION` | bool | — | Marks the process as a child session spawned by another Claude Code instance (for example in a tmux pane), which suppresses duplicate transcript persistence. Also read back out of the tmux global environment. |
| `CLAUDE_CODE_DD_ERROR_TRACKING_FLUSH_INTERVAL_MS` | — | `30000` | Flush interval in milliseconds for batched Datadog error-tracking reports; must be at least 1, and unparseable values fall back to 30000. |
| `CLAUDE_CODE_DESIGN_OAUTH_CLIENT_ID` | string | — | OAuth client id for the Claude Design integration; required when the build has none baked in. |
| `CLAUDE_CODE_DONT_INHERIT_ENV` | bool | `false` | Don't inherit env vars in spawned processes |
| `CLAUDE_CODE_EAGER_FLUSH` | bool | `false` | Flush persisted session data eagerly after writes |
| `CLAUDE_CODE_ENHANCED_TELEMETRY_BETA` | bool | inherits `ENABLE_ENHANCED_TELEMETRY_BETA` when unset | Alternate flag name for enhanced telemetry beta |
| `CLAUDE_CODE_ENVIRONMENT_KIND` | string | — | Environment kind label (e.g. `bridge`) |
| `CLAUDE_CODE_EXECPATH` | — | — | Path to the Claude Code executable, exported so child processes and shell integrations can re-invoke the same binary. |
| `CLAUDE_CODE_EXIT_AFTER_FIRST_RENDER` | bool | `false` | Exit after the first UI render; useful for smoke tests and harnesses |
| `CLAUDE_CODE_EXIT_AFTER_STOP_DELAY` | int | — | Delay process exit after stop/termination logic |
| `CLAUDE_CODE_EXPERIMENTAL_OBSERVER_AGENTS` | bool | — | Gate for experimental observer agents, combined with the `tengu_observer_agents_enabled` experiment (default on). |
| `CLAUDE_CODE_EXTRA_BODY` | string | — | Extra JSON body to include in API requests |
| `CLAUDE_CODE_EXTRA_METADATA` | string | — | JSON object merged into the emitted `user_id` telemetry payload |
| `CLAUDE_CODE_FLEETVIEW_SIMPLE` | bool | — | Forces on the `tengu_fleetview_simple` experiment, switching FleetView to a simplified UI. |
| `CLAUDE_CODE_FORCE_BRIDGE` | bool | — | Declared in the typed env registry but with no consumer anywhere else in the bundle — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_CODE_FORCE_MID_CONVERSATION_SYSTEM` | bool | — | Truthy forces injection of the mid-conversation system prompt regardless of the usual gating (companion to `CLAUDE_CODE_MID_CONVERSATION_SYSTEM`) (v2.1.154) |
| `CLAUDE_CODE_FORCE_SESSION_PERSISTENCE` | bool | — | Forces the session transcript to keep persisting even where it would otherwise be turned off. |
| `CLAUDE_CODE_FORCE_STRIKETHROUGH` | bool | — | Forces markdown strikethrough rendering, overriding the autodetection that disables it for Apple Terminal and the Linux console. |
| `CLAUDE_CODE_FORCE_WINDOWS_CREDMAN` | string | — | Declared in the typed env registry but with no consumer anywhere else in the bundle — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_CODE_FORWARD_SUBAGENT_TEXT` | bool | — | Forwards subagent text and thinking blocks as assistant/user messages carrying `parent_tool_use_id`. Equivalent to `--forward-subagent-text`. |
| `CLAUDE_CODE_FRAME_TIMING_SAMPLE_EVERY` | int | `1` | Sample every Nth frame for frame-timing instrumentation; values below 1 are clamped to 1. |
| `CLAUDE_CODE_GB_BASE_URL` | string | — | Declared in the typed env registry but with no consumer anywhere else in the bundle — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_CODE_GB_DISK_CACHE_WHEN_TELEMETRY_OFF` | bool | — | Allows GrowthBook flags to be cached on disk even when telemetry is disabled. Only takes effect when DISABLE_GROWTHBOOK is not set. |
| `CLAUDE_CODE_GB_REFRESH_INTERVAL_MS` | int | — | Declared in the typed env registry but with no consumer anywhere else in the bundle — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_CODE_GZIP_REQUEST_BODIES` | tri-bool | — | Tri-state override for the `tengu_gzip_request_bodies` experiment controlling gzip compression of outgoing request bodies (default off). |
| `CLAUDE_CODE_HFI_BEARER_TOKEN` | string | — | Declared in the typed env registry but with no consumer anywhere else in the bundle — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_CODE_HIDE_SETTINGS_HINT` | string | — | Suppresses the settings hint line for entrypoints that are not already excluded from it. |
| `CLAUDE_CODE_HOST_CREDS_FILE` | string | — | Path to a host-provided credentials file, used when `CLAUDE_CODE_PROVIDER_MANAGED_BY_HOST` is set. Ignored with a warning if the file's permissions or owner are wrong. Redacted from logs. |
| `CLAUDE_CODE_INVESTIGATE_FIRST` | bool | — | Controls the "investigate first" prelude mode; accepts `additive`, `compact`, or a boolean-style toggle |
| `CLAUDE_CODE_INVOKED_SKILLS` | — | — | Present in the allowlist of variables forwarded to Bash-tool shell-snapshot subprocesses, but nothing in this bundle writes its value — treat it as a passthrough slot rather than a knob. |
| `CLAUDE_CODE_IS_COWORK` | bool | `false` | Mark the session as cowork/bridge mode for eager flushing and related behaviors |
| `CLAUDE_CODE_JSONL_TRANSCRIPT` | string | — | Declared in the typed env registry but with no consumer anywhere else in the bundle — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_CODE_KB_COHESION_FIXES` | bool | — | Enables keybinding-cohesion fixes so custom `app:interrupt` / `app:exit` bindings are honored instead of falling back to hardcoded Ctrl-C / Ctrl-D. |
| `CLAUDE_CODE_LOOP_KEEPALIVE` | bool | — | Forces on the `tengu_kairos_loop_keepalive` experiment (default off), keeping the agentic loop alive between iterations. |
| `CLAUDE_CODE_MANAGED_SETTINGS_PATH` | string | — | Overrides the directory managed (enterprise policy) settings are read from. Set internally for sandboxed subprocesses. |
| `CLAUDE_CODE_MAX_SUBAGENTS_PER_SESSION` | — | `200` | Caps how many subagents (Task tool invocations) a single session may spawn. Settings-injectable. |
| `CLAUDE_CODE_MAX_WEB_SEARCHES_PER_SESSION` | — | `200` | Caps how many WebSearch calls a single session may make. Settings-injectable. |
| `CLAUDE_CODE_MCP_AUTO_BACKGROUND_MS` | int | `120000` | Milliseconds after which a slow MCP tool call is automatically backgrounded, clamped to [0, 2147483647]. Settings-injectable. |
| `CLAUDE_CODE_MCP_TOOL_IDLE_TIMEOUT` | int | `1800000` stdio / `300000` other | Idle milliseconds before an MCP tool call with no response or progress is aborted; the default differs by transport and `0` disables the timeout. Settings-injectable. |
| `CLAUDE_CODE_NEW_INIT` | bool | `false` | Use new init flow |
| `CLAUDE_CODE_NO_MODEL_FALLBACK` | bool | — | When exactly true, disables automatic model substitution — an attempted fallback pivot throws a tripwire error instead of quietly switching models. |
| `CLAUDE_CODE_OTEL_DIAG_STDERR` | bool | — | Writes OpenTelemetry diagnostic output to stderr. Its presence, like any OTEL_* variable, also marks the process as telemetry-configured. |
| `CLAUDE_CODE_PACKAGE_MANAGER_AUTO_UPDATE` | bool | — | Truthy opts the installer into automatic package-manager-driven self-updates |
| `CLAUDE_CODE_PERFETTO_WRITE_INTERVAL_S` | int | — | Declared in the typed env registry but with no consumer anywhere else in the bundle — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_CODE_PERFORCE_MODE` | string | `false` | Enable Perforce-aware code paths (extra safeguards/adaptations for Perforce workspaces) |
| `CLAUDE_CODE_POWERUP_ONBOARDING` | string | — | Forces the powerup onboarding presentation to `banner` or `step`. Any other value defers to the `tengu_birch_lantern` experiment (default off); onboarding also shows when the user has not completed onboarding. |
| `CLAUDE_CODE_PROACTIVE` | bool | — | Truthy turns on the proactive/Kairos assistant mode in the TUI |
| `CLAUDE_CODE_PROCESS_WRAPPER` | — | — | Corporate-launcher argv prefix used to re-exec the background-agent supervisor and its sessions/workers. Overrides the `processWrapper` setting and is forwarded to child processes. |
| `CLAUDE_CODE_PROFILE_QUERY` | bool | — | Enables query profiling output; without it a "profiling not enabled" message is shown instead. |
| `CLAUDE_CODE_PROVIDER_MANAGED_BY_HOST` | bool | `false` | Provider credentials are managed by the host application |
| `CLAUDE_CODE_QUESTION_PREVIEW_FORMAT` | string | `markdown` outside SDK mode | Format for question previews: `markdown` or `html` |
| `CLAUDE_CODE_REFUSAL_FALLBACK_CATCH_ALL` | tri-bool | `false` | Tri-state controlling whether the refusal-fallback router applies a catch-all retry when no specific refusal-category route matches. |
| `CLAUDE_CODE_RELAUNCH_TERMINAL_SIZE` | string | — | A `COLSxROWS` string carrying terminal dimensions across a relaunch, consumed once and then removed from the environment. |
| `CLAUDE_CODE_REPL` | tri-bool | — | Enable REPL mode |
| `CLAUDE_CODE_REPORT_FINDINGS` | bool | — | Gates the ReportFindings-specific system-prompt instructions, and only for agents whose tool set already includes the ReportFindings tool. |
| `CLAUDE_CODE_REPO_CHECKOUTS` | string | — | Repository checkout paths |
| `CLAUDE_CODE_SAFE_MODE` | bool | — | Disables all user customizations at once — CLAUDE.md, skills, plugins, hooks, MCP servers, custom commands and agents, themes and keybindings. Equivalent to `--safe-mode`; also re-set to `1` on respawn so the mode survives relaunch. |
| `CLAUDE_CODE_SEND_FEEDBACK` | tri-bool | — | Tri-state gate for the in-product feedback feature (the `tengu_juniper_relay` experiment); an explicit false disables it outright. |
| `CLAUDE_CODE_SESSIONEND_HOOKS_TIMEOUT_MS` | int | — | Timeout for session-end hooks (ms) |
| `CLAUDE_CODE_SIMPLE_SYSTEM_PROMPT` | string | — | Truthy forces the simplified system prompt; falsy explicitly disables it |
| `CLAUDE_CODE_SKIP_ANTHROPIC_GOOGLE_CLOUD_AUTH` | bool | — | Skips the Anthropic-on-Google-Cloud credential flow, assuming auth is supplied out of band. Settings-injectable. |
| `CLAUDE_CODE_SKIP_AWS_CRED_CACHE` | bool | — | Disables caching of resolved AWS credentials, forcing the credential chain to run on every request. |
| `CLAUDE_CODE_SKIP_FAST_MODE_NETWORK_ERRORS` | bool | `false` | Skip network errors in fast mode |
| `CLAUDE_CODE_SKIP_FAST_MODE_ORG_CHECK` | bool | `false` | Skip organization check for fast mode eligibility |
| `CLAUDE_CODE_SKIP_HFI_VERSION_CHECK` | bool | — | Declared in the typed env registry but with no consumer anywhere else in the bundle — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_CODE_SKIP_PROJECT_BACKFILL` | bool | — | Declared in the typed env registry but with no consumer anywhere else in the bundle — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_CODE_SKIP_REPO_UPLOAD` | bool | — | Declared in the typed env registry but with no consumer anywhere else in the bundle — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_CODE_STOP_HOOK_BLOCK_CAP` | int | — | Raises the cap on how many consecutive times a Stop/SubagentStop hook may block a turn before Claude Code overrides it and ends the turn; parsed as an integer (v2.1.145) |
| `CLAUDE_CODE_SUBAGENT_CACHE_EVICT` | bool | — | Forces on the `tengu_subagent_cache_evict` experiment, enabling eviction of subagent prompt-cache entries. Requires prompt-cache diagnostics to be active. |
| `CLAUDE_CODE_SUBPROCESS_ENV_SCRUB` | tri-bool | `false` | Scrub a built-in set of sensitive credentials from inherited subprocess environments before spawning child processes |
| `CLAUDE_CODE_SUPERVISED` | bool | — | Truthy marks the process as supervised; uncaught exceptions/rejections exit instead of being swallowed |
| `CLAUDE_CODE_SUPPRESS_SESSION_ATTRIBUTION` | bool | — | Suppresses the session-URL attribution otherwise attached to outbound content such as commit trailers. |
| `CLAUDE_CODE_SYNC_SESSION_REFS` | bool | — | Enables remote syncing of session reference data, gated on a session-persistence path being configured. |
| `CLAUDE_CODE_SYNC_SKILLS_INSTALL_TIMEOUT_MS` | int | `30000` | Timeout in milliseconds for installing a skill during skill sync. |
| `CLAUDE_CODE_TAGS` | string | — | Tags for telemetry |
| `CLAUDE_CODE_TAG_ISMETA_MESSAGES` | bool | — | Declared in the typed env registry but with no consumer anywhere else in the bundle — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_CODE_TASK_LIST_ID` | string | — | Task list ID override |
| `CLAUDE_CODE_TERMINAL_MCP_TOOLS` | string | `""` | Comma-separated MCP tool names that get special inline terminal rendering of their tool_use output; blank entries ignored. |
| `CLAUDE_CODE_TERMINAL_RECORDING` | string | — | Declared in the typed env registry but with no consumer anywhere else in the bundle — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_CODE_TODO_REMINDER_MODE` | enum baseline \| `baseline` | Overrides the `tengu_soft_slate_nudge` experiment for the todo-list reminder in the system prompt: `baseline` keeps it, `off` removes it. | Todo reminder behavior: "baseline" emits the standard reminders, "off" suppresses them. |
| `CLAUDE_CODE_TOTAL_TOKENS_REMINDER` | string | — | Selects the total-tokens reminder mode — `off`, `infinite`, `fixed`, `countdown` or `padded-countdown` — which emits a `<total_tokens>N tokens left</total_tokens>` block into the prompt. |
| `CLAUDE_CODE_TOTAL_TOKENS_REMINDER_AFTER_USER_TURN` | tri-bool | — | Tri-state control over whether the total-tokens reminder block is also emitted after each regular user turn. |
| `CLAUDE_CODE_TOTAL_TOKENS_REMINDER_BUDGET` | int | `15000000` | Starting token budget used by the total-tokens reminder's `padded-countdown` mode. |
| `CLAUDE_CODE_TRANSCRIPT_LOCAL_GC` | tri-bool | — | Tri-state override for the `tengu_transcript_local_gc` experiment controlling local garbage collection of old transcript files (default off). |
| `CLAUDE_CODE_TRIGGER_ID` | string | — | Identifier of the trigger that started this session; attached to telemetry as trigger_id. |
| `CLAUDE_CODE_TWO_STAGE_CLASSIFIER` | bool | — | Declared in the typed env registry but with no consumer anywhere else in the bundle — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_CODE_USE_ANTHROPIC_GOOGLE_CLOUD` | bool | — | Routes requests through the Anthropic-on-Google-Cloud provider. One of the mutually exclusive provider selectors. Settings-injectable. |
| `CLAUDE_CODE_USE_GATEWAY` | bool | — | Routes API traffic through the Anthropic cloud gateway. Requires both `ANTHROPIC_BASE_URL` and `ANTHROPIC_AUTH_TOKEN`; without them it is ignored with a warning. Gateway session expiry tells you to refresh the token rather than run /login. Settings-injectable. |
| `CLAUDE_CODE_VOICE_FORWARD_INTERIMS_TYPED` | bool | — | Truthy forwards interim voice transcription results as typed input |
| `CLAUDE_CODE_WORKER_EPOCH` | int | — | Worker epoch for process management |
| `CLAUDE_CODE_WORKSPACE_HOST_PATHS` | string | — | Pipe-separated host workspace paths attached to telemetry events |
| `CLAUDE_CONTEXT_COLLAPSE` | bool | — | Declared in the typed env registry but with no consumer anywhere else in the bundle (verified by both string-literal and minified-symbol search) — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_CONTEXT_COLLAPSE_MODEL` | string | — | Declared in the typed env registry but with no consumer anywhere else in the bundle (verified by both string-literal and minified-symbol search) — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_DISABLE_ADOPT` | bool | — | Disables adopting an existing session; when unset adoption is enabled. |
| `CLAUDE_EFFORT` | — | — | Injected into hook and Bash subprocess environments (and `${CLAUDE_EFFORT}` template substitutions) with the turn's active reasoning effort: `low`, `medium`, `high`, `xhigh` or `max`. |
| `CLAUDE_FORCE_DISPLAY_SURVEY` | bool | `false` | Force the feedback survey to appear when the user is otherwise eligible |
| `CLAUDE_GATEWAY_ALLOW_LOOPBACK` | bool | — | Allows the cloud gateway to accept loopback addresses, which is otherwise refused. |
| `CLAUDE_IMPORT_CONVERSATIONS` | bool | — | Truthy enables importing prior conversations on startup (gated by `!xH(process.env.CLAUDE_IMPORT_CONVERSATIONS)`) (v2.1.154) |
| `CLAUDE_INTERNAL_FC_OVERRIDES` | string | — | Intended to carry JSON-encoded GrowthBook feature-flag overrides, but **dead code at this tag**: the reading function returns unconditionally before it ever reaches the `process.env` read, so the value is never parsed. |
| `CLAUDE_LOCAL_OAUTH_API_BASE` | string | `http://localhost:8000` | Local OAuth API base URL |
| `CLAUDE_LOCAL_OAUTH_APPS_BASE` | string | `http://localhost:4000` | Local OAuth apps base URL |
| `CLAUDE_LOCAL_OAUTH_CONSOLE_BASE` | string | `http://localhost:3000` | Local OAuth console base URL |
| `CLAUDE_PID` | — | — | The CLI's own process id (`String(process.pid)`), injected into child and hook subprocess environments. |
| `CLAUDE_PREVIEW_CLASSIFIER_FLOOR` | tri-bool | `false` | Tri-state override for the preview classifier floor; unset is treated as false. |
| `CLAUDE_PROJECT_DIR` | — | — | Project root path, set on hook and subprocess environments and substituted for `${CLAUDE_PROJECT_DIR}` in hook commands. Preserved when the environment is scrubbed for child processes. |
| `CLAUDE_PROJECT_UUID` | string | — | Associates the working directory with a claude.ai Project by UUID so that Project's context is fetched and synced. The value is trimmed; an empty string counts as unset. |
| `CLAUDE_REPL_VARIANT` | string | — | REPL variant identifier |
| `CLAUDE_RUNNER_ACTIVITY_FD` | — | — | File descriptor number the runner writes activity heartbeats to, so the supervising process can tell the run is alive. |
| `CLAUDE_RUNNER_DISABLE_AWAITING_ACTION_OVERRIDE` | bool | — | Declared in the typed env registry but with no consumer anywhere else in the bundle (verified by both string-literal and minified-symbol search) — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_RUNNER_FETCH_DEPTH` | string | — | Declared in the typed env registry but with no consumer anywhere else in the bundle (verified by both string-literal and minified-symbol search) — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_SDK_CAN_USE_TOOL_SHADOWED` | — | — | **Not an environment variable.** This is the `code` of a Node process warning emitted when bare `allowedTools` entries shadow the SDK `canUseTool` callback. Listed here only because the name appears env-var-shaped; setting it does nothing. |
| `CLAUDE_SNIP` | string | — | Declared in the typed env registry but with no consumer anywhere else in the bundle (verified by both string-literal and minified-symbol search) — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_SSH_LOCAL_BINARY` | string | — | Declared in the typed env registry but with no consumer anywhere else in the bundle (verified by both string-literal and minified-symbol search) — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_SSH_VERSION` | string | — | Declared in the typed env registry but with no consumer anywhere else in the bundle (verified by both string-literal and minified-symbol search) — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_STAGE_FILE_ROOT` | string | `/mnt/user-data/uploads` | Staging root that file-write tools use for staging-kind writes instead of the real filesystem (e.g. remote or read-only-FS mode). Must be an absolute path — a relative value throws. |
| `CLAUDE_WORKFLOW_NAME_ONLY` | bool | — | Restricts the Workflow tool to named bundled workflows, blocking `workflow({scriptPath})` invocations. |
| `COREPACK_ENABLE_AUTO_PIN` | — | forced to `0` | Corepack auto-pin is disabled by Claude Code at startup |
| `DATABASE_URL` | string | — | Not read by the CLI. It appears only as an example row ("Postgres connection string") inside an embedded documentation template, plus a declaration in the typed env registry. Setting it has no effect. |
| `DEMO_VERSION` | string | — | Demo mode version string |
| `DO_NOT_TRACK` | string | — | Standard signal to disable telemetry (respects the do-not-track convention) |
| `DS_CHROMIUM_PATH` | — | — | Overrides the Chromium `executablePath` Puppeteer uses when the bundled design-agent renders Storybook component previews. |
| `ENVIRONMENT_SERVICE_KEY` | string | — | Declared in the typed env registry but with no consumer anywhere else in the bundle (verified by both string-literal and minified-symbol search) — reserved or dead at this tag; setting it has no effect. |
| `FALLBACK_FOR_ALL_PRIMARY_MODELS` | string | — | Enable fallback for all primary models |
| `FORCE_HYPERLINK` | — | — | Standard supports-hyperlinks override. Its mere presence in the environment forces terminal hyperlink support on, regardless of value. |
| `GCM_INTERACTIVE` | string | `never` (when unset) | Git Credential Manager interactivity. Claude Code injects `never` into git subprocess environments only when the variable is undefined, suppressing GCM prompts while preserving an explicit value. |
| `GIT_ASKPASS` | string | `""` (when unset) | Program git uses to prompt for credentials. Claude Code injects an empty string into git subprocess environments only when the variable is undefined, disabling the askpass GUI helper without clobbering an existing setting. |
| `GIT_SSH_COMMAND` | string | `ssh` | SSH command git uses. Claude Code wraps it (defaulting to `ssh`) with `-o BatchMode=yes -o StrictHostKeyChecking=yes`, may route it through the sandbox SOCKS proxy, and uses its presence to gate agent-proxy GitHub URL rewriting. |
| `GIT_TERMINAL_PROMPT` | string | `0` (when unset) | Controls whether git may prompt on the terminal. Claude Code injects `0` into git subprocess environments only when the variable is undefined, so an explicit value you set is preserved. |
| `HISTFILE` | string | — | Shell history file path, read only behind explicit user opt-in during an environment/context-gathering scan. |
| `INK_SCREEN_READER` | bool | — | Forces the renderer into screen-reader mode; only honored when stdout is a TTY. |
| `JAVA_HOME` | string | — | Java installation root; used to locate `keytool` when a Java trust store must be modified. |
| `JAVA_TOOL_OPTIONS` | string | — | Standard JVM options variable. Claude Code appends `-Dhttps.proxyHost` / trust-store options to any existing value when propagating a detected proxy or custom CA, rather than replacing it. |
| `LOCAL_BRIDGE` | bool | `false` | Alias flag that enables local OAuth / local bridge behavior |
| `MAX_STRUCTURED_OUTPUT_RETRIES` | int | `5` | Maximum retries for structured-output validation loops |
| `MODE` | — | — | Not read from `process.env` by the CLI at this tag — it appears only inside an embedded esbuild `define` map (`import.meta.env.MODE`) for the bundled Storybook-to-image sub-bundler. Setting it has no effect on Claude Code. |
| `MODIFIERS_NODE_PATH` | — | — | Path for modifiers node |
| `NODE_ENV` | — | — | Standard Node environment name; read by bundled dependencies and forwarded to child processes as a recognized build variable. |
| `NoDefaultCurrentDirectoryInExePath` | — | forced to `1` | Windows security: prevent current directory in exe path resolution |
| `PLAYWRIGHT_BROWSERS_PATH` | string | — | Declared in the typed env registry but with no consumer anywhere else in the bundle (verified by both string-literal and minified-symbol search) — reserved or dead at this tag; setting it has no effect. |
| `SCREENSHOT_DIR` | — | `/tmp/shots` | Output directory for screenshots captured by bundled helper/skill code (v2.1.145) |
| `SYSTEMROOT` | string | `C:\Windows` | Windows system root, used to locate system executables such as cmd.exe. Falls back to C:\Windows. |
| `TEMP` | string | — | Windows temporary directory; part of the Windows env allowlist forwarded to child processes. |
| `TEST_ENABLE_SESSION_PERSISTENCE` | bool | `false` | Enable session persistence in test mode |
| `ULTRAPLAN_PROMPT_FILE` | string | — | Declared in the typed env registry but with no consumer anywhere else in the bundle (verified by both string-literal and minified-symbol search) — reserved or dead at this tag; setting it has no effect. |
| `USE_LOCAL_OAUTH` | bool | `false` | Use local OAuth / local bridge endpoints instead of the production OAuth flow |
| `USE_STAGING_OAUTH` | bool | `false` | Use staging OAuth |
| `VCR_RECORD` | bool | — | VCR recording mode for CI test replays |
| `VOICE_STREAM_BASE_URL` | string | — | WebSocket URL for voice streaming |
| `all_proxy` | string | — | Lowercase generic proxy URL, checked as a fallback by bundled HTTP clients when no protocol-specific proxy is set. |

## Experiment & Feature Gates

Each of these forces on a server-side experiment gate that would otherwise be decided by a `tengu_*` flag. Most inject or remove a specific system-prompt section, so they are the closest thing the bundle has to a switchboard for model behavior.

| Variable | Type | Default | Description |
|---|---|---|---|
| `CLAUDE_CODE_AMBER_ASTROLABE` | bool | — | Forces on the `tengu_amber_astrolabe` experiment, which injects system-prompt guidance telling autonomous sessions to act without asking for confirmation. |
| `CLAUDE_CODE_BASALT_COVE` | bool | — | Forces on the internal `basalt_cove` feature override, bypassing the per-item config-override checklist that normally gates it. |
| `CLAUDE_CODE_BISON_CAIRN` | bool | — | Forces on the `tengu_bison_cairn` experiment, which adds the "delivering work" system-prompt section. |
| `CLAUDE_CODE_GAULT_KESTREL` | bool | — | Forces on the `tengu_gault_kestrel` experiment, which adds system-prompt guidance to confirm before hard-to-reverse or outward-facing actions. |
| `CLAUDE_CODE_GORSE_PLOVER` | bool | — | Forces on the `tengu_gorse_plover` experiment, which adds system-prompt guidance that commands are cheap to run so the agent should just run one. |
| `CLAUDE_CODE_HERON_TALLOW` | bool | — | Forces on the `tengu_heron_tallow` experiment. |
| `CLAUDE_CODE_LANTERN_PRISM` | bool | — | Forces on the `tengu_lantern_prism` experiment, gating an internal UI feature tied to plugin display (paired with `tengu_walnut_spire`). |
| `CLAUDE_CODE_LARCH_CISTERN` | bool | — | Forces on the `tengu_larch_cistern` experiment, which adds the "overcorrection" system-prompt section. |
| `CLAUDE_CODE_MARL_CORMORANT` | bool | — | Forces on the `tengu_marl_cormorant` experiment, which adds a system-prompt note that command output is shown to the agent but not reliably to the user. |
| `CLAUDE_CODE_NANKEEN_KESTREL` | bool | — | Forces on the `tengu_nankeen_kestrel` experiment, which enables Windows sandbox support for the Bash tool. |
| `CLAUDE_CODE_PEWTER_OWL` | tri-bool | — | Tri-state override for the pewter-owl experiment family (brief, model-scoped tool descriptions), overriding the per-model rollout. |
| `CLAUDE_CODE_PEWTER_OWL_TOOL` | tri-bool | — | Tri-state override for the `pewter_owl_tool` sub-experiment specifically; falls back to `CLAUDE_CODE_PEWTER_OWL` and then the flag when unset. |
| `CLAUDE_CODE_THISTLE_GREBE` | string | — | Overrides the model-steering mode — `default`, `no_nudges` or `counter_steer` — that otherwise comes from client data or a GrowthBook experiment. |
| `CLAUDE_CODE_WALNUT_SPIRE` | bool | — | Forces on the `tengu_walnut_spire` experiment, gating an internal UI feature (paired with `tengu_lantern_prism`). |

## Testing & Development Hooks

| Variable | Type | Default | Description |
|---|---|---|---|
| `BUGHUNTER_DEV_BUNDLE_B64` | — | — | Base64-encoded bughunter dev bundle |
| `BUGHUNTER_FLEET_SIZE` | — | `5` | Number of parallel Bug Hunter subagents to launch; default sourced from GrowthBook (`fleet_size`, capped 5–20) |
| `CLAUDE_CODE_DOWNLOAD_DEADLINE_MS_FOR_TESTING` | string | `600000` | Test-only override of the download deadline in milliseconds. |
| `CLAUDE_CODE_FORCE_TIP_ID` | string | — | Declared in the typed env registry but with no consumer anywhere else in the bundle — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_CODE_MOCK_REMOTE_SETTINGS` | bool | — | Supplies mock remote settings instead of fetching them; part of the proxy/settings test env set. |
| `CLAUDE_CODE_MOCK_TRIAL` | bool | — | Declared in the typed env registry but with no consumer anywhere else in the bundle — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_CODE_OVERRIDE_DATE` | string | — | Declared in the typed env registry but with no consumer anywhere else in the bundle — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_CODE_STALL_TIMEOUT_MS_FOR_TESTING` | string | test default | Override the internal stall timeout in test harnesses |
| `CLAUDE_CODE_TEST_FIXTURES_ROOT` | string | current workspace | Override the root directory used for JSON test fixtures |
| `CLAUDE_CODE_TEST_FORCE_DENY` | bool | — | Declared in the typed env registry but with no consumer anywhere else in the bundle — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_CODE_TEST_NO_GIT_BASH` | string | — | Declared in the typed env registry but with no consumer anywhere else in the bundle — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_CODE_TEST_NO_PWSH` | string | — | Declared in the typed env registry but with no consumer anywhere else in the bundle — reserved or dead at this tag; setting it has no effect. |
| `CLAUDE_CODE_ULTRAREVIEW_PREFLIGHT_FIXTURE` | string | — | Override the preflight fixture path for ultrareview |
| `CLAUDE_MOCK_HEADERLESS_429` | bool | — | Declared in the typed env registry but with no consumer anywhere else in the bundle (verified by both string-literal and minified-symbol search) — reserved or dead at this tag; setting it has no effect. |
| `FORCE_VCR` | bool | — | Declared in the typed env registry but with no consumer anywhere else in the bundle (verified by both string-literal and minified-symbol search) — reserved or dead at this tag; setting it has no effect. |
| `IS_DEMO` | string | `false` | Running in demo mode |
| `SWE_BENCH_INSTANCE_ID` | — | — | SWE-bench instance identifier for telemetry |
| `SWE_BENCH_RUN_ID` | — | — | SWE-bench run identifier for telemetry |
| `SWE_BENCH_TASK_ID` | — | — | SWE-bench task identifier for telemetry |

## GitHub Actions

| Variable | Type | Default | Description |
|---|---|---|---|
| `ACTIONS_ID_TOKEN_REQUEST_TOKEN` | — | — | Listed in the credential-redaction array, so it (and its `INPUT_`-prefixed form) is stripped from the environment passed to subprocesses and MCP servers. Not a Claude Code setting. |
| `ACTIONS_ID_TOKEN_REQUEST_URL` | — | — | Listed in the credential-redaction array, so it (and its `INPUT_`-prefixed form) is stripped from the environment passed to subprocesses and MCP servers. Not a Claude Code setting. |
| `ACTIONS_RUNTIME_TOKEN` | — | — | Listed in the credential-redaction array, so it (and its `INPUT_`-prefixed form) is stripped from the environment passed to subprocesses and MCP servers. Not a Claude Code setting. |
| `ACTIONS_RUNTIME_URL` | — | — | Listed in the credential-redaction array, so it (and its `INPUT_`-prefixed form) is stripped from the environment passed to subprocesses and MCP servers. Not a Claude Code setting. |
| `GH_ENTERPRISE_TOKEN` | string | — | Token used for GitHub Enterprise hosts that match `GH_HOST` (preferred over `GITHUB_ENTERPRISE_TOKEN`) |
| `GH_HOST` | string | — | Hostname for GitHub Enterprise; enterprise-token vars are only honored for hosts matching this value |
| `GH_TOKEN` | string | ambient | Alternative GitHub token name; scrubbed alongside `GITHUB_TOKEN` when `CLAUDE_CODE_SUBPROCESS_ENV_SCRUB` is enabled |
| `GITHUB_ACTIONS` | string | ambient | Detect GitHub Actions and adapt telemetry/session classification |
| `GITHUB_ACTION_INPUTS` | string | ambient | GitHub Action input parameters |
| `GITHUB_ACTION_PATH` | string | ambient | Path to the running GitHub Action |
| `GITHUB_ACTOR` | string | ambient | GitHub username that triggered the workflow |
| `GITHUB_ACTOR_ID` | string | ambient | GitHub user ID that triggered the workflow |
| `GITHUB_ENTERPRISE_TOKEN` | string | — | Fallback token used for GitHub Enterprise hosts when `GH_ENTERPRISE_TOKEN` is unset |
| `GITHUB_ENV` | string | ambient | Path to the GitHub Actions workflow env file (denywrite-listed inside sandbox) |
| `GITHUB_EVENT_NAME` | string | ambient | GitHub event that triggered the workflow |
| `GITHUB_EVENT_PATH` | string | ambient | Path to the event payload JSON file (denywrite-listed inside sandbox) |
| `GITHUB_REPOSITORY` | string | ambient | GitHub repository (`owner/repo`) |
| `GITHUB_REPOSITORY_ID` | string | ambient | GitHub repository ID |
| `GITHUB_REPOSITORY_OWNER` | string | ambient | GitHub repository owner |
| `GITHUB_REPOSITORY_OWNER_ID` | string | ambient | GitHub repository owner ID |
| `GITHUB_TOKEN` | string | ambient | GitHub token used by actions; also redacted when scrubbing subprocess environments |
| `GITHUB_WORKSPACE` | string | ambient | GitHub Actions workspace directory |
| `OVERRIDE_GITHUB_TOKEN` | — | — | Listed in the credential-redaction array, so it (and its `INPUT_`-prefixed form) is stripped from the environment passed to subprocesses and MCP servers. Not a Claude Code setting. |

## CI / Hosted Environment Detection

| Variable | Type | Default | Description |
|---|---|---|---|
| `APP_URL` | string | ambient | Detect DigitalOcean App Platform when the app URL matches that host pattern |
| `AWS_EXECUTION_ENV` | string | ambient | Distinguish AWS ECS/Fargate execution environments |
| `AWS_LAMBDA_FUNCTION_NAME` | string | ambient | Detect AWS Lambda runtimes |
| `AZURE_FUNCTIONS_ENVIRONMENT` | string | ambient | Detect Azure Functions runtimes |
| `BUILDKITE` | string | ambient | Detect Buildkite CI environments |
| `C9_PID` | — | ambient | Detect Cloud9 environments |
| `C9_USER` | — | ambient | Detect Cloud9 user |
| `CF_PAGES` | string | ambient | Detect Cloudflare Pages deployments |
| `CIRCLECI` | string | ambient | Detect CircleCI environments |
| `CLOUD_RUN_JOB` | — | ambient | Detect Google Cloud Run jobs |
| `CODER` | string | ambient | Detect Coder environments |
| `CODER_WORKSPACE_NAME` | string | ambient | Detect Coder workspace name |
| `CODESPACES` | string | ambient | Detect GitHub Codespaces |
| `DAYTONA_WS_ID` | string | ambient | Detect Daytona environments |
| `DENO_DEPLOYMENT_ID` | string | ambient | Detect Deno Deploy environments |
| `DEVPOD` | — | ambient | Detect DevPod environments |
| `DEVPOD_WORKSPACE_UID` | — | ambient | Detect DevPod workspace UID |
| `DYNO` | string | ambient | Detect Heroku dynos |
| `FLY_APP_NAME` | — | ambient | Detect Fly.io workloads |
| `FLY_MACHINE_ID` | — | ambient | Detect Fly.io workloads |
| `FUNCTION_NAME` | — | ambient | Detect Google Cloud Functions (v1) |
| `FUNCTION_TARGET` | — | ambient | Detect Google Cloud Functions (v2) |
| `GAE_MODULE_NAME` | — | ambient | Detect Google App Engine (legacy) |
| `GAE_SERVICE` | — | ambient | Detect Google App Engine |
| `GITLAB_CI` | string | ambient | Detect GitLab CI environments |
| `GITPOD_WORKSPACE_ID` | string | ambient | Detect Gitpod |
| `GOOGLE_CLOUD_PROJECT` | string | ambient | Detect generic Google Cloud environments and supply project context |
| `GOOGLE_CLOUD_WORKSTATIONS` | string | — | Truthy presence flags the session as running inside a Google Cloud Workstation |
| `KUBERNETES_SERVICE_HOST` | string | ambient | Detect Kubernetes environments |
| `K_CONFIGURATION` | — | ambient | Detect Google Cloud Run configuration |
| `K_SERVICE` | string | ambient | Detect Google Cloud Run services |
| `NETLIFY` | string | ambient | Detect Netlify-hosted environments |
| `PROJECT_DOMAIN` | string | ambient | Detect Glitch from its project-domain environment |
| `RAILWAY_ENVIRONMENT_NAME` | — | ambient | Detect Railway-hosted environments |
| `RAILWAY_SERVICE_NAME` | — | ambient | Detect Railway-hosted environments |
| `RENDER` | string | ambient | Detect Render-hosted environments |
| `REPL_ID` | string | ambient | Detect Replit when `REPL_ID` is present |
| `REPL_SLUG` | string | ambient | Detect Replit when `REPL_SLUG` is present |
| `RUNNER_ENVIRONMENT` | string | ambient | Detect GitHub-hosted or self-hosted runners |
| `RUNNER_OS` | string | ambient | Detect runner OS in CI environments |
| `SPACE_CREATOR_USER_ID` | string | ambient | Detect Hugging Face Spaces environments |
| `SYSTEM_OIDCREQUESTURI` | — | ambient | Detect Azure DevOps / Azure Pipelines environments |
| `VERCEL` | string | ambient | Detect Vercel-hosted environments |
| `WEBSITE_SITE_NAME` | string | ambient | Detect Azure App Service deployments |
| `WEBSITE_SKU` | string | ambient | Detect Azure App Service deployments |

## GCP Metadata & Detection

| Variable | Type | Default | Description |
|---|---|---|---|
| `CLOUDSDK_AUTH_ACCESS_TOKEN` | string | — | gcloud access token, checked before `GOOGLE_APPLICATION_CREDENTIALS`. The agent proxy sets it to a `proxy-injected` sentinel when no real GCP credentials exist, and it is scrubbed from proxied child environments. |
| `CLOUDSDK_CONFIG` | string | — | Override the path to the `gcloud` SDK configuration directory; falls back to the platform default if unset |
| `DETECT_GCP_RETRIES` | — | — | Number of retries for GCP detection |
| `GCE_METADATA_HOST` | — | — | Override GCE metadata server hostname |
| `GCE_METADATA_IP` | — | — | Override GCE metadata server IP address |
| `GOOGLE_API_CERTIFICATE_CONFIG` | — | — | Path to the mTLS certificate configuration used by the bundled google-auth-library. |
| `GOOGLE_EXTERNAL_ACCOUNT_ALLOW_EXECUTABLES` | — | — | google-auth-library gate that must be set before an external-account credential may run an executable to fetch a subject token. |
| `GOOGLE_EXTERNAL_ACCOUNT_AUDIENCE` | — | — | Audience passed to the external-account executable by google-auth-library. |
| `GOOGLE_EXTERNAL_ACCOUNT_IMPERSONATED_EMAIL` | — | — | Impersonated service-account email passed to the external-account executable. |
| `GOOGLE_EXTERNAL_ACCOUNT_INTERACTIVE` | — | `0` | Set to "0" by google-auth-library so the external-account executable runs non-interactively. |
| `GOOGLE_EXTERNAL_ACCOUNT_OUTPUT_FILE` | — | — | Output file path passed to the external-account executable for caching its token. |
| `GOOGLE_EXTERNAL_ACCOUNT_TOKEN_TYPE` | — | — | Subject token type passed to the external-account executable. |
| `GOOGLE_SDK_NODE_LOGGING` | — | — | Enables logging in the bundled Google SDK. |
| `METADATA_SERVER_DETECTION` | — | — | Control GCE metadata server detection behavior |
| `gcloud_project` | string | — | Lowercase GCP project fallback, read after `GOOGLE_CLOUD_PROJECT` (`GOOGLE_CLOUD_PROJECT |
| `google_application_credentials` | string | — | Lowercase fallback for the service-account key path, read after `GOOGLE_APPLICATION_CREDENTIALS` (v2.1.154) |
| `google_cloud_project` | string | — | Lowercase GCP project fallback, read last in the `GOOGLE_CLOUD_PROJECT |

## Azure Identity (Bundled SDK)

| Variable | Type | Default | Description |
|---|---|---|---|
| `AZURE_ADDITIONALLY_ALLOWED_TENANTS` | — | ambient | Additional allowed Azure AD tenants |
| `AZURE_AUTHORITY_HOST` | — | ambient | Azure AD authority host URL |
| `AZURE_CLIENT_CERTIFICATE_PASSWORD` | — | ambient | Password for Azure AD client certificate |
| `AZURE_CLIENT_CERTIFICATE_PATH` | — | ambient | Path to Azure AD client certificate |
| `AZURE_CLIENT_ID` | — | ambient | Azure AD application (client) ID |
| `AZURE_CLIENT_SECRET` | — | ambient | Azure AD client secret |
| `AZURE_CLIENT_SEND_CERTIFICATE_CHAIN` | — | ambient | Send certificate chain for SNI |
| `AZURE_FEDERATED_TOKEN_FILE` | — | ambient | Path to federated token file for workload identity |
| `AZURE_IDENTITY_DISABLE_MULTITENANTAUTH` | — | ambient | Disable multi-tenant authentication |
| `AZURE_PASSWORD` | — | ambient | Azure password for password-based auth |
| `AZURE_POD_IDENTITY_AUTHORITY_HOST` | — | ambient | Azure pod identity authority host |
| `AZURE_REGIONAL_AUTHORITY_NAME` | — | ambient | Azure regional authority name |
| `AZURE_TENANT_ID` | — | ambient | Azure AD tenant ID |
| `AZURE_TOKEN_CREDENTIALS` | — | ambient | Azure token credentials |
| `AZURE_USERNAME` | — | ambient | Azure username for password-based auth |

## Terminal Emulator Detection

| Variable | Type | Default | Description |
|---|---|---|---|
| `ALACRITTY_LOG` | string | ambient | Detect Alacritty terminal |
| `COLORFGBG` | string | ambient | Foreground/background color pair |
| `COLORTERM` | string | ambient | Color terminal capability |
| `ConEmuANSI` | — | ambient | Detect ConEmu terminal |
| `ConEmuPID` | — | ambient | Detect ConEmu process ID |
| `ConEmuTask` | — | ambient | Detect ConEmu task |
| `GNOME_TERMINAL_SERVICE` | string | ambient | Detect GNOME Terminal |
| `ITERM_SESSION_ID` | string | ambient | Detect iTerm2 terminal |
| `KITTY_WINDOW_ID` | string | ambient | Detect Kitty terminal |
| `KONSOLE_VERSION` | string | ambient | Detect KDE Konsole |
| `LC_TERMINAL` | string | ambient | Terminal identifier (e.g., iTerm2) |
| `MSYSTEM` | string | ambient | Detect MSYS2/MinGW environment |
| `STY` | string | ambient | Detect GNU Screen session |
| `TERM` | string | ambient | Terminal type |
| `TERMINAL` | string | ambient | Terminal name |
| `TERMINAL_EMULATOR` | string | ambient | Generic terminal emulator name |
| `TERMINATOR_UUID` | string | ambient | Detect Terminator |
| `TERMINUS_SUBLIME` | — | — | Presence indicates the Terminus terminal inside Sublime Text; used for terminal capability detection. |
| `TERM_PROGRAM` | string | ambient | Terminal program name |
| `TERM_PROGRAM_VERSION` | string | ambient | Terminal program version |
| `TILIX_ID` | string | ambient | Detect Tilix terminal |
| `TMUX` | string | ambient | Detect tmux session |
| `TMUX_PANE` | string | ambient | Detect tmux pane |
| `VTE_VERSION` | string | ambient | VTE (Virtual Terminal Emulator) version |
| `WAYLAND_DISPLAY` | string | — | Wayland display name. Its absence together with DISPLAY marks a headless Linux session; its presence also enables the wl-copy clipboard path. |
| `WSL_DISTRO_NAME` | string | ambient | Detect WSL (Windows Subsystem for Linux) |
| `WT_SESSION` | string | ambient | Detect Windows Terminal |
| `XTERM_VERSION` | string | ambient | xterm version |
| `ZED_TERM` | string | ambient | Detect Zed editor terminal |
| `ZELLIJ` | string | ambient | Detect Zellij terminal multiplexer (gates DECSTBM and scroll-region features) |
| `__CFBundleIdentifier` | — | ambient | Detect Conductor/macOS app bundle context via `__CFBundleIdentifier` inspection |

## System & Shell

| Variable | Type | Default | Description |
|---|---|---|---|
| `APPDATA` | string | ambient | Application data directory (Windows) |
| `BROWSER` | string | ambient | Default browser |
| `BUN_INSTALL` | string | ambient | Bun installer prefix path; used to identify when Claude Code is running under Bun's global install layout |
| `CLAUDE_CODE_POWERSHELL_RESPECT_EXECUTION_POLICY` | bool | — | When unset/falsy, Claude Code bypasses the PowerShell execution policy when invoking `powershell`; set truthy to respect the configured policy instead (v2.1.145) |
| `CLAUDE_PTY_HEARTBEAT_MS` | — | `60000` | Heartbeat interval in milliseconds for the PTY channel; unparseable values fall back to 60000. |
| `CLAUDE_PTY_HOST_EXEC` | string | — | When set to `"1"`, routes command execution through the PTY host; consumed and deleted from the environment after it is read (v2.1.145) |
| `CLAUDE_PTY_ORPHAN_CHECK_MS` | — | `2000` | Interval (ms) for the PTY host's orphan-process check; parsed via `Number(...)` and falls back to `2000` (v2.1.154) |
| `COMSPEC` | string | ambient | Windows shell path fallback when `SHELL` is unset |
| `DISPLAY` | string | `:99` | X11 display read by bundled helper code; falls back to `:99` (v2.1.145) |
| `EDITOR` | string | ambient | Default text editor |
| `FORCE_COLOR` | string | ambient | Force color output (value: level 0-3) |
| `GIT_CONFIG_COUNT` | string | — | Standard git env-config counter. Read and incremented via the `GIT_CONFIG_COUNT`/`_KEY`/`_VALUE` mechanism to inject temporary git config (e.g. `credential.interactive=false`) without touching config files. |
| `GIT_CONFIG_GLOBAL` | string | — | Overrides the global git config path. Read when the agent proxy rewrites git configuration. |
| `GRACEFUL_FS_PLATFORM` | — | ambient | Override platform for graceful-fs |
| `HOME` | string | ambient | User home directory |
| `LANG` | string | ambient | System locale |
| `LC_ALL` | string | ambient | Override all locale settings |
| `LC_TIME` | string | ambient | Time locale |
| `LOCALAPPDATA` | string | ambient | Local application data directory (Windows) |
| `NODE_DEBUG` | — | ambient | Node.js debug modules |
| `NODE_OPTIONS` | string | ambient | Node.js CLI options |
| `NO_COLOR` | string | ambient | Disable color output |
| `OSTYPE` | — | ambient | Operating system type |
| `P4PORT` | string | ambient | Perforce server port |
| `PATH` | string | ambient | System PATH |
| `PATHEXT` | string | ambient | Executable file extensions (Windows) |
| `PKG_CONFIG_PATH` | — | ambient | pkg-config search path |
| `PREFIX` | string | — | Termux install prefix; combined with `TERMUX_VERSION` to derive the tmp directory (`$PREFIX/tmp`) |
| `PWD` | string | ambient | Current working directory |
| `ProgramData` | — | ambient | Program data directory (Windows) |
| `ProgramFiles` | — | ambient | Program files directory (Windows) |
| `SAFEUSER` | string | ambient | Safe username identifier |
| `SESSIONNAME` | string | ambient | Session name (Windows) |
| `SHELL` | string | ambient | User's default shell |
| `SSH_AUTH_SOCK` | string | — | SSH agent socket path. Its absence together with an unset GIT_SSH_COMMAND is used to detect that no SSH auth is available. |
| `SSH_CLIENT` | string | ambient | Detect SSH connection |
| `SSH_CONNECTION` | string | ambient | Detect SSH connection details |
| `SSH_TTY` | string | ambient | Detect SSH TTY |
| `SystemRoot` | — | `C:\Windows` | Mixed-case Windows system root, read as a fallback when SYSTEMROOT is unset. |
| `TERMUX_VERSION` | string | — | Termux version string; presence (together with `PREFIX`) switches the tmp directory to `$PREFIX/tmp` |
| `TEST_GRACEFUL_FS_GLOBAL_PATCH` | — | ambient | Test flag for graceful-fs global patching |
| `TMP` | string | — | Temporary directory fallback; part of the env set forwarded to spawned shells. |
| `TMPDIR` | string | — | POSIX temporary directory, forwarded to spawned processes so temporary files land in the expected place. |
| `USER` | string | ambient | Current username |
| `USERNAME` | string | ambient | Current username (Windows) |
| `USERPROFILE` | string | ambient | User profile directory (Windows) |
| `UV_THREADPOOL_SIZE` | string | ambient | libuv thread pool size |
| `VISUAL` | string | ambient | Default visual editor |
| `WSL_INTEROP` | string | — | Declared in the typed env registry but never consumed — WSL detection in this bundle keys off `WSL_DISTRO_NAME` instead. Setting it has no effect. |
| `XDG_CONFIG_HOME` | string | ambient | XDG configuration home |
| `XDG_DATA_HOME` | string | `~/.local/share` | XDG data directory root used for CLI data storage and version installs; falls back to `~/.local/share` per the XDG Base Directory spec. |
| `XDG_RUNTIME_DIR` | string | ambient | XDG runtime directory |

## gRPC (Bundled SDK)

| Variable | Type | Default | Description |
|---|---|---|---|
| `GRPC_DEFAULT_SSL_ROOTS_FILE_PATH` | — | ambient | Path to custom gRPC SSL root certificates |
| `GRPC_EXPERIMENTAL_ENABLE_OUTLIER_DETECTION` | — | `true` | Enable gRPC outlier detection |
| `GRPC_NODE_TRACE` | — | ambient | gRPC Node.js tracing config |
| `GRPC_NODE_USE_ALTERNATIVE_RESOLVER` | — | `false` | Use alternative DNS resolver in gRPC Node.js |
| `GRPC_NODE_VERBOSITY` | — | ambient | gRPC Node.js verbosity level |
| `GRPC_SSL_CIPHER_SUITES` | — | ambient | Custom gRPC SSL cipher suites |
| `GRPC_TRACE` | — | ambient | gRPC tracing config |
| `GRPC_VERBOSITY` | — | ambient | gRPC verbosity level |
| `grpc_proxy` | — | ambient | gRPC-specific proxy URL |
| `no_grpc_proxy` | — | ambient | gRPC-specific proxy bypass list |

## Dependency / Library Internals

| Variable | Type | Default | Description |
|---|---|---|---|
| `CHOKIDAR_INTERVAL` | — | ambient | Chokidar polling interval |
| `CHOKIDAR_USEPOLLING` | — | ambient | Force chokidar to use polling for file watching |
| `SHARP_FORCE_GLOBAL_LIBVIPS` | — | ambient | Force use of system-wide libvips for sharp |
| `SHARP_IGNORE_GLOBAL_LIBVIPS` | — | ambient | Ignore system-wide libvips installation for sharp |
| `SRT_DEBUG` | — | ambient | Suppress structured-clone debug output |
| `npm_package_config_libvips` | — | ambient | Override the libvips version requirement that `sharp` reports during its install/postinstall probe |

## Removed / Legacy

Documented in earlier revisions of this file but no longer present anywhere in `cli.unpack.js` at `2.1.220` (verified with `rg -c`, zero matches). Setting these has no effect.

| Variable | Last seen | Note |
|---|---|---|
| `CLAUDE_CODE_OPUS_4_6_FAST_MODE_OVERRIDE` | 2.1.154 | Truthy forces the "Opus 4.6 fast mode" override label/model selection |
| `BEDROCK_BASE_URL` | 2.1.154 | Alternative Bedrock endpoint URL (checked in addition to `ANTHROPIC_BEDROCK_BASE_URL`) |
| `VERTEX_BASE_URL` | 2.1.154 | Alternative Vertex AI endpoint URL (checked in addition to `ANTHROPIC_VERTEX_BASE_URL`) |
| `ANTHROPIC_BEDROCK_MANTLE_API_KEY` | 2.1.154 | API key used for Bedrock Mantle (redacted from logs and subprocess forwarding) |
| `OTEL_EXPORTER_OTLP_METRICS_CLIENT_CERTIFICATE` | 2.1.154 | Metrics-specific client certificate chain for OTLP mTLS |
| `OTEL_EXPORTER_OTLP_METRICS_CLIENT_KEY` | 2.1.154 | Metrics-specific client private key for OTLP mTLS |
| `CCR_UPSTREAM_PROXY_ENABLED` | 2.1.154 | Enable upstream proxy for CCR connections |
| `CCR_EGRESS_GATEWAY_ENABLED` | 2.1.154 | Enables the Claude Code Remote egress proxy gateway (paired with `CCR_UPSTREAM_PROXY_ENABLED`) |
| `CLAUDE_CODE_USE_CCR_V2` | 2.1.154 | Use CCR v2 |
| `CLAUDE_BRIDGE_USE_CCR_V2` | 2.1.154 | Force bridge/session handling onto the CCR v2 path |
| `CLAUDE_CODE_AGENT_LIST_IN_MESSAGES` | 2.1.154 | Control agent list inclusion in messages (`true` to always include, `false` to never) |
| `CLAUDE_CODE_AGENT_COST_STEER` | 2.1.154 | Enable agent cost steering (`true` to enable, `false` to disable) |
| `CLAUDE_CODE_TEAM_ONBOARDING` | 2.1.154 | Force team onboarding flavor: `banner` shows a persistent banner, `step` inserts a dedicated onboarding step |
| `CLAUDE_CODE_PLAN_MODE_INTERVIEW_PHASE` | 2.1.154 | Plan mode interview phase config |
| `TEAM_MEMORY_SYNC_URL` | 2.1.154 | URL for team memory synchronization |
| `AUDIO_CAPTURE_NODE_PATH` | 2.1.154 | Path for audio capture node |
| `COMPUTER_USE_INPUT_NODE_PATH` | 2.1.154 | Path for computer-use input native module |
| `COMPUTER_USE_SWIFT_NODE_PATH` | 2.1.154 | Path for computer-use Swift native module |
| `URL_HANDLER_NODE_PATH` | 2.1.154 | Path for URL handler native module |
| `CLAUDE_CODE_MID_CONVERSATION_SYSTEM` | 2.1.154 | Marker string used to detect (and inject) a mid-conversation system prompt; falls back to the `tengu_fennel_kite_model` GrowthBook flag |
| `CLAUDE_CODE_VERIFY_PROMPT` | 2.1.154 | Truthy enables verifier-style prompt validation before sending |
| `CLOUD_WORKSTATIONS_CLUSTER_ID` | 2.1.154 | Detect Google Cloud Workstations |
| `JEST_WORKER_ID` | 2.1.154 | Detect Jest test worker |
| `NODE_V8_COVERAGE` | 2.1.154 | V8 code coverage output directory |
| `WS_NO_BUFFER_UTIL` | 2.1.154 | Disable ws buffer-util native addon |
| `WS_NO_UTF_8_VALIDATE` | 2.1.154 | Disable ws UTF-8 validation native addon |
| `UNDICI_NO_FG` | 2.1.154 | Disable undici FinalizationRegistry |
| `CLAUDE_REPL_MODE` | 2.1.154 | Superseded by `CLAUDE_CODE_REPL` for gating REPL behavior |
| `CLAUDE_CODE_SAVE_HOOK_ADDITIONAL_CONTEXT` | 2.1.154 | Hook-context persistence flag; no matching `process.env` read in v2.1.112 |
| `GITHUB_PATH` | 2.1.154 | GitHub Actions `GITHUB_PATH` file path — no matching `process.env` read in v2.1.112 |
| `GITHUB_OUTPUT` | 2.1.154 | GitHub Actions step output file path — no matching `process.env` read in v2.1.112 |
| `GITHUB_STATE` | 2.1.154 | GitHub Actions step state file path — no matching `process.env` read in v2.1.112 |
| `GITHUB_STEP_SUMMARY` | 2.1.154 | GitHub Actions step summary file path — no matching `process.env` read in v2.1.112 |
