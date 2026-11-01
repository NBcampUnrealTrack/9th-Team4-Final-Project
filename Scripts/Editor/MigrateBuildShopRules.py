"""Run in Unreal Editor after rebuilding native code and restarting the editor.
Tools > Execute Python Script. Does not compile C++ or Blueprints.
"""
import unreal

RULES_PATH = "/Game/ProjectNF/Dev/GamePlay/BuildShop/DA_Rules/DA_NF_BuildRules_Prototype"
SHOP_PATHS = (
    "/Game/ProjectNF/Dev/GamePlay/BuildShop/BP_NF_BuildShopZone",
    "/Game/ProjectNF/Persnal/JunHyun/BP_NF_BuildShopZone",
)


def migrate():
    rules = unreal.load_asset(RULES_PATH)
    if not rules:
        raise RuntimeError("Rules asset was not found: " + RULES_PATH)
    targets = []
    # Validate every target before saving anything. Old native code fails here.
    for path in SHOP_PATHS:
        blueprint = unreal.load_asset(path)
        if not blueprint:
            raise RuntimeError("Shop Blueprint was not found: " + path)
        defaults = unreal.get_default_object(blueprint.generated_class())
        try:
            existing = defaults.get_editor_property("rules")
        except Exception as exc:
            raise RuntimeError("Rebuild C++ and restart Unreal Editor before running this script.") from exc
        if existing and existing != rules:
            raise RuntimeError("Shop already uses a different Rules asset: " + path)
        targets.append((path, defaults))
    with unreal.ScopedEditorTransaction("Move build rules configuration to shops"):
        for path, defaults in targets:
            defaults.modify()
            defaults.set_editor_property("rules", rules)
            if not unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False):
                raise RuntimeError("Could not save shop Blueprint: " + path)
            unreal.log("Build shop Rules assigned: " + path)


migrate()
