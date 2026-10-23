"""Rebuild only the requested Dev M_Sonar asset; back up its saved package first."""
import datetime
import importlib.util
import os
import shutil
import unreal

root = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
relative = "ProjectNF/Dev/Resource/Material/Skill/M_Sonar.uasset"
source = os.path.join(root, "Content", relative)
backup_dir = os.path.join(root, "Saved", "AssetBackups", "Sonar", datetime.datetime.now().strftime("%Y%m%d_%H%M%S"))
os.makedirs(backup_dir, exist_ok=True)
if not os.path.isfile(source):
    raise RuntimeError("Requested saved M_Sonar asset is missing")
shutil.copy2(source, os.path.join(backup_dir, "M_Sonar.uasset"))
unreal.log("SONAR_BACKUP: " + backup_dir)

spec = importlib.util.spec_from_file_location("nf_sonar_material_builder", os.path.join(root, "Scripts", "CreateSonarMaterial.py"))
builder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)
builder.create_material("/Game/ProjectNF/Dev/Resource/Material/Skill/M_Sonar", rebuild=True)
