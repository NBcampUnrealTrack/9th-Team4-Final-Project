"""Read-only monster capsule / level collision inspection."""
import unreal

def props(obj, fields):
    result = {}
    for field in fields:
        try:
            result[field] = str(obj.get_editor_property(field))
        except Exception:
            pass
    return result

for name in ['BP_NF_Monster', 'BP_NF_Monster_Brute', 'BP_NF_Monster_Range']:
    cls = unreal.load_class(None, '/Game/ProjectNF/Dev/Monster/Blueprint/Monster/' + name + '.' + name + '_C')
    actor = unreal.get_default_object(cls)
    unreal.log('SPAWN_INSPECT ' + name + ' ' + str(props(actor, ['replicates', 'actor_enable_collision', 'hidden'])))
    for comp in actor.get_components_by_class(unreal.PrimitiveComponent):
        unreal.log('SPAWN_INSPECT component=' + comp.get_name() + ' collision=' + str(comp.get_collision_enabled()) + ' profile=' + str(comp.get_collision_profile_name()) + ' ' + str(props(comp, ['relative_location', 'relative_scale3d', 'capsule_half_height', 'capsule_radius', 'body_instance'])))

world = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/ProjectNF/Persnal/GyeongHwan/TestLevel_GH')
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    label = actor.get_actor_label()
    if actor.get_components_by_class(unreal.PrimitiveComponent):
        unreal.log('SPAWN_INSPECT level_actor=' + label + ' location=' + str(actor.get_actor_location()))
        for comp in actor.get_components_by_class(unreal.PrimitiveComponent):
            unreal.log('SPAWN_INSPECT level_component=' + comp.get_name() + ' collision=' + str(comp.get_collision_enabled()) + ' profile=' + str(comp.get_collision_profile_name()) + ' mesh=' + str(props(comp, ['static_mesh', 'relative_scale3d'])))
