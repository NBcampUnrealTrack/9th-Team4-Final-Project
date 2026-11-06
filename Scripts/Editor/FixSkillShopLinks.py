"""스킬 리팩토링(74201fde) 이후 빌드룰 ItemSources에서 끊어진 스킬 DA 경로만 다시 연결한다.
Tools > Execute Python Script로 실행. 여러 번 실행해도 결과가 같다.

- 값이 비어 있는(None) 스킬 키만 채운다. 이미 연결된 항목은 덮어쓰지 않는다.
- GA 부모 클래스, 스킬 DA 내용/클래스는 수정하지 않는다.
- 연결 후 해석에 실패한 카탈로그 항목을 Output Log에 출력한다. 이 항목들은 상점에서 제외된다.
"""
import unreal

SKILL = "/Game/ProjectNF/Dev/GamePlay/Combat/Skill/"
RULES_PATH = "/Game/ProjectNF/Dev/GamePlay/BuildShop/DA_Rules/DA_NF_BuildRules_Prototype"
EAL = unreal.EditorAssetLibrary

ITEM_SOURCES = {
    "Acceleration.T1": "Active/Movement/Acceleration/DA_Acceleration",
    "AirStrike.T1": "Active/Attack/AirStrike/DA_AirStrike",
    "Barrier.T1": "Active/Utility/Barrier/DA_Barrier",
    "Clairvoyance.T1": "Active/Search/Clairvoyance/DA_Clairvoyance",
    "EMP.T1": "Active/Utility/EMP/DA_EMP",
    "Grab.T1": "Active/Utility/Grab/DA_Grab",
    "Grenade.T1": "Active/Attack/Grenade/DA_Grenade",
    "Jump.T1": "Active/Utility/Jump/DA_Jump",
    "Laser.T1": "Active/Attack/Laser/DA_Laser",
    "Portal.T1": "Active/Movement/Portal/DA_Portal",
    "RollingBomb.T1": "Active/Attack/RolliingBomb/DA_RollingBomb",
    "Rush.T1": "Active/Movement/Rush/DA_Rush",
    "Sonar.T1": "Active/Search/Sonar/DA_Sonar",
    "AttackUP.T1": "Passive/Attack/AttackDamageUp/DA_AttackDamageUp",
    "Berserker.T1": "Passive/Attack/Berserker/DA_Berserker",
    "Magic_Bullet.T1": "Passive/Attack/MagicBullet/DA_MagicBullet",
    "DefenseUp.T1": "Passive/Utility/DefenseUp/DA_DefenseUp",
    "Double_Jump.T1": "Passive/Utility/DoubleJump/DA_DoubleJump",
    "HealthUp.T1": "Passive/Utility/HealthUp/DA_HealthUp",
    "Indomitable.T1": "Passive/Utility/Indomitable/DA_Indomitable",
    "Thorn_Armor.T1": "Passive/Utility/Thorns/DA_Thorns",
}


def load(path):
    asset = unreal.load_asset(path)
    if not asset:
        raise RuntimeError("에셋을 찾을 수 없습니다: " + path)
    return asset


def relink_item_sources(rules):
    sources = rules.get_editor_property("item_sources")
    linked = 0
    for item_id, rel in ITEM_SOURCES.items():
        key = unreal.Name(item_id)
        if sources.get(key):
            unreal.log("[skip] 이미 연결됨: " + item_id)
            continue
        sources[key] = load(SKILL + rel)
        linked += 1
        unreal.log("[fix] " + item_id + " -> " + rel)
    if linked:
        rules.set_editor_property("item_sources", sources)
        EAL.save_loaded_asset(rules, False)
    unreal.log("[done] ItemSources 스킬 %d개 연결" % linked)


def report(rules):
    sources = rules.get_editor_property("item_sources")
    bad = []
    for definition in rules.get_editor_property("catalog"):
        item_id = definition.get_editor_property("item_id")
        source = sources.get(item_id)
        # 해석 실패 시 Cost가 -1로 남는다. 이 항목은 검증기에서 제외되어 상점에 표시되지 않는다.
        if not source or source.get_build_item_data().get_editor_property("cost") < 0:
            bad.append(str(item_id))
    if bad:
        unreal.log_warning("상점에서 제외되는 카탈로그 항목: " + ", ".join(bad))
    else:
        unreal.log("카탈로그 전체 해석 성공")


def main():
    rules = load(RULES_PATH)
    with unreal.ScopedEditorTransaction("Relink skill shop item sources"):
        relink_item_sources(rules)
    report(rules)


main()
