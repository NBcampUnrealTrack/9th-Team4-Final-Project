"""Read-only inspection of the requested sonar material."""
import unreal

path = "/Game/ProjectNF/Dev/Resource/Material/Skill/M_Sonar"
material = unreal.load_asset(path)
if not isinstance(material, unreal.Material):
    raise RuntimeError("Expected Material at " + path)
lib = unreal.MaterialEditingLibrary
for field in ["use_material_attributes", "blend_mode", "is_blendable", "blendable_output_alpha", "user_scene_texture", "disable_pre_exposure_scale", "enable_stencil_test"]:
    try:
        unreal.log("SONAR_PROPERTY {}={}".format(field, material.get_editor_property(field)))
    except Exception as error:
        unreal.log("SONAR_PROPERTY {} unreadable: {}".format(field, error))
front = lib.get_material_property_input_node(material, unreal.MaterialProperty.MP_FRONT_MATERIAL)
unreal.log("SONAR_FRONT_MATERIAL " + (front.get_class().get_name() if front else "None"))
output = lib.get_material_property_input_node(material, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
unreal.log("SONAR_INSPECT path={} domain={} location={} nodes={} output={}".format(
    material.get_path_name(), material.get_editor_property("material_domain"),
    material.get_editor_property("blendable_location"), lib.get_num_material_expressions(material),
    output.get_class().get_name() if output else "None"))
unreal.log("SONAR_INSPECT scalars={} vectors={}".format(
    lib.get_scalar_parameter_names(material), lib.get_vector_parameter_names(material)))
if output:
    for name, expression in zip(lib.get_material_expression_input_names(output), lib.get_inputs_for_material_expression(material, output)):
        detail = expression.get_class().get_name() if expression else "None"
        if isinstance(expression, unreal.MaterialExpressionSceneTexture):
            detail += " " + str(expression.get_editor_property("scene_texture_id"))
        if isinstance(expression, (unreal.MaterialExpressionScalarParameter, unreal.MaterialExpressionVectorParameter)):
            detail += " " + str(expression.get_editor_property("parameter_name"))
        unreal.log("SONAR_INPUT {} -> {}".format(name, detail))
    if isinstance(output, unreal.MaterialExpressionCustom):
        unreal.log("SONAR_HLSL " + output.get_editor_property("code"))
