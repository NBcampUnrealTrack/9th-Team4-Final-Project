import unreal

# Preserve existing tuning and extra granted abilities when the native helper
# updates BP_TestBoss to match the generated tree's exact ability classes.
boss_path = "/Game/ProjectNF/Persnal/Myungwoo/BP_TestBoss"
boss_class = unreal.EditorAssetLibrary.load_blueprint_class(boss_path)
defaults = unreal.get_default_object(boss_class)
preserved = {name: defaults.get_editor_property(name) for name in
             ["boss_combat_data_table", "initial_pattern_ability", "last_gasp_ability"]}
previous_abilities = list(defaults.get_editor_property("combat_pattern_abilities"))

tree = unreal.NF_BossStateTreeAssetLibrary.create_boss_pattern_cycle_state_tree()
if not tree:
    raise RuntimeError("Could not create or assign the boss pattern StateTree")

for name, value in preserved.items():
    defaults.set_editor_property(name, value)
abilities = list(defaults.get_editor_property("combat_pattern_abilities"))
for ability in previous_abilities:
    if ability and ability not in abilities:
        abilities.append(ability)
defaults.set_editor_property("combat_pattern_abilities", abilities)
if not unreal.EditorAssetLibrary.save_asset(boss_path, only_if_is_dirty=False):
    raise RuntimeError("Could not save preserved boss defaults")

editor_data = tree.get_editor_property("editor_data")
roots = editor_data.get_editor_property("sub_trees")
states = list(roots[0].get_editor_property("children"))
names = [str(state.get_editor_property("name")) for state in states]
expected = ["Dormant", "FixedPointVolley", "RandomTargetVolley", "GroundDamageZones",
            "LineStrike", "HomingVolley", "CenterBlast", "LastGasp"]
if names != expected:
    raise RuntimeError("Unexpected StateTree sequence: " + str(names))
for state in states:
    unreal.log("BOSS_TREE_STATE: " + str(state.get_editor_property("name")))
    for task in state.get_editor_property("tasks"):
        unreal.log("BOSS_TREE_TASK: " + task.export_text())
unreal.log("BOSS_TREE_UPDATED: " + tree.get_path_name())
