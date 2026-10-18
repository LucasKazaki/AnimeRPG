import copy
import json
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'Tools/DialoguePresentation'))
import validate_contract as v


class ContractTests(unittest.TestCase):
    def setUp(self):
        self.data = v.load(ROOT / 'Content/Narrative/voice-dialogue-contract.v1.json')

    def reject(self, mutate):
        data = copy.deepcopy(self.data)
        mutate(data)
        with self.assertRaises(ValueError):
            v.validate(data)

    def test_valid_source_fixture(self):
        self.assertEqual(v.validate(self.data), (3, 3))

    def test_schema_bool_and_float_rejected(self):
        for value in (True, 1.0, 2):
            self.reject(lambda d: d.update(schema_version=value))

    def test_unknown_field_rejected(self):
        self.reject(lambda d: d.update(runtime_approved=True))

    def test_runtime_claim_rejected(self):
        self.reject(lambda d: d.update(status='playable'))

    def test_missing_or_duplicate_language_rejected(self):
        self.reject(lambda d: d.update(required_locales=['en-US']))
        self.reject(lambda d: d.update(required_locales=['en-US', 'en-US']))

    def test_party_switching_rejected(self):
        self.reject(lambda d: d.update(allow_party_switching=True))

    def test_second_playable_character_rejected(self):
        self.reject(lambda d: d['characters'][1].update(playable=True))

    def test_no_protagonist_rejected(self):
        self.reject(lambda d: d['characters'][0].update(playable=False))

    def test_playable_integer_rejected(self):
        self.reject(lambda d: d['characters'][0].update(playable=1))

    def test_character_gacha_rejected(self):
        self.reject(lambda d: d['gacha']['permitted_categories'].append('characters'))
        self.reject(lambda d: d['gacha'].update(characters_permitted=True))

    def test_unapproved_gacha_activation_rejected(self):
        self.reject(lambda d: d['gacha'].update(enabled=True))
        self.reject(lambda d: d['gacha'].update(real_money_authorized=True))

    def test_forced_waits_and_nonboolean_settings_rejected(self):
        self.reject(lambda d: d['presentation'].update(manual_advance_waits_for_voice=True))
        self.reject(lambda d: d['presentation'].update(manual_advance_waits_for_animation=True))
        self.reject(lambda d: d['presentation'].update(history_is_read_only=1))

    def test_unsafe_choice_skip_rejected(self):
        self.reject(lambda d: d['presentation'].update(skip_stops_at_meaningful_choice=False))

    def test_missing_japanese_casting_rejected(self):
        self.reject(lambda d: d['characters'][1]['locales'].pop('ja-JP'))

    def test_missing_personality_rejected(self):
        self.reject(lambda d: d['characters'][1].update(personality=[]))

    def test_blank_voice_direction_rejected(self):
        self.reject(lambda d: d['characters'][1]['locales']['en-US'].update(direction=' '))

    def test_duplicate_character_rejected(self):
        self.reject(lambda d: d['characters'].append(copy.deepcopy(d['characters'][0])))

    def test_shared_casting_key_rejected(self):
        self.reject(lambda d: d['characters'][1]['locales']['en-US'].update(casting_key='protagonist.en-US.v1'))

    def test_fabricated_generation_or_approval_rejected(self):
        self.reject(lambda d: d['characters'][1]['locales']['en-US'].update(provider_voice_id='made-up'))
        self.reject(lambda d: d['characters'][1]['locales']['en-US'].update(rights_review='approved'))
        self.reject(lambda d: d['sample_lines'][0].update(audio_status='generated'))

    def test_unapproved_recording_rejected(self):
        self.reject(lambda d: d['characters'][1]['locales']['en-US'].update(reference_asset='actor.wav'))

    def test_unknown_line_speaker_rejected(self):
        self.reject(lambda d: d['sample_lines'][0].update(speaker='missing'))

    def test_duplicate_line_id_rejected(self):
        self.reject(lambda d: d['sample_lines'].append(copy.deepcopy(d['sample_lines'][0])))

    def test_missing_translation_rejected(self):
        self.reject(lambda d: d['sample_lines'][0]['texts'].pop('ja-JP'))

    def test_invalid_expression_and_intensity_rejected(self):
        self.reject(lambda d: d['sample_lines'][0].update(expression='missing'))
        for value in (True, -1, 1001, 0.5):
            self.reject(lambda d: d['sample_lines'][0].update(intensity_permille=value))

    def test_duplicate_json_keys_and_nonfinite_numbers_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            p = Path(temp) / 'bad.json'
            for raw in ('{"a":1,"a":2}', '{"a": {"b":1,"b":2}}', '{"a":NaN}'):
                p.write_text(raw)
                with self.assertRaises(ValueError):
                    v.load(p)

    def test_unicode_draft_roundtrip(self):
        restored = json.loads(json.dumps(self.data, ensure_ascii=False))
        self.assertEqual(v.validate(restored), (3, 3))
        self.assertIn('亀裂', restored['sample_lines'][0]['texts']['ja-JP'])


if __name__ == '__main__':
    unittest.main()
