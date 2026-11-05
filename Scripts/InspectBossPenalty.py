"""Read-only inspection of saved GameMode penalty references and effect defaults."""
import unreal

for path in unreal.EditorAssetLibrary.list_assets('/Game/ProjectNF', recursive=True):
    if 'gamemode' not in path.lower():
        continue
    cls = unreal.EditorAssetLibrary.load_blueprint_class(path)
    if not cls:
        continue
    obj = unreal.get_default_object(cls)
    try:
        component = obj.get_editor_property('boss_contribution_reward_component')
        effect = component.get_editor_property('lowest_team_penalty_effect_class')
        unreal.log('PENALTY_REF: ' + path + ' -> ' + (effect.get_path_name() if effect else 'None'))
    except Exception as error:
        unreal.log('PENALTY_INSPECT_SKIP: ' + path + ': ' + str(error))

for name in ['NFGE_LowestContributionPenalty', 'NFGE_ShieldClamp']:
    cls = unreal.EditorAssetLibrary.load_blueprint_class('/Game/ProjectNF/Dev/GAS/Effects/GE/' + name)
    if not cls:
        continue
    obj = unreal.get_default_object(cls)
    for prop in ['duration_policy', 'period', 'modifiers', 'executions', 'ge_components']:
        try:
            value = obj.get_editor_property(prop)
            unreal.log('PENALTY_GE: ' + name + ' ' + prop + '=' + str(value))
            if prop in ['modifiers', 'executions']:
                for item in value:
                    unreal.log('PENALTY_DETAIL: ' + name + ' ' + item.export_text())
            if prop == 'ge_components':
                for item in value:
                    unreal.log('PENALTY_COMPONENT: ' + item.get_path_name())
                    for field in ['on_application_gameplay_effects', 'on_complete_always', 'on_complete_normal', 'on_complete_prematurely']:
                        try:
                            unreal.log('PENALTY_COMPONENT_FIELD: ' + field + '=' + str(item.get_editor_property(field)))
                        except Exception:
                            pass
        except Exception as error:
            unreal.log('PENALTY_PROPERTY: ' + prop + ': ' + str(error))
