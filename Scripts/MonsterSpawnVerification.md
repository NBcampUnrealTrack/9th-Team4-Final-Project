# Dedicated-server monster spawn regression

## Root cause (2026-10-23)

`TestLevel_GH` keeps its three monster spawners and baked NavMesh in the
persistent level. The ground is loaded through `DevLevel_Instance`.
The original spawner spawned during BeginPlay using only the baked NavMesh,
before the dedicated server had registered the streamed ground collision.
The resulting fall was replicated correctly to clients: the monsters existed,
but were underneath the level. Also, the NavMesh foot position was passed as
the character capsule center (the three monster BPs have half-height 88 cm).

Evidence: `Saved/Logs/MonsterSpawnBefore.log` shows all nine monsters at
Z=-11875 after five seconds and Z=-51841 after fifteen seconds, in falling
movement mode, with no supporting floor. The earlier PIE log also shows
negative mesh bounds on clients of the dedicated server.

## Fix

- Require an actual walkable ground trace using the chosen monster capsule's
  collision channel and responses before accepting a NavMesh candidate.
- Place the capsule center at the ground hit plus its scaled half-height and
  2 cm clearance. Preserve collision-aware actor spawning.
- Retry only outstanding initial spawns every 0.5 seconds until navigation,
  streamed collision and free space are available. Clear timers on EndPlay.
- Preserve the existing authority-only spawning, population cap and respawns.

No Blueprint or map asset changes are required. Existing user changes to
attributes, relevance and visual debugging were preserved.

## Repeat the network test

Close the editor and build `Project_NFEditor Win64 Development`, then run:

```powershell
& ./Scripts/TestMonsterDedicated.ps1
```

The script starts one local dedicated server and two headless clients on UDP
port 17877. Each process runs `NF.Monster.VerifySpawn`; clients exit after
35 seconds and the server after 60 seconds. Logs are under `Saved/Logs` with
`MonsterDedicated-` and `MonsterClient1/2-` prefixes. The script requires the
last sample in every log to pass. Early samples may fail while clients load.

`count=9 supported=9 visible=9 result=PASS` means nine monster actors, nine
supporting floor traces, and nine visible mesh components with mesh assets.
This checks runtime replication, positions and visibility state using NullRHI;
it is not a screenshot/render-output comparison. For a visual check, reopen
`TestLevel_GH` and run PIE as Client with two players.

Validated runs: `MonsterSpawnAfter.log` (server at 5 and 15 seconds), and
`MonsterDedicated-20261023-200241.log`,
`MonsterClient1-20261023-200241.log`,
`MonsterClient2-20261023-200241.log` (server plus two separate clients).
