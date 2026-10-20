"""Generate/verify the portal screen-projection surface material; no C++ build or BP edits."""
import unreal

PATH = "/Game/ProjectNF/Skills/Portal/M_PortalView"
LIB = unreal.MaterialEditingLibrary


def verify(material):
    if material.get_editor_property("material_domain") != unreal.MaterialDomain.MD_SURFACE:
        raise RuntimeError("Portal view material has the wrong domain")
    texture = LIB.get_material_property_input_node(material, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if not isinstance(texture, unreal.MaterialExpressionTextureSampleParameter2D):
        raise RuntimeError("Portal emissive output is not connected to the render-target parameter")
    inputs = LIB.get_inputs_for_material_expression(material, texture)
    if not any(isinstance(value, unreal.MaterialExpressionScreenPosition) for value in inputs):
        raise RuntimeError("Portal image must use viewport projection instead of mesh UVs")
    unreal.log("PORTAL_VIEW_MATERIAL_VERIFIED: screen UV -> PortalTexture -> emissive")


if unreal.EditorAssetLibrary.does_asset_exist(PATH):
    verify(unreal.load_asset(PATH))
else:
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_PortalView", "/Game/ProjectNF/Skills/Portal", unreal.Material, unreal.MaterialFactoryNew())
    material.set_editor_property("material_domain", unreal.MaterialDomain.MD_SURFACE)
    material.set_editor_property("two_sided", True)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    screen = LIB.create_material_expression(material, unreal.MaterialExpressionScreenPosition, -500, 0)
    texture = LIB.create_material_expression(material, unreal.MaterialExpressionTextureSampleParameter2D, -200, 0)
    texture.set_editor_property("parameter_name", "PortalTexture")
    fallback = unreal.load_asset("/Engine/EngineResources/DefaultTexture")
    if not fallback:
        raise RuntimeError("Default texture is missing")
    texture.set_editor_property("texture", fallback)
    if not LIB.connect_material_expressions(screen, "ViewportUV", texture, "UVs"):
        raise RuntimeError("Could not connect portal viewport UVs")
    if not LIB.connect_material_property(texture, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR):
        raise RuntimeError("Could not connect portal emissive output")
    LIB.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material):
        raise RuntimeError("Could not save portal view material")
    verify(material)
    unreal.log("PORTAL_VIEW_MATERIAL_CREATED: " + material.get_path_name())
