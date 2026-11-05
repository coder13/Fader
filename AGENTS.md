# Project working conventions

- Maintain the long-term design in `README.md` and `docs/`. Record user requirements distinctly from proposed implementation choices and measured results.
- The user authorizes direct commits and pushes of documentation changes to the configured repository, currently `origin` at `https://github.com/coder13/Fader.git`, on `main`. Check the remote/current branch, review the diff, validate local links, and preserve unrelated user changes. Use normal pushes; do not force-push.
- This standing authorization covers documentation. Keep implementation, hardware experiments, and external issue publication within the user's current requested scope.
- Keep the initial work focused on research and the Mac/S4 MK3 POC. VST3 effects are a version 2/3 requirement. Broad local-library import is a product requirement; the completed POC includes MP3 and WAV import.
- The user's reference music library is `~/Music`. Inspect it read-only for compatibility work. Do not commit music, full track lists, or private absolute media paths. Keep public evidence to aggregate inventory and redistributable/synthetic test fixtures.

- Share screenshots at meaningful UI development checkpoints, identifying synthetic fixtures and unverified behavior. Keep user library screenshots local to the conversation.
- Reuse the existing macOS preview name, bundle identifier and app path when updating builds; do not create a new preview application for each task. Preserve unsaved sets before a relaunch. The current preview is `Fader Dark Preview`, bundle ID `org.fader.poc.dark-preview`, at `/private/tmp/Fader Dark Preview.app`.
