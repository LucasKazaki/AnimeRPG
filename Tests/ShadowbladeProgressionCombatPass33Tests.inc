namespace {

void TestShadowbladeProgressionCombatPass33() {
    using namespace Astral::Scene;

    CharacterProgression dashProgression;
    dashProgression.GrantMasteryPoints(200);
    Expect(dashProgression.UpgradeTalent(CoreTalent::ShadowStep)
            == ProgressionActionResult::Success,
        "pass33 upgrades ShadowStep for progression combat");
    Expect(dashProgression.EquipTalent(0, CoreTalent::ShadowStep)
            == ProgressionActionResult::Success,
        "pass33 equips ShadowStep for progression combat");
    Expect(dashProgression.RecordSkillPractice(ShadowSkill::Dash, 40) == 40,
        "pass33 records bounded dash mastery practice");

    ShadowbladeActions dashActions;
    const ProgressionCombatBriefing dashBriefing =
        dashActions.CurrentProgressionCombatBriefing(dashProgression);
    Expect(dashBriefing.shadowStepTier == 1
            && dashBriefing.dashSkillRank == CharacterProgression::MaximumSkillRank
            && Near(dashBriefing.dashResourceRefund, 6.0f)
            && Near(dashBriefing.dashEffectiveCost, 19.0f),
        "pass33 briefing exposes exact ShadowStep and dash-mastery economy");

    const ProgressionShadowActionReport dash = dashActions.TryProgressionDash(
        {}, {1.0f, 0.0f, 0.0f}, dashProgression);
    Expect(dash.action.result == ShadowActionResult::Activated
            && Near(dash.resourceRestored, 6.0f)
            && Near(dash.effectiveResourceSpent, 19.0f)
            && Near(dashActions.Resource(), 81.0f),
        "pass33 ShadowStep and dash mastery reduce live dash resource spend");
    const float dashResourceBeforeRejectedRetry = dashActions.Resource();
    const ProgressionShadowActionReport rejectedDash = dashActions.TryProgressionDash(
        {}, {1.0f, 0.0f, 0.0f}, dashProgression);
    Expect(rejectedDash.action.result == ShadowActionResult::Cooldown
            && Near(rejectedDash.resourceRestored, 0.0f)
            && Near(rejectedDash.effectiveResourceSpent, 0.0f)
            && Near(dashActions.Resource(), dashResourceBeforeRejectedRetry),
        "pass33 rejected progression dash cannot mint a refund or mutate resource");

    CharacterProgression unequippedProgression;
    unequippedProgression.GrantMasteryPoints(200);
    Expect(unequippedProgression.UpgradeTalent(CoreTalent::EclipseEdge)
            == ProgressionActionResult::Success,
        "pass33 can upgrade an unequipped comparison talent");
    ShadowbladeActions unequippedActions;
    const ProgressionCombatBriefing unequippedBriefing =
        unequippedActions.CurrentProgressionCombatBriefing(unequippedProgression);
    Expect(unequippedBriefing.eclipseEdgeTier == 0
            && Near(unequippedBriefing.defenseCounterResourceRefund, 0.0f),
        "pass33 only applies talent bonuses when the talent is actually equipped");

    CharacterProgression counterProgression;
    counterProgression.GrantMasteryPoints(200);
    Expect(counterProgression.UpgradeTalent(CoreTalent::EclipseEdge)
            == ProgressionActionResult::Success
            && counterProgression.EquipTalent(0, CoreTalent::EclipseEdge)
                == ProgressionActionResult::Success,
        "pass33 equips EclipseEdge for counter specialization");
    Expect(counterProgression.RecordSkillPractice(ShadowSkill::FatalStrike, 40) == 40,
        "pass33 records bounded fatal-strike mastery practice");

    ShadowbladeActions counterActions;
    CombatSandbox counterCombat;
    Expect(counterActions.BeginIncomingAttack({1.0f, 20, 20, true}),
        "pass33 queues a deterministic counter threat");
    counterActions.AdvanceTime(0.90f);
    const DefenseReport perfectGuard = counterActions.TryDefend(DefenseInput::Guard);
    Expect(perfectGuard.result == DefenseResult::PerfectGuard
            && counterActions.HasDefenseCounter(),
        "pass33 counter setup earns the existing perfect-defense opening");
    const ProgressionShadowActionReport counterStrike =
        counterActions.TryProgressionFatalStrike({}, counterCombat, counterProgression);
    Expect(counterStrike.action.result == ShadowActionResult::Activated
            && counterStrike.action.followUp
            && Near(counterStrike.resourceRestored, 6.0f)
            && Near(counterStrike.effectiveResourceSpent, 29.0f)
            && Near(counterActions.Resource(), 71.0f),
        "pass33 EclipseEdge and fatal mastery reduce counter follow-up cost");

    ShadowbladeActions rejectedStrikeActions;
    CombatSandbox rejectedStrikeCombat;
    const ProgressionShadowActionReport rejectedStrike =
        rejectedStrikeActions.TryProgressionFatalStrike(
            {-10.0f, 0.0f, 0.0f}, rejectedStrikeCombat, counterProgression);
    Expect(rejectedStrike.action.result == ShadowActionResult::OutOfRange
            && Near(rejectedStrike.resourceRestored, 0.0f)
            && Near(rejectedStrike.effectiveResourceSpent, 0.0f)
            && Near(rejectedStrikeActions.Resource(), ShadowbladeActions::MaximumResource),
        "pass33 rejected progression Fatal Strike cannot mint a refund or spend resource");

    CharacterProgression staggerProgression;
    staggerProgression.GrantMasteryPoints(200);
    Expect(staggerProgression.UpgradeTalent(CoreTalent::BreakerFocus)
            == ProgressionActionResult::Success
            && staggerProgression.EquipTalent(0, CoreTalent::BreakerFocus)
                == ProgressionActionResult::Success,
        "pass33 equips BreakerFocus for stagger specialization");
    Expect(staggerProgression.RecordSkillPractice(ShadowSkill::FatalStrike, 40) == 40,
        "pass33 shares fatal-strike mastery with stagger follow-ups");

    CombatSandbox staggerCombat;
    Expect(staggerCombat.SetTrainingEnemyProfile(TrainingEnemyProfile::Boss),
        "pass33 uses the existing boss target for nonlethal stagger setup");
    for (int hit = 0; hit < 3; ++hit) {
        const AttackReport heavy = staggerCombat.TryAttack(AttackType::Heavy, {});
        Expect(heavy.result == AttackResult::Hit,
            "pass33 boss stagger setup lands each heavy attack");
        if (hit < 2) staggerCombat.AdvanceTime(1.0f);
    }
    Expect(staggerCombat.IsStaggered(),
        "pass33 boss setup reaches the authoritative stagger opening");

    ShadowbladeActions staggerActions;
    const ProgressionShadowActionReport staggerStrike =
        staggerActions.TryProgressionFatalStrike({}, staggerCombat, staggerProgression);
    Expect(staggerStrike.action.result == ShadowActionResult::Activated
            && staggerStrike.action.followUp
            && Near(staggerStrike.resourceRestored, 6.0f)
            && Near(staggerStrike.effectiveResourceSpent, 24.0f)
            && Near(staggerActions.Resource(), 76.0f),
        "pass33 BreakerFocus and fatal mastery reduce stagger follow-up cost");

    CharacterProgression defenseProgression;
    Expect(defenseProgression.RecordSkillPractice(ShadowSkill::Defense, 40) == 40,
        "pass33 records bounded defense mastery practice");
    ShadowbladeActions defenseActions;
    const ShadowActionReport setupDash = defenseActions.TryDash({});
    Expect(setupDash.result == ShadowActionResult::Activated
            && Near(defenseActions.Resource(), 75.0f),
        "pass33 defense restore setup creates bounded resource room");
    Expect(defenseActions.BeginIncomingAttack({1.0f, 20, 20, true}),
        "pass33 queues deterministic perfect-dodge threat");
    defenseActions.AdvanceTime(0.90f);
    Expect(Near(defenseActions.Resource(), 88.5f),
        "pass33 defense fixture accounts for deterministic resource regeneration");
    const ProgressionDefenseReport perfectDodge =
        defenseActions.TryProgressionDefend(DefenseInput::Dodge, defenseProgression);
    Expect(perfectDodge.defense.result == DefenseResult::PerfectDodge
            && Near(perfectDodge.resourceRestored, 8.0f)
            && Near(defenseActions.Resource(), 96.5f),
        "pass33 defense mastery rewards precise defense with bounded resource recovery");

    ShadowbladeActions ordinaryDefenseActions;
    Expect(ordinaryDefenseActions.TryDash({}).result == ShadowActionResult::Activated
            && ordinaryDefenseActions.BeginIncomingAttack({0.50f, 20, 20, true}),
        "pass33 ordinary-defense fixture creates resource room and a blockable threat");
    const float ordinaryDefenseResource = ordinaryDefenseActions.Resource();
    const ProgressionDefenseReport ordinaryDefense =
        ordinaryDefenseActions.TryProgressionDefend(DefenseInput::Guard, defenseProgression);
    Expect(ordinaryDefense.defense.result == DefenseResult::Guarded
            && Near(ordinaryDefense.resourceRestored, 0.0f)
            && Near(ordinaryDefenseActions.Resource(), ordinaryDefenseResource),
        "pass33 non-perfect defense cannot receive mastery resource recovery");

    ShadowbladeActions cappedDefenseActions;
    Expect(cappedDefenseActions.TryDash({}).result == ShadowActionResult::Activated
            && Near(cappedDefenseActions.RestoreResource(23.0f), 23.0f)
            && Near(cappedDefenseActions.Resource(), 98.0f)
            && cappedDefenseActions.BeginIncomingAttack({0.10f, 20, 20, true}),
        "pass33 capped-defense fixture starts two resource below the maximum");
    const ProgressionDefenseReport cappedDefense =
        cappedDefenseActions.TryProgressionDefend(DefenseInput::Guard, defenseProgression);
    Expect(cappedDefense.defense.result == DefenseResult::PerfectGuard
            && Near(cappedDefense.resourceRestored, 2.0f)
            && Near(cappedDefenseActions.Resource(), ShadowbladeActions::MaximumResource),
        "pass33 perfect-defense mastery restore reports only the amount admitted by the cap");

    CharacterProgression briefingProgression;
    briefingProgression.GrantMasteryPoints(500);
    Expect(briefingProgression.UpgradeTalent(CoreTalent::ShadowStep)
            == ProgressionActionResult::Success
            && briefingProgression.UpgradeTalent(CoreTalent::ShadowStep)
                == ProgressionActionResult::Success
            && briefingProgression.UpgradeTalent(CoreTalent::ShadowStep)
                == ProgressionActionResult::Success
            && briefingProgression.EquipTalent(0, CoreTalent::ShadowStep)
                == ProgressionActionResult::Success,
        "pass33 briefing setup reaches max ShadowStep tier");
    Expect(briefingProgression.UpgradeTalent(CoreTalent::EclipseEdge)
            == ProgressionActionResult::Success
            && briefingProgression.EquipTalent(1, CoreTalent::EclipseEdge)
                == ProgressionActionResult::Success,
        "pass33 briefing setup equips a second specialization");
    briefingProgression.RecordSkillPractice(ShadowSkill::Dash, 40);
    briefingProgression.RecordSkillPractice(ShadowSkill::FatalStrike, 20);
    briefingProgression.RecordSkillPractice(ShadowSkill::Defense, 10);

    ShadowbladeActions briefingActions;
    const ProgressionCombatBriefing briefing =
        briefingActions.CurrentProgressionCombatBriefing(briefingProgression);
    Expect(briefing.shadowStepTier == 3 && briefing.eclipseEdgeTier == 1
            && briefing.breakerFocusTier == 0
            && briefing.dashSkillRank == 5
            && briefing.fatalStrikeSkillRank == 3
            && briefing.defenseSkillRank == 2
            && Near(briefing.dashResourceRefund, 10.0f)
            && Near(briefing.dashEffectiveCost, 15.0f)
            && Near(briefing.fatalStrikeResourceRefund, 2.0f)
            && Near(briefing.fatalStrikeEffectiveCost, 48.0f)
            && Near(briefing.defenseCounterResourceRefund, 4.0f)
            && Near(briefing.defenseCounterEffectiveCost, 31.0f)
            && Near(briefing.staggerFollowUpResourceRefund, 2.0f)
            && Near(briefing.staggerFollowUpEffectiveCost, 28.0f)
            && Near(briefing.perfectDefenseResourceRestore, 2.0f),
        "pass33 build briefing exposes exact bounded deltas without mutating combat");

    int briefingLevelsGained = 0;
    briefingProgression.GrantExperience(100000, briefingLevelsGained);
    Expect(briefingActions.Loadout().AcquireWeapon(ShadowbladeWeapon::RiftsteelSabre)
            && briefingActions.Loadout().TryEquipWeapon(
                ShadowbladeWeapon::RiftsteelSabre, briefingProgression)
                == LoadoutActionResult::Success,
        "pass33 briefing fixture equips Riftsteel through existing ownership and level gates");
    const ProgressionCombatBriefing riftsteelBriefing =
        briefingActions.CurrentProgressionCombatBriefing(briefingProgression);
    Expect(Near(riftsteelBriefing.fatalStrikeEffectiveCost, 48.0f)
            && Near(riftsteelBriefing.defenseCounterEffectiveCost, 26.0f)
            && Near(riftsteelBriefing.staggerFollowUpEffectiveCost, 23.0f),
        "pass33 briefing includes Riftsteel only in authoritative follow-up costs");

    const float briefingResource = briefingActions.Resource();
    const int briefingHealth = briefingActions.PlayerHealth();
    const int briefingGuard = briefingActions.GuardIntegrity();
    (void)briefingActions.CurrentProgressionCombatBriefing(briefingProgression);
    Expect(Near(briefingActions.Resource(), briefingResource)
            && briefingActions.PlayerHealth() == briefingHealth
            && briefingActions.GuardIntegrity() == briefingGuard,
        "pass33 build briefing is read-only and safe to inspect repeatedly");
}

} // namespace
