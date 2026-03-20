/**
 * heartfull_membrane.c
 * NSIGII Perspective Membrane Calibration Implementation
 * OBINexus Computing | Nnamdi Michael Okpala
 * Version: 0.1-DRAFT | 20 March 2026
 *
 * Implements the six-phase NSIGII boot calibrator.
 * Needs-gate: only PASS allows OS to proceed.
 *
 * Drift Theorem integration:
 *   D(t) = dV(t)/dt   where V(t) = P(t) - C(t)
 *   Radial drift Dr = d||V||/dt
 */

#include "heartfull_firmware.h"
#include "bzy_mpda.h"
#include "tripartite_discriminant.h"

#include <math.h>
#include <string.h>
#include <stdio.h>

/* =========================================================================
 * RIFT TRINARY COMPOSITION TABLE
 *
 *  MAYBE propagated: (-1)^3 = -1  (does not collapse under triple derivation)
 *  YES  (1)  × YES  (1)  = YES  (1)
 *  NO   (0)  × anything  = NO   (0)
 *  MAYBE(-1) × MAYBE(-1) = YES  (1)  [double negation resolves]
 *  MAYBE(-1) × YES  (1)  = MAYBE(-1) [uncertainty persists]
 *  MAYBE_NOT(-2) absorbs: anything × MAYBE_NOT = MAYBE_NOT (deferred)
 * ========================================================================= */
TrinaryState trinary_compose(TrinaryState a, TrinaryState b)
{
    /* Defer wins: if either side is MAYBE_NOT, result is deferred */
    if (a == TRINARY_MAYBE_NOT || b == TRINARY_MAYBE_NOT)
        return TRINARY_MAYBE_NOT;

    /* NO absorbs YES and MAYBE */
    if (a == TRINARY_NO || b == TRINARY_NO)
        return TRINARY_NO;

    /* Both YES */
    if (a == TRINARY_YES && b == TRINARY_YES)
        return TRINARY_YES;

    /* Both MAYBE: double negation resolves to YES */
    if (a == TRINARY_MAYBE && b == TRINARY_MAYBE)
        return TRINARY_YES;

    /* One MAYBE + one YES: uncertainty persists */
    return TRINARY_MAYBE;
}

/* =========================================================================
 * TRINARY RESOLVE — compose all three pointer states
 * α=WANT, β=NEED, γ=SHOULD
 * Only YES in all three → PASS
 * ========================================================================= */
TrinaryState trinary_resolve(TrinaryState want,
                              TrinaryState need,
                              TrinaryState should)
{
    TrinaryState ab = trinary_compose(want, need);
    return trinary_compose(ab, should);
}

/* =========================================================================
 * ENZYME APPLICATION — enzymatic action on MAYBE states
 * MAYBE  → may CREATE or DESTROY (thread-level action)
 * MAYBE_NOT → defers entirely
 * ========================================================================= */
TrinaryState enzyme_apply(EnzymeOp op, TrinaryState current)
{
    switch (op) {
        case ENZYME_CREATE:
            /* MAYBE creates a YES state from uncertainty */
            if (current == TRINARY_MAYBE) return TRINARY_YES;
            return current;

        case ENZYME_DESTROY:
            /* MAYBE destroys: collapses YES back to NO */
            if (current == TRINARY_MAYBE) return TRINARY_NO;
            return current;

        case ENZYME_BUILD:
            /* Increments toward resolution: MAYBE → YES, NO → MAYBE */
            if (current == TRINARY_MAYBE)     return TRINARY_YES;
            if (current == TRINARY_NO)        return TRINARY_MAYBE;
            return current;

        case ENZYME_BREAK:
            /* Decrements: YES → MAYBE, MAYBE → NO */
            if (current == TRINARY_YES)       return TRINARY_MAYBE;
            if (current == TRINARY_MAYBE)     return TRINARY_NO;
            return current;

        case ENZYME_RENEW:
            /* Refreshes: preserves YES, resets NO/MAYBE to MAYBE */
            if (current == TRINARY_YES)       return TRINARY_YES;
            return TRINARY_MAYBE;

        case ENZYME_REPAIR:
            /* Patches inconsistency: MAYBE_NOT → MAYBE, NO → MAYBE */
            if (current == TRINARY_MAYBE_NOT) return TRINARY_MAYBE;
            if (current == TRINARY_NO)        return TRINARY_MAYBE;
            return current;

        default:
            return current;
    }
}

/* =========================================================================
 * QUBIT COMPASS ROTATION
 * θ = 0°   → THERE_AND_THEN   (past record)
 * θ = 120° → HERE_AND_NOW     (present scan)
 * θ = 240° → WHEN_AND_WHERE   (projected)
 * ========================================================================= */
void compass_rotate(QubitCompass *c, float new_theta)
{
    if (!c) return;
    float old_theta = c->theta_degrees;
    c->theta_degrees = fmodf(new_theta, 360.0f);
    c->delta_theta   = fabsf(c->theta_degrees - old_theta);

    /* Assign temporal frame labels based on θ position */
    if (c->theta_degrees < 120.0f) {
        /* Ring 1 zone — reading from past record */
        c->ring1_past = c->ring2_present;  /* shift present → past */
    } else if (c->theta_degrees < 240.0f) {
        /* Ring 2 zone — live scan */
        /* ring2_present updated by calibrate() */
    } else {
        /* Ring 3 zone — predictive */
        c->ring3_future = trinary_compose(c->ring2_present, c->ring1_past);
    }
}

/* =========================================================================
 * DRIFT THEOREM — radial drift between two tripolar scans
 * V(t) = (alpha, beta, gamma) as a 3-vector
 * D_r = d||V||/dt  ≈ (||V_t|| - ||V_{t-1}||) / Δt
 * ========================================================================= */
double drift_radial(const TripointerScan *v_prev, const TripointerScan *v_curr)
{
    if (!v_prev || !v_curr) return 0.0;

    double mag_prev = sqrt((double)(v_prev->alpha   * v_prev->alpha) +
                           (double)(v_prev->beta    * v_prev->beta)  +
                           (double)(v_prev->gamma   * v_prev->gamma));

    double mag_curr = sqrt((double)(v_curr->alpha   * v_curr->alpha) +
                           (double)(v_curr->beta    * v_curr->beta)  +
                           (double)(v_curr->gamma   * v_curr->gamma));

    return mag_curr - mag_prev;  /* positive = diverging, negative = converging */
}

/* =========================================================================
 * MEMBRANE INIT — reset to pre-boot state
 * ========================================================================= */
void membrane_init(PerspectiveMembrane *m)
{
    if (!m) return;
    memset(m, 0, sizeof(PerspectiveMembrane));

    m->outcome                = MEMBRANE_HOLD;
    m->scan.alpha             = TRINARY_MAYBE;  /* WANT  unknown at boot   */
    m->scan.beta              = TRINARY_MAYBE;  /* NEED  unknown at boot   */
    m->scan.gamma             = TRINARY_MAYBE;  /* SHOULD unknown at boot  */
    m->compass.theta_degrees  = 0.0f;
    m->compass.ring1_past     = TRINARY_MAYBE;
    m->compass.ring2_present  = TRINARY_MAYBE;
    m->compass.ring3_future   = TRINARY_MAYBE;
    m->discriminant           = 0.0;
    m->boot_phase             = 0;

    /* All tiers start as MAYBE (unverified) */
    for (int i = 1; i <= 5; i++)
        m->needs.tier[i] = TRINARY_MAYBE;
}

/* =========================================================================
 * SIX-PHASE NSIGII BOOT RUNNER
 * Each phase sets compass rotation and evaluates a scan component
 * ========================================================================= */
bool nsigii_run_phase(PerspectiveMembrane *m, NSIGIIPhase phase)
{
    if (!m) return false;
    m->boot_phase = (uint32_t)phase;

    switch (phase) {
        case NSIGII_PHASE_N: {
            /* November: Need-state initialisation
             * Rotate to HERE_AND_NOW, read T1/T2 from hardware probes */
            compass_rotate(&m->compass, 120.0f);
            /* At hardware level, BIOS/UEFI signals feed T1 via ACPI */
            m->needs.tier[MASLOW_TIER1_PHYSIOLOGICAL] = TRINARY_MAYBE; /* probe pending */
            printf("[NSIGII N] Need-state initialised — probing T1/T2\n");
            return true;
        }

        case NSIGII_PHASE_S: {
            /* Sierra: Safety scan — T2 check */
            compass_rotate(&m->compass, 120.0f);
            m->needs.tier[MASLOW_TIER2_SAFETY] = TRINARY_MAYBE;
            printf("[NSIGII S] Safety scan running\n");
            return true;
        }

        case NSIGII_PHASE_I: {
            /* India: Identity calibration (Uche/Obi/Eze tripolar) */
            compass_rotate(&m->compass, 120.0f);
            m->scan.alpha = enzyme_apply(ENZYME_RENEW, m->scan.alpha);
            printf("[NSIGII I] Identity calibration — alpha pointer refreshed\n");
            return true;
        }

        case NSIGII_PHASE_G: {
            /* Golf/Gold: Governance layer — OHA/IWU/IJI check */
            compass_rotate(&m->compass, 240.0f);
            m->scan.beta = enzyme_apply(ENZYME_BUILD, m->scan.beta);
            printf("[NSIGII G] Governance layer check — beta pointer built\n");
            return true;
        }

        case NSIGII_PHASE_I2: {
            /* India (5): Internal probe P_I activation */
            m->scan.gamma = enzyme_apply(ENZYME_REPAIR, m->scan.gamma);
            TrinaryState result = trinary_resolve(m->scan.alpha,
                                                   m->scan.beta,
                                                   m->scan.gamma);
            printf("[NSIGII I2] Internal probe — resolve result: %d\n", result);
            m->compass.ring2_present = result;
            return (result != TRINARY_NO);
        }

        case NSIGII_PHASE_I3: {
            /* India (6): Integrity verification — discriminant check */
            TripartiteState tri = {
                .u_state = m->scan.alpha,
                .v_state = m->scan.beta,
                .w_state = m->scan.gamma,
                .a = 1.0, .b = (double)m->scan.beta, .c = 1.0
            };
            m->discriminant = tripartite_discriminant(&tri);
            printf("[NSIGII I3] Integrity Δ = %.4f\n", m->discriminant);
            return (m->discriminant >= 0.0);
        }

        default:
            return false;
    }
}

/* =========================================================================
 * MEMBRANE CALIBRATE — full six-phase run
 * Returns PASS, HOLD, or ALERT
 * ========================================================================= */
MembraneOutcome membrane_calibrate(PerspectiveMembrane *m,
                                   const MaslowNeedsState *needs)
{
    if (!m || !needs) return MEMBRANE_ALERT;

    /* Copy needs state into membrane */
    memcpy(&m->needs, needs, sizeof(MaslowNeedsState));

    /* Run all six phases in sequence */
    for (NSIGIIPhase phase = NSIGII_PHASE_N; phase <= NSIGII_PHASE_I3; phase++) {
        if (!nsigii_run_phase(m, phase)) {
            printf("[MEMBRANE] Phase %d failed — issuing HOLD\n", phase);
            m->outcome = MEMBRANE_HOLD;
            return m->outcome;
        }
    }

    /* Final gate: T1 + T2 must be satisfied or MAYBE (not NO or MAYBE_NOT) */
    bool t1_ok = (m->needs.tier[MASLOW_TIER1_PHYSIOLOGICAL] == TRINARY_YES ||
                  m->needs.tier[MASLOW_TIER1_PHYSIOLOGICAL] == TRINARY_MAYBE);
    bool t2_ok = (m->needs.tier[MASLOW_TIER2_SAFETY]        == TRINARY_YES ||
                  m->needs.tier[MASLOW_TIER2_SAFETY]        == TRINARY_MAYBE);

    m->needs.tier1_satisfied = t1_ok;
    m->needs.tier2_satisfied = t2_ok;

    /* ALERT: T1 violated OR discriminant negative */
    if (!t1_ok || m->discriminant < 0.0) {
        printf("[MEMBRANE] ALERT — T1 violated or Δ < 0\n");
        m->outcome = MEMBRANE_ALERT;
        return m->outcome;
    }

    /* HOLD: T1 pending (MAYBE) */
    if (m->needs.tier[MASLOW_TIER1_PHYSIOLOGICAL] == TRINARY_MAYBE) {
        printf("[MEMBRANE] HOLD — T1 pending calibration\n");
        m->outcome = MEMBRANE_HOLD;
        return m->outcome;
    }

    /* PASS: All conditions met */
    printf("[MEMBRANE] PASS — proceeding to OS boot\n");
    m->outcome = MEMBRANE_PASS;
    return m->outcome;
}

/* =========================================================================
 * KANBAN TRACK B UNLOCK CHECK
 * Track B (aspiration) locked until T1+T2 both PASS
 * ========================================================================= */
bool kanban_track_b_unlocked(const PerspectiveMembrane *m)
{
    if (!m) return false;
    return (m->outcome == MEMBRANE_PASS &&
            m->needs.tier1_satisfied &&
            m->needs.tier2_satisfied);
}
