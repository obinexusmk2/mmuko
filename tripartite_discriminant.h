/**
 * tripartite_discriminant.h
 * Tripartite Discriminant — G = {U, V, W}
 * OBINexus Computing | Nnamdi Michael Okpala
 * Version: 0.1-DRAFT | 20 March 2026
 *
 * Extends bipartite discriminant (U=citizen, V=institution) with
 * W = adversary/attacker/third-party.
 *
 * Δ = b² − 4ac over three actors encodes:
 *   Δ > 0  → two real roots: stable constitutional relationship
 *   Δ = 0  → critical point: W present but system still soluble
 *   Δ < 0  → complex roots: W disrupting U–V; Byzantine fault
 *
 * Byzantine fault tolerance: 3f+1 nodes for f=1 → minimum 4
 * With U and V always present, W detection via Δ < 0 = minimum viable
 * constitutional Byzantine configuration.
 */

#ifndef TRIPARTITE_DISCRIMINANT_H
#define TRIPARTITE_DISCRIMINANT_H

#include "heartfull_firmware.h"

/* =========================================================================
 * TRIPARTITE STATE — three-actor configuration
 * U = user / citizen / operator
 * V = institution (duty-bearer: council, university, employer)
 * W = adversary / attacker / blockchain defender / third party
 * ========================================================================= */
typedef struct {
    TrinaryState u_state;    /* U actor's trinary signal   */
    TrinaryState v_state;    /* V actor's trinary signal   */
    TrinaryState w_state;    /* W actor's trinary signal   */

    double a;                /* quadratic coefficient a    */
    double b;                /* quadratic coefficient b (coherence) */
    double c;                /* quadratic coefficient c    */
} TripartiteState;

/* =========================================================================
 * BYZANTINE CONSENSUS STATES (maps YES/NO/MAYBE)
 * ========================================================================= */
typedef enum {
    BYZ_ALL_AGREE       = 0,   /* YES(1): all three actors in agreement     */
    BYZ_V_OR_W_FAILING  = 1,   /* NO(0):  V or W actively failing           */
    BYZ_NOISE_DECOHERENT= 2    /* MAYBE(-1): W introducing noise / V absent */
} ByzantineConsensus;

/* =========================================================================
 * DISCRIMINANT REGION
 * ========================================================================= */
typedef enum {
    DISC_STABLE    = 1,    /* Δ > 0: two real roots, W benign or absent    */
    DISC_CRITICAL  = 0,    /* Δ = 0: W present, creating pressure          */
    DISC_FAULT     = -1    /* Δ < 0: W actively disrupting U–V, Byzantine  */
} DiscriminantRegion;

/* =========================================================================
 * FUNCTION DECLARATIONS
 * ========================================================================= */

/* Compute raw discriminant Δ = b² − 4ac */
double tripartite_discriminant(const TripartiteState *tri);

/* Classify discriminant into region */
DiscriminantRegion tripartite_classify(double delta);

/* Derive Byzantine consensus from three trinary states */
ByzantineConsensus tripartite_consensus(const TripartiteState *tri);

/* Build a TripartiteState from three actors' signals */
TripartiteState tripartite_build(TrinaryState u, TrinaryState v, TrinaryState w);

/* Check if a Byzantine fault is detected (Δ < 0) */
bool tripartite_fault_detected(const TripartiteState *tri);

/* Compute quadratic roots if real (returns false if complex) */
bool tripartite_roots(const TripartiteState *tri, double *root_pos, double *root_neg);

/* Run full constitutional check: returns DISC_STABLE, CRITICAL, or FAULT */
DiscriminantRegion tripartite_check(TrinaryState u, TrinaryState v, TrinaryState w);

#endif /* TRIPARTITE_DISCRIMINANT_H */
