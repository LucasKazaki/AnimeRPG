"""Validate the bounded VD-001 authoring fixture, not a runtime asset loader."""
from __future__ import annotations
import argparse
import json
from pathlib import Path

LOCALES = {"en-US", "ja-JP"}
EXPRESSIONS = {"neutral", "warm", "concerned", "skeptical", "determined", "amused", "sad", "angry", "afraid", "surprised"}


def require(value: bool, message: str) -> None:
    if not value:
        raise ValueError(message)


def keys(value: object, expected: set[str], label: str) -> None:
    require(type(value) is dict and set(value) == expected, f"{label}: missing or unknown fields")


def text(value: object, label: str) -> None:
    require(type(value) is str and bool(value.strip()) and len(value) <= 4096, f"{label}: invalid text")


def string_set(value: object, expected: set[str], label: str) -> None:
    require(type(value) is list and all(type(x) is str for x in value), f"{label}: expected string list")
    require(len(value) == len(expected) and set(value) == expected, f"{label}: values or duplicates")


def validate(data: object) -> tuple[int, int]:
    keys(data, {"schema_version", "status", "required_locales", "playable_character_id", "allow_party_switching", "gacha", "presentation", "expressions", "characters", "sample_lines"}, "root")
    require(type(data["schema_version"]) is int and data["schema_version"] == 1, "schema version")
    require(data["status"] == "authoring_fixture_not_runtime", "must remain source-only")
    string_set(data["required_locales"], LOCALES, "locales")
    require(data["playable_character_id"] == "protagonist" and data["allow_party_switching"] is False, "only protagonist is controllable")
    g = data["gacha"]
    keys(g, {"enabled", "permitted_categories", "characters_permitted", "real_money_authorized"}, "gacha")
    require(g["enabled"] is False and g["characters_permitted"] is False and g["real_money_authorized"] is False, "gacha fixture is not a monetization approval")
    string_set(g["permitted_categories"], {"armor", "tools", "weapons", "cosmetics"}, "equipment categories")
    p = data["presentation"]
    expected_p = {"default_text": "instant", "manual_advance_waits_for_voice": False,
                  "manual_advance_waits_for_animation": False, "skip_stops_at_meaningful_choice": True,
                  "history_is_read_only": True, "independent_voice_and_text_locales": True,
                  "ui_advance_event": "ui.dialogue.advance", "ui_sound_can_be_muted": True}
    keys(p, set(expected_p), "presentation")
    require(all(type(p[k]) is type(v) and p[k] == v for k, v in expected_p.items()), "presentation invariant")
    string_set(data["expressions"], EXPRESSIONS, "expressions")
    chars = data["characters"]
    require(type(chars) is list and 1 <= len(chars) <= 128, "character count")
    ids, casting = set(), set()
    playable = []
    for c in chars:
        keys(c, {"id", "playable", "role", "personality", "speech_style", "voice_identity", "locales"}, "character")
        for k in ("id", "role", "speech_style", "voice_identity"):
            text(c[k], k)
        require(c["id"] not in ids, "duplicate character id")
        ids.add(c["id"])
        require(type(c["playable"]) is bool, "playable must be boolean")
        if c["playable"]:
            playable.append(c["id"])
        traits = c["personality"]
        require(type(traits) is list and 2 <= len(traits) <= 8, "personality traits")
        for trait in traits:
            text(trait, "trait")
        require(len(set(traits)) == len(traits), "duplicate traits")
        keys(c["locales"], LOCALES, "character locales")
        for loc, voice in c["locales"].items():
            keys(voice, {"casting_key", "status", "provider_voice_id", "direction", "reference_origin", "reference_asset", "rights_review", "native_language_review"}, "voice")
            require(voice["casting_key"] == f'{c["id"]}.{loc}.v1', "casting identity mismatch")
            require(voice["casting_key"] not in casting, "shared casting slot")
            casting.add(voice["casting_key"])
            text(voice["direction"], "locale direction")
            require(voice["status"] == "unassigned" and voice["provider_voice_id"] is None, "no fabricated generated voice")
            require(voice["reference_origin"] == "designed_synthetic" and voice["reference_asset"] is None, "no unauthorized recording in fixture")
            require(voice["rights_review"] == "pending" and voice["native_language_review"] == "pending", "review is not complete")
    require(playable == ["protagonist"], "exactly one playable protagonist")
    lines = data["sample_lines"]
    require(type(lines) is list and 1 <= len(lines) <= 256, "sample line count")
    line_ids = set()
    for line in lines:
        keys(line, {"id", "speaker", "texts", "expression", "intensity_permille", "delivery", "audio_status", "translation_status"}, "line")
        text(line["id"], "line id")
        require(line["id"] not in line_ids, "duplicate line id")
        line_ids.add(line["id"])
        text(line["speaker"], "speaker")
        require(line["speaker"] in ids, "unknown speaker")
        keys(line["texts"], LOCALES, "localized text")
        for value in line["texts"].values():
            text(value, "line text")
        require(type(line["expression"]) is str and line["expression"] in EXPRESSIONS, "unknown expression")
        require(type(line["intensity_permille"]) is int and 0 <= line["intensity_permille"] <= 1000, "expression intensity")
        text(line["delivery"], "delivery")
        require(line["audio_status"] == "not_generated" and line["translation_status"] == "draft_needs_native_review", "sample is not a shipping asset")
    return len(chars), len(lines)


def unique_pairs(pairs: list[tuple[str, object]]) -> dict:
    result = {}
    for key, value in pairs:
        require(key not in result, f"duplicate JSON key: {key}")
        result[key] = value
    return result


def load(path: Path) -> dict:
    require(path.stat().st_size <= 1_000_000, "fixture exceeds size limit")
    return json.loads(path.read_text(encoding="utf-8"), object_pairs_hook=unique_pairs,
                      parse_constant=lambda x: (_ for _ in ()).throw(ValueError(f"invalid number: {x}")))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("contract", type=Path)
    args = parser.parse_args()
    try:
        characters, lines = validate(load(args.contract))
    except (ValueError, OSError, TypeError, RecursionError) as exc:
        parser.exit(1, f"FAIL: {exc}\n")
    print(f"PASS: {characters} character briefs, {lines} bilingual sample lines; source-only")


if __name__ == "__main__":
    main()
