"""Run with UnrealEditor -RenderOffscreen -ExecutePythonScript=<this file>."""
import importlib.util
import os
import unreal

spec = importlib.util.spec_from_file_location("nf_sonar_frame_validation", os.path.join(os.path.dirname(__file__), "ValidateSonarRender.py"))
renderer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(renderer)
renderer.validate(unreal.load_asset("/Game/ProjectNF/Dev/Resource/Material/Skill/M_Sonar"))
