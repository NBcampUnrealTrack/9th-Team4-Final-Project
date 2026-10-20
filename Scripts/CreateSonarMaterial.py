"""Create the sonar post-process asset; never edits character materials or blueprints.

Run with UnrealEditor-Cmd -run=pythonscript -script=<absolute path> -unattended -nullrhi.
This only generates content. It does not build C++.
"""
import unreal

ASSET = "/Game/ProjectNF/Skills/Sonar/M_PP_Sonar"
LIB = unreal.MaterialEditingLibrary


def verify_material(material):
    output = LIB.get_material_property_input_node(material, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if not isinstance(output, unreal.MaterialExpressionCustom):
        raise RuntimeError("Sonar output is not connected to the silhouette shader")
    connected = LIB.get_inputs_for_material_expression(material, output)
    if len(connected) != 10 or any(value is None for value in connected):
        raise RuntimeError("Sonar shader has missing material inputs")
    if material.get_editor_property("blendable_location") != unreal.BlendableLocation.BL_SCENE_COLOR_AFTER_TONEMAPPING:
        raise RuntimeError("Sonar uses the wrong post-process location")
    unreal.log("SONAR_MATERIAL_VERIFIED: 10 connected inputs, post-process domain, after tonemapping")


def create_material():
    if unreal.EditorAssetLibrary.does_asset_exist(ASSET):
        existing = unreal.load_asset(ASSET)
        if existing.get_editor_property("material_domain") != unreal.MaterialDomain.MD_POST_PROCESS:
            raise RuntimeError("Existing sonar asset is not a post-process material; refusing to overwrite")
        verify_material(existing)
        unreal.log("SONAR_MATERIAL_EXISTS: " + existing.get_path_name())
        return

    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_PP_Sonar", "/Game/ProjectNF/Skills/Sonar", unreal.Material, unreal.MaterialFactoryNew()
    )
    material.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
    material.set_editor_property("blendable_location", unreal.BlendableLocation.BL_SCENE_COLOR_AFTER_TONEMAPPING)
    material.set_editor_property("blendable_priority", 100)

    def node(cls, x, y):
        return LIB.create_material_expression(material, cls, x, y)

    custom = node(unreal.MaterialExpressionCustom, 0, 0)
    custom.set_editor_property("description", "Occluded sonar silhouette + outline, isolated by local-player stencil")
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
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

    for index, (name, texture_id) in enumerate([
        ("SceneColor", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0),
        ("SceneDepth", unreal.SceneTextureId.PPI_SCENE_DEPTH),
        ("CustomDepth", unreal.SceneTextureId.PPI_CUSTOM_DEPTH),
        ("Stencil", unreal.SceneTextureId.PPI_CUSTOM_STENCIL),
    ]):
        expression = node(unreal.MaterialExpressionSceneTexture, -700, index * 160)
        expression.set_editor_property("scene_texture_id", texture_id)
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
return lerp(SceneColor.rgb, RevealColor.rgb, saturate(alpha));
""")
    if not LIB.connect_material_property(custom, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR):
        raise RuntimeError("Could not connect sonar material output")
    LIB.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material):
        raise RuntimeError("Could not save sonar material")
    verify_material(material)
    unreal.log("SONAR_MATERIAL_CREATED: " + material.get_path_name())


create_material()
