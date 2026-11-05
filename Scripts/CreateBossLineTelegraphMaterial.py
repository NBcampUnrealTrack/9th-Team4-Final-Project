"""Create the full-rectangle Substrate decal used by BossLineStrike. Existing assets are preserved."""
import unreal

path = "/Game/ProjectNF/Dev/GamePlay/Boss/M_BossLineTelegraph"
lib = unreal.MaterialEditingLibrary
if unreal.EditorAssetLibrary.does_asset_exist(path):
    unreal.log("BOSS_LINE_MATERIAL_EXISTS: " + path)
else:
    folder, name = path.rsplit("/", 1)
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, folder, unreal.Material, unreal.MaterialFactoryNew())
    material.set_editor_property("material_domain", unreal.MaterialDomain.MD_DEFERRED_DECAL)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    color = lib.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -500, 0)
    color.set_editor_property("parameter_name", "WarningColor")
    color.set_editor_property("default_value", unreal.LinearColor(3.0, 0.08, 0.01, 1.0))
    opacity = lib.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -500, 200)
    opacity.set_editor_property("parameter_name", "WarningOpacity")
    opacity.set_editor_property("default_value", 0.55)
    slab = lib.create_material_expression(material, unreal.MaterialExpressionSubstrateSlabBSDF, -200, 0)
    decal = lib.create_material_expression(material, unreal.MaterialExpressionSubstrateConvertToDecal, 100, 0)

    def connect(source, output, target, input_key):
        names = lib.get_material_expression_input_names(target)
        match = next((str(n) for n in names if input_key in str(n).replace(" ", "").lower()), None)
        if not match or not lib.connect_material_expressions(source, output, target, match):
            raise RuntimeError("Missing input " + input_key + ": " + str(names))

    connect(color, "RGB", slab, "emissive")
    connect(slab, "", decal, "decalmaterial")
    connect(opacity, "", decal, "coverage")
    if not lib.connect_material_property(decal, "", unreal.MaterialProperty.MP_FRONT_MATERIAL):
        raise RuntimeError("Cannot connect decal Front Material")
    lib.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material):
        raise RuntimeError("Cannot save boss line decal")
    unreal.log("BOSS_LINE_MATERIAL_CREATED: " + path)
