# Agent Rules (Unreal Engine 5.8, project: Nowaproba)

## 1. Role & priority
Implementation agent, not designer. Do only what the task asks; no scope expansion.
Priority when sources conflict: user task > this file > Docs/ARCHITECTURE.md (if exists) > existing implementation > own assumptions.
Do not invent requirements.

## 2. Project facts
- C++ + Blueprint project (runtime module Source/Nowaproba/, subfolders per system). Prefer Blueprints; use C++ only where Blueprints cannot do it or it is clearly impractical (e.g. GAS AttributeSets, performance-critical or complex systems), exposing it to Blueprints (BlueprintCallable/BlueprintImplementableEvent/UPROPERTY) so behaviour and tuning stay in Blueprints/Data Assets.
- New C++ classes go in the matching Source/Nowaproba/<System>/ folder. New modules, plugins, Build.cs dependency or .uproject changes require user approval first.
- Editor tools are available via unreal-mcp (http://127.0.0.1:8000/mcp) only while the editor is running.
- Blender is available via blender-mcp only while Blender is running with the addon connected.
- Modify Content assets (.uasset/.umap) only through the editor/MCP, never by editing files on disk.
- Source art lives outside Content: SourceArt/<Topic>/ (.blend files, exported .fbx). Blender generator scripts live in Tools/Blender/.
- Git repo, branch main, remote origin = https://github.com/Bokiczownik/gametest
- Only code, scripts, config and docs go to git (including Tools/**). Assets (.uasset/.umap/media) and source art binaries (.blend/.fbx/.glb) are ignored and stay local; never commit or force-add them.
- Uncommitted changes found at task start are the user's work: do not include them in your commit unless the task concerns them; never reset, force-push, rewrite history, or delete unrelated files.

## 3. Token & work efficiency
- Read Docs/PROJECT_MAP.txt first if the task touches existing systems; it may already answer where things are.
- Read/search only what the task needs; targeted search, never full-project sweeps.
- Never re-read unchanged files or redo a search already done in this session.
- Before creating any system/asset, check PROJECT_MAP.txt and do one targeted search for an existing one; extend it rather than duplicate.
- After a task, add newly discovered durable facts to Docs/PROJECT_MAP.txt (one line each: system -> asset path, purpose; working verify steps). Do not record what is already there or obvious.
- No explanations of routine steps, no alternatives unless asked, no restating the task, no narration. Exception: a plan or information the task explicitly requests.
- Smallest implementation that satisfies the task.

## 4. Scope
Do not: add unrequested features/mechanics, refactor, rename, reorganize, reformat, or "improve" unrelated code/assets.
Unrelated problem found: do not fix; mention in NOTES.
Small refactor allowed only if it blocks the task and is directly related; large refactor -> BLOCKED.

## 5. Ambiguity
Small impact: take the smallest reasonable assumption, list it in NOTES, continue.
Major impact (architecture, gameplay design, save data, networking, new dependency/module): BLOCKED.

## 6. Architecture
Follow UE conventions and naming (A/U/F/E/I prefixes; asset prefixes BP_, WBP_, DA_, DT_, SM_, SK_, M_, MI_, MF_, T_, MPC_, RT_ etc.).
- No reusable gameplay logic in Level Blueprints; keep unrelated systems out of the Player Character.
- Prefer Actor Components for reusable behaviour, interfaces for decoupling, Data Assets/Data Tables for config data.
- Avoid global state, unnecessary dependencies, new patterns without need.
- Decision order: correctness > existing architecture > simplicity > maintainability > performance. Prefer boring and predictable.
- Avoid: unneeded Tick (prefer events), per-frame expensive work, uncontrolled spawning, unbounded arrays, repeated searches/casts, unneeded asset loads.
- Guard against invalid refs/destroyed actors/init order only where invalid state is realistically possible.

## 7. Dependencies & assets
- No new plugins/modules/external dependencies without approval; prefer built-in UE features.
- No placeholder assets unless asked. Never reference an asset path you have not confirmed exists; use an existing suitable asset or report it missing. New folders/assets are created only at paths given by the task or matching existing project structure.
- Do not modify engine source or generated files (Intermediate/, Saved/, DerivedDataCache/).

## 8. Errors
Fix root cause, not symptoms. If the same approach fails twice, stop and reassess. Never hide errors, warnings or failed checks.

## 9. Verification
- Blueprint changes: compile affected Blueprints via MCP and check for errors/broken references.
- C++ changes: build the editor target (NowaprobaEditor) and report errors/warnings; if the editor is running, use Live Coding or ask the user to restart it.
- Imported meshes/textures: confirm the asset exists, check scale/bounds, triangle count and import warnings via MCP.
- Blender work: verify geometry/attribute data by reading it, not only visually; use a viewport screenshot for visual checks.
- Run only checks relevant to the change.
- If the editor/MCP is unavailable, report VERIFICATION: NOT RUN (reason). Never claim untested work works.

## 10. Stop conditions -> BLOCKED
Major design/architecture decision needed; missing dependency or asset; contradictory requirements or conflict with docs; large refactor needed; risk of losing user work; unresolved critical error.

## 11. Done
DONE = requested behaviour implemented, follows existing architecture, nothing extra added, affected Blueprints compile, relevant verification passed (or explicitly NOT RUN with reason).

## 12. Commit after every change
After any task that changed tracked files (including verification NOT RUN, excluding BLOCKED with no changes):
- Stage only files you changed (git add <paths>, never git add -A blindly), commit, then git push origin main.
- Message: short imperative summary line, e.g. "Add health component to player".
- If push fails, report it in NOTES; do not retry with force.
Add the commit hash to the report under CHANGED.

## 13. Report format
If the task requires a plan first: output PLAN (max 6 lines) and wait for approval before changing anything.
Final report, only this:
STATUS: DONE | BLOCKED
CHANGED: files/assets (or none)
SUMMARY: 1-3 lines
VERIFICATION: PASS - what | FAIL - exact error | NOT RUN - reason
NOTES: assumptions, unrelated issues (omit if none)
REQUIRED: decision/info needed (only if BLOCKED)
Extra sections only when the task explicitly names them.
