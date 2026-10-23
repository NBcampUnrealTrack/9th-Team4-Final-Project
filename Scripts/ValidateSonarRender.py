"""Transient offscreen render checks; never saves or changes a map/player/BP."""
import json
import os
import unreal


def validate(material):
    lib = unreal.MaterialEditingLibrary
    unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    # An unlit orange wall gives a deterministic non-black scene without lighting.
    wall_mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_SonarValidationWall", "/Game/__NFSonarValidation", unreal.Material, unreal.MaterialFactoryNew())
    wall_mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    color = lib.create_material_expression(wall_mat, unreal.MaterialExpressionVectorParameter)
    color.set_editor_property("parameter_name", "WallColor")
    color.set_editor_property("default_value", unreal.LinearColor(0.5, 0.12, 0.02, 1))
    lib.connect_material_property(color, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    lib.recompile_material(wall_mat)
    wall = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(500, 0, 0))
    wall.set_actor_location(unreal.Vector(500, 0, 0), False, True)
    wall.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
    wall.static_mesh_component.set_material(0, wall_mat)
    wall.set_actor_scale3d(unreal.Vector(1, 8, 8))
    proxy = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(700, 0, 0))
    proxy.set_actor_location(unreal.Vector(700, 0, 0), False, True)
    component = proxy.static_mesh_component
    component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
    component.set_material(0, wall_mat)
    proxy.set_actor_scale3d(unreal.Vector(1, 2, 2))
    component.set_editor_property("render_in_main_pass", False)
    component.set_editor_property("render_in_depth_pass", False)
    component.set_render_custom_depth(True)
    component.set_custom_depth_stencil_value(240)
    capture = actors.spawn_actor_from_class(unreal.SceneCapture2D, unreal.Vector(0, 0, 0))
    capture.set_actor_location(unreal.Vector(0, 0, 0), False, True)
    capture.set_actor_rotation(unreal.Rotator(0, 0, 0), True)
    camera = capture.get_component_by_class(unreal.SceneCaptureComponent2D)
    camera.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
    camera.set_editor_property("capture_every_frame", False)
    camera.set_editor_property("capture_on_movement", False)
    camera.set_editor_property("always_persist_rendering_state", True)
    target = unreal.RenderingLibrary.create_render_target2d(world, 256, 256, unreal.TextureRenderTargetFormat.RTF_RGBA8)
    camera.set_editor_property("texture_target", target)
    destination = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "SonarValidation")
    os.makedirs(destination, exist_ok=True)

    def snapshot(name):
        camera.capture_scene()
        pixel = unreal.RenderingLibrary.read_render_target_pixel(world, target, 128, 128)
        rgb = [pixel.r, pixel.g, pixel.b]
        unreal.RenderingLibrary.export_render_target(world, target, destination, name + ".png")
        unreal.log("SONAR_RENDER {}={}".format(name, rgb))
        return rgb

    instance = unreal.MaterialLibrary.create_dynamic_material_instance(world, material)
    for name, value in [("RevealStencil", 240), ("FillOpacity", 0.45), ("OutlineOpacity", 0.9), ("OutlineWidth", 1.5), ("DepthBias", 3)]:
        instance.set_scalar_parameter_value(name, value)
    instance.set_vector_parameter_value("RevealColor", unreal.LinearColor(0, 0.8, 1, 1))
    debug_mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_SonarValidationBlue", "/Game/__NFSonarValidation", unreal.Material, unreal.MaterialFactoryNew())
    debug_mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
    debug_mat.set_editor_property("blendable_location", unreal.BlendableLocation.BL_SCENE_COLOR_AFTER_TONEMAPPING)
    debug_color = lib.create_material_expression(debug_mat, unreal.MaterialExpressionVectorParameter)
    debug_color.set_editor_property("parameter_name", "DebugColor")
    debug_color.set_editor_property("default_value", unreal.LinearColor(0, 0, 1, 1))
    lib.connect_material_property(debug_color, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    lib.recompile_material(debug_mat)
    stencil_mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_SonarValidationStencil", "/Game/__NFSonarValidation", unreal.Material, unreal.MaterialFactoryNew())
    stencil_mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
    stencil_mat.set_editor_property("blendable_location", unreal.BlendableLocation.BL_SCENE_COLOR_AFTER_TONEMAPPING)
    stencil_node = lib.create_material_expression(stencil_mat, unreal.MaterialExpressionSceneTexture)
    stencil_node.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_CUSTOM_STENCIL)
    stencil_r = lib.create_material_expression(stencil_mat, unreal.MaterialExpressionComponentMask)
    stencil_r.set_editor_property("r", True)
    for channel in ["g", "b", "a"]:
        stencil_r.set_editor_property(channel, False)
    divide = lib.create_material_expression(stencil_mat, unreal.MaterialExpressionDivide)
    divide.set_editor_property("const_b", 255.0)
    lib.connect_material_expressions(stencil_node, "Color", stencil_r, "")
    lib.connect_material_expressions(stencil_r, "", divide, "A")
    lib.connect_material_property(divide, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    lib.recompile_material(stencil_mat)
    probes = {}
    for label, index, output_name in [("scene", 0, ""), ("mask", 2, ""), ("color", 1, "RGB")]:
        probe = unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset(
            "M_SonarProbe_" + label, "/Game/__NFSonarValidation", material)
        root = lib.get_material_property_input_node(probe, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        source = lib.get_inputs_for_material_expression(probe, root)[index]
        lib.connect_material_property(source, output_name, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        front = lib.get_material_property_input_node(probe, unreal.MaterialProperty.MP_FRONT_MATERIAL)
        if front:
            lib.connect_material_expressions(source, output_name, front, lib.get_material_expression_input_names(front)[0])
        lib.recompile_material(probe)
        probes[label] = probe
    unreal.AutomationLibrary.finish_loading_before_screenshot()
    settings = unreal.PostProcessSettings()
    settings.set_editor_property("weighted_blendables", unreal.WeightedBlendables(array=[unreal.WeightedBlendable(weight=1, object=instance)]))
    # Component render states need real editor frames. A commandlet's immediate
    # captures can return black even with no post-process material at all.
    state = {"frame": 0, "result": {}}
    def tick(delta):
        state["frame"] += 1
        frame = state["frame"]
        try:
            if frame in (30, 60, 90, 120, 150, 180, 210, 240):
                camera.capture_scene()
            if frame == 45:
                state["result"]["baseline"] = snapshot("baseline")
                camera.set_editor_property("post_process_settings", settings)
                camera.set_editor_property("post_process_blend_weight", 1.0)
            if frame == 75:
                state["result"]["silhouette"] = snapshot("silhouette")
                instance.set_scalar_parameter_value("FillOpacity", 0)
                instance.set_scalar_parameter_value("OutlineOpacity", 0)
            if frame == 105:
                state["result"]["disabled"] = snapshot("disabled")
                camera.set_editor_property("post_process_settings", unreal.PostProcessSettings())
                camera.add_or_update_blendable(debug_mat, 1.0)
            if frame == 135:
                state["result"]["debug_blue"] = snapshot("debug_blue")
                camera.set_editor_property("post_process_settings", unreal.PostProcessSettings())
                camera.add_or_update_blendable(stencil_mat, 1.0)
            if frame == 165:
                state["result"]["debug_stencil"] = snapshot("debug_stencil")
                camera.set_editor_property("post_process_settings", unreal.PostProcessSettings())
                camera.add_or_update_blendable(probes["scene"], 1.0)
            if frame == 195:
                state["result"]["probe_scene"] = snapshot("probe_scene")
                camera.set_editor_property("post_process_settings", unreal.PostProcessSettings())
                camera.add_or_update_blendable(probes["mask"], 1.0)
            if frame == 225:
                state["result"]["probe_mask"] = snapshot("probe_mask")
                camera.set_editor_property("post_process_settings", unreal.PostProcessSettings())
                camera.add_or_update_blendable(probes["color"], 1.0)
            if frame == 255:
                result = state["result"]
                result["probe_color"] = snapshot("probe_color")
                with open(os.path.join(destination, "result.json"), "w", encoding="utf-8") as stream:
                    json.dump(result, stream)
                baseline, silhouette, disabled = result["baseline"], result["silhouette"], result["disabled"]
                if max(baseline) < 20:
                    raise RuntimeError("Validation scene failed to render its unlit wall")
                if max(abs(a-b) for a, b in zip(baseline, disabled)) > 15:
                    raise RuntimeError("Sonar does not preserve the scene when disabled")
                if silhouette[1] <= baseline[1] + 10 or silhouette[2] <= baseline[2] + 10:
                    raise RuntimeError("Occluded stencil target was not revealed")
                unreal.log("SONAR_RENDER_VERIFIED: scene preserved, occluded target revealed")
                unreal.unregister_slate_post_tick_callback(state["handle"])
                unreal.EditorPythonScripting.set_keep_python_script_alive(False)
        except Exception as error:
            unreal.log_error("SONAR_RENDER_FAILED: " + str(error))
            unreal.unregister_slate_post_tick_callback(state["handle"])
            unreal.EditorPythonScripting.set_keep_python_script_alive(False)
    state["handle"] = unreal.register_slate_post_tick_callback(tick)
    unreal.EditorPythonScripting.set_keep_python_script_alive(True)
    # Keep the closure and its transient scene objects alive until the captures finish.
    globals()["_sonar_render_state"] = (state, tick, wall_mat, material, instance, wall, proxy, capture, target, debug_mat, stencil_mat, probes)
