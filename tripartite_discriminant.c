/**
 * tripartite_discriminant.c
 * G = {U, V, W} Three-Actor Byzantine Discriminant Implementation
 * OBINexus Computing | Nnamdi Michael Okpala
 * Version: 0.1-DRAFT | 20 March 2026
 *
 * Quadratic formula applied to constitutional relationships:
 *   b² = coherence between all three parties
 *   4ac = product of institutional and adversarial energy
 *
 * Trinary → Byzantine mapping:
 *   YES  (1)  → all three actors agree
 *   NO   (0)  → V or W actively failing
 *   MAYBE(-1) → W introducing noise / V decoherent
 */

#include "tripartite_discriminant.h"
#include <math.h>
#include <stdio.h>

/* =========================================================================
 * BUILD TRIPARTITE STATE WITH AUTO-DERIVED COEFFICIENTS
 * Maps trinary values to quadratic coefficients:
 *   a = 1.0 (unit institutional weight)
 *   b = sum of U+V+W signals (coherence measure)
 *   c = 1.0 (unit constitutional constant)
 *
 * This gives:
 *   All YES  (1+1+1=3):  Δ = 9 - 4 = 5   > 0 → STABLE
 *   Mixed MAYBE (-1):    Δ may drop below 0 depending on combination
 *   All NO   (0):        b=0, Δ = 0 - 4 = -4 < 0 → FAULT
 * ========================================================================= */
TripartiteState tripartite_build(TrinaryState u, TrinaryState v, TrinaryState w)
{
    TripartiteState tri;
    tri.u_state = u;
    tri.v_state = v;
    tri.w_state = w;

    /* Coherence b = sum of all three signals (range: -3 to 3) */
    double b = (double)u + (double)v + (double)w;

    tri.a = 1.0;
    tri.b = b;
    tri.c = 1.0;

    return tri;
}

/* =========================================================================
 * COMPUTE DISCRIMINANT
 * Δ = b² − 4ac
 * ========================================================================= */
double tripartite_discriminant(const TripartiteState *tri)
{
    if (!tri) return -1.0;  /* null input → treat as fault */
    return (tri->b * tri->b) - (4.0 * tri->a * tri->c);
}

/* =========================================================================
 * CLASSIFY DISCRIMINANT REGION
 * ========================================================================= */
DiscriminantRegion tripartite_classify(double delta)
{
    if (delta > 0.0)  return DISC_STABLE;
    if (delta == 0.0) return DISC_CRITICAL;
    return DISC_FAULT;
}

/* =========================================================================
 * BYZANTINE CONSENSUS — maps three trinary inputs to consensus state
 * ========================================================================= */
ByzantineConsensus tripartite_consensus(const TripartiteState *tri)
{
    if (!tri) return BYZ_V_OR_W_FAILING;

    /* All agree (YES) */
    if (tri->u_state == TRINARY_YES &&
        tri->v_state == TRINARY_YES &&
        tri->w_state == TRINARY_YES)
        return BYZ_ALL_AGREE;

    /* Any NO → active failure */
    if (tri->u_state == TRINARY_NO  ||
        tri->v_state == TRINARY_NO  ||
        tri->w_state == TRINARY_NO)
        return BYZ_V_OR_W_FAILING;

    /* Any MAYBE → noise/decoherence */
    return BYZ_NOISE_DECOHERENT;
}

/* =========================================================================
 * FAULT DETECTION
 * ========================================================================= */
bool tripartite_fault_detected(const TripartiteState *tri)
{
    double delta = tripartite_discriminant(tri);
    return (delta < 0.0);
}

/* =========================================================================
 * QUADRATIC ROOTS (positive/negative solution paths)
 * x = (-b ± √Δ) / 2a
 * Positive root (+) → BUILD/CREATE constitutional path
 * Negative root (−) → BREAK/DESTROY adversarial path
 * ========================================================================= */
bool tripartite_roots(const TripartiteState *tri,
                      double *root_pos, double *root_neg)
{
    if (!tri || !root_pos || !root_neg) return false;

    double delta = tripartite_discriminant(tri);
    if (delta < 0.0) {
        printf("[TRIPARTITE] Complex roots — Byzantine fault detected\n");
        return false;
    }

    double sqrt_delta = sqrt(delta);
    *root_pos = (-tri->b + sqrt_delta) / (2.0 * tri->a);
    *root_neg = (-tri->b - sqrt_delta) / (2.0 * tri->a);

    return true;
}

/* =========================================================================
 * FULL CONSTITUTIONAL CHECK — convenience wrapper
 * ========================================================================= */
DiscriminantRegion tripartite_check(TrinaryState u, TrinaryState v, TrinaryState w)
{
    TripartiteState tri = tripartite_build(u, v, w);
    double delta = tripartite_discriminant(&tri);

    DiscriminantRegion region = tripartite_classify(delta);

    printf("[TRIPARTITE] U=%d V=%d W=%d  b=%.1f  Δ=%.4f  → %s\n",
           u, v, w, tri.b, delta,
           region == DISC_STABLE   ? "STABLE"   :
           region == DISC_CRITICAL ? "CRITICAL" : "FAULT");

    if (region == DISC_STABLE) {
        double rp, rn;
        if (tripartite_roots(&tri, &rp, &rn)) {
            printf("[TRIPARTITE] roots: BUILD=%.4f  BREAK=%.4f\n", rp, rn);
        }
    }

    return region;
}
