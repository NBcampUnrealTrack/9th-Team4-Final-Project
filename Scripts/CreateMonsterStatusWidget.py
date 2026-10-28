"""Create an editable monster status WBP and report the saved monster names."""
import unreal

folder = '/Game/ProjectNF/Dev/UI/BluePrint/Monster'
path = folder + '/WBP_NF_MonsterStatus'
asset = unreal.load_asset(path)
if not asset:
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property('parent_class', unreal.load_class(None, '/Script/Project_NF.NF_MonsterStatusWidget'))
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        'WBP_NF_MonsterStatus', folder, unreal.WidgetBlueprint, factory)

# Designer contents are authored in the editor. The native class connects
# TXT_MonsterName and PB_Health automatically when those controls are added.
unreal.BlueprintEditorLibrary.compile_blueprint(asset)
unreal.EditorAssetLibrary.save_loaded_asset(asset)

widget_class = unreal.load_class(None, path + '.WBP_NF_MonsterStatus_C')
for bp_name in ['BP_NF_Monster', 'BP_NF_Monster_Brute', 'BP_NF_Monster_Range']:
    bp_path = '/Game/ProjectNF/Dev/Monster/Blueprint/Monster/' + bp_name
    bp = unreal.load_asset(bp_path)
    cls = unreal.load_class(None, bp_path + '.' + bp_name + '_C')
    defaults = unreal.get_default_object(cls)
    unreal.log('MONSTER_UI_NAME ' + bp_name + ' = ' + str(defaults.get_editor_property('monster_display_name')))
unreal.log('MONSTER_UI_CREATED ' + path)
