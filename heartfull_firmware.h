/**
 * heartfull_firmware.h
 * NSIGII Heartfull Firmware — Perspective Membrane Calibration
 * OBINexus Computing | Nnamdi Michael Okpala
 * Version: 0.1-DRAFT | 20 March 2026
 *
 * Pre-boot constitutional layer of MMUKO-OS.
 * Runs before any OS process. Enforces Maslow Tier 1/2 needs-gate.
 *
 * Trinary alphabet: YES=1, NO=0, MAYBE=-1, MAYBE_NOT=-2 (defer/unresolved)
 */

#ifndef HEARTFULL_FIRMWARE_H
#define HEARTFULL_FIRMWARE_H

#include <stdint.h>
#include <stdbool.h>

/* =========================================================================
 * TRINARY STATE ALPHABET  Σ = {YES, NO, MAYBE, MAYBE_NOT}
 * YES       =  1  → needs met, contract honoured
 * NO        =  0  → needs violated, contract breached
 * MAYBE     = -1  → needs uncertain, response delayed  (enzymatic: create/destroy)
 * MAYBE_NOT = -2  → defer — do NOT handle for operator (system absorbs)
 * ========================================================================= */
typedef enum {
    TRINARY_YES       =  1,
    TRINARY_NO        =  0,
    TRINARY_MAYBE     = -1,
    TRINARY_MAYBE_NOT = -2
} TrinaryState;

/* =========================================================================
 * MEMBRANE OUTCOME — what the calibration scan returns
 * ========================================================================= */
typedef enum {
    MEMBRANE_PASS  = 0,   /* Maslow T1+T2 satisfied → OS may proceed        */
    MEMBRANE_HOLD  = 1,   /* T1 pending  → pause, calibrate, re-scan        */
    MEMBRANE_ALERT = 2    /* T1 violated OR discriminant < 0 → halt+escalate */
} MembraneOutcome;

/* =========================================================================
 * MASLOW TIER IDENTIFIERS
 * ========================================================================= */
typedef enum {
    MASLOW_TIER1_PHYSIOLOGICAL = 1,   /* food, water, shelter, sleep */
    MASLOW_TIER2_SAFETY        = 2,   /* personal safety, employment, health */
    MASLOW_TIER3_BELONGING     = 3,
    MASLOW_TIER4_ESTEEM        = 4,
    MASLOW_TIER5_ACTUALISATION = 5
} MaslowTier;

/* =========================================================================
 * THREE-POINTER CALIBRATION SCAN  (α=WANT, β=NEED, γ=SHOULD)
 * Maps to Tripolar Algebra temporal frames
 * ========================================================================= */
typedef struct {
    TrinaryState alpha;   /* α — WANT:   what the operator desires            */
    TrinaryState beta;    /* β — NEED:   what the operator requires (T1/T2)   */
    TrinaryState gamma;   /* γ — SHOULD: what they are constitutionally owed  */
} TripointerScan;

/* =========================================================================
 * THREE-RING QUBIT COMPASS
 * Ring 1 (outer)  — THERE_AND_THEN  → historical needs record
 * Ring 2 (middle) — HERE_AND_NOW    → current needs state
 * Ring 3 (inner)  — WHEN_AND_WHERE  → anticipated/predictive needs
 * ========================================================================= */
typedef enum {
    TEMPORAL_THERE_AND_THEN  = 0,    /* θ = 0°   */
    TEMPORAL_HERE_AND_NOW    = 120,  /* θ = 120° */
    TEMPORAL_WHEN_AND_WHERE  = 240   /* θ = 240° */
} TemporalFrame;

typedef struct {
    TrinaryState ring1_past;      /* THERE_AND_THEN calibration record   */
    TrinaryState ring2_present;   /* HERE_AND_NOW   current scan         */
    TrinaryState ring3_future;    /* WHEN_AND_WHERE projected state      */
    float        theta_degrees;   /* current compass rotation 0–360°     */
    float        delta_theta;     /* drift between rings (coherence gap) */
} QubitCompass;

/* =========================================================================
 * MASLOW NEEDS STATE — full tier record
 * ========================================================================= */
typedef struct {
    TrinaryState tier[6];          /* index 1–5, 0 unused                */
    bool         tier1_satisfied;
    bool         tier2_satisfied;
    uint32_t     pending_mask;     /* bitmask of pending tiers           */
    uint32_t     violated_mask;    /* bitmask of violated tiers          */
} MaslowNeedsState;

/* =========================================================================
 * PERSPECTIVE MEMBRANE — top-level calibration structure
 * ========================================================================= */
typedef struct {
    MembraneOutcome  outcome;       /* PASS / HOLD / ALERT                 */
    TripointerScan   scan;          /* α/β/γ three-pointer result          */
    QubitCompass     compass;       /* three-ring temporal state           */
    MaslowNeedsState needs;         /* full tier record                    */
    double           discriminant;  /* Δ = b²-4ac tripartite result        */
    uint32_t         boot_phase;    /* current NSIGII phase (1–6)         */
} PerspectiveMembrane;

/* =========================================================================
 * NSIGII SIX-PHASE BOOT SEQUENCE
 * N-S-I-G-I-I  (NATO bands)
 * ========================================================================= */
typedef enum {
    NSIGII_PHASE_N = 1,   /* November — Need-state initialisation    */
    NSIGII_PHASE_S = 2,   /* Sierra   — Safety scan                  */
    NSIGII_PHASE_I = 3,   /* India    — Identity calibration          */
    NSIGII_PHASE_G = 4,   /* Golf/Gold — Governance layer check       */
    NSIGII_PHASE_I2= 5,   /* India    — Internal probe (P_I)          */
    NSIGII_PHASE_I3= 6    /* India    — Integrity verification        */
} NSIGIIPhase;

/* =========================================================================
 * ENZYME OPERATIONS  (Maybe-state degradation pathway)
 * MAYBE → triggers enzymatic action on the computation thread
 * ========================================================================= */
typedef enum {
    ENZYME_CREATE  = 0,   /* MAYBE creates a new state                 */
    ENZYME_DESTROY = 1,   /* MAYBE destroys an existing state          */
    ENZYME_BUILD   = 2,   /* MAYBE+ builds toward resolution           */
    ENZYME_BREAK   = 3,   /* MAYBE- breaks apart ambiguous state       */
    ENZYME_RENEW   = 4,   /* MAYBE refreshes/restores prior state      */
    ENZYME_REPAIR  = 5    /* MAYBE patches inconsistency               */
} EnzymeOp;

/* =========================================================================
 * KANBAN TRACK IDENTIFIERS  (Three-track operational interface)
 * ========================================================================= */
typedef enum {
    KANBAN_TRACK_A = 0,   /* Foundation: T1+T2 — PASS or HOLD          */
    KANBAN_TRACK_B = 1,   /* Aspiration: T3–T5 — PASS only             */
    KANBAN_TRACK_W = 2    /* Adversarial: W-actor monitoring channel    */
} KanbanTrack;

/* =========================================================================
 * FUNCTION DECLARATIONS
 * ========================================================================= */

/* Initialise a fresh membrane (all unknown, phase 1) */
void membrane_init(PerspectiveMembrane *m);

/* Run the full six-phase NSIGII calibration scan */
MembraneOutcome membrane_calibrate(PerspectiveMembrane *m,
                                   const MaslowNeedsState *needs);

/* Apply trinary logic to a pointer (α, β, or γ) */
TrinaryState trinary_resolve(TrinaryState want,
                              TrinaryState need,
                              TrinaryState should);

/* Compose two trinary states (RIFT trinary logic) */
TrinaryState trinary_compose(TrinaryState a, TrinaryState b);

/* Apply enzymatic action based on MAYBE state */
TrinaryState enzyme_apply(EnzymeOp op, TrinaryState current);

/* Rotate qubit compass to a new temporal frame */
void compass_rotate(QubitCompass *c, float new_theta);

/* Check if Track B (aspiration) is unlocked */
bool kanban_track_b_unlocked(const PerspectiveMembrane *m);

/* Run a single NSIGII boot phase; returns true if phase passed */
bool nsigii_run_phase(PerspectiveMembrane *m, NSIGIIPhase phase);

/* Drift theorem: compute radial drift between two need vectors */
double drift_radial(const TripointerScan *v_prev, const TripointerScan *v_curr);

#endif /* HEARTFULL_FIRMWARE_H */
