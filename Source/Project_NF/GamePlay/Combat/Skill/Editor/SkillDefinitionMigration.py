"""UE Editor Python only. Export with the OLD loaded classes BEFORE rebuilding.

export_snapshot() never edits assets. After restarting with the new classes:
apply_snapshot(path) prints the plan; apply_snapshot(path, dry_run=False) creates
new skill DAs, links the skill BPs, compiles and saves only those assets.
Existing build/UI/monster assets and existing skill DAs are not rewritten.
"""

import json
from pathlib import Path

import unreal


IDENTITY = ("skill_type", "skill_category", "skill_activation_method", "build_cost")
ACTIVE = (
    "cast_montage", "cast_montage_play_rate", "cast_montage_start_section",
    "affected_by_emp", "cancel_on_emp", "execution_actor_class", "default_preview_data",
    "input_cancel_tags", "requires_confirmed_target_data", "activate_on_input_release",
    "preview_actor_class", "skill_execution_mode", "cast_time", "execution_duration",
)
PASSIVE = (
    "activation_cue_tag", "active_cue_tag", "passive_effects", "passive_event_tags",
    "activate_passive_on_grant",
)
STRUCT_FIELDS = {
    "GameplayTag": ("tag_name",),
    "GameplayTagContainer": ("gameplay_tags",),
    "Vector": ("x", "y", "z"),
    "NF_SkillPreviewData": (
        "display_radius", "target_range", "ground_search_depth", "has_projectile_path",
        "launch_location", "launch_velocity", "projectile_radius", "gravity_z", "flight_time",
    ),
}


def _object_path(value):
    if isinstance(value, type) and issubclass(value, unreal.Object):
        return value.static_class().get_path_name()
    return value.get_path_name()


def _pack(value):
    if value is None or isinstance(value, (bool, int, float, str)):
        return value
    if isinstance(value, unreal.Object):
        return {"object": value.get_path_name()}
    if isinstance(value, type) and issubclass(value, unreal.Object):
        return {"object": _object_path(value)}
    type_name = type(value).__name__
    if type_name == "Name":
        return {"name": str(value)}
    if type_name in STRUCT_FIELDS:
        return {"struct": type_name, "fields": {
            key: _pack(value.get_editor_property(key)) for key in STRUCT_FIELDS[type_name]
        }}
    if isinstance(value, unreal.EnumBase):
        return {"enum": type_name, "member": value.name}
    if isinstance(value, (unreal.Array, list, tuple)):
        return [_pack(item) for item in value]
    raise TypeError(f"Unsupported snapshot value: {type_name}; no snapshot was written")


def _unpack(value):
    if isinstance(value, list):
        return [_unpack(item) for item in value]
    if not isinstance(value, dict):
        return value
    if "object" in value:
        loaded = unreal.load_object(None, value["object"])
        if loaded is None:
            raise RuntimeError(f"Missing snapshot reference: {value['object']}")
        return loaded
    if "name" in value:
        return unreal.Name(value["name"])
    if "enum" in value:
        return getattr(getattr(unreal, value["enum"]), value["member"])
    if "struct" in value:
        result = getattr(unreal, value["struct"])()
        for key, item in value["fields"].items():
            result.set_editor_property(key, _unpack(item))
        return result
    raise TypeError(f"Unknown snapshot record: {value}")


def export_snapshot(path=None, content_root="/Game/ProjectNF"):
    """Run before rebuilding, while old GA properties are still loaded in Editor."""
    if path is None:
        path = Path(unreal.Paths.project_saved_dir()) / "SkillMigration" / "skill_defaults.json"
    target = Path(path)
    if target.exists():
        raise FileExistsError(f"Snapshot already exists; choose another path: {target}")
    rows = []
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    assets = registry.get_assets_by_path(content_root, recursive=True)
    sources = {}
    for asset in assets:
        if str(asset.asset_class_path.asset_name) not in (
            "NF_SkillDataAsset", "NF_ActiveSkillDataAsset", "NF_PassiveSkillDataAsset"
        ):
            continue
        data = unreal.EditorAssetLibrary.load_asset(str(asset.package_name))
        ability = data.get_editor_property("ability_class")
        if ability:
            sources.setdefault(_object_path(ability), []).append({
                "icon": _pack(data.get_editor_property("icon")),
                "ability_level": int(data.get_editor_property("ability_level")),
            })
    for asset in assets:
        if str(asset.asset_class_path.asset_name) != "Blueprint":
            continue
        package = str(asset.package_name)
        generated = unreal.EditorAssetLibrary.load_blueprint_class(package)
        if generated is None:
            continue
        defaults = unreal.get_default_object(generated)
        if not isinstance(defaults, unreal.NFGA_SkillBase):
            continue
        # Deliberately refuse to infer lost custom values after the ownership change.
        try:
            identity = {key: _pack(defaults.get_editor_property(key)) for key in IDENTITY}
        except Exception as error:
            raise RuntimeError(
                "Old identity properties are unavailable. Use an Editor with the pre-split "
                "classes, or recover from the pre-migration asset backup. " + package
            ) from error
        kind = defaults.get_editor_property("skill_type").name
        settings = ACTIVE if kind == "ACTIVE" else PASSIVE
        rows.append({"blueprint": package, "class": _object_path(generated), "kind": kind,
                     "identity": identity,
                     "source_data": sources.get(_object_path(generated), []),
                     "settings": {key: _pack(defaults.get_editor_property(key)) for key in settings}})
    if not rows:
        raise RuntimeError("No skill BP defaults found; no snapshot was written")
    target.parent.mkdir(parents=True, exist_ok=True)
    # Exclusive creation prevents overwriting a known-good export.
    with target.open("x", encoding="utf-8") as stream:
        json.dump({"version": 1, "skills": rows}, stream, ensure_ascii=False, indent=2)
    unreal.log(f"[SkillMigration] Exported {len(rows)} skill BPs: {target}")
    return str(target)


def _state_rule(tag, block=True, cancel=True, force=False):
    rule = unreal.NF_SkillStateRule()
    rule.set_editor_property("state_tag", tag)
    rule.set_editor_property("block_activation", block)
    rule.set_editor_property("cancel_running", cancel)
    rule.set_editor_property("force_cancel", force)
    return rule


def apply_snapshot(path, dry_run=True):
    """Default is a read-only plan. Stop PIE and back up assets before dry_run=False."""
    snapshot = json.loads(Path(path).read_text(encoding="utf-8"))
    if snapshot.get("version") != 1:
        raise ValueError("Unsupported snapshot version")
    plan = []
    # Validate every reference before creating or editing any asset.
    for row in snapshot["skills"]:
        if row["kind"] not in ("ACTIVE", "PASSIVE"):
            raise ValueError(f"Unknown skill kind: {row['kind']}")
        bp = unreal.EditorAssetLibrary.load_asset(row["blueprint"])
        cls = unreal.EditorAssetLibrary.load_blueprint_class(row["blueprint"])
        if bp is None or cls is None:
            raise RuntimeError(f"Missing skill BP: {row['blueprint']}")
        settings = {key: _unpack(item) for key, item in row["settings"].items()}
        identity = {key: _unpack(item) for key, item in row["identity"].items()}
        active = row["kind"] == "ACTIVE"
        parent = unreal.NFGA_ActiveSkillBase if active else unreal.NFGA_PassiveSkillBase
        defaults = unreal.get_default_object(cls)
        reparent = not isinstance(defaults, parent)
        if reparent and bp.get_editor_property("parent_class") != unreal.NFGA_SkillBase.static_class():
            raise RuntimeError("Custom BP parent needs manual migration: " + row["blueprint"])
        directory, bp_name = row["blueprint"].rsplit("/", 1)
        name = "DA_Migrated_" + bp_name
        data_path = directory + "/" + name
        if unreal.EditorAssetLibrary.does_asset_exist(data_path):
            raise RuntimeError("Destination already exists; refusing to overwrite: " + data_path)
        if defaults.get_editor_property("skill_definition") is not None:
            raise RuntimeError("BP already has a definition; merge it manually: " + row["blueprint"])
        plan.append((row, bp, cls, parent, reparent, directory, name, identity, settings))
        unreal.log(f"[SkillMigration] {row['blueprint']} -> {data_path}; reparent={reparent}")
    if dry_run:
        return len(plan)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    for row, bp, cls, parent, reparent, directory, name, identity, settings in plan:
        if reparent:
            unreal.BlueprintEditorLibrary.reparent_blueprint(bp, parent.static_class())
            cls = bp.generated_class()
        defaults = unreal.get_default_object(cls)
        active = row["kind"] == "ACTIVE"
        data_class = unreal.NF_ActiveSkillDataAsset if active else unreal.NF_PassiveSkillDataAsset
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", data_class.static_class())
        data = tools.create_asset(name, directory, data_class.static_class(), factory)
        if data is None:
            raise RuntimeError("Failed to create skill DA: " + name)
        data.set_editor_property("ability_class", cls)
        source_data = row.get("source_data", [])
        if source_data:
            data.set_editor_property("icon", _unpack(source_data[0]["icon"]))
            if len(source_data) == 1:
                data.set_editor_property("ability_level", source_data[0]["ability_level"])
            else:
                unreal.log_warning("Multiple existing DAs: reconcile tier/level/icon manually for " + row["blueprint"])
        data.set_editor_property("override_identity", True)
        for key in ("skill_category", "skill_activation_method", "build_cost"):
            data.set_editor_property(key, identity[key])
        data.set_editor_property("override_active_settings" if active else "override_passive_settings", True)
        if active:
            aliases = {"default_preview_data": "preview_data", "skill_execution_mode": "execution_mode"}
            for key, item in settings.items():
                if key not in ("affected_by_emp", "cancel_on_emp", "input_cancel_tags"):
                    data.set_editor_property(aliases.get(key, key), item)
            # Death remains mandatory in the common base. Carry old input/EMP policy into DA rules.
            rules = [_state_rule(tag) for tag in settings["input_cancel_tags"].get_editor_property("gameplay_tags")]
            if settings["affected_by_emp"]:
                tag = unreal.GameplayTag()
                tag.set_editor_property("tag_name", unreal.Name("State.Debuff.EMP"))
                rules.append(_state_rule(tag, cancel=settings["cancel_on_emp"], force=settings["cancel_on_emp"]))
            data.set_editor_property("override_state_rules", True)
            data.set_editor_property("state_rules", rules)
        else:
            for key, item in settings.items():
                data.set_editor_property(key, item)
        defaults.set_editor_property("skill_definition", data)
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        if "ERROR" in str(bp.get_editor_property("status")).upper():
            raise RuntimeError("BP compile failed; not saving BP. Inspect moved member nodes: " + row["blueprint"])
        if not unreal.EditorAssetLibrary.save_loaded_asset(data):
            raise RuntimeError("Failed to save DA: " + data.get_path_name())
        if not unreal.EditorAssetLibrary.save_loaded_asset(bp):
            raise RuntimeError("Failed to save BP: " + row["blueprint"])
        unreal.log(f"[SkillMigration] Saved {row['blueprint']} with {data.get_path_name()}")
    unreal.log_warning("Existing build DA references/icons/levels are unchanged. Reconcile them with the new DAs before merging.")
    return len(plan)
