# Boss StateTree and pattern setup

## Runtime structure

- `ANF_BossBase` owns the replicated GAS Ability System Component, `UNF_BossAttributeSet`, and a `UStateTreeComponent`.
- The server StateTree starts in `Dormant` and waits until `ANF_BossBase::WakeBoss()` is called. It then has separate states for fixed-point volley, random-target volley, ground damage zones, takeoff-and-dive, center blast, and last gasp. Each combat state starts one GAS ability and waits for it to end; transitions advance through the five combat patterns and return to the first.
- Boss health is reduced by `UNFGE_PlayerDamageToBoss`. Its execution writes negative damage to the boss Health attribute and the AttributeSet records the original instigating player for damage contribution tracking. The random-target volley does not use damage contribution when selecting a target.
- Boss attacks use `UNFGE_BossDamageToPlayer` with positive `Data.Damage` SetByCaller magnitude. `UNFGE_BossStun` grants `State.Stunned` for one second, and the player movement handler disables movement while that tag is present.
- `UNFGA_BossGroundDamageZone` follows `UNFGA_BossThreatVolley`: it selects up to three players, shows a two-second floor warning, then applies damage in the matching 350 cm default radius every 0.25 seconds for two seconds. Its damage, radius, target range, and timings are editable on a Blueprint child.
- The ground-zone start and end cues are `GameplayCue.Boss.GroundDamageZone.Start` and `.End`. Create matching GameplayCue Blueprint assets to attach Niagara/audio. `OnGroundDamageZoneStarted` and `OnGroundDamageZoneEnded` are Blueprint events on the ability and fire on the server for each zone.
- When Health reaches zero, a critical-priority transition from any combat state enters `LastGasp`. `UNFGA_BossLastGasp` marks each player's floor location, waits four seconds, damages and stuns players who remain in the marked radius, then destroys the boss.

## Generate and assign the StateTree

The project includes an editor utility at `UNF_BossStateTreeAssetLibrary::CreateBossPatternCycleStateTree`. After compiling the editor target and restarting the editor, run `Scripts/GenerateBossPatternStateTree.py` once from the Unreal Editor Output Log with `py "C:/.../Scripts/GenerateBossPatternStateTree.py"`. It creates `/Game/ProjectNF/Gameplay/Boss/ST_BossPatternCycle` with seven named states and assigns it, the existing `GA_BossAreaVolley`, the other combat abilities, and `UNFGA_BossLastGasp` to `BP_TestBoss`.

The project enables the Unreal **Python Editor Script Plugin** for editor builds. The generated StateTree uses the **StateTree Component** schema. Its root `BossEncounter` contains `Dormant → FixedPointVolley → RandomTargetVolley → GroundDamageZones → RelocateAndEmerge → CenterBlast → FixedPointVolley`; `RelocateAndEmerge` takes the boss upward, warns at a selected ground point, then dives there. Critical-priority defeated transitions from `Dormant` and all five combat states lead to the terminal `LastGasp` state.

## Tuning and project assets

- Boss max health defaults to 10,000 and is editable through the `NF_BossAttributeSet` defaults.
- Runtime boss health and pattern damage are loaded on the server from `/Game/ProjectNF/Data/Boss/DT_BossCombat`, row `DefaultBoss`. The StateTree generation helper creates this DataTable with a default row if it does not exist, then assigns it to `BP_TestBoss`; edit the row in the DataTable to tune values. `Content/ProjectNF/Data/Boss/DT_BossCombat.csv` is an importable copy of those defaults. If the table or row is unavailable at runtime, the existing C++/ability defaults are used and a warning is logged.
- Pattern radius, timing, damage, projectile count/speed, and launch speeds are editable on each ability class or Blueprint child.
- The fixed-point volley starts immediately, repeats every five seconds, and ends after four volleys once the last warning has spawned its projectiles. Each volley still selects up to five players.
- `GameplayCue.Boss.Telegraph` uses the existing boss telegraph marker and decal material. `GameplayCue.Boss.TargetWarning` creates a local `!` marker above the boss at takeoff. The existing emerge cue fires when the boss completes its dive.
- The impact, stun, and airborne effects are native GameplayEffects; stun duration is one second, airborne tag duration is two seconds, and relocation launches affected players upward.
- To make threat selection work with player weapons, their damage GameplayEffect must use `UNFGE_PlayerDamageToBoss` and pass the positive `Data.Damage` SetByCaller magnitude.
