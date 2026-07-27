## AGENTS.md for ShadowMask

### General

- Prefix every shell command with `scripts/env.py` to ensure that commands are executed with the correct environment.

### App

- All application related source code lives under `app` directory. When working on the application codebase, use `app` as the working directory.
- The `app` directory is itself a Gradle project. Use `./gradlew` with corresponding tasks to build the app.
- The ShadowMask app is written in Kotlin and Java. Prefer Kotlin for all new code.
- After doing changes in `app`, make sure to build the relevant modules to ensure they build successfully.
