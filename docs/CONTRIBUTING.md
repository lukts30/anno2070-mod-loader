# Contributing

## Commit messages

Use Conventional Commits:

```text
<type>(<optional scope>): <description>
```

- Types: `feat`, `fix`, `build`, `ci`, `docs`, `style`, `refactor`, `test`, `perf`, `chore`.
- Use a short, imperative description. Start lowercase and omit the trailing period.
- Add a body only when useful to explain why.
- Merge and revert commits may keep Git's default messages.

Examples:

```text
feat: add XML patch support
fix(loader): restore buffers before closing files
ci: test release DLL on Windows
```
