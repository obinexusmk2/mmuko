/**
 * bzy_mpda.h
 * Byzantine Maybe Perspectivation — Magnetic Pushdown Deterministic Automaton
 * OBINexus Computing | Nnamdi Michael Okpala
 * Version: 0.1-DRAFT | 20 March 2026
 *
 * Formal 5-tuple:  M = (Q, Σ, Γ, δ, q₀, Z₀, F)
 *
 * Σ = {YES, NO, MAYBE}   (trinary input alphabet)
 * Magnetic extension:    δ_magnetic(q, a, θ) → (q', θ')
 * Temporal frames:
 *   θ ∈ [0,120)   → THERE_AND_THEN
 *   θ ∈ [120,240) → HERE_AND_NOW
 *   θ ∈ [240,360) → WHEN_AND_WHERE
 */

#ifndef BZY_MPDA_H
#define BZY_MPDA_H

#include "heartfull_firmware.h"
#include <stddef.h>

/* =========================================================================
 * AUTOMATON STATE SET  Q
 * ========================================================================= */
typedef enum {
    BZY_STATE_PREBOOT     = 0,   /* q₀ — initial: all needs unknown       */
    BZY_STATE_CALIBRATING = 1,   /* needs scan in progress                */
    BZY_STATE_PENDING     = 2,   /* HOLD — T1 unresolved on stack         */
    BZY_STATE_VIOLATED    = 3,   /* ALERT — T1 violated; Byzantine fault  */
    BZY_STATE_ESCALATED   = 4,   /* fault escalated to governance layer   */
    BZY_STATE_ACCEPTED    = 5    /* F — all needs satisfied, stack empty  */
} BZY_State;

/* =========================================================================
 * STACK ENTRY — perspective membrane storage layer
 * ========================================================================= */
#define BZY_STACK_MAX 64

typedef struct {
    MaslowTier     tier;
    TrinaryState   result;       /* PASS(YES) / HOLD(MAYBE) / ALERT(NO)  */
    TemporalFrame  frame;        /* which ring this was calibrated in     */
    float          theta;        /* compass θ at time of calibration      */
} BZY_StackEntry;

typedef struct {
    BZY_StackEntry entries[BZY_STACK_MAX];
    int            top;           /* -1 = empty                           */
} BZY_Stack;

/* =========================================================================
 * MAGNETIC TRANSITION FUNCTION
 * δ_magnetic(q, a, θ) → (q', θ')
 * ========================================================================= */
typedef struct {
    BZY_State     from_state;
    TrinaryState  input_symbol;
    float         theta_in;       /* compass position when transition fires */
    BZY_State     to_state;
    float         theta_out;      /* resulting compass position             */
    EnzymeOp      enzyme_action;  /* enzymatic effect on MAYBE transitions  */
} BZY_Transition;

/* =========================================================================
 * MPDA — full pushdown automaton instance
 * ========================================================================= */
#define BZY_MAX_TRANSITIONS 64

typedef struct {
    /* 5-tuple components */
    BZY_State    current_state;    /* q — current state in Q              */
    BZY_Stack    stack;            /* Γ — perspective membrane storage    */
    float        theta;            /* magnetic pole orientation 0–360°    */

    /* Transition table */
    BZY_Transition transitions[BZY_MAX_TRANSITIONS];
    int            n_transitions;

    /* Initial state */
    BZY_State    q0;               /* always BZY_STATE_PREBOOT            */

    /* Acceptance criteria (runtime check) */
    bool         stack_empty_accept;   /* must stack be empty to accept?  */
    double       discriminant_floor;   /* Δ must be ≥ this to accept      */
} MPDA;

/* =========================================================================
 * FUNCTION DECLARATIONS
 * ========================================================================= */

/* Initialise MPDA with default constitutional transitions */
void mpda_init(MPDA *m);

/* Push a calibration result onto the stack */
bool mpda_push(MPDA *m, MaslowTier tier, TrinaryState result,
               TemporalFrame frame, float theta);

/* Pop top entry from stack */
bool mpda_pop(MPDA *m, BZY_StackEntry *out);

/* Peek at top of stack without popping */
bool mpda_peek(const MPDA *m, BZY_StackEntry *out);

/* Fire a transition: δ_magnetic(current, input, θ) → (next, θ') */
bool mpda_transition(MPDA *m, TrinaryState input);

/* Process a full needs-state string through the MPDA */
BZY_State mpda_run(MPDA *m, const TrinaryState *input_str, size_t len);

/* Check acceptance condition:
 *   1. final state in F (ACCEPTED)
 *   2. stack empty (no unresolved needs)
 *   3. discriminant Δ ≥ 0
 */
bool mpda_accepts(const MPDA *m, double discriminant);

/* Reverse stack traversal (LTCodec reversibility — re-read past calibrations) */
bool mpda_reverse_read(MPDA *m, size_t steps, TrinaryState *out_buf);

/* Map theta to temporal frame */
TemporalFrame mpda_theta_to_frame(float theta);

/* Print MPDA state for debugging */
void mpda_print_state(const MPDA *m);

#endif /* BZY_MPDA_H */
