# Security policy

## Supported versions

Security fixes are applied to the latest source revision. Until the project reaches a stable `1.x` release, older beta revisions are not maintained separately.

## Reporting a vulnerability

Use the repository's private security-advisory channel when available. Include the affected version, descriptor or project setup, impact, and minimal reproduction. Please do not publish exploit details in a public issue before a fix is available.

If private reporting is not enabled, open a public issue requesting a private contact channel without including sensitive details.

Path traversal, malformed binary input, unsafe asset creation, and crashes caused by untrusted `.fnt` files are considered security-relevant. Reports about arbitrary files that Unreal Engine itself cannot safely decode may also need to be coordinated upstream.
