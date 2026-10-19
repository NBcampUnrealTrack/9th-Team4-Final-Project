import unreal

tree = unreal.NF_BossStateTreeAssetLibrary.create_boss_pattern_cycle_state_tree()
if not tree:
    raise RuntimeError("Could not create or assign the boss pattern StateTree")

unreal.log(f"Created or found boss StateTree: {tree.get_path_name()}")
unreal.log("The editor helper assigned the StateTree and pattern abilities, and created/assigned DT_BossCombat")
