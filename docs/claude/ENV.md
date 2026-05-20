# Claude Code Environment Variables (v2.1.145)

Reverse-engineered from `cli.unpack.js` at tag `2.1.145` (last updated 2026-05-20). This catalog includes first-party Claude Code knobs plus ambient/dependency environment variables that are read somewhere in the bundled runtime.

> **v2.1.145 refresh:** 16 environment variables newly read in the bundle since `2.1.142` were added (each tagged `(v2.1.145)` in its row), covering host-managed auth (`CLAUDE_CODE_HOST_AUTH_*`, `CLAUDE_CODE_SDK_HAS_HOST_AUTH_REFRESH`), self-hosted agent environments (`ANTHROPIC_ENVIRONMENT_ID`/`_KEY`), and assorted runtime knobs. The existing rows below were carried forward from the `2.1.142` revision; a small number of pre-existing entries (e.g. `CLAUDE_CODE_USE_NATIVE_FILE_SEARCH`, `TEAM_MEMORY_SYNC_URL`, `EMBEDDED_SEARCH_TOOLS`, the `*_NODE_PATH` modules) were already absent as literal `process.env` reads at `2.1.142` and have not been re-verified against `2.1.145`; they are retained pending a separate full-table audit.

## Authentication & API

| Variable | Default | Description |
|---|---|---|
| `ANTHROPIC_API_KEY` | — | Primary API key for Anthropic API |
| `ANTHROPIC_AUTH_TOKEN` | — | Alternative authentication token (OAuth) |
| `ANTHROPIC_BASE_URL` | `https://api.anthropic.com` | Custom Anthropic API endpoint |
| `ANTHROPIC_MODEL` | — | Override the default model used |
| `ANTHROPIC_BETAS` | — | Comma-separated beta feature flags to send to API |
| `ANTHROPIC_CUSTOM_HEADERS` | — | Custom HTTP headers for API requests |
| `ANTHROPIC_UNIX_SOCKET` | — | Unix socket path for Anthropic API (Bun runtime) |
| `CLAUDE_CODE_OAUTH_TOKEN` | — | OAuth token for authentication |
| `CLAUDE_CODE_OAUTH_TOKEN_FILE_DESCRIPTOR` | — | File descriptor to read OAuth token from |
| `CLAUDE_CODE_API_KEY_FILE_DESCRIPTOR` | — | File descriptor to read API key from |
| `CLAUDE_CODE_API_KEY_HELPER_TTL_MS` | — | TTL in ms for API key helper cache |
| `CLAUDE_CODE_CUSTOM_OAUTH_URL` | — | Custom OAuth endpoint URL |
| `CLAUDE_CODE_OAUTH_CLIENT_ID` | — | Custom OAuth client ID |
| `CLAUDE_CODE_OAUTH_REFRESH_TOKEN` | — | OAuth refresh token |
| `CLAUDE_CODE_OAUTH_SCOPES` | — | OAuth scopes |
| `CLAUDE_CODE_API_BASE_URL` | — | Alternative base API URL |
| `CLAUDE_CODE_SESSION_ACCESS_TOKEN` | — | Session access token |
| `CLAUDE_CODE_ACCOUNT_UUID` | — | Account UUID for telemetry |
| `CLAUDE_CODE_USER_EMAIL` | — | User email for telemetry |
| `CLAUDE_CODE_ORGANIZATION_UUID` | — | Organization UUID |
| `CLAUDE_TRUSTED_DEVICE_TOKEN` | — | Trusted device token for authentication |
| `ANTHROPIC_CONFIG_DIR` | — | Overrides the directory used to read/write the Anthropic SDK profile config |
| `ANTHROPIC_PROFILE` | `default` | Selects which named Anthropic SDK profile to load from the config file |
| `ANTHROPIC_ORGANIZATION_ID` | — | Anthropic organization ID for workload-identity / federation authentication |
| `ANTHROPIC_WORKSPACE_ID` | — | Workspace ID used for workload-identity authentication when a federation rule is scoped to multiple workspaces |
| `ANTHROPIC_FEDERATION_RULE_ID` | — | OIDC federation rule ID for workload-identity auth; required for `oidc_federation` configs unless set in the profile |
| `CLAUDE_CODE_RATE_LIMIT_TIER` | — | Rate-limit tier label reported with each request; populated from the auth snapshot |
| `CLAUDE_CODE_SUBSCRIPTION_TYPE` | — | Subscription type reported with each request; populated from the auth snapshot |
| `CLAUDE_CODE_HOST_AUTH_ENV_VAR` | `ANTHROPIC_AUTH_TOKEN` | Names which environment variable the host process exposes the auth token in; read when the host manages authentication (v2.1.145) |
| `CLAUDE_CODE_HOST_AUTH_REFRESH_TIMEOUT_MS` | — | Timeout in ms to wait for the host to refresh the auth token before giving up; coerced via `Number()` (v2.1.145) |
| `CLAUDE_CODE_SDK_HAS_HOST_AUTH_REFRESH` | — | Set to `"1"` by the runtime to signal that the SDK host can refresh host-managed auth tokens on behalf of Claude Code (v2.1.145) |
| `ANTHROPIC_ENVIRONMENT_ID` | — | Self-hosted agent environment ID read by the agent-environment worker runtime (`config:{type:self_hosted}`) (v2.1.145) |
| `ANTHROPIC_ENVIRONMENT_KEY` | — | Self-hosted agent environment key paired with `ANTHROPIC_ENVIRONMENT_ID` for `EnvironmentWorker.run` / `ant beta:worker poll` flows (v2.1.145) |

## Model Configuration

| Variable | Default | Description |
|---|---|---|
| `ANTHROPIC_DEFAULT_SONNET_MODEL` | — | Override the default Sonnet model ID |
| `ANTHROPIC_DEFAULT_SONNET_MODEL_NAME` | value of `ANTHROPIC_DEFAULT_SONNET_MODEL` | Display label for custom Sonnet model |
| `ANTHROPIC_DEFAULT_SONNET_MODEL_DESCRIPTION` | `Custom Sonnet model` | Display description for custom Sonnet model |
| `ANTHROPIC_DEFAULT_SONNET_MODEL_SUPPORTED_CAPABILITIES` | — | Comma-separated capabilities for custom Sonnet model |
| `ANTHROPIC_DEFAULT_OPUS_MODEL` | — | Override the default Opus model ID |
| `ANTHROPIC_DEFAULT_OPUS_MODEL_NAME` | value of `ANTHROPIC_DEFAULT_OPUS_MODEL` | Display label for custom Opus model |
| `ANTHROPIC_DEFAULT_OPUS_MODEL_DESCRIPTION` | `Custom Opus model` | Display description for custom Opus model |
| `ANTHROPIC_DEFAULT_OPUS_MODEL_SUPPORTED_CAPABILITIES` | — | Comma-separated capabilities for custom Opus model |
| `ANTHROPIC_DEFAULT_HAIKU_MODEL` | — | Override the default Haiku model ID |
| `ANTHROPIC_DEFAULT_HAIKU_MODEL_NAME` | value of `ANTHROPIC_DEFAULT_HAIKU_MODEL` | Display label for custom Haiku model |
| `ANTHROPIC_DEFAULT_HAIKU_MODEL_DESCRIPTION` | `Custom Haiku model` | Display description for custom Haiku model |
| `ANTHROPIC_DEFAULT_HAIKU_MODEL_SUPPORTED_CAPABILITIES` | — | Comma-separated capabilities for custom Haiku model |
| `ANTHROPIC_SMALL_FAST_MODEL` | — | Override the small/fast model used internally |
| `ANTHROPIC_SMALL_FAST_MODEL_AWS_REGION` | — | AWS region for the small/fast model |
| `CLAUDE_CODE_SUBAGENT_MODEL` | — | Override model for subagents |
| `CLAUDE_CODE_MAX_OUTPUT_TOKENS` | model-dependent | Maximum output tokens per response |
| `MAX_THINKING_TOKENS` | — | Maximum thinking tokens (set >0 to enable thinking) |
| `CLAUDE_CODE_EFFORT_LEVEL` | — | Effort level: `low`, `medium`, `high`, `max`, or `unset`/`auto` |
| `CLAUDE_CODE_ALWAYS_ENABLE_EFFORT` | `false` | Force enable effort/thinking for all models |
| `CLAUDE_CODE_MAX_RETRIES` | — | Maximum API retry count |
| `API_TIMEOUT_MS` | `600000` | API request timeout in milliseconds |
| `ANTHROPIC_CUSTOM_MODEL_OPTION` | — | Add a custom model ID to the model picker and model-validation path |
| `ANTHROPIC_CUSTOM_MODEL_OPTION_NAME` | value of `ANTHROPIC_CUSTOM_MODEL_OPTION` | Display label for `ANTHROPIC_CUSTOM_MODEL_OPTION` |
| `ANTHROPIC_CUSTOM_MODEL_OPTION_DESCRIPTION` | `Custom model (<id>)` | Display description for `ANTHROPIC_CUSTOM_MODEL_OPTION` |
| `ANTHROPIC_CUSTOM_MODEL_OPTION_SUPPORTED_CAPABILITIES` | — | Comma-separated capability list advertised for `ANTHROPIC_CUSTOM_MODEL_OPTION` |
| `CLAUDE_CODE_MAX_CONTEXT_TOKENS` | — | Override the per-model context window limit (only honored when `DISABLE_COMPACT` is set) |
| `CLAUDE_CODE_MAX_TURNS` | — | Hard cap on conversation turns; must be a positive integer or startup fails |
| `CLAUDE_CODE_OPUS_4_6_FAST_MODE_OVERRIDE` | — | Truthy forces the "Opus 4.6 fast mode" override label/model selection |

## AWS Bedrock

| Variable | Default | Description |
|---|---|---|
| `CLAUDE_CODE_USE_BEDROCK` | `false` | Enable AWS Bedrock as the API provider |
| `CLAUDE_CODE_SKIP_BEDROCK_AUTH` | `false` | Skip Bedrock authentication setup |
| `ANTHROPIC_BEDROCK_BASE_URL` | `https://bedrock-runtime.{region}.amazonaws.com` | Custom Bedrock endpoint |
| `BEDROCK_BASE_URL` | — | Alternative Bedrock endpoint URL (checked in addition to `ANTHROPIC_BEDROCK_BASE_URL`) |
| `CLAUDE_ENABLE_BYTE_WATCHDOG_BEDROCK` | — | Truthy enables the byte-level stream watchdog for Bedrock `vnd.amazon.eventstream` responses (v2.1.145) |
| `AWS_BEARER_TOKEN_BEDROCK` | — | Bearer token for Bedrock authentication |
| `AWS_REGION` | `us-east-1` | AWS region |
| `AWS_DEFAULT_REGION` | `us-east-1` | Fallback AWS region |
| `AWS_ACCESS_KEY_ID` | — | AWS access key |
| `AWS_SECRET_ACCESS_KEY` | — | AWS secret key |
| `AWS_SESSION_TOKEN` | — | Temporary AWS session token |
| `AWS_PROFILE` | — | Named AWS profile |
| `AWS_LOGIN_CACHE_DIRECTORY` | — | Directory for AWS login cache |
| `ANTHROPIC_BEDROCK_SERVICE_TIER` | — | Value sent as the `X-Amzn-Bedrock-Service-Tier` header when calling Bedrock-hosted Claude models |

## AWS SDK Credential Chain

| Variable | Default | Description |
|---|---|---|
| `AWS_CONFIG_FILE` | ambient | Override the path to the AWS shared config file |
| `AWS_SHARED_CREDENTIALS_FILE` | ambient | Override the path to the AWS shared credentials file |
| `AWS_WEB_IDENTITY_TOKEN_FILE` | ambient | Path to the web-identity token file used for STS role assumption |
| `AWS_ROLE_ARN` | ambient | IAM role ARN to assume when web-identity credentials are used |
| `AWS_ROLE_SESSION_NAME` | ambient | Session name to use when assuming `AWS_ROLE_ARN` |
| `AWS_CONTAINER_CREDENTIALS_RELATIVE_URI` | ambient | Relative ECS container-credentials endpoint path |
| `AWS_CONTAINER_CREDENTIALS_FULL_URI` | ambient | Full container-credentials endpoint URL |
| `AWS_CONTAINER_AUTHORIZATION_TOKEN` | ambient | Authorization token for container-credentials endpoint requests |
| `AWS_CONTAINER_AUTHORIZATION_TOKEN_FILE` | ambient | File containing the authorization token for container-credentials endpoint requests |
| `AWS_EC2_METADATA_DISABLED` | ambient | Disable EC2 instance-metadata credential and region resolution in bundled AWS SDK providers |
| `AWS_EC2_METADATA_SERVICE_ENDPOINT` | ambient | Override the EC2 instance metadata service endpoint |
| `AWS_EC2_METADATA_SERVICE_ENDPOINT_MODE` | ambient | Select IPv4 or IPv6 EC2 metadata endpoint mode |
| `AWS_EC2_METADATA_V1_DISABLED` | ambient | Disable IMDSv1 fallback in bundled AWS SDK providers |
| `AWS_DEFAULTS_MODE` | `legacy` | AWS SDK defaults mode (`auto`, `in-region`, `cross-region`, `mobile`, `standard`, `legacy`) |
| `AWS_ENDPOINT_URL` | ambient | Override the AWS service endpoint base URL; service-specific endpoint suffixes are also supported by bundled AWS SDK components |
| `AWS_USE_DUALSTACK_ENDPOINT` | ambient | Prefer dual-stack AWS service endpoints when supported |
| `AWS_USE_FIPS_ENDPOINT` | ambient | Prefer FIPS AWS service endpoints when supported |
| `AWS_MAX_ATTEMPTS` | ambient | Override the AWS SDK retry attempt count |
| `AWS_RETRY_MODE` | ambient | Override the AWS SDK retry strategy selection |
| `AWS_AUTH_SCHEME_PREFERENCE` | ambient | Comma-separated preferred AWS auth schemes for bundled SDK auth resolution |
| `AWS_SDK_UA_APP_ID` | ambient | Attach an application ID to bundled AWS SDK user-agent strings |
| `AWS_CREDENTIAL_EXPIRATION` | ambient | Expiration timestamp paired with env-provided AWS credentials |
| `AWS_CREDENTIAL_SCOPE` | ambient | Credential scope paired with env-provided AWS credentials |
| `AWS_ACCOUNT_ID` | ambient | Account ID paired with env-provided AWS credentials |

## Google Vertex AI

| Variable | Default | Description |
|---|---|---|
| `CLAUDE_CODE_USE_VERTEX` | `false` | Enable Google Vertex AI as the API provider |
| `CLAUDE_CODE_SKIP_VERTEX_AUTH` | `false` | Skip Vertex AI authentication setup |
| `ANTHROPIC_VERTEX_BASE_URL` | — | Custom Vertex AI endpoint |
| `ANTHROPIC_VERTEX_PROJECT_ID` | — | Google Cloud project ID for Vertex AI |
| `VERTEX_BASE_URL` | — | Alternative Vertex AI endpoint URL (checked in addition to `ANTHROPIC_VERTEX_BASE_URL`) |
| `CLOUD_ML_REGION` | `us-east5` | Google Cloud region for Vertex AI |
| `VERTEX_REGION_CLAUDE_HAIKU_4_5` | falls back to `CLOUD_ML_REGION` / `us-east5` | Per-model Vertex region override for `claude-haiku-4-5` |
| `VERTEX_REGION_CLAUDE_3_5_HAIKU` | falls back to `CLOUD_ML_REGION` / `us-east5` | Per-model Vertex region override for `claude-3-5-haiku` |
| `VERTEX_REGION_CLAUDE_3_5_SONNET` | falls back to `CLOUD_ML_REGION` / `us-east5` | Per-model Vertex region override for `claude-3-5-sonnet` |
| `VERTEX_REGION_CLAUDE_3_7_SONNET` | falls back to `CLOUD_ML_REGION` / `us-east5` | Per-model Vertex region override for `claude-3-7-sonnet` |
| `VERTEX_REGION_CLAUDE_4_7_OPUS` | falls back to `CLOUD_ML_REGION` / `us-east5` | Per-model Vertex region override for `claude-opus-4-7` |
| `VERTEX_REGION_CLAUDE_4_6_OPUS` | falls back to `CLOUD_ML_REGION` / `us-east5` | Per-model Vertex region override for `claude-opus-4-6` |
| `VERTEX_REGION_CLAUDE_4_5_OPUS` | falls back to `CLOUD_ML_REGION` / `us-east5` | Per-model Vertex region override for `claude-opus-4-5` |
| `VERTEX_REGION_CLAUDE_4_1_OPUS` | falls back to `CLOUD_ML_REGION` / `us-east5` | Per-model Vertex region override for `claude-opus-4-1` |
| `VERTEX_REGION_CLAUDE_4_0_OPUS` | falls back to `CLOUD_ML_REGION` / `us-east5` | Per-model Vertex region override for `claude-opus-4` |
| `VERTEX_REGION_CLAUDE_4_6_SONNET` | falls back to `CLOUD_ML_REGION` / `us-east5` | Per-model Vertex region override for `claude-sonnet-4-6` |
| `VERTEX_REGION_CLAUDE_4_5_SONNET` | falls back to `CLOUD_ML_REGION` / `us-east5` | Per-model Vertex region override for `claude-sonnet-4-5` |
| `VERTEX_REGION_CLAUDE_4_0_SONNET` | falls back to `CLOUD_ML_REGION` / `us-east5` | Per-model Vertex region override for `claude-sonnet-4` |
| `GOOGLE_APPLICATION_CREDENTIALS` | — | Path to Google Cloud service account credentials JSON |
| `GOOGLE_CLOUD_PROJECT` | — | Google Cloud project ID |
| `GOOGLE_CLOUD_QUOTA_PROJECT` | — | Google Cloud quota project ID |
| `GCLOUD_PROJECT` | — | Alternative Google Cloud project ID |

## Anthropic Foundry

| Variable | Default | Description |
|---|---|---|
| `CLAUDE_CODE_USE_FOUNDRY` | `false` | Enable Anthropic Foundry as the API provider |
| `CLAUDE_CODE_SKIP_FOUNDRY_AUTH` | `false` | Skip Foundry authentication setup |
| `ANTHROPIC_FOUNDRY_BASE_URL` | — | Foundry API endpoint |
| `ANTHROPIC_FOUNDRY_API_KEY` | — | Foundry API key |
| `ANTHROPIC_FOUNDRY_RESOURCE` | — | Foundry resource identifier |

## Anthropic AWS

| Variable | Default | Description |
|---|---|---|
| `CLAUDE_CODE_USE_ANTHROPIC_AWS` | `false` | Enable Anthropic AWS as the API provider |
| `CLAUDE_CODE_SKIP_ANTHROPIC_AWS_AUTH` | `false` | Skip Anthropic AWS authentication setup |
| `ANTHROPIC_AWS_BASE_URL` | — | Anthropic AWS API endpoint |
| `ANTHROPIC_AWS_API_KEY` | — | Anthropic AWS API key |
| `ANTHROPIC_AWS_WORKSPACE_ID` | — | Anthropic AWS workspace identifier |

## Bedrock Mantle

New Bedrock Mantle route, selected when `CLAUDE_CODE_USE_MANTLE` is truthy. Base URL defaults to `https://bedrock-mantle.<region>.api.aws/anthropic` when not explicitly set.

| Variable | Default | Description |
|---|---|---|
| `CLAUDE_CODE_USE_MANTLE` | `false` | Enable the Bedrock Mantle route as the API provider |
| `CLAUDE_CODE_SKIP_MANTLE_AUTH` | `false` | Skip Bedrock Mantle authentication setup |
| `ANTHROPIC_BEDROCK_MANTLE_BASE_URL` | computed from region | Override the Bedrock Mantle base URL |
| `ANTHROPIC_BEDROCK_MANTLE_API_KEY` | — | API key used for Bedrock Mantle (redacted from logs and subprocess forwarding) |

## Configuration & Directories

| Variable | Default | Description |
|---|---|---|
| `CLAUDE_CONFIG_DIR` | `~/.claude` | Claude configuration directory |
| `CLAUDE_CODE_TMPDIR` | `/tmp` | Temporary directory for Claude Code |
| `CLAUDE_TMPDIR` | `/tmp/claude` | Temporary directory used in sandbox |
| `CLAUDE_SECURESTORAGE_CONFIG_DIR` | — | Override the directory used by the secure-storage backend for its config (v2.1.145) |
| `CLAUDE_ENV_FILE` | — | Path to environment file to load at session start |
| `CLAUDE_CODE_SHELL` | — | Override shell used for Bash tool |
| `CLAUDE_CODE_SHELL_PREFIX` | — | Prefix command for shell executions |
| `CLAUDE_CODE_GIT_BASH_PATH` | — | Path to Git Bash on Windows |

## Tool Configuration

| Variable | Default | Description |
|---|---|---|
| `BASH_MAX_OUTPUT_LENGTH` | `30000` (upper: `150000`) | Maximum Bash tool output length in characters |
| `BASH_MAX_TIMEOUT_MS` | `600000` | Maximum Bash tool timeout in milliseconds |
| `BASH_DEFAULT_TIMEOUT_MS` | `120000` | Default Bash tool timeout in milliseconds before max-timeout clamping is applied |
| `CLAUDE_CODE_GLOB_TIMEOUT_SECONDS` | `0` (unlimited) | Glob tool timeout in seconds |
| `CLAUDE_CODE_FILE_READ_MAX_OUTPUT_TOKENS` | — | Maximum output tokens for file reads |
| `TASK_MAX_OUTPUT_LENGTH` | — | Maximum TaskOutput length in characters |
| `CLAUDE_CODE_MAX_TOOL_USE_CONCURRENCY` | `10` | Maximum concurrent tool executions |
| `SLASH_COMMAND_TOOL_CHAR_BUDGET` | — | Character budget for slash command tools |
| `CLAUDE_CODE_GLOB_NO_IGNORE` | `true` | Whether glob ignores gitignore patterns |
| `CLAUDE_CODE_GLOB_HIDDEN` | `true` | Whether glob includes hidden files |
| `USE_BUILTIN_RIPGREP` | `false` | Use built-in ripgrep instead of system |
| `CLAUDE_CODE_USE_POWERSHELL_TOOL` | `false` | Allow the Bash/input-box execution path to run via PowerShell when `defaultShell` is `powershell` and sandbox policy permits it |
| `CLAUDE_CODE_PWSH_PARSE_TIMEOUT_MS` | — | Timeout for PowerShell command parsing (ms) |
| `CLAUDE_CODE_USE_NATIVE_FILE_SEARCH` | `false` | Use native file search |

## MCP (Model Context Protocol)

| Variable | Default | Description |
|---|---|---|
| `MCP_TIMEOUT` | `30000` | Default timeout for MCP operations (ms) |
| `MCP_TOOL_TIMEOUT` | — | Timeout for individual MCP tool calls (ms) |
| `MAX_MCP_OUTPUT_TOKENS` | `25000` | Maximum tokens for MCP output |
| `MCP_SERVER_CONNECTION_BATCH_SIZE` | `3` | Local MCP server connection batch size |
| `MCP_REMOTE_SERVER_CONNECTION_BATCH_SIZE` | `20` | Remote MCP server connection batch size |
| `MCP_CONNECTION_NONBLOCKING` | `false` | Make MCP server connections non-blocking |
| `MCP_OAUTH_CALLBACK_PORT` | — | Port for MCP OAuth callbacks |
| `MCP_OAUTH_CLIENT_METADATA_URL` | — | Override the client metadata URL advertised during MCP OAuth registration flows |
| `MCP_CLIENT_SECRET` | — | OAuth client secret for MCP servers |
| `MCP_XAA_IDP_CLIENT_SECRET` | — | OAuth client secret for MCP XAA IdP connections |
| `MCP_TRUNCATION_PROMPT_OVERRIDE` | — | Override the truncation prompt used for MCP output |
| `ENABLE_MCP_LARGE_OUTPUT_FILES` | `false` | Enable large file output for MCP |
| `ENABLE_CLAUDEAI_MCP_SERVERS` | `false` | Enable claude.ai MCP proxy servers |
| `MCP_CONNECT_TIMEOUT_MS` | — | Override (ms) for the MCP server connection timeout; parsed as an integer |

## Feature Disable Flags

| Variable | Default | Description |
|---|---|---|
| `CLAUDE_CODE_DISABLE_NONESSENTIAL_TRAFFIC` | `false` | Disable all non-essential network traffic (telemetry, updates, etc.) |
| `CLAUDE_CODE_DISABLE_THINKING` | `false` | Disable extended thinking |
| `CLAUDE_CODE_DISABLE_ADAPTIVE_THINKING` | `false` | Disable adaptive thinking |
| `CLAUDE_CODE_DISABLE_FAST_MODE` | `false` | Disable fast mode |
| `CLAUDE_CODE_DISABLE_BACKGROUND_TASKS` | `false` | Disable background task execution |
| `CLAUDE_CODE_DISABLE_CLAUDE_MDS` | `false` | Disable CLAUDE.md file scanning |
| `CLAUDE_CODE_DISABLE_AUTO_MEMORY` | `false` | Disable automatic memory saving |
| `CLAUDE_CODE_DISABLE_TERMINAL_TITLE` | `false` | Don't update terminal title |
| `CLAUDE_CODE_DISABLE_FILE_CHECKPOINTING` | `false` | Disable file checkpointing (undo) |
| `CLAUDE_CODE_DISABLE_GIT_INSTRUCTIONS` | `false` | Disable git instructions in system prompt |
| `CLAUDE_CODE_DISABLE_VIRTUAL_SCROLL` | `false` | Disable virtual scroll in UI |
| `CLAUDE_CODE_DISABLE_EXPERIMENTAL_BETAS` | `false` | Disable experimental beta features |
| `CLAUDE_CODE_DISABLE_LEGACY_MODEL_REMAP` | `false` | Disable legacy model name remapping |
| `CLAUDE_CODE_DISABLE_1M_CONTEXT` | `false` | Disable 1M token context window |
| `CLAUDE_CODE_DISABLE_ATTACHMENTS` | `false` | Disable attachments feature |
| `CLAUDE_CODE_DISABLE_CRON` | `false` | Disable cron/scheduled tasks |
| `CLAUDE_CODE_DISABLE_PRECOMPACT_SKIP` | `false` | Disable pre-compaction skip optimization |
| `CLAUDE_CODE_DISABLE_FEEDBACK_SURVEY` | `false` | Disable feedback survey |
| `CLAUDE_CODE_DISABLE_OFFICIAL_MARKETPLACE_AUTOINSTALL` | `false` | Disable auto-install of official marketplace items |
| `CLAUDE_CODE_DISABLE_ADVISOR_TOOL` | `false` | Disable the advisor tool |
| `CLAUDE_CODE_DISABLE_MOUSE` | `false` | Disable mouse input entirely |
| `CLAUDE_CODE_DISABLE_NONSTREAMING_FALLBACK` | `false` | Disable fallback from streaming to non-streaming API calls |
| `CLAUDE_CODE_DISABLE_POLICY_SKILLS` | `false` | Disable policy-driven skills loading |
| `CLAUDE_CODE_DISABLE_CLAUDE_API_SKILL` | `false` | Disable the Claude API skill |
| `DISABLE_AUTOUPDATER` | `false` | Disable auto-updater |
| `DISABLE_TELEMETRY` | `false` | Disable all telemetry |
| `DISABLE_COMPACT` | `false` | Disable context compaction entirely |
| `DISABLE_AUTO_COMPACT` | `false` | Disable automatic context compaction |
| `DISABLE_PROMPT_CACHING` | `false` | Disable prompt caching for all models |
| `DISABLE_PROMPT_CACHING_HAIKU` | `false` | Disable prompt caching for Haiku |
| `DISABLE_PROMPT_CACHING_SONNET` | `false` | Disable prompt caching for Sonnet |
| `DISABLE_PROMPT_CACHING_OPUS` | `false` | Disable prompt caching for Opus |
| `DISABLE_INTERLEAVED_THINKING` | `false` | Disable interleaved thinking |
| `DISABLE_ERROR_REPORTING` | `false` | Disable error reporting |
| `DISABLE_INSTALLATION_CHECKS` | `false` | Disable installation health checks |
| `DISABLE_COST_WARNINGS` | `false` | Disable cost warning notifications |
| `DISABLE_FEEDBACK_COMMAND` | `false` | Disable the `/feedback` command |
| `DISABLE_BUG_COMMAND` | `false` | Disable the `/bug` command |
| `DISABLE_LOGIN_COMMAND` | `false` | Disable the `/login` command |
| `DISABLE_LOGOUT_COMMAND` | `false` | Disable the `/logout` command |
| `DISABLE_DOCTOR_COMMAND` | `false` | Disable the `/doctor` command |
| `DISABLE_UPGRADE_COMMAND` | `false` | Disable the `/upgrade` command |
| `DISABLE_INSTALL_GITHUB_APP_COMMAND` | `false` | Disable the GitHub App install command |
| `DISABLE_EXTRA_USAGE_COMMAND` | `false` | Disable extra usage display |
| `CLAUDE_CODE_DISABLE_AGENT_VIEW` | — | Truthy disables the agent view (`claude agents`, `--bg`, `/background`, on-demand daemon); mirrors the `disableAgentView` setting |
| `CLAUDE_CODE_DISABLE_ALTERNATE_SCREEN` | — | Truthy disables the terminal alternate-screen buffer (same effect as `CLAUDE_CODE_NO_FLICKER`) |
| `DISABLE_GROWTHBOOK` | — | Truthy disables GrowthBook feature-flag lookups |
| `DISABLE_UPDATES` | — | Truthy disables auto-update checks and installs |
| `DISABLE_BRIEF_MODE_STOP_HOOK` | — | Truthy suppresses the Brief-mode stop hook in SDK/REPL sessions that have the Brief tool |

## Feature Enable Flags

| Variable | Default | Description |
|---|---|---|
| `CLAUDE_CODE_ENABLE_TELEMETRY` | `false` | Enable OpenTelemetry telemetry |
| `CLAUDE_CODE_ENABLE_TASKS` | `false` | Enable tasks feature |
| `CLAUDE_CODE_ENABLE_TOKEN_USAGE_ATTACHMENT` | `false` | Enable token usage attachment |
| `CLAUDE_CODE_ENABLE_PROMPT_SUGGESTION` | — | Enable/disable prompt suggestions (`false` to disable) |
| `CLAUDE_CODE_ENABLE_SDK_FILE_CHECKPOINTING` | `false` | Enable file checkpointing in SDK mode |
| `CLAUDE_CODE_ENABLE_FINE_GRAINED_TOOL_STREAMING` | `false` | Enable fine-grained tool streaming |
| `CLAUDE_CODE_ENABLE_CFC` | — | Enable CFC feature |
| `CLAUDE_CODE_ENABLE_XAA` | — | Enable XAA IdP authentication feature |
| `CLAUDE_CODE_ENABLE_AWAY_SUMMARY` | — | Enable away summary feature |
| `CLAUDE_CODE_ENABLE_BACKGROUND_PLUGIN_REFRESH` | `false` | Enable background refresh for plugins |
| `CLAUDE_CODE_ENABLE_EXPERIMENTAL_ADVISOR_TOOL` | `false` | Enable the experimental advisor tool |
| `ENABLE_TOOL_SEARCH` | — | Enable tool search/deferred tools (`true`, `auto`, `auto:N`) |
| `ENABLE_PROMPT_CACHING_1H` | `false` | Enable 1-hour prompt caching |
| `ENABLE_PROMPT_CACHING_1H_BEDROCK` | `false` | Enable 1-hour prompt caching on Bedrock |
| `ENABLE_ENHANCED_TELEMETRY_BETA` | `false` | Enable enhanced telemetry beta |
| `ENABLE_BETA_TRACING_DETAILED` | `false` | Enable detailed beta tracing |
| `CLAUDE_CODE_FORCE_FULL_LOGO` | `false` | Force display of full logo |
| `FORCE_PROMPT_CACHING_5M` | `false` | Force 5-minute prompt caching |
| `EMBEDDED_SEARCH_TOOLS` | `false` | Enable embedded search tools |
| `USE_API_CONTEXT_MANAGEMENT` | `false` | Use API context management |
| `CLAUDE_CODE_ENABLE_FEEDBACK_SURVEY_FOR_OTEL` | — | Truthy enables the in-product feedback survey for OTEL-instrumented installations |
| `CLAUDE_CODE_ENABLE_GATEWAY_MODEL_DISCOVERY` | — | Truthy enables gateway-side model discovery (first-party provider only) |

## Compaction & Context

| Variable | Default | Description |
|---|---|---|
| `CLAUDE_CODE_AUTO_COMPACT_WINDOW` | — | Auto-compact window size |
| `CLAUDE_AUTOCOMPACT_PCT_OVERRIDE` | — | Override auto-compact percentage threshold |
| `CLAUDE_CODE_BLOCKING_LIMIT_OVERRIDE` | — | Override blocking limit for compaction |
| `CLAUDE_CODE_COLD_COMPACT` | — | Truthy forces the "cold compact" conversation-compaction path |
| `CLAUDE_CODE_CLASSIFIER_SUMMARY` | — | Overrides summary classifier mode: truthy = `llm`, falsy = `heuristic`; unset falls back to the default selector |

## Debug & Logging

| Variable | Default | Description |
|---|---|---|
| `CLAUDE_CODE_DEBUG_LOGS_DIR` | `~/.claude/debug/{timestamp}.txt` | Debug log file directory |
| `CLAUDE_CODE_DEBUG_LOG_LEVEL` | `debug` | Log level: `debug`, `info`, `warn`, `error` |
| `CLAUDE_CODE_DEBUG_REPAINTS` | `false` | Enable debug logging for UI repaints |
| `CLAUDE_CODE_SLOW_OPERATION_THRESHOLD_MS` | — | Threshold for logging slow operations (ms) |
| `CLAUDE_CODE_PROFILE_STARTUP` | — | Set to `1` to enable startup profiling |
| `CLAUDE_CODE_DIAGNOSTICS_FILE` | — | Path to diagnostics output file |
| `CLAUDE_CODE_FRAME_TIMING_LOG` | — | Path to frame timing log |
| `CLAUDE_CODE_PERFETTO_TRACE` | — | Path for Perfetto trace output |
| `CLAUDE_CODE_COMMIT_LOG` | — | Path to commit log file |
| `CLAUDE_CODE_DATADOG_FLUSH_INTERVAL_MS` | — | Datadog flush interval |
| `DEBUG` | — | npm debug module namespace patterns |
| `DEBUG_SDK` | `false` | Enable SDK debug logging |
| `DEBUG_AUTH` | — | Enable authentication debug logging |
| `CLAUDE_DEBUG` | `false` | Enable Claude debug logging |
| `ANTHROPIC_LOG` | library default | Anthropic SDK log-level override used by the bundled client logger |
| `CLAUDE_CODE_BENCH_LIVE_COUNTS` | — | Truthy enables live Yoga/DOM/Fiber node-count sampling in the renderer for benchmark instrumentation |
| `CLAUDE_CODE_RETRY_WATCHDOG` | — | Truthy enables the API retry watchdog that surfaces stalled retry loops |
| `CLAUDE_CODE_TEE_SDK_STDOUT` | — | Truthy tees SDK stdout to the parent process for debugging |
| `CLAUDE_PTY_RECORD` | — | Path to write a PTY session recording for the current terminal session |
| `DEBUG_CLAUDE_AGENT_SDK` | — | Truthy enables verbose debug logging and pipes stderr from the Claude Agent SDK helper process |
| `CLAUDE_CODE_BYOC_ENABLE_DATADOG` | — | When `CLAUDE_CODE_ENVIRONMENT_KIND=byoc`, truthy re-enables Datadog telemetry (off by default in BYOC) |

## OpenTelemetry

| Variable | Default | Description |
|---|---|---|
| `OTEL_EXPORTER_OTLP_ENDPOINT` | — | OTLP collector endpoint URL |
| `OTEL_EXPORTER_OTLP_HEADERS` | — | Custom OTLP headers |
| `OTEL_EXPORTER_OTLP_PROTOCOL` | — | OTLP protocol (`grpc`/`http`) |
| `OTEL_EXPORTER_OTLP_INSECURE` | — | Skip TLS verification |
| `OTEL_EXPORTER_OTLP_TIMEOUT` | — | Global OTLP export timeout in milliseconds; signal-specific `OTEL_EXPORTER_OTLP_{TRACES|METRICS|LOGS}_TIMEOUT` overrides are also honored |
| `OTEL_EXPORTER_OTLP_COMPRESSION` | — | Global OTLP compression mode (`none` or `gzip`); signal-specific `OTEL_EXPORTER_OTLP_{TRACES|METRICS|LOGS}_COMPRESSION` overrides are also honored |
| `OTEL_EXPORTER_OTLP_CERTIFICATE` | — | Path to a custom root certificate bundle for OTLP exporters; signal-specific `OTEL_EXPORTER_OTLP_{TRACES|METRICS|LOGS}_CERTIFICATE` overrides are also honored |
| `OTEL_EXPORTER_OTLP_CLIENT_CERTIFICATE` | — | Path to a client certificate chain for OTLP mTLS; signal-specific `OTEL_EXPORTER_OTLP_{TRACES|METRICS|LOGS}_CLIENT_CERTIFICATE` overrides are also honored |
| `OTEL_EXPORTER_OTLP_CLIENT_KEY` | — | Path to a client private key for OTLP mTLS; signal-specific `OTEL_EXPORTER_OTLP_{TRACES|METRICS|LOGS}_CLIENT_KEY` overrides are also honored |
| `OTEL_EXPORTER_OTLP_METRICS_PROTOCOL` | — | Metrics-specific OTLP protocol |
| `OTEL_EXPORTER_OTLP_METRICS_HEADERS` | — | Metrics-specific OTLP headers |
| `OTEL_EXPORTER_OTLP_METRICS_CLIENT_CERTIFICATE` | — | Metrics-specific client certificate chain for OTLP mTLS |
| `OTEL_EXPORTER_OTLP_METRICS_CLIENT_KEY` | — | Metrics-specific client private key for OTLP mTLS |
| `OTEL_EXPORTER_OTLP_METRICS_TEMPORALITY_PREFERENCE` | `delta` | Metrics temporality preference |
| `OTEL_EXPORTER_OTLP_TRACES_PROTOCOL` | — | Traces-specific OTLP protocol |
| `OTEL_EXPORTER_OTLP_TRACES_HEADERS` | — | Trace-specific OTLP headers |
| `OTEL_EXPORTER_OTLP_LOGS_PROTOCOL` | — | Logs-specific OTLP protocol |
| `OTEL_EXPORTER_OTLP_LOGS_HEADERS` | — | Logs-specific OTLP headers |
| `OTEL_TRACES_EXPORTER` | — | Traces exporter type |
| `OTEL_METRICS_EXPORTER` | — | Metrics exporter type |
| `OTEL_LOGS_EXPORTER` | — | Logs exporter type |
| `OTEL_TRACES_EXPORT_INTERVAL` | — | Traces export interval (ms) |
| `OTEL_METRIC_EXPORT_INTERVAL` | — | Metrics export interval (ms) |
| `OTEL_LOGS_EXPORT_INTERVAL` | — | Logs export interval (ms) |
| `OTEL_BSP_MAX_EXPORT_BATCH_SIZE` | `512` | Maximum batch size for the OpenTelemetry span batch processor |
| `OTEL_BSP_MAX_QUEUE_SIZE` | `2048` | Maximum queue size for the OpenTelemetry span batch processor |
| `OTEL_BSP_SCHEDULE_DELAY` | `5000` | Scheduled delay in milliseconds for the OpenTelemetry span batch processor |
| `OTEL_BSP_EXPORT_TIMEOUT` | `30000` | Export timeout in milliseconds for the OpenTelemetry span batch processor |
| `OTEL_BLRP_MAX_EXPORT_BATCH_SIZE` | `512` | Maximum batch size for the OpenTelemetry log-record batch processor |
| `OTEL_BLRP_MAX_QUEUE_SIZE` | `2048` | Maximum queue size for the OpenTelemetry log-record batch processor |
| `OTEL_BLRP_SCHEDULE_DELAY` | `5000` | Scheduled delay in milliseconds for the OpenTelemetry log-record batch processor |
| `OTEL_BLRP_EXPORT_TIMEOUT` | `30000` | Export timeout in milliseconds for the OpenTelemetry log-record batch processor |
| `OTEL_EXPORTER_PROMETHEUS_HOST` | `localhost` | Prometheus exporter host |
| `OTEL_EXPORTER_PROMETHEUS_PORT` | `9464` | Prometheus exporter port |
| `OTEL_METRICS_INCLUDE_SESSION_ID` | `true` | Include the Claude Code session ID in emitted OTEL metric attributes |
| `OTEL_METRICS_INCLUDE_VERSION` | `false` | Include the Claude Code version in emitted OTEL metric attributes |
| `OTEL_METRICS_INCLUDE_ACCOUNT_UUID` | `true` | Include the tagged account identifier in emitted OTEL metric attributes |
| `OTEL_LOG_USER_PROMPTS` | `false` | Log user prompts in OTEL spans |
| `OTEL_LOG_TOOL_DETAILS` | `false` | Log tool details in OTEL spans |
| `OTEL_LOG_TOOL_CONTENT` | `false` | Log tool content in OTEL spans |
| `OTEL_LOG_RAW_API_BODIES` | `false` | Include raw API request/response bodies in OTEL spans (truncated by an internal size cap); verbose and sensitive — intended for debugging only |
| `OTEL_RESOURCE_ATTRIBUTES` | — | Additional OTEL resource attributes |
| `OTEL_SERVICE_NAME` | — | Override the OTEL service name resource attribute |
| `OTEL_ATTRIBUTE_COUNT_LIMIT` | `128` | Global OpenTelemetry attribute-count limit |
| `OTEL_ATTRIBUTE_VALUE_LENGTH_LIMIT` | `Infinity` | Global OpenTelemetry attribute-value length limit |
| `OTEL_LOGRECORD_ATTRIBUTE_COUNT_LIMIT` | falls back to `OTEL_ATTRIBUTE_COUNT_LIMIT` / `128` | Log-record attribute-count limit |
| `OTEL_LOGRECORD_ATTRIBUTE_VALUE_LENGTH_LIMIT` | falls back to `OTEL_ATTRIBUTE_VALUE_LENGTH_LIMIT` / `Infinity` | Log-record attribute-value length limit |
| `OTEL_SPAN_ATTRIBUTE_COUNT_LIMIT` | falls back to `OTEL_ATTRIBUTE_COUNT_LIMIT` / SDK default | Span attribute-count limit |
| `OTEL_SPAN_ATTRIBUTE_VALUE_LENGTH_LIMIT` | falls back to `OTEL_ATTRIBUTE_VALUE_LENGTH_LIMIT` / SDK default | Span attribute-value length limit |
| `OTEL_SPAN_LINK_COUNT_LIMIT` | `128` | Maximum number of links recorded per span |
| `OTEL_SPAN_EVENT_COUNT_LIMIT` | `128` | Maximum number of events recorded per span |
| `OTEL_SPAN_ATTRIBUTE_PER_EVENT_COUNT_LIMIT` | `128` | Maximum number of attributes recorded per span event |
| `OTEL_SPAN_ATTRIBUTE_PER_LINK_COUNT_LIMIT` | `128` | Maximum number of attributes recorded per span link |
| `OTEL_TRACES_SAMPLER` | `parentbased_always_on` | OpenTelemetry trace sampler selection |
| `OTEL_TRACES_SAMPLER_ARG` | sampler-dependent | Optional numeric argument for the selected OpenTelemetry trace sampler |
| `CLAUDE_CODE_OTEL_SHUTDOWN_TIMEOUT_MS` | `2000` | OTEL shutdown timeout |
| `CLAUDE_CODE_OTEL_FLUSH_TIMEOUT_MS` | `5000` | OTEL flush timeout |
| `CLAUDE_CODE_OTEL_HEADERS_HELPER_DEBOUNCE_MS` | — | OTEL headers helper debounce |
| `BETA_TRACING_ENDPOINT` | — | Beta tracing endpoint URL |
| `TRACEPARENT` | — | W3C trace context propagation: trace parent header (used in SDK mode) |
| `TRACESTATE` | — | W3C trace context propagation: trace state header (used in SDK mode) |

## Network & Proxy

| Variable | Default | Description |
|---|---|---|
| `HTTP_PROXY` / `http_proxy` | — | HTTP proxy URL |
| `HTTPS_PROXY` / `https_proxy` | — | HTTPS proxy URL |
| `NO_PROXY` / `no_proxy` | — | Comma-separated proxy bypass list |
| `ALL_PROXY` / `all_proxy` | — | SOCKS proxy URL used by sandbox/proxy setup and compatible network clients |
| `CLAUDE_CODE_PROXY_RESOLVES_HOSTS` | `false` | Let proxy resolve hostnames |
| `CLAUDE_CODE_CLIENT_CERT` | — | Path to client TLS certificate |
| `CLAUDE_CODE_CLIENT_KEY` | — | Path to client TLS private key |
| `CLAUDE_CODE_CLIENT_KEY_PASSPHRASE` | — | Passphrase for client TLS key |
| `CLAUDE_CODE_CERT_STORE` | — | Select the CA certificate store strategy (`system`, `bundled`, or a custom path); when unset, Node defaults are used |
| `NODE_EXTRA_CA_CERTS` | — | Path to additional CA certificates |
| `SSL_CERT_FILE` | — | Path to SSL certificate file (used with proxy config) |
| `REQUESTS_CA_BUNDLE` | — | Path to CA bundle file (forwarded to subprocesses for Python compatibility) |
| `CURL_CA_BUNDLE` | — | Path to CA bundle file (forwarded to subprocesses for curl compatibility) |
| `CLAUDE_CODE_ADDITIONAL_PROTECTION` | `false` | Enable additional API protection headers |
| `CCR_UPSTREAM_PROXY_ENABLED` | `false` | Enable upstream proxy for CCR connections |
| `CLAUDE_CODE_SIMULATE_PROXY_USAGE` | `false` | Simulate proxy usage for testing |
| `CLAUDE_CODE_ENABLE_PROXY_AUTH_HELPER` | `false` | Enable the proxy-auth helper (`proxyAuthHelper` settings entry) that supplies dynamic `Proxy-Authorization` headers; must be `1` to activate |
| `CLAUDE_CODE_PROXY_AUTH_HELPER_TTL_MS` | helper default | Cache TTL (ms) for credentials produced by the proxy-auth helper |
| `CLAUDE_SLOW_FIRST_BYTE_MS` | `30000` | Timeout threshold for first byte from API (ms) |
| `CLAUDE_ENABLE_BYTE_WATCHDOG` | — | Enable byte-level stream watchdog |
| `API_FORCE_IDLE_TIMEOUT` | — | Truthy keeps the default idle timeout on Anthropic API requests; otherwise the request timeout is disabled |
| `AGENT_PROXY_URL` | — | Upstream proxy URL used by the remote agent egress gateway; consumed and deleted at startup |
| `AGENT_PROXY_AUTH_TOKEN` | — | Bearer token paired with `AGENT_PROXY_URL` for the remote agent egress proxy; consumed and deleted at startup |
| `CCR_EGRESS_GATEWAY_ENABLED` | — | Enables the Claude Code Remote egress proxy gateway (paired with `CCR_UPSTREAM_PROXY_ENABLED`) |

## Remote / Headless Mode

| Variable | Default | Description |
|---|---|---|
| `CLAUDE_CODE_REMOTE` | `false` | Running in remote/headless mode |
| `CLAUDE_CODE_REMOTE_MEMORY_DIR` | — | Memory directory for remote mode |
| `CLAUDE_CODE_REMOTE_SESSION_ID` | — | Remote session identifier |
| `CLAUDE_CODE_REMOTE_ENVIRONMENT_TYPE` | — | Remote environment type label |
| `CLAUDE_CODE_REMOTE_SEND_KEEPALIVES` | `false` | Send keepalive pings in remote mode |
| `CLAUDE_CODE_CONTAINER_ID` | — | Container ID for remote environments |
| `SESSION_INGRESS_URL` | — | Session ingress URL for remote |
| `CLAUDE_SESSION_INGRESS_TOKEN_FILE` | — | Path to session ingress token file |
| `CLAUDE_CODE_WEBSOCKET_AUTH_FILE_DESCRIPTOR` | — | WebSocket auth file descriptor |
| `CLAUDE_STREAM_IDLE_TIMEOUT_MS` | `90000` | Stream idle timeout before disconnect (ms) |
| `CLAUDE_REMOTE_CONTROL_SESSION_NAME_PREFIX` | — | Prefix for remote control session names |
| `CLAUDE_CODE_USE_CCR_V2` | `false` | Use CCR v2 |
| `CLAUDE_CODE_POST_FOR_SESSION_INGRESS_V2` | `false` | Use POST transport for session-ingress v2 websocket URLs |
| `CLAUDE_BRIDGE_USE_CCR_V2` | `false` | Force bridge/session handling onto the CCR v2 path |
| `CCR_ENABLE_BUNDLE` | `false` | Enable the CCR bundle path used by remote/background task bootstrap logic |
| `CCR_FORCE_BUNDLE` | `false` | Force the CCR bundle path even when normal preflight heuristics would not select it |
| `CLAUDE_CODE_ENVIRONMENT_RUNNER_VERSION` | — | Attach an environment-runner version header in remote bridge mode |
| `CLAUDE_CODE_RESUME_FROM_SESSION` | — | Resume from a specific session ID |
| `CCR_SPAWN_TIMESTAMP_MS` | — | Unix spawn timestamp (ms) injected by the Claude Code Remote launcher; used to compute `spawn_to_first_checkpoint_ms` / `spawn_to_exec_ms` startup-performance telemetry (v2.1.145) |
| `CLAUDE_ENABLE_STREAM_WATCHDOG` | `false` | Enable stream watchdog |
| `CLAUDE_CODE_SYSTEM_PROMPT_GB_FEATURE` | — | When running in remote mode, selects a Growthbook/feature-flag key whose evaluated string value is used to override the Agent SDK `systemPrompt` option |
| `CLAUDE_BRIDGE_REATTACH_SEQ` | — | Sequence number passed when the TUI bridge reattaches to an existing session; consumed and deleted on read |
| `CLAUDE_BRIDGE_REATTACH_SESSION` | — | Session ID used to reattach the TUI bridge to an existing session; consumed and deleted on read |

## IDE Integration

| Variable | Default | Description |
|---|---|---|
| `CLAUDE_CODE_SSE_PORT` | — | SSE port for IDE integration |
| `CLAUDE_CODE_IDE_SKIP_VALID_CHECK` | `false` | Skip IDE validation check |
| `CLAUDE_CODE_IDE_SKIP_AUTO_INSTALL` | `false` | Skip IDE auto-install |
| `CLAUDE_CODE_IDE_HOST_OVERRIDE` | — | Override IDE host detection |
| `CLAUDE_CODE_AUTO_CONNECT_IDE` | — | Auto-connect to IDE |
| `FORCE_CODE_TERMINAL` | `false` | Force code terminal mode |
| `VSCODE_GIT_ASKPASS_MAIN` | ambient | Detect VS Code terminal environment |
| `CURSOR_TRACE_ID` | ambient | Detect Cursor editor environment |
| `VisualStudioVersion` | ambient | Detect Visual Studio environment |
| `INTELLIJ_TERMINAL_COMMAND_BLOCKS` | — | Presence (any value) indicates the JetBrains IntelliJ terminal command-blocks integration is active |
| `INTELLIJ_TERMINAL_COMMAND_BLOCKS_REWORKED` | — | Presence (any value) indicates the reworked JetBrains terminal command-blocks integration is active |

## UI & Display

| Variable | Default | Description |
|---|---|---|
| `CLAUDE_CODE_ACCESSIBILITY` | `false` | Enable accessibility mode |
| `CLAUDE_CODE_SYNTAX_HIGHLIGHT` | — | Control syntax highlighting (`false` to disable); falls back to `BAT_THEME` |
| `BAT_THEME` | — | Fallback syntax highlight theme (bat/batcat theme name) |
| `CLAUDE_CODE_SIMPLE` | `false` | Simplified mode (disables CLAUDE.md, attachments, etc.) |
| `CLAUDE_CODE_BRIEF` | `false` | Brief output mode |
| `CLAUDE_CODE_BRIEF_UPLOAD` | `false` | Brief mode for uploads |
| `CLAUDE_CODE_EMIT_TOOL_USE_SUMMARIES` | `false` | Emit tool use summaries |
| `CLAUDE_CODE_NO_FLICKER` | — | Control flicker reduction (`true` to enable, `false` to disable) |
| `CLAUDE_CODE_SCROLL_SPEED` | — | Override scroll speed |
| `CLAUDE_CODE_DISABLE_MOUSE` | `false` | Disable mouse input entirely |
| `CLI_WIDTH` | ambient | Override terminal width detection |
| `CLAUDE_CODE_FORCE_FULLSCREEN_UPSELL` | `false` | Force fullscreen upsell display |
| `CLAUDE_CODE_TUI_JUST_SWITCHED` | — | Internal: track TUI mode switch state (set/dropped across process restarts) |
| `CLAUDE_CODE_DECSTBM` | — | When truthy, force-enable DECSTBM (top/bottom-margin) scroll-region rendering; bypasses the `tengu_marlin_porch` feature-flag gate |
| `CLAUDE_CODE_HIDE_CWD` | — | Truthy hides the current working directory from the TUI footer/status line |
| `CLAUDE_CODE_NATIVE_CURSOR` | — | Truthy enables native terminal cursor rendering instead of the simulated one |
| `CLAUDE_CODE_FORCE_SYNC_OUTPUT` | — | Truthy forces synchronous TTY output, bypassing terminal-based auto-detection |
| `CLAUDE_CODE_ALT_SCREEN_FULL_REPAINT` | — | Truthy forces a full repaint of the alternate screen buffer on each render instead of incremental updates (v2.1.145) |

## Sandbox

| Variable | Default | Description |
|---|---|---|
| `CLAUDE_CODE_BUBBLEWRAP` | — | Set to `1` when running inside bubblewrap sandbox |
| `CLAUDE_CODE_SANDBOXED` | — | Short-circuit trust check; when truthy, the project is always treated as trusted |
| `CLAUDE_CODE_HOST_HTTP_PROXY_PORT` | — | Host HTTP proxy port for sandbox |
| `CLAUDE_CODE_HOST_SOCKS_PROXY_PORT` | — | Host SOCKS proxy port for sandbox |
| `CLAUDE_CODE_HOST_PLATFORM` | — | Host platform when running in sandbox |
| `CLAUDE_CODE_BASH_SANDBOX_SHOW_INDICATOR` | `false` | Show sandbox indicator in bash |
| `CLAUDE_BG_TCC_DISCLAIMED` | — | Set by the parent and consumed (then deleted from env) to signal that a background process has been disclaimed from the macOS TCC privacy prompt (v2.1.145) |
| `CLAUDE_CODE_SCRIPT_CAPS` | — | Override the sandbox script capability set for bash-tool executions |
| `CLAUDE_CODE_MCP_ALLOWLIST_ENV` | — | Override the sandbox environment allowlist forwarded to MCP server subprocesses |
| `IS_SANDBOX` | — | Set to `1` inside sandbox |

## Agent SDK

| Variable | Default | Description |
|---|---|---|
| `CLAUDE_AGENT_SDK_VERSION` | — | Agent SDK version string |
| `CLAUDE_AGENT_SDK_CLIENT_APP` | — | Agent SDK client app identifier |
| `CLAUDE_AGENT_SDK_DISABLE_BUILTIN_AGENTS` | `false` | Disable built-in agent types |
| `CLAUDE_AGENT_SDK_MCP_NO_PREFIX` | `false` | Don't prefix MCP tool names in SDK mode |
| `CLAUDE_CODE_ENTRYPOINT` | `cli` | Entry point identifier (`cli`, `sdk-ts`, `sdk-py`, `sdk-cli`, `local-agent`, `claude-desktop`, `remote`, `mcp`, `claude-vscode`, `claude-code-github-action`) |
| `CLAUDE_CODE_EMIT_SESSION_STATE_EVENTS` | `false` | Emit session state change events in SDK mode |
| `CLAUDE_CODE_INCLUDE_PARTIAL_MESSAGES` | `false` | Include partial stream events/messages in SDK output |
| `CLAUDE_CODE_AGENT_LIST_IN_MESSAGES` | — | Control agent list inclusion in messages (`true` to always include, `false` to never) |
| `CLAUDE_CODE_AGENT_COST_STEER` | — | Enable agent cost steering (`true` to enable, `false` to disable) |
| `CLAUDE_CODE_SDK_HAS_OAUTH_REFRESH` | `false` | Signal that the SDK host can refresh OAuth tokens on behalf of Claude Code (gated to specific entry points) |
| `CLAUDE_CODE_ENABLE_APPEND_SUBAGENT_PROMPT` | `false` | Gate for opting-in to appending `appendSubagentSystemPrompt` (from the Agent SDK options) onto every Task-tool subagent's system prompt (propagates to nested subagents) |
| `AI_AGENT` | `claude-code/harness` | Identifier of the AI agent harness running the CLI; auto-set if missing or already a `claude-code` value, and re-set to `claude-code/agent` inside spawned subagents |
| `SDK_NATIVE_BIN` | `claude` | Path/name of the native Claude binary the Agent SDK spawns |
| `CLAUDE_AGENTS_SELECT` | — | Pre-selected agent name(s) passed through to a spawned `claude agents` invocation; consumed and re-injected per child |
| `CLAUDE_CODE_AGENT` | — | Agent name recorded on the session metadata (alongside `CLAUDE_CODE_SESSION_NAME`/`_LOG`) |
| `CLAUDE_CODE_FORK_SUBAGENT` | — | Truthy forces the fork-based subagent execution path (otherwise governed by the `tengu_copper_fox` flag) |
| `CLAUDE_SUBAGENT_BG_SHELL_MAX_MS` | — | Max milliseconds a subagent background shell may run; falls back to the built-in `o53` default |

## Teams / Teammates

| Variable | Default | Description |
|---|---|---|
| `CLAUDE_CODE_EXPERIMENTAL_AGENT_TEAMS` | `false` | Enable experimental agent teams |
| `CLAUDE_CODE_TEAMMATE_COMMAND` | — | Command to spawn teammate processes |
| `CLAUDE_CODE_TEAM_ONBOARDING` | — | Force team onboarding flavor: `banner` shows a persistent banner, `step` inserts a dedicated onboarding step |
| `CLAUDE_CODE_PLAN_V2_AGENT_COUNT` | — | Number of agents in plan v2 |
| `CLAUDE_CODE_PLAN_V2_EXPLORE_AGENT_COUNT` | — | Number of explore agents in plan v2 |
| `CLAUDE_CODE_PLAN_MODE_INTERVIEW_PHASE` | — | Plan mode interview phase config |
| `CLAUDE_CODE_PLAN_MODE_REQUIRED` | `false` | Force plan mode |
| `CLAUDE_CODE_TMUX_SESSION` | — | Tmux session name for teams |
| `CLAUDE_CODE_TMUX_PREFIX` | — | Tmux prefix key |
| `CLAUDE_CODE_TMUX_PREFIX_CONFLICTS` | — | Whether tmux prefix conflicts exist |
| `CLAUDE_CODE_TMUX_TRUECOLOR` | — | Override truecolor detection for tmux sessions |
| `TEAM_MEMORY_SYNC_URL` | — | URL for team memory synchronization |

## Plugins / Cowork

| Variable | Default | Description |
|---|---|---|
| `CLAUDE_CODE_USE_COWORK_PLUGINS` | `false` | Enable cowork plugins |
| `CLAUDE_CODE_PLUGIN_CACHE_DIR` | — | Plugin cache directory |
| `CLAUDE_CODE_PLUGIN_SEED_DIR` | — | Plugin seed directory |
| `CLAUDE_CODE_PLUGIN_GIT_TIMEOUT_MS` | — | Git timeout for plugin operations (ms) |
| `CLAUDE_CODE_PLUGIN_USE_ZIP_CACHE` | `false` | Use zip cache for plugins |
| `CLAUDE_CODE_PLUGIN_KEEP_MARKETPLACE_ON_FAILURE` | `false` | Keep marketplace state on plugin failure |
| `CLAUDE_CODE_SYNC_PLUGIN_INSTALL` | `false` | Synchronous plugin installation |
| `CLAUDE_CODE_SYNC_PLUGIN_INSTALL_TIMEOUT_MS` | — | Timeout for synchronous plugin installation |
| `FORCE_AUTOUPDATE_PLUGINS` | `false` | Force auto-update plugins |
| `CLAUDE_PLUGIN_ROOT` | injected for plugin execution | Absolute path to the active plugin root, exposed to plugin commands and templated config expansion |
| `CLAUDE_PLUGIN_DATA` | injected for plugin execution | Per-plugin data directory, exposed to plugin commands and templated config expansion |
| `CLAUDE_CODE_PLUGIN_PREFER_HTTPS` | — | Truthy makes plugin git operations prefer HTTPS over SSH (also implied by `CLAUDE_CODE_REMOTE`) |

## Memory & Prompt History

| Variable | Default | Description |
|---|---|---|
| `CLAUDE_COWORK_MEMORY_PATH_OVERRIDE` | — | Override cowork memory path |
| `CLAUDE_COWORK_MEMORY_EXTRA_GUIDELINES` | — | Extra guidelines for cowork memory |
| `CLAUDE_CODE_SKIP_PROMPT_HISTORY` | `false` | Skip saving prompt history |
| `CLAUDE_CODE_ADDITIONAL_DIRECTORIES_CLAUDE_MD` | `false` | Scan additional directories for CLAUDE.md |
| `CLAUDE_COWORK_MEMORY_GUIDELINES` | — | Extra guideline text inserted into the cowork memory system prompt |
| `CLAUDE_COWORK_MEMORY_INDEX_CONTENT` | — | Override content for the cowork auto-memory index; empty string disables the index, otherwise parsed in place of the file |
| `CLAUDE_MEMORY_STORES` | — | JSON-encoded memory-store configuration; parsed at load time and rejected with an error if it is not valid JSON (v2.1.145) |

## Idle, Resume & Background

| Variable | Default | Description |
|---|---|---|
| `CLAUDE_CODE_IDLE_THRESHOLD_MINUTES` | `75` | Minutes of idle time before idle behavior triggers |
| `CLAUDE_CODE_IDLE_TOKEN_THRESHOLD` | `100000` | Token threshold for idle detection |
| `CLAUDE_CODE_RESUME_THRESHOLD_MINUTES` | `70` | Minutes threshold for resume-interrupted-turn eligibility |
| `CLAUDE_CODE_RESUME_TOKEN_THRESHOLD` | `100000` | Token threshold for resume-interrupted-turn eligibility |
| `CLAUDE_CODE_RESUME_INTERRUPTED_TURN` | — | Resume an interrupted turn/session by identifier or token |
| `CLAUDE_AUTO_BACKGROUND_TASKS` | `false` | Auto-spawn background tasks |
| `CLAUDE_BG_BACKEND` | — | Set to `daemon` to run background tasks via the daemon backend (ignores SIGHUP so the daemon survives terminal disconnects) |
| `CLAUDE_ASYNC_AGENT_STALL_TIMEOUT_MS` | `600000` | Stall timeout (ms) for async-agent turns; turns with no progress for this long are considered stalled |
| `CLAUDE_CODE_RESUME_PROMPT` | `Continue from where you left off.` | Prompt text injected when a session is resumed via `--resume` |
| `CLAUDE_CODE_LOOP_PERSISTENT` | — | Truthy enables persistent loop behavior in long-running session loops |
| `CLAUDE_CODE_DAEMON_COLD_START` | — | Overrides daemon cold-start behavior; accepts `transient` (spawn for this login) or `ask` (prompt to install persistently) |
| `CLAUDE_CODE_SESSION_ID` | — | Current session UUID; re-randomized on `/clear`-style resets and propagated to child processes |
| `CLAUDE_CODE_SESSION_KIND` | — | Session classification; `bg` marks a background-agent session and forces full-screen / bg-specific code paths |
| `CLAUDE_CODE_SESSION_LOG` | — | Path to write the session log file for the current session |
| `CLAUDE_CODE_SESSION_NAME` | — | Human-readable session label recorded on session metadata |
| `CLAUDE_JOB_DIR` | — | Directory representing the current background job; basename is used as the `jobId` in session metadata |
| `CLAUDE_BG_SOURCE` | `shell` | Origin label for a background session (e.g. `shell`); stripped before passing env to children |
| `CLAUDE_BG_ISOLATION` | — | Background-session isolation mode; `worktree` instructs the subagent to call `EnterWorktree` before any file or command action |
| `CLAUDE_BG_AUTH_SNAPSHOT_PATH` | — | Path to an auth snapshot file used to seed a background job's credentials; consumed and deleted on read |
| `CLAUDE_BG_RENDEZVOUS_SOCK` | — | Unix socket path used by background jobs to rendezvous with the parent daemon; consumed and deleted on read |
| `CLAUDE_BG_STARTUP_WEDGE_MS` | `45000` | Milliseconds to delay (`unref`'d timer) before the background job's startup wedge fires |

## Miscellaneous

| Variable | Default | Description |
|---|---|---|
| `CLAUDE_CODE_PERFORCE_MODE` | `false` | Enable Perforce-aware code paths (extra safeguards/adaptations for Perforce workspaces) |
| `CLAUDE_CODE_EXTRA_BODY` | — | Extra JSON body to include in API requests |
| `CLAUDE_CODE_EXTRA_METADATA` | — | JSON object merged into the emitted `user_id` telemetry payload |
| `CLAUDE_CODE_ATTRIBUTION_HEADER` | — | Custom attribution header |
| `CLAUDE_CODE_BASE_REF` | — | Override base git ref for diffs |
| `CLAUDE_CODE_BASE_REFS` | — | Override base git refs (comma-separated list) |
| `CLAUDE_CODE_REPO_CHECKOUTS` | — | Repository checkout paths |
| `CLAUDE_CODE_ENVIRONMENT_KIND` | — | Environment kind label (e.g. `bridge`) |
| `CLAUDE_CODE_TAGS` | — | Tags for telemetry |
| `CLAUDE_CODE_WORKER_EPOCH` | — | Worker epoch for process management |
| `CLAUDE_CODE_SESSIONEND_HOOKS_TIMEOUT_MS` | — | Timeout for session-end hooks (ms) |
| `CLAUDE_CODE_STOP_HOOK_BLOCK_CAP` | — | Raises the cap on how many consecutive times a Stop/SubagentStop hook may block a turn before Claude Code overrides it and ends the turn; parsed as an integer (v2.1.145) |
| `SCREENSHOT_DIR` | `/tmp/shots` | Output directory for screenshots captured by bundled helper/skill code (v2.1.145) |
| `CLAUDE_CODE_DONT_INHERIT_ENV` | `false` | Don't inherit env vars in spawned processes |
| `CLAUDE_CODE_SKIP_FAST_MODE_NETWORK_ERRORS` | `false` | Skip network errors in fast mode |
| `CLAUDE_CODE_SKIP_FAST_MODE_ORG_CHECK` | `false` | Skip organization check for fast mode eligibility |
| `CLAUDE_CODE_SUBPROCESS_ENV_SCRUB` | `false` | Scrub a built-in set of sensitive credentials from inherited subprocess environments before spawning child processes |
| `CLAUDE_CODE_NEW_INIT` | `false` | Use new init flow |
| `CLAUDE_CODE_TASK_LIST_ID` | — | Task list ID override |
| `CLAUDE_CODE_PROVIDER_MANAGED_BY_HOST` | `false` | Provider credentials are managed by the host application |
| `CLAUDE_BASH_MAINTAIN_PROJECT_WORKING_DIR` | `false` | Maintain working directory between bash calls |
| `CLAUDE_CODE_REPL` | — | Enable REPL mode |
| `CLAUDE_REPL_VARIANT` | — | REPL variant identifier |
| `CLAUDE_AFTER_LAST_COMPACT` | `false` | Flag set after last compaction |
| `CLAUDE_CHROME_PERMISSION_MODE` | — | Permission mode for Chrome integration |
| `CLAUDE_CODE_ACTION` | `false` | Running as a GitHub Action |
| `USE_STAGING_OAUTH` | `false` | Use staging OAuth |
| `USE_LOCAL_OAUTH` | `false` | Use local OAuth / local bridge endpoints instead of the production OAuth flow |
| `LOCAL_BRIDGE` | `false` | Alias flag that enables local OAuth / local bridge behavior |
| `CLAUDE_LOCAL_OAUTH_API_BASE` | `http://localhost:8000` | Local OAuth API base URL |
| `CLAUDE_LOCAL_OAUTH_APPS_BASE` | `http://localhost:4000` | Local OAuth apps base URL |
| `CLAUDE_LOCAL_OAUTH_CONSOLE_BASE` | `http://localhost:3000` | Local OAuth console base URL |
| `MODIFIERS_NODE_PATH` | — | Path for modifiers node |
| `AUDIO_CAPTURE_NODE_PATH` | — | Path for audio capture node |
| `COMPUTER_USE_INPUT_NODE_PATH` | — | Path for computer-use input native module |
| `COMPUTER_USE_SWIFT_NODE_PATH` | — | Path for computer-use Swift native module |
| `URL_HANDLER_NODE_PATH` | — | Path for URL handler native module |
| `VOICE_STREAM_BASE_URL` | — | WebSocket URL for voice streaming |
| `CLAUBBIT` | `false` | Running in Claubbit mode |
| `IS_DEMO` | `false` | Running in demo mode |
| `DEMO_VERSION` | — | Demo mode version string |
| `CLAUDECODE` | — | Set to `1` in spawned processes |
| `BUGHUNTER_DEV_BUNDLE_B64` | — | Base64-encoded bughunter dev bundle |
| `CLAUDE_CODE_ACCOUNT_TAGGED_ID` | — | Override the tagged account identifier emitted in OpenTelemetry metric attributes |
| `CLAUDE_CODE_ENHANCED_TELEMETRY_BETA` | inherits `ENABLE_ENHANCED_TELEMETRY_BETA` when unset | Alternate flag name for enhanced telemetry beta |
| `CLAUDE_CODE_WORKSPACE_HOST_PATHS` | — | Pipe-separated host workspace paths attached to telemetry events |
| `CLAUDE_FORCE_DISPLAY_SURVEY` | `false` | Force the feedback survey to appear when the user is otherwise eligible |
| `CLAUDE_CODE_EAGER_FLUSH` | `false` | Flush persisted session data eagerly after writes |
| `CLAUDE_CODE_IS_COWORK` | `false` | Mark the session as cowork/bridge mode for eager flushing and related behaviors |
| `CLAUDE_CODE_EXIT_AFTER_FIRST_RENDER` | `false` | Exit after the first UI render; useful for smoke tests and harnesses |
| `CLAUDE_CODE_EXIT_AFTER_STOP_DELAY` | — | Delay process exit after stop/termination logic |
| `CLAUDE_CODE_QUESTION_PREVIEW_FORMAT` | `markdown` outside SDK mode | Format for question previews: `markdown` or `html` |
| `MAX_STRUCTURED_OUTPUT_RETRIES` | `5` | Maximum retries for structured-output validation loops |
| `CLAUDE_CODE_STALL_TIMEOUT_MS_FOR_TESTING` | test default | Override the internal stall timeout in test harnesses |
| `CLAUDE_CODE_TEST_FIXTURES_ROOT` | current workspace | Override the root directory used for JSON test fixtures |
| `CLAUDE_CODE_ULTRAREVIEW_PREFLIGHT_FIXTURE` | — | Override the preflight fixture path for ultrareview |
| `FALLBACK_FOR_ALL_PRIMARY_MODELS` | — | Enable fallback for all primary models |
| `AWS_LAMBDA_BENCHMARK_MODE` | `false` | AWS SDK benchmark/testing flag observed in the bundled dependencies |
| `AWS_LAMBDA_NODEJS_NO_GLOBAL_AWSLAMBDA` | `false` | AWS Lambda Node.js runtime flag; when `"true"`/`"1"` suppresses the `awslambda` global that the bundled Lambda runtime adapter would otherwise install |
| `DO_NOT_TRACK` | — | Standard signal to disable telemetry (respects the do-not-track convention) |
| `COREPACK_ENABLE_AUTO_PIN` | forced to `0` | Corepack auto-pin is disabled by Claude Code at startup |
| `NoDefaultCurrentDirectoryInExePath` | forced to `1` | Windows security: prevent current directory in exe path resolution |
| `SWE_BENCH_RUN_ID` | — | SWE-bench run identifier for telemetry |
| `SWE_BENCH_INSTANCE_ID` | — | SWE-bench instance identifier for telemetry |
| `SWE_BENCH_TASK_ID` | — | SWE-bench task identifier for telemetry |
| `VCR_RECORD` | — | VCR recording mode for CI test replays |
| `TEST_ENABLE_SESSION_PERSISTENCE` | `false` | Enable session persistence in test mode |
| `CLAUDE_CODE_PROACTIVE` | — | Truthy turns on the proactive/Kairos assistant mode in the TUI |
| `CLAUDE_CODE_INVESTIGATE_FIRST` | — | Controls the "investigate first" prelude mode; accepts `additive`, `compact`, or a boolean-style toggle |
| `CLAUDE_CODE_MID_CONVERSATION_SYSTEM` | — | Marker string used to detect (and inject) a mid-conversation system prompt; falls back to the `tengu_fennel_kite_model` GrowthBook flag |
| `CLAUDE_CODE_SIMPLE_SYSTEM_PROMPT` | — | Truthy forces the simplified system prompt; falsy explicitly disables it |
| `CLAUDE_CODE_VERIFY_PROMPT` | — | Truthy enables verifier-style prompt validation before sending |
| `CLAUDE_CODE_SUPERVISED` | — | Truthy marks the process as supervised; uncaught exceptions/rejections exit instead of being swallowed |
| `CLAUDE_CODE_VOICE_FORWARD_INTERIMS_TYPED` | — | Truthy forwards interim voice transcription results as typed input |
| `CLAUDE_CODE_PACKAGE_MANAGER_AUTO_UPDATE` | — | Truthy opts the installer into automatic package-manager-driven self-updates |
| `BUGHUNTER_FLEET_SIZE` | `5` | Number of parallel Bug Hunter subagents to launch; default sourced from GrowthBook (`fleet_size`, capped 5–20) |

## GitHub Actions

| Variable | Default | Description |
|---|---|---|
| `GITHUB_ACTIONS` | ambient | Detect GitHub Actions and adapt telemetry/session classification |
| `GITHUB_ACTION_INPUTS` | ambient | GitHub Action input parameters |
| `GITHUB_ACTION_PATH` | ambient | Path to the running GitHub Action |
| `GITHUB_ACTOR` | ambient | GitHub username that triggered the workflow |
| `GITHUB_ACTOR_ID` | ambient | GitHub user ID that triggered the workflow |
| `GITHUB_EVENT_NAME` | ambient | GitHub event that triggered the workflow |
| `GITHUB_EVENT_PATH` | ambient | Path to the event payload JSON file (denywrite-listed inside sandbox) |
| `GITHUB_REPOSITORY` | ambient | GitHub repository (`owner/repo`) |
| `GITHUB_REPOSITORY_ID` | ambient | GitHub repository ID |
| `GITHUB_REPOSITORY_OWNER` | ambient | GitHub repository owner |
| `GITHUB_REPOSITORY_OWNER_ID` | ambient | GitHub repository owner ID |
| `GITHUB_ENV` | ambient | Path to the GitHub Actions workflow env file (denywrite-listed inside sandbox) |
| `GITHUB_TOKEN` | ambient | GitHub token used by actions; also redacted when scrubbing subprocess environments |
| `GH_TOKEN` | ambient | Alternative GitHub token name; scrubbed alongside `GITHUB_TOKEN` when `CLAUDE_CODE_SUBPROCESS_ENV_SCRUB` is enabled |
| `GITHUB_WORKSPACE` | ambient | GitHub Actions workspace directory |
| `GH_HOST` | — | Hostname for GitHub Enterprise; enterprise-token vars are only honored for hosts matching this value |
| `GH_ENTERPRISE_TOKEN` | — | Token used for GitHub Enterprise hosts that match `GH_HOST` (preferred over `GITHUB_ENTERPRISE_TOKEN`) |
| `GITHUB_ENTERPRISE_TOKEN` | — | Fallback token used for GitHub Enterprise hosts when `GH_ENTERPRISE_TOKEN` is unset |

## CI / Hosted Environment Detection

| Variable | Default | Description |
|---|---|---|
| `GITLAB_CI` | ambient | Detect GitLab CI environments |
| `BUILDKITE` | ambient | Detect Buildkite CI environments |
| `CIRCLECI` | ambient | Detect CircleCI environments |
| `RUNNER_ENVIRONMENT` | ambient | Detect GitHub-hosted or self-hosted runners |
| `RUNNER_OS` | ambient | Detect runner OS in CI environments |
| `SYSTEM_OIDCREQUESTURI` | ambient | Detect Azure DevOps / Azure Pipelines environments |
| `CODESPACES` | ambient | Detect GitHub Codespaces |
| `CODER` | ambient | Detect Coder environments |
| `CODER_WORKSPACE_NAME` | ambient | Detect Coder workspace name |
| `DEVPOD` | ambient | Detect DevPod environments |
| `DEVPOD_WORKSPACE_UID` | ambient | Detect DevPod workspace UID |
| `DAYTONA_WS_ID` | ambient | Detect Daytona environments |
| `CLOUD_WORKSTATIONS_CLUSTER_ID` | ambient | Detect Google Cloud Workstations |
| `GOOGLE_CLOUD_WORKSTATIONS` | — | Truthy presence flags the session as running inside a Google Cloud Workstation |
| `C9_PID` | ambient | Detect Cloud9 environments |
| `C9_USER` | ambient | Detect Cloud9 user |
| `GITPOD_WORKSPACE_ID` | ambient | Detect Gitpod |
| `REPL_ID` | ambient | Detect Replit when `REPL_ID` is present |
| `REPL_SLUG` | ambient | Detect Replit when `REPL_SLUG` is present |
| `PROJECT_DOMAIN` | ambient | Detect Glitch from its project-domain environment |
| `VERCEL` | ambient | Detect Vercel-hosted environments |
| `RAILWAY_ENVIRONMENT_NAME` | ambient | Detect Railway-hosted environments |
| `RAILWAY_SERVICE_NAME` | ambient | Detect Railway-hosted environments |
| `RENDER` | ambient | Detect Render-hosted environments |
| `NETLIFY` | ambient | Detect Netlify-hosted environments |
| `DYNO` | ambient | Detect Heroku dynos |
| `FLY_APP_NAME` | ambient | Detect Fly.io workloads |
| `FLY_MACHINE_ID` | ambient | Detect Fly.io workloads |
| `CF_PAGES` | ambient | Detect Cloudflare Pages deployments |
| `DENO_DEPLOYMENT_ID` | ambient | Detect Deno Deploy environments |
| `AWS_LAMBDA_FUNCTION_NAME` | ambient | Detect AWS Lambda runtimes |
| `AWS_EXECUTION_ENV` | ambient | Distinguish AWS ECS/Fargate execution environments |
| `KUBERNETES_SERVICE_HOST` | ambient | Detect Kubernetes environments |
| `K_SERVICE` | ambient | Detect Google Cloud Run services |
| `K_CONFIGURATION` | ambient | Detect Google Cloud Run configuration |
| `CLOUD_RUN_JOB` | ambient | Detect Google Cloud Run jobs |
| `GOOGLE_CLOUD_PROJECT` | ambient | Detect generic Google Cloud environments and supply project context |
| `WEBSITE_SITE_NAME` | ambient | Detect Azure App Service deployments |
| `WEBSITE_SKU` | ambient | Detect Azure App Service deployments |
| `AZURE_FUNCTIONS_ENVIRONMENT` | ambient | Detect Azure Functions runtimes |
| `APP_URL` | ambient | Detect DigitalOcean App Platform when the app URL matches that host pattern |
| `SPACE_CREATOR_USER_ID` | ambient | Detect Hugging Face Spaces environments |
| `FUNCTION_NAME` | ambient | Detect Google Cloud Functions (v1) |
| `FUNCTION_TARGET` | ambient | Detect Google Cloud Functions (v2) |
| `GAE_MODULE_NAME` | ambient | Detect Google App Engine (legacy) |
| `GAE_SERVICE` | ambient | Detect Google App Engine |

## GCP Metadata & Detection

| Variable | Default | Description |
|---|---|---|
| `GCE_METADATA_HOST` | — | Override GCE metadata server hostname |
| `GCE_METADATA_IP` | — | Override GCE metadata server IP address |
| `METADATA_SERVER_DETECTION` | — | Control GCE metadata server detection behavior |
| `DETECT_GCP_RETRIES` | — | Number of retries for GCP detection |
| `CLOUDSDK_CONFIG` | — | Override the path to the `gcloud` SDK configuration directory; falls back to the platform default if unset |

## Azure Identity (Bundled SDK)

| Variable | Default | Description |
|---|---|---|
| `AZURE_CLIENT_ID` | ambient | Azure AD application (client) ID |
| `AZURE_CLIENT_SECRET` | ambient | Azure AD client secret |
| `AZURE_TENANT_ID` | ambient | Azure AD tenant ID |
| `AZURE_CLIENT_CERTIFICATE_PATH` | ambient | Path to Azure AD client certificate |
| `AZURE_CLIENT_CERTIFICATE_PASSWORD` | ambient | Password for Azure AD client certificate |
| `AZURE_CLIENT_SEND_CERTIFICATE_CHAIN` | ambient | Send certificate chain for SNI |
| `AZURE_FEDERATED_TOKEN_FILE` | ambient | Path to federated token file for workload identity |
| `AZURE_AUTHORITY_HOST` | ambient | Azure AD authority host URL |
| `AZURE_ADDITIONALLY_ALLOWED_TENANTS` | ambient | Additional allowed Azure AD tenants |
| `AZURE_IDENTITY_DISABLE_MULTITENANTAUTH` | ambient | Disable multi-tenant authentication |
| `AZURE_REGIONAL_AUTHORITY_NAME` | ambient | Azure regional authority name |
| `AZURE_POD_IDENTITY_AUTHORITY_HOST` | ambient | Azure pod identity authority host |
| `AZURE_USERNAME` | ambient | Azure username for password-based auth |
| `AZURE_PASSWORD` | ambient | Azure password for password-based auth |
| `AZURE_TOKEN_CREDENTIALS` | ambient | Azure token credentials |

## Terminal Emulator Detection

| Variable | Default | Description |
|---|---|---|
| `TERM` | ambient | Terminal type |
| `TERM_PROGRAM` | ambient | Terminal program name |
| `TERM_PROGRAM_VERSION` | ambient | Terminal program version |
| `COLORTERM` | ambient | Color terminal capability |
| `COLORFGBG` | ambient | Foreground/background color pair |
| `LC_TERMINAL` | ambient | Terminal identifier (e.g., iTerm2) |
| `ITERM_SESSION_ID` | ambient | Detect iTerm2 terminal |
| `KITTY_WINDOW_ID` | ambient | Detect Kitty terminal |
| `KONSOLE_VERSION` | ambient | Detect KDE Konsole |
| `GNOME_TERMINAL_SERVICE` | ambient | Detect GNOME Terminal |
| `TERMINAL_EMULATOR` | ambient | Generic terminal emulator name |
| `TERMINAL` | ambient | Terminal name |
| `TERMINATOR_UUID` | ambient | Detect Terminator |
| `TILIX_ID` | ambient | Detect Tilix terminal |
| `ALACRITTY_LOG` | ambient | Detect Alacritty terminal |
| `VTE_VERSION` | ambient | VTE (Virtual Terminal Emulator) version |
| `XTERM_VERSION` | ambient | xterm version |
| `ZED_TERM` | ambient | Detect Zed editor terminal |
| `WT_SESSION` | ambient | Detect Windows Terminal |
| `WSL_DISTRO_NAME` | ambient | Detect WSL (Windows Subsystem for Linux) |
| `MSYSTEM` | ambient | Detect MSYS2/MinGW environment |
| `STY` | ambient | Detect GNU Screen session |
| `TMUX` | ambient | Detect tmux session |
| `TMUX_PANE` | ambient | Detect tmux pane |
| `ZELLIJ` | ambient | Detect Zellij terminal multiplexer (gates DECSTBM and scroll-region features) |
| `ConEmuANSI` | ambient | Detect ConEmu terminal |
| `ConEmuPID` | ambient | Detect ConEmu process ID |
| `ConEmuTask` | ambient | Detect ConEmu task |
| `__CFBundleIdentifier` | ambient | Detect Conductor/macOS app bundle context via `__CFBundleIdentifier` inspection |

## System & Shell

| Variable | Default | Description |
|---|---|---|
| `HOME` | ambient | User home directory |
| `USER` | ambient | Current username |
| `USERNAME` | ambient | Current username (Windows) |
| `USERPROFILE` | ambient | User profile directory (Windows) |
| `SHELL` | ambient | User's default shell |
| `PATH` | ambient | System PATH |
| `PATHEXT` | ambient | Executable file extensions (Windows) |
| `PWD` | ambient | Current working directory |
| `EDITOR` | ambient | Default text editor |
| `VISUAL` | ambient | Default visual editor |
| `BROWSER` | ambient | Default browser |
| `LANG` | ambient | System locale |
| `LC_ALL` | ambient | Override all locale settings |
| `LC_TIME` | ambient | Time locale |
| `OSTYPE` | ambient | Operating system type |
| `SYSTEMROOT` / `SystemRoot` | ambient | Windows system root |
| `APPDATA` | ambient | Application data directory (Windows) |
| `LOCALAPPDATA` | ambient | Local application data directory (Windows) |
| `ProgramData` | ambient | Program data directory (Windows) |
| `ProgramFiles` | ambient | Program files directory (Windows) |
| `comspec` | `cmd.exe` | Windows command processor |
| `CLAUDE_CODE_POWERSHELL_RESPECT_EXECUTION_POLICY` | — | When unset/falsy, Claude Code bypasses the PowerShell execution policy when invoking `powershell`; set truthy to respect the configured policy instead (v2.1.145) |
| `CLAUDE_PTY_HOST_EXEC` | — | When set to `"1"`, routes command execution through the PTY host; consumed and deleted from the environment after it is read (v2.1.145) |
| `DISPLAY` | `:99` | X11 display read by bundled helper code; falls back to `:99` (v2.1.145) |
| `XDG_CONFIG_HOME` | ambient | XDG configuration home |
| `XDG_RUNTIME_DIR` | ambient | XDG runtime directory |
| `SESSIONNAME` | ambient | Session name (Windows) |
| `SSH_CLIENT` | ambient | Detect SSH connection |
| `SSH_CONNECTION` | ambient | Detect SSH connection details |
| `SSH_TTY` | ambient | Detect SSH TTY |
| `SAFEUSER` | ambient | Safe username identifier |
| `P4PORT` | ambient | Perforce server port |
| `PKG_CONFIG_PATH` | ambient | pkg-config search path |
| `UV_THREADPOOL_SIZE` | ambient | libuv thread pool size |
| `JEST_WORKER_ID` | ambient | Detect Jest test worker |
| `BUN_INSTALL` | ambient | Bun installer prefix path; used to identify when Claude Code is running under Bun's global install layout |
| `NODE_OPTIONS` | ambient | Node.js CLI options |
| `NODE_DEBUG` | ambient | Node.js debug modules |
| `NODE_V8_COVERAGE` | ambient | V8 code coverage output directory |
| `NO_COLOR` | ambient | Disable color output |
| `FORCE_COLOR` | ambient | Force color output (value: level 0-3) |
| `GRACEFUL_FS_PLATFORM` | ambient | Override platform for graceful-fs |
| `TEST_GRACEFUL_FS_GLOBAL_PATCH` | ambient | Test flag for graceful-fs global patching |
| `COMSPEC` | ambient | Windows shell path fallback when `SHELL` is unset |
| `PREFIX` | — | Termux install prefix; combined with `TERMUX_VERSION` to derive the tmp directory (`$PREFIX/tmp`) |
| `TERMUX_VERSION` | — | Termux version string; presence (together with `PREFIX`) switches the tmp directory to `$PREFIX/tmp` |

## gRPC (Bundled SDK)

| Variable | Default | Description |
|---|---|---|
| `GRPC_DEFAULT_SSL_ROOTS_FILE_PATH` | ambient | Path to custom gRPC SSL root certificates |
| `GRPC_SSL_CIPHER_SUITES` | ambient | Custom gRPC SSL cipher suites |
| `GRPC_NODE_TRACE` | ambient | gRPC Node.js tracing config |
| `GRPC_NODE_VERBOSITY` | ambient | gRPC Node.js verbosity level |
| `GRPC_NODE_USE_ALTERNATIVE_RESOLVER` | `false` | Use alternative DNS resolver in gRPC Node.js |
| `GRPC_EXPERIMENTAL_ENABLE_OUTLIER_DETECTION` | `true` | Enable gRPC outlier detection |
| `GRPC_TRACE` | ambient | gRPC tracing config |
| `GRPC_VERBOSITY` | ambient | gRPC verbosity level |
| `grpc_proxy` | ambient | gRPC-specific proxy URL |
| `no_grpc_proxy` | ambient | gRPC-specific proxy bypass list |

## Dependency / Library Internals

| Variable | Default | Description |
|---|---|---|
| `WS_NO_BUFFER_UTIL` | ambient | Disable ws buffer-util native addon |
| `WS_NO_UTF_8_VALIDATE` | ambient | Disable ws UTF-8 validation native addon |
| `UNDICI_NO_FG` | ambient | Disable undici FinalizationRegistry |
| `CHOKIDAR_USEPOLLING` | ambient | Force chokidar to use polling for file watching |
| `CHOKIDAR_INTERVAL` | ambient | Chokidar polling interval |
| `SRT_DEBUG` | ambient | Suppress structured-clone debug output |
| `SHARP_IGNORE_GLOBAL_LIBVIPS` | ambient | Ignore system-wide libvips installation for sharp |
| `SHARP_FORCE_GLOBAL_LIBVIPS` | ambient | Force use of system-wide libvips for sharp |
| `npm_package_config_libvips` | ambient | Override the libvips version requirement that `sharp` reports during its install/postinstall probe |

## Bash Environment Allowlist

These environment variables are passed through to Bash tool executions:

`GOEXPERIMENT`, `GOOS`, `GOARCH`, `CGO_ENABLED`, `GO111MODULE`, `RUST_BACKTRACE`, `RUST_LOG`, `NODE_ENV`, `PYTHONUNBUFFERED`, `PYTHONDONTWRITEBYTECODE`, `PYTEST_DISABLE_PLUGIN_AUTOLOAD`, `PYTEST_DEBUG`, `ANTHROPIC_API_KEY`, `LANG`, `LANGUAGE`, `LC_ALL`, `LC_CTYPE`, `LC_TIME`, `CHARSET`, `TERM`, `COLORTERM`, `NO_COLOR`, `FORCE_COLOR`, `TZ`, `LS_COLORS`, `LSCOLORS`, `GREP_COLOR`, `GREP_COLORS`, `GCC_COLORS`, `TIME_STYLE`, `BLOCK_SIZE`, `BLOCKSIZE`, `COLUMNS`, `LINES`, `CLICOLOR`, `CLICOLOR_FORCE`, `CI`, `DEBIAN_FRONTEND`, `GIT_TERMINAL_PROMPT`

## Teammate Env Vars Forwarded to Subprocesses

`CLAUDE_CODE_USE_BEDROCK`, `CLAUDE_CODE_USE_VERTEX`, `CLAUDE_CODE_USE_FOUNDRY`, `CLAUDE_CODE_USE_ANTHROPIC_AWS`, `CLAUDE_CODE_USE_MANTLE`, `ANTHROPIC_AWS_WORKSPACE_ID`, `ANTHROPIC_AWS_BASE_URL`, `ANTHROPIC_AWS_API_KEY`, `CLAUDE_CODE_SKIP_ANTHROPIC_AWS_AUTH`, `AWS_BEARER_TOKEN_BEDROCK`, `ANTHROPIC_BEDROCK_MANTLE_BASE_URL`, `CLAUDE_CODE_SKIP_MANTLE_AUTH`, `AWS_REGION`, `ANTHROPIC_BASE_URL`, `CLAUDE_CONFIG_DIR`, `CLAUDE_CODE_REMOTE`, `CLAUDE_CODE_REMOTE_MEMORY_DIR`, `HTTPS_PROXY`, `https_proxy`, `HTTP_PROXY`, `http_proxy`, `NO_PROXY`, `no_proxy`, `SSL_CERT_FILE`, `NODE_EXTRA_CA_CERTS`, `REQUESTS_CA_BUNDLE`, `CURL_CA_BUNDLE`

Teammate launchers also inject `CLAUDECODE=1` and `CLAUDE_CODE_EXPERIMENTAL_AGENT_TEAMS=1` before appending the forwarded parent environment variables above.

## Provider-Sensitive Env Vars

These environment variables are tracked as provider-sensitive (changes may trigger re-authentication):

`CLAUDE_CODE_PROVIDER_MANAGED_BY_HOST`, `CLAUDE_CODE_USE_BEDROCK`, `CLAUDE_CODE_USE_VERTEX`, `CLAUDE_CODE_USE_FOUNDRY`, `CLAUDE_CODE_USE_ANTHROPIC_AWS`, `CLAUDE_CODE_USE_MANTLE`, `ANTHROPIC_BASE_URL`, `ANTHROPIC_BEDROCK_BASE_URL`, `ANTHROPIC_VERTEX_BASE_URL`, `ANTHROPIC_FOUNDRY_BASE_URL`, `ANTHROPIC_AWS_BASE_URL`, `ANTHROPIC_BEDROCK_MANTLE_BASE_URL`, `ANTHROPIC_FOUNDRY_RESOURCE`, `ANTHROPIC_VERTEX_PROJECT_ID`, `ANTHROPIC_AWS_WORKSPACE_ID`, `CLOUD_ML_REGION`, `ANTHROPIC_API_KEY`, `ANTHROPIC_AUTH_TOKEN`, `CLAUDE_CODE_OAUTH_TOKEN`, `AWS_BEARER_TOKEN_BEDROCK`, `ANTHROPIC_FOUNDRY_API_KEY`, `ANTHROPIC_AWS_API_KEY`, `ANTHROPIC_BEDROCK_MANTLE_API_KEY`, `CLAUDE_CODE_SKIP_BEDROCK_AUTH`, `CLAUDE_CODE_SKIP_VERTEX_AUTH`, `CLAUDE_CODE_SKIP_FOUNDRY_AUTH`, `CLAUDE_CODE_SKIP_ANTHROPIC_AWS_AUTH`, `CLAUDE_CODE_SKIP_MANTLE_AUTH`, `ANTHROPIC_MODEL`, `ANTHROPIC_DEFAULT_HAIKU_MODEL`, `ANTHROPIC_DEFAULT_HAIKU_MODEL_DESCRIPTION`, `ANTHROPIC_DEFAULT_HAIKU_MODEL_NAME`, `ANTHROPIC_DEFAULT_HAIKU_MODEL_SUPPORTED_CAPABILITIES`, `ANTHROPIC_DEFAULT_OPUS_MODEL`, `ANTHROPIC_DEFAULT_OPUS_MODEL_DESCRIPTION`, `ANTHROPIC_DEFAULT_OPUS_MODEL_NAME`, `ANTHROPIC_DEFAULT_OPUS_MODEL_SUPPORTED_CAPABILITIES`, `ANTHROPIC_DEFAULT_SONNET_MODEL`, `ANTHROPIC_DEFAULT_SONNET_MODEL_DESCRIPTION`, `ANTHROPIC_DEFAULT_SONNET_MODEL_NAME`, `ANTHROPIC_DEFAULT_SONNET_MODEL_SUPPORTED_CAPABILITIES`, `ANTHROPIC_SMALL_FAST_MODEL`, `ANTHROPIC_SMALL_FAST_MODEL_AWS_REGION`, `CLAUDE_CODE_SUBAGENT_MODEL`, `CLAUDE_CODE_CERT_STORE`

Provider-sensitive prefixes (any env var starting with these is also tracked): `VERTEX_REGION_CLAUDE_`

## Config-Panel Env Vars

These environment variables can be overridden via the config panel / settings UI:

`ANTHROPIC_CUSTOM_HEADERS`, `ANTHROPIC_CUSTOM_MODEL_OPTION`, `ANTHROPIC_CUSTOM_MODEL_OPTION_DESCRIPTION`, `ANTHROPIC_CUSTOM_MODEL_OPTION_NAME`, `ANTHROPIC_CUSTOM_MODEL_OPTION_SUPPORTED_CAPABILITIES`, `ANTHROPIC_DEFAULT_HAIKU_MODEL`, `ANTHROPIC_DEFAULT_HAIKU_MODEL_DESCRIPTION`, `ANTHROPIC_DEFAULT_HAIKU_MODEL_NAME`, `ANTHROPIC_DEFAULT_HAIKU_MODEL_SUPPORTED_CAPABILITIES`, `ANTHROPIC_DEFAULT_OPUS_MODEL`, `ANTHROPIC_DEFAULT_OPUS_MODEL_DESCRIPTION`, `ANTHROPIC_DEFAULT_OPUS_MODEL_NAME`, `ANTHROPIC_DEFAULT_OPUS_MODEL_SUPPORTED_CAPABILITIES`, `ANTHROPIC_DEFAULT_SONNET_MODEL`, `ANTHROPIC_DEFAULT_SONNET_MODEL_DESCRIPTION`, `ANTHROPIC_DEFAULT_SONNET_MODEL_NAME`, `ANTHROPIC_DEFAULT_SONNET_MODEL_SUPPORTED_CAPABILITIES`, `ANTHROPIC_FOUNDRY_API_KEY`, `ANTHROPIC_MODEL`, `ANTHROPIC_SMALL_FAST_MODEL_AWS_REGION`, `ANTHROPIC_SMALL_FAST_MODEL`, `AWS_DEFAULT_REGION`, `AWS_PROFILE`, `AWS_REGION`, `BASH_DEFAULT_TIMEOUT_MS`, `BASH_MAX_OUTPUT_LENGTH`, `BASH_MAX_TIMEOUT_MS`, `CLAUDE_BASH_MAINTAIN_PROJECT_WORKING_DIR`, `CLAUDE_CODE_API_KEY_HELPER_TTL_MS`, `CLAUDE_CODE_DISABLE_EXPERIMENTAL_BETAS`, `CLAUDE_CODE_DISABLE_NONESSENTIAL_TRAFFIC`, `CLAUDE_CODE_DISABLE_TERMINAL_TITLE`, `CLAUDE_CODE_ENABLE_TELEMETRY`, `CLAUDE_CODE_EXPERIMENTAL_AGENT_TEAMS`, `CLAUDE_CODE_IDE_SKIP_AUTO_INSTALL`, `CLAUDE_CODE_MAX_OUTPUT_TOKENS`, `CLAUDE_CODE_SKIP_BEDROCK_AUTH`, `CLAUDE_CODE_SKIP_FOUNDRY_AUTH`, `CLAUDE_CODE_SKIP_ANTHROPIC_AWS_AUTH`, `CLAUDE_CODE_SKIP_MANTLE_AUTH`, `CLAUDE_CODE_SKIP_VERTEX_AUTH`, `CLAUDE_CODE_SUBAGENT_MODEL`, `CLAUDE_CODE_USE_BEDROCK`, `CLAUDE_CODE_USE_FOUNDRY`, `CLAUDE_CODE_USE_ANTHROPIC_AWS`, `CLAUDE_CODE_USE_MANTLE`, `CLAUDE_CODE_USE_VERTEX`, `DISABLE_AUTOUPDATER`, `DISABLE_BUG_COMMAND`, `DISABLE_COST_WARNINGS`, `DISABLE_ERROR_REPORTING`, `DISABLE_FEEDBACK_COMMAND`, `DISABLE_INSTALLATION_CHECKS`, `DISABLE_TELEMETRY`, `ENABLE_TOOL_SEARCH`, `MAX_MCP_OUTPUT_TOKENS`, `MAX_THINKING_TOKENS`, `MCP_TIMEOUT`, `MCP_TOOL_TIMEOUT`, `OTEL_EXPORTER_OTLP_HEADERS`, `OTEL_EXPORTER_OTLP_LOGS_HEADERS`, `OTEL_EXPORTER_OTLP_LOGS_PROTOCOL`, `OTEL_EXPORTER_OTLP_METRICS_CLIENT_CERTIFICATE`, `OTEL_EXPORTER_OTLP_METRICS_CLIENT_KEY`, `OTEL_EXPORTER_OTLP_METRICS_HEADERS`, `OTEL_EXPORTER_OTLP_METRICS_PROTOCOL`, `OTEL_EXPORTER_OTLP_PROTOCOL`, `OTEL_EXPORTER_OTLP_TRACES_HEADERS`, `OTEL_LOG_TOOL_CONTENT`, `OTEL_LOG_TOOL_DETAILS`, `OTEL_LOG_USER_PROMPTS`, `OTEL_LOGS_EXPORT_INTERVAL`, `OTEL_LOGS_EXPORTER`, `OTEL_METRIC_EXPORT_INTERVAL`, `OTEL_METRICS_EXPORTER`, `OTEL_METRICS_INCLUDE_ACCOUNT_UUID`, `OTEL_METRICS_INCLUDE_SESSION_ID`, `OTEL_METRICS_INCLUDE_VERSION`, `OTEL_RESOURCE_ATTRIBUTES`, `USE_BUILTIN_RIPGREP`, `VERTEX_REGION_CLAUDE_3_5_HAIKU`, `VERTEX_REGION_CLAUDE_3_5_SONNET`, `VERTEX_REGION_CLAUDE_3_7_SONNET`, `VERTEX_REGION_CLAUDE_4_0_OPUS`, `VERTEX_REGION_CLAUDE_4_0_SONNET`, `VERTEX_REGION_CLAUDE_4_1_OPUS`, `VERTEX_REGION_CLAUDE_4_5_OPUS`, `VERTEX_REGION_CLAUDE_4_6_OPUS`, `VERTEX_REGION_CLAUDE_4_7_OPUS`, `VERTEX_REGION_CLAUDE_4_5_SONNET`, `VERTEX_REGION_CLAUDE_4_6_SONNET`, `VERTEX_REGION_CLAUDE_HAIKU_4_5`

## Sensitive Env Vars Redacted from Logs

These environment variables are redacted from debug logs and subprocess visibility:

`ANTHROPIC_API_KEY`, `CLAUDE_CODE_OAUTH_TOKEN`, `ANTHROPIC_AUTH_TOKEN`, `ANTHROPIC_FOUNDRY_API_KEY`, `ANTHROPIC_AWS_API_KEY`, `ANTHROPIC_BEDROCK_MANTLE_API_KEY`, `ANTHROPIC_CUSTOM_HEADERS`, `OTEL_EXPORTER_OTLP_HEADERS`, `OTEL_EXPORTER_OTLP_LOGS_HEADERS`, `OTEL_EXPORTER_OTLP_METRICS_HEADERS`, `OTEL_EXPORTER_OTLP_TRACES_HEADERS`, `AWS_SECRET_ACCESS_KEY`, `AWS_SESSION_TOKEN`, `AWS_BEARER_TOKEN_BEDROCK`, `GOOGLE_APPLICATION_CREDENTIALS`, `AZURE_CLIENT_SECRET`, `AZURE_CLIENT_CERTIFICATE_PATH`, `ACTIONS_ID_TOKEN_REQUEST_TOKEN`, `ACTIONS_ID_TOKEN_REQUEST_URL`, `ACTIONS_RUNTIME_TOKEN`, `ACTIONS_RUNTIME_URL`, `ALL_INPUTS`, `OVERRIDE_GITHUB_TOKEN`, `DEFAULT_WORKFLOW_TOKEN`, `SSH_SIGNING_KEY`

## Removed / Legacy

These variables were documented in prior revisions of this file but are no longer read by the bundled runtime as of v2.1.112. They are retained here for historical reference.

| Variable | Last Seen | Notes |
|---|---|---|
| `CLAUDE_REPL_MODE` | ≤ v2.1.97 | Superseded by `CLAUDE_CODE_REPL` for gating REPL behavior |
| `CLAUDE_CODE_SAVE_HOOK_ADDITIONAL_CONTEXT` | — | Hook-context persistence flag; no matching `process.env` read in v2.1.112 |
| `GITHUB_PATH` | — | GitHub Actions `GITHUB_PATH` file path — no matching `process.env` read in v2.1.112 |
| `GITHUB_OUTPUT` | — | GitHub Actions step output file path — no matching `process.env` read in v2.1.112 |
| `GITHUB_STATE` | — | GitHub Actions step state file path — no matching `process.env` read in v2.1.112 |
| `GITHUB_STEP_SUMMARY` | — | GitHub Actions step summary file path — no matching `process.env` read in v2.1.112 |
