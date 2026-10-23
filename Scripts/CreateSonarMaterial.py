"""Create the sonar post-process asset; never edits character materials or blueprints.

Run with UnrealEditor-Cmd -run=pythonscript -script=<absolute path> -unattended -nullrhi.
This only generates content. It does not build C++.
"""
import unreal

ASSET = "/Game/ProjectNF/Skills/Sonar/M_PP_Sonar"
LIB = unreal.MaterialEditingLibrary


def verify_material(material):
    output = LIB.get_material_property_input_node(material, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if not isinstance(output, unreal.MaterialExpressionLinearInterpolate):
        raise RuntimeError("Sonar output must preserve the original scene through a Lerp")
    front = LIB.get_material_property_input_node(material, unreal.MaterialProperty.MP_FRONT_MATERIAL)
    if material.get_editor_property("material_domain") != unreal.MaterialDomain.MD_POST_PROCESS:
        raise RuntimeError("Sonar domain changed away from Post Process")
    if not isinstance(front, unreal.MaterialExpressionSubstratePostProcess):
        raise RuntimeError("Sonar is missing its Substrate Front Material output")
    if output not in LIB.get_inputs_for_material_expression(material, front):
        raise RuntimeError("Sonar screen composite is not connected to Substrate Post Process color")
    final_inputs = LIB.get_inputs_for_material_expression(material, output)
    if len(final_inputs) != 3 or any(item is None for item in final_inputs):
        raise RuntimeError("Sonar Lerp is missing the original screen")
    background = final_inputs[0]
    if isinstance(background, unreal.MaterialExpressionComponentMask):
        background = LIB.get_inputs_for_material_expression(material, background)[0]
    if not isinstance(background, unreal.MaterialExpressionSceneTexture) or background.get_editor_property("scene_texture_id") != unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0:
        raise RuntimeError("Sonar background is not PostProcessInput0")
    mask = final_inputs[2]
    if not isinstance(mask, unreal.MaterialExpressionCustom):
        raise RuntimeError("Sonar alpha is not connected to the silhouette mask")
    connected = LIB.get_inputs_for_material_expression(material, mask)
    if len(connected) != 10 or any(value is None for value in connected):
        raise RuntimeError("Sonar shader has missing material inputs")
    if material.get_editor_property("blendable_location") != unreal.BlendableLocation.BL_SCENE_COLOR_AFTER_TONEMAPPING:
        raise RuntimeError("Sonar uses the wrong post-process location")
    unreal.log("SONAR_MATERIAL_VERIFIED: PostProcessInput0 -> Lerp -> Substrate Post Process -> Front Material")


def create_material(asset_path=ASSET, rebuild=False):
    material = None
    asset = asset_path
    if unreal.EditorAssetLibrary.does_asset_exist(asset):
        existing = unreal.load_asset(asset)
        if not rebuild and existing.get_editor_property("material_domain") != unreal.MaterialDomain.MD_POST_PROCESS:
            raise RuntimeError("Existing sonar asset is not a post-process material; refusing to overwrite")
        if not rebuild:
            verify_material(existing)
            unreal.log("SONAR_MATERIAL_EXISTS: " + existing.get_path_name())
            return
        material = existing
        LIB.delete_all_material_expressions(material)

    if material is None:
        folder, name = asset.rsplit("/", 1)
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, folder, unreal.Material, unreal.MaterialFactoryNew()
        )
    material.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
    material.set_editor_property("blendable_location", unreal.BlendableLocation.BL_SCENE_COLOR_AFTER_TONEMAPPING)
    material.set_editor_property("blendable_priority", 100)

    def node(cls, x, y):
        return LIB.create_material_expression(material, cls, x, y)

    custom = node(unreal.MaterialExpressionCustom, 0, 0)
    custom.set_editor_property("description", "Sonar occlusion + outline mask (screen color bypasses Custom node)")
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    inputs = []

    def connect(expression, output, name):
        value = unreal.CustomInput()
        value.set_editor_property("input_name", name)
        inputs.append(value)
        custom.set_editor_property("inputs", inputs)
        if not LIB.connect_material_expressions(expression, output, custom, name):
            raise RuntimeError("Could not connect sonar input " + name)
        # Re-read the connected structs before adding another input, preserving ExpressionInputs.
        inputs[:] = list(custom.get_editor_property("inputs"))

    scene_color = None
    for index, (name, texture_id) in enumerate([
        ("SceneColor", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0),
        ("SceneDepth", unreal.SceneTextureId.PPI_SCENE_DEPTH),
        ("CustomDepth", unreal.SceneTextureId.PPI_CUSTOM_DEPTH),
        ("Stencil", unreal.SceneTextureId.PPI_CUSTOM_STENCIL),
    ]):
        expression = node(unreal.MaterialExpressionSceneTexture, -700, index * 160)
        expression.set_editor_property("scene_texture_id", texture_id)
        if name == "SceneColor":
            scene_color = expression
        connect(expression, "Color", name)

    color = node(unreal.MaterialExpressionVectorParameter, -700, 650)
    color.set_editor_property("parameter_name", "RevealColor")
    color.set_editor_property("default_value", unreal.LinearColor(0.0, 0.8, 1.0, 1.0))
    connect(color, "", "RevealColor")
    for index, (name, default) in enumerate([
        ("RevealStencil", 240.0), ("FillOpacity", 0.45), ("OutlineOpacity", 0.9),
        ("OutlineWidth", 1.5), ("DepthBias", 3.0),
    ]):
        parameter = node(unreal.MaterialExpressionScalarParameter, -350, 200 + index * 100)
        parameter.set_editor_property("parameter_name", name)
        parameter.set_editor_property("default_value", default)
        connect(parameter, "", name)

    custom.set_editor_property("code", r"""
float center = (abs(Stencil.r - RevealStencil) < 0.5 && CustomDepth.r > SceneDepth.r + DepthBias) ? 1.0 : 0.0;
float edge = 0.0;
float2 uv = GetDefaultSceneTextureUV(Parameters, PPI_CustomDepth);
float2 sceneUV = GetDefaultSceneTextureUV(Parameters, PPI_SceneDepth);
float2 stepUV = GetSceneTextureViewSize(PPI_CustomDepth).zw * max(0.0, OutlineWidth);
float2 sceneStepUV = GetSceneTextureViewSize(PPI_SceneDepth).zw * max(0.0, OutlineWidth);
const float2 directions[4] = {float2(1,0), float2(-1,0), float2(0,1), float2(0,-1)};
[unroll] for (int i = 0; i < 4; ++i)
{
    float2 sampleUV = ClampSceneTextureUV(uv + directions[i] * stepUV, PPI_CustomDepth);
    float2 sampleSceneUV = ClampSceneTextureUV(sceneUV + directions[i] * sceneStepUV, PPI_SceneDepth);
    float stencil = SceneTextureLookup(sampleUV, PPI_CustomStencil, false).r;
    float depth = SceneTextureLookup(sampleUV, PPI_CustomDepth, false).r;
    float sceneDepth = SceneTextureLookup(sampleSceneUV, PPI_SceneDepth, false).r;
    float mask = (abs(stencil - RevealStencil) < 0.5 && depth > sceneDepth + DepthBias) ? 1.0 : 0.0;
    edge = max(edge, mask * (1.0 - center));
}
float alpha = max(center * saturate(FillOpacity), edge * saturate(OutlineOpacity));
return saturate(alpha);
""")
    final = node(unreal.MaterialExpressionLinearInterpolate, 400, 0)
    # Explicitly select RGB, so alpha never becomes part of the screen color.
    rgb = node(unreal.MaterialExpressionComponentMask, 150, -200)
    for channel in ["r", "g", "b"]:
        rgb.set_editor_property(channel, True)
    rgb.set_editor_property("a", False)
    for source, output_name, target, input_name in [
        (scene_color, "Color", rgb, ""), (rgb, "", final, "A"),
        (color, "RGB", final, "B"), (custom, "", final, "Alpha")]:
        if not LIB.connect_material_expressions(source, output_name, target, input_name):
            raise RuntimeError("Could not connect sonar composite " + input_name)
    if not LIB.connect_material_property(final, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR):
        raise RuntimeError("Could not connect sonar material output")
    # This project enables Substrate. An existing converted material with no Front
    # Material evaluates black even when the legacy Emissive input is connected.
    front = node(unreal.MaterialExpressionSubstratePostProcess, 650, 0)
    input_names = LIB.get_material_expression_input_names(front)
    if not input_names or not LIB.connect_material_expressions(final, "", front, input_names[0]):
        raise RuntimeError("Could not connect sonar to Substrate Post Process color")
    if not LIB.connect_material_property(front, "", unreal.MaterialProperty.MP_FRONT_MATERIAL):
        raise RuntimeError("Could not connect sonar Substrate Front Material")
    LIB.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material):
        raise RuntimeError("Could not save sonar material")
    verify_material(material)
    unreal.log("SONAR_MATERIAL_CREATED: " + material.get_path_name())


if __name__ == "__main__":
    create_material()
