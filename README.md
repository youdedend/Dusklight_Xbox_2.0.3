# Build this UWP candidate in GitHub Actions

No local Windows PC, signing certificate, personal access token, or published Aurora branch is required for this **build attempt**. The workflow restores the exact saved candidates from two incremental Git bundles, then builds on a GitHub-hosted Windows runner. It does not push changes or publish releases.

## One-time setup

Commit these three files, preserving their paths, to the **default branch of a repository you control**. A personal fork or a dedicated build repository works; write access to XboxEmulationHub or SternXD is not required.

```text
.github/workflows/uwp-candidate.yml
ci/port-candidate/dusklight.bundle
ci/port-candidate/aurora.bundle
```

You can use GitHub's web file-upload interface or your normal Git client. The bundles contain code and commit metadata, not credentials.

Then open **Actions → Build UWP candidate (Windows) → Run workflow**. Enable Actions if the repository requires it. GitHub-hosted runner usage may consume Actions minutes according to the repository's plan.

## What to expect

This is the first complete Windows build attempt for an unverified port. It may fail because of source incompatibilities, missing UWP components on the hosted runner, dependencies, compiler errors or link errors. Those failures are useful diagnostics, not evidence of a completed port.

The workflow requests an unsigned package and saves `uwp-windows-diagnostics` even when the build step fails. Share the failed job log or download/upload that diagnostic artifact so the errors can be fixed. If source restoration fails before the build, use the Actions job log because a transcript may not yet exist.

A successful build does not prove Xbox compatibility. The unsigned package must be signed appropriately before installation. Code-mod support remains disabled/unfinished in this candidate.

## Security and scope

- Workflow permission is `contents: read`; checkout does not persist its credentials.
- No personal access token or signing key is needed or requested.
- It does not publish the Aurora or Dusklight branches, modify repositories, sign binaries, or create a release.
- It downloads dependencies and runs the recovered source/build scripts on an ephemeral runner.
- The YAML has been checked structurally here, but this workflow has **not yet been executed on GitHub or Windows**.
