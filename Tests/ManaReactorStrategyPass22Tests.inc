namespace {

void TestManaReactorStrategyPass22() {
    using namespace Astral::Scene;

    const ExplorationFieldGuide readyGuide = ManaReactorReadyGuide();

    // GAME-107: selectable pressure presets change reactor pressure without changing the objective graph.
    ManaReactorExpedition invalidDifficulty;
    Expect(!invalidDifficulty.TryBegin(readyGuide, ManaReactorMode::Expedition,
            static_cast<ManaReactorDifficulty>(255)),
        "invalid Mana Reactor difficulty fails closed");
    Expect(!invalidDifficulty.Active() && invalidDifficulty.ProgressPercent() == 0,
        "invalid difficulty cannot mutate or start the reactor run");

    ManaReactorExpedition guided;
    ManaReactorExpedition standard;
    ManaReactorExpedition critical;
    Expect(guided.TryBegin(readyGuide, ManaReactorMode::Expedition,
                ManaReactorDifficulty::Guided)
            && standard.TryBegin(readyGuide, ManaReactorMode::Expedition,
                ManaReactorDifficulty::Standard)
            && critical.TryBegin(readyGuide, ManaReactorMode::Expedition,
                ManaReactorDifficulty::Critical),
        "all declared Mana Reactor difficulty presets start from the same unlocked objective graph");
    Expect(guided.ApplyControl(ManaReactorControl::Balance) == ManaReactorControlResult::Applied
            && standard.ApplyControl(ManaReactorControl::Balance) == ManaReactorControlResult::Applied
            && critical.ApplyControl(ManaReactorControl::Balance) == ManaReactorControlResult::Applied,
        "difficulty comparison applies the same first control without changing objective semantics");
    Expect(guided.Heat() == 22 && standard.Heat() == 26 && critical.Heat() == 32
            && guided.Stability() == 90 && standard.Stability() == 90
            && critical.Stability() == 86,
        "Guided, Standard, and Critical produce deterministic bounded pressure differences");

    // GAME-108: an opt-in protocol changes control behavior while Baseline preserves pass-21 tuning.
    ManaReactorExpedition invalidProtocol;
    Expect(!invalidProtocol.TryBegin(readyGuide, ManaReactorMode::Expedition,
            ManaReactorDifficulty::Standard, static_cast<ManaReactorProtocol>(255)),
        "invalid Mana Reactor protocol fails closed");
    ManaReactorExpedition baseline;
    ManaReactorExpedition thermal;
    ManaReactorExpedition stability;
    Expect(baseline.TryBegin(readyGuide)
            && thermal.TryBegin(readyGuide, ManaReactorMode::Expedition,
                ManaReactorDifficulty::Standard, ManaReactorProtocol::ThermalSink)
            && stability.TryBegin(readyGuide, ManaReactorMode::Expedition,
                ManaReactorDifficulty::Standard, ManaReactorProtocol::StabilityMesh),
        "Baseline and opt-in reactor protocols admit valid runs");
    baseline.ApplyControl(ManaReactorControl::Balance);
    thermal.ApplyControl(ManaReactorControl::Balance);
    stability.ApplyControl(ManaReactorControl::Balance);
    Expect(baseline.Heat() == 26 && baseline.Stability() == 90
            && thermal.Heat() == 23 && thermal.Stability() == 90
            && stability.Heat() == 26 && stability.Stability() == 93,
        "Thermal Sink and Stability Mesh modify only their declared control axis at rank one");

    // GAME-109: one emergency vent per stage trades stability and score potential for immediate cooling.
    ManaReactorExpedition ventRun;
    Expect(ventRun.TryBegin(readyGuide), "emergency-vent fixture starts");
    Expect(ventRun.ApplyControl(ManaReactorControl::Balance) == ManaReactorControlResult::Applied
            && ventRun.Heat() == 26 && ventRun.Stability() == 90,
        "emergency-vent fixture reaches deterministic pre-vent state");
    Expect(ventRun.UseEmergencyVent() && ventRun.Heat() == 1 && ventRun.Stability() == 85
            && !ventRun.EmergencyVentAvailable() && ventRun.EmergencyVentsUsed() == 1,
        "emergency vent cools by exactly 25, costs five stability, and consumes the stage use");
    const auto afterVent = ventRun.Snapshot();
    Expect(!ventRun.UseEmergencyVent() && ventRun.Heat() == afterVent.heat
            && ventRun.Stability() == afterVent.stability
            && ventRun.EmergencyVentsUsed() == afterVent.emergencyVentsUsed,
        "a consumed emergency vent rejects duplicate use without state mutation");
    Expect(ventRun.ApplyControl(ManaReactorControl::Balance)
                == ManaReactorControlResult::StageCleared
            && ventRun.EmergencyVentAvailable() && ventRun.EmergencyVentsUsed() == 1,
        "the next stage receives a fresh vent while run-level usage remains accountable");

    ManaReactorExpedition lowHeatVent;
    Expect(lowHeatVent.TryBegin(readyGuide)
            && lowHeatVent.ApplyControl(ManaReactorControl::Stabilize)
                == ManaReactorControlResult::Applied
            && lowHeatVent.Heat() == 16 && lowHeatVent.Stability() == 98,
        "low-heat vent fixture reaches less than the declared 25-point cooling amount");
    const auto beforeLowHeatVent = lowHeatVent.Snapshot();
    Expect(!lowHeatVent.UseEmergencyVent()
            && lowHeatVent.Heat() == beforeLowHeatVent.heat
            && lowHeatVent.Stability() == beforeLowHeatVent.stability
            && lowHeatVent.ObjectiveProgress() == beforeLowHeatVent.objectiveProgress
            && lowHeatVent.EmergencyVentAvailable()
            && lowHeatVent.EmergencyVentsUsed() == 0,
        "vent rejects insufficient heat without partial cooling, stability cost, use consumption, or objective mutation");

    // GAME-110: completing the optional stage target amplifies the selected protocol for later stages.
    ManaReactorExpedition amplified;
    Expect(amplified.TryBegin(readyGuide, ManaReactorMode::Expedition,
            ManaReactorDifficulty::Standard, ManaReactorProtocol::SurgeHarness),
        "protocol-amplification fixture starts with Surge Harness");
    Expect(amplified.ApplyControl(ManaReactorControl::Stabilize)
                == ManaReactorControlResult::Applied
            && amplified.ApplyControl(ManaReactorControl::Stabilize)
                == ManaReactorControlResult::StageCleared
            && amplified.OptionalTargetsComplete() == 1 && amplified.ProtocolRank() == 2,
        "a clean optional Intake target raises the selected protocol exactly one rank");
    Expect(amplified.ApplyControl(ManaReactorControl::Overdrive)
                == ManaReactorControlResult::StageCleared
            && amplified.CurrentStage() == ManaReactorStage::CoreChamber
            && amplified.ProtocolRank() == 2,
        "rank-two Surge Harness turns one Cooling overdrive into the full three-unit objective without fabricating another rank");
    Expect(amplified.StartOver() && amplified.Protocol() == ManaReactorProtocol::SurgeHarness
            && amplified.Difficulty() == ManaReactorDifficulty::Standard
            && amplified.ProtocolRank() == 1 && amplified.OptionalTargetsComplete() == 0,
        "Start Over retains the selected strategy but resets earned in-run protocol amplification");

    // GAME-111: varied protocol controls build a bounded precision chain and earn real pulse effects/score.
    ManaReactorExpedition precision;
    Expect(precision.TryBegin(readyGuide, ManaReactorMode::Expedition,
            ManaReactorDifficulty::Standard, ManaReactorProtocol::ThermalSink),
        "precision-chain fixture starts with Thermal Sink");
    Expect(precision.ApplyControl(ManaReactorControl::Stabilize)
                == ManaReactorControlResult::Applied
            && precision.ApplyControl(ManaReactorControl::Stabilize)
                == ManaReactorControlResult::StageCleared
            && precision.ProtocolRank() == 2,
        "precision fixture earns rank two before the longer Cooling sequence");
    Expect(precision.ApplyControl(ManaReactorControl::Stabilize)
                == ManaReactorControlResult::Applied
            && precision.ApplyControl(ManaReactorControl::Balance)
                == ManaReactorControlResult::Applied
            && precision.ApplyControl(ManaReactorControl::Stabilize)
                == ManaReactorControlResult::StageCleared,
        "alternating three protocol controls clears Cooling through the normal objective path");
    Expect(precision.BestPrecisionChain() == 3 && precision.PrecisionPulses() == 1
            && precision.PrecisionChain() == 0 && precision.ProtocolRank() == 3,
        "the third varied control earns one bounded precision pulse and stage transition resets only the live chain");
    Expect(precision.ApplyControl(ManaReactorControl::Stabilize)
                == ManaReactorControlResult::Applied
            && precision.ApplyControl(ManaReactorControl::Balance)
                == ManaReactorControlResult::Applied
            && precision.ApplyControl(ManaReactorControl::Stabilize)
                == ManaReactorControlResult::RunCompleted,
        "a second varied three-control sequence completes the Core Chamber");
    const auto precisionSummary = precision.CompletionSummary();
    Expect(precisionSummary.grade == ManaReactorGrade::Gold
            && precisionSummary.protocolRank == ManaReactorExpedition::MaximumProtocolRank
            && precisionSummary.bestPrecisionChain == 3
            && precisionSummary.precisionPulses == 2
            && precisionSummary.score == 1446,
        "precision pulses and protocol amplification feed the deterministic completion score without changing first-clear rewards");

    // QOL-023: compact control forecast is exact, non-mutating, and follows committed terminal transitions.
    ManaReactorExpedition previewRun;
    Expect(previewRun.TryBegin(readyGuide, ManaReactorMode::Expedition,
            ManaReactorDifficulty::Standard, ManaReactorProtocol::ThermalSink),
        "control-preview fixture starts");
    const auto beforePreview = previewRun.Snapshot();
    const auto preview = previewRun.PreviewControl(ManaReactorControl::Balance);
    const auto afterPreview = previewRun.Snapshot();
    Expect(preview.valid && preview.result == ManaReactorControlResult::Applied
            && preview.projectedObjectiveProgress == 1
            && preview.projectedHeat == 23 && preview.projectedStability == 90
            && preview.projectedPrecisionChain == 1
            && preview.protocolApplied && !preview.precisionPulse,
        "control forecast exposes concise projected progress, pressure, strategy, and chain consequences");
    Expect(afterPreview.heat == beforePreview.heat
            && afterPreview.stability == beforePreview.stability
            && afterPreview.objectiveProgress == beforePreview.objectiveProgress
            && afterPreview.precisionChain == beforePreview.precisionChain,
        "previewing a reactor control never mutates authoritative run state");
    Expect(previewRun.ApplyControl(ManaReactorControl::Balance) == preview.result
            && previewRun.Heat() == preview.projectedHeat
            && previewRun.Stability() == preview.projectedStability
            && previewRun.ObjectiveProgress() == preview.projectedObjectiveProgress
            && previewRun.PrecisionChain() == preview.projectedPrecisionChain,
        "committing the previewed control lands on the exact forecasted nonterminal state");

    ManaReactorExpedition stageClearPreviewRun;
    Expect(stageClearPreviewRun.TryBegin(readyGuide, ManaReactorMode::Expedition,
                ManaReactorDifficulty::Standard, ManaReactorProtocol::ThermalSink)
            && stageClearPreviewRun.ApplyControl(ManaReactorControl::Balance)
                == ManaReactorControlResult::Applied,
        "stage-clear preview fixture reaches one-unit Intake progress");
    const auto beforeStageClearPreview = stageClearPreviewRun.Snapshot();
    const auto stageClearPreview = stageClearPreviewRun.PreviewControl(ManaReactorControl::Balance);
    const auto afterStageClearPreview = stageClearPreviewRun.Snapshot();
    Expect(stageClearPreview.valid
            && stageClearPreview.result == ManaReactorControlResult::StageCleared
            && stageClearPreview.projectedObjectiveProgress == 0
            && stageClearPreview.projectedHeat == 45
            && stageClearPreview.projectedStability == 85
            && stageClearPreview.projectedPrecisionChain == 0,
        "stage-clear forecast exposes the authoritative next-stage reset state rather than the completed old stage");
    Expect(afterStageClearPreview.stage == beforeStageClearPreview.stage
            && afterStageClearPreview.heat == beforeStageClearPreview.heat
            && afterStageClearPreview.stability == beforeStageClearPreview.stability
            && afterStageClearPreview.objectiveProgress == beforeStageClearPreview.objectiveProgress
            && afterStageClearPreview.precisionChain == beforeStageClearPreview.precisionChain,
        "stage-clear forecast remains non-mutating before commit");
    Expect(stageClearPreviewRun.ApplyControl(ManaReactorControl::Balance)
                == stageClearPreview.result
            && stageClearPreviewRun.Heat() == stageClearPreview.projectedHeat
            && stageClearPreviewRun.Stability() == stageClearPreview.projectedStability
            && stageClearPreviewRun.ObjectiveProgress() == stageClearPreview.projectedObjectiveProgress
            && stageClearPreviewRun.PrecisionChain() == stageClearPreview.projectedPrecisionChain,
        "stage-clear commit lands on the exact forecasted next-stage state");

    ManaReactorExpedition completionPreviewRun;
    Expect(completionPreviewRun.TryBegin(readyGuide, ManaReactorMode::Expedition,
                ManaReactorDifficulty::Standard, ManaReactorProtocol::ThermalSink)
            && completionPreviewRun.ApplyControl(ManaReactorControl::Stabilize)
                == ManaReactorControlResult::Applied
            && completionPreviewRun.ApplyControl(ManaReactorControl::Stabilize)
                == ManaReactorControlResult::StageCleared
            && completionPreviewRun.ApplyControl(ManaReactorControl::Stabilize)
                == ManaReactorControlResult::Applied
            && completionPreviewRun.ApplyControl(ManaReactorControl::Stabilize)
                == ManaReactorControlResult::Applied
            && completionPreviewRun.ApplyControl(ManaReactorControl::Stabilize)
                == ManaReactorControlResult::StageCleared
            && completionPreviewRun.ApplyControl(ManaReactorControl::Stabilize)
                == ManaReactorControlResult::Applied
            && completionPreviewRun.ApplyControl(ManaReactorControl::Stabilize)
                == ManaReactorControlResult::Applied,
        "run-completion preview fixture reaches the final Core objective unit");
    const auto beforeCompletionPreview = completionPreviewRun.Snapshot();
    const auto completionPreview = completionPreviewRun.PreviewControl(ManaReactorControl::Stabilize);
    const auto afterCompletionPreview = completionPreviewRun.Snapshot();
    Expect(completionPreview.valid
            && completionPreview.result == ManaReactorControlResult::RunCompleted
            && completionPreview.projectedObjectiveProgress == 0
            && completionPreview.projectedPrecisionChain == 0,
        "run-completion forecast exposes post-completion objective and chain resets");
    Expect(afterCompletionPreview.stage == beforeCompletionPreview.stage
            && afterCompletionPreview.heat == beforeCompletionPreview.heat
            && afterCompletionPreview.stability == beforeCompletionPreview.stability
            && !afterCompletionPreview.complete,
        "run-completion forecast remains non-mutating before commit");
    Expect(completionPreviewRun.ApplyControl(ManaReactorControl::Stabilize)
                == completionPreview.result
            && completionPreviewRun.Complete()
            && completionPreviewRun.Heat() == completionPreview.projectedHeat
            && completionPreviewRun.Stability() == completionPreview.projectedStability
            && completionPreviewRun.ObjectiveProgress() == completionPreview.projectedObjectiveProgress
            && completionPreviewRun.PrecisionChain() == completionPreview.projectedPrecisionChain,
        "run-completion commit lands on the exact forecasted terminal state");

    ManaReactorExpedition failurePreviewRun;
    Expect(failurePreviewRun.TryBegin(readyGuide, ManaReactorMode::Expedition,
                ManaReactorDifficulty::Critical, ManaReactorProtocol::SurgeHarness)
            && failurePreviewRun.ApplyControl(ManaReactorControl::Overdrive)
                == ManaReactorControlResult::StageCleared
            && failurePreviewRun.ApplyControl(ManaReactorControl::Overdrive)
                == ManaReactorControlResult::Applied,
        "failure preview fixture reaches high-pressure Cooling state");
    const auto beforeFailurePreview = failurePreviewRun.Snapshot();
    const auto failurePreview = failurePreviewRun.PreviewControl(ManaReactorControl::Overdrive);
    const auto afterFailurePreview = failurePreviewRun.Snapshot();
    Expect(failurePreview.valid
            && failurePreview.result == ManaReactorControlResult::Failed
            && failurePreview.projectedHeat == ManaReactorExpedition::MaximumHeat
            && failurePreview.projectedPrecisionChain == 0,
        "failure forecast exposes the committed failed-state chain reset");
    Expect(afterFailurePreview.heat == beforeFailurePreview.heat
            && afterFailurePreview.stability == beforeFailurePreview.stability
            && afterFailurePreview.objectiveProgress == beforeFailurePreview.objectiveProgress
            && !afterFailurePreview.failed,
        "failure forecast remains non-mutating before commit");
    Expect(failurePreviewRun.ApplyControl(ManaReactorControl::Overdrive)
                == failurePreview.result
            && failurePreviewRun.Failed()
            && failurePreviewRun.Heat() == failurePreview.projectedHeat
            && failurePreviewRun.Stability() == failurePreview.projectedStability
            && failurePreviewRun.ObjectiveProgress() == failurePreview.projectedObjectiveProgress
            && failurePreviewRun.PrecisionChain() == failurePreview.projectedPrecisionChain,
        "failed commit lands on the exact forecasted terminal pressure and chain state");

    const auto beforeInvalidPreview = previewRun.Snapshot();
    const auto invalidPreview = previewRun.PreviewControl(static_cast<ManaReactorControl>(255));
    const auto afterInvalidPreview = previewRun.Snapshot();
    Expect(!invalidPreview.valid && invalidPreview.result == ManaReactorControlResult::Rejected
            && afterInvalidPreview.heat == beforeInvalidPreview.heat
            && afterInvalidPreview.stability == beforeInvalidPreview.stability
            && afterInvalidPreview.objectiveProgress == beforeInvalidPreview.objectiveProgress,
        "invalid control preview fails closed and remains non-mutating");
}

} // namespace
