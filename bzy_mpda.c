/**
 * bzy_mpda.c
 * Magnetic Pushdown Deterministic Automaton Implementation
 * OBINexus Computing | Nnamdi Michael Okpala
 * Version: 0.1-DRAFT | 20 March 2026
 */

#include "bzy_mpda.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

/* =========================================================================
 * STACK OPERATIONS
 * ========================================================================= */
static bool stack_full(const BZY_Stack *s)  { return s->top >= BZY_STACK_MAX - 1; }
static bool stack_empty(const BZY_Stack *s) { return s->top < 0; }

bool mpda_push(MPDA *m, MaslowTier tier, TrinaryState result,
               TemporalFrame frame, float theta)
{
    if (!m || stack_full(&m->stack)) return false;
    m->stack.top++;
    m->stack.entries[m->stack.top] = (BZY_StackEntry){
        .tier   = tier,
        .result = result,
        .frame  = frame,
        .theta  = theta
    };
    return true;
}

bool mpda_pop(MPDA *m, BZY_StackEntry *out)
{
    if (!m || stack_empty(&m->stack)) return false;
    if (out) *out = m->stack.entries[m->stack.top];
    m->stack.top--;
    return true;
}

bool mpda_peek(const MPDA *m, BZY_StackEntry *out)
{
    if (!m || stack_empty(&m->stack)) return false;
    if (out) *out = m->stack.entries[m->stack.top];
    return true;
}

/* =========================================================================
 * THETA → TEMPORAL FRAME MAPPING
 * ========================================================================= */
TemporalFrame mpda_theta_to_frame(float theta)
{
    theta = fmodf(theta, 360.0f);
    if (theta < 120.0f)  return TEMPORAL_THERE_AND_THEN;
    if (theta < 240.0f)  return TEMPORAL_HERE_AND_NOW;
    return TEMPORAL_WHEN_AND_WHERE;
}

/* =========================================================================
 * DEFAULT TRANSITION TABLE — constitutional transitions
 * Maps (from_state, input_symbol, θ_range) → (to_state, θ', enzyme)
 * ========================================================================= */
static const BZY_Transition DEFAULT_TRANSITIONS[] = {
    /* Pre-boot receives YES → start calibrating at θ=120 (present) */
    { BZY_STATE_PREBOOT,     TRINARY_YES,       0.0f,   BZY_STATE_CALIBRATING, 120.0f, ENZYME_CREATE  },
    /* Pre-boot receives MAYBE → start calibrating with repair enzyme */
    { BZY_STATE_PREBOOT,     TRINARY_MAYBE,     0.0f,   BZY_STATE_CALIBRATING, 120.0f, ENZYME_REPAIR  },
    /* Pre-boot receives NO → immediately violated */
    { BZY_STATE_PREBOOT,     TRINARY_NO,        0.0f,   BZY_STATE_VIOLATED,    0.0f,   ENZYME_DESTROY },

    /* Calibrating + YES → continue, advance θ by 120 */
    { BZY_STATE_CALIBRATING, TRINARY_YES,       120.0f, BZY_STATE_CALIBRATING, 240.0f, ENZYME_BUILD   },
    { BZY_STATE_CALIBRATING, TRINARY_YES,       240.0f, BZY_STATE_ACCEPTED,    360.0f, ENZYME_RENEW   },
    /* Calibrating + MAYBE → go pending, push onto stack */
    { BZY_STATE_CALIBRATING, TRINARY_MAYBE,     120.0f, BZY_STATE_PENDING,     120.0f, ENZYME_CREATE  },
    /* Calibrating + NO → violated */
    { BZY_STATE_CALIBRATING, TRINARY_NO,        120.0f, BZY_STATE_VIOLATED,    0.0f,   ENZYME_BREAK   },

    /* Pending + YES → resume calibration, pop stack */
    { BZY_STATE_PENDING,     TRINARY_YES,       120.0f, BZY_STATE_CALIBRATING, 240.0f, ENZYME_REPAIR  },
    /* Pending + MAYBE → stay pending (stack grows) */
    { BZY_STATE_PENDING,     TRINARY_MAYBE,     120.0f, BZY_STATE_PENDING,     120.0f, ENZYME_RENEW   },
    /* Pending + NO → escalate */
    { BZY_STATE_PENDING,     TRINARY_NO,        120.0f, BZY_STATE_ESCALATED,   0.0f,   ENZYME_BREAK   },

    /* Violated can be repaired by YES from governance layer */
    { BZY_STATE_VIOLATED,    TRINARY_YES,       0.0f,   BZY_STATE_CALIBRATING, 120.0f, ENZYME_REPAIR  },
    { BZY_STATE_VIOLATED,    TRINARY_MAYBE,     0.0f,   BZY_STATE_ESCALATED,   0.0f,   ENZYME_CREATE  },

    /* Escalated can only be reset by YES (constitutional mandate) */
    { BZY_STATE_ESCALATED,   TRINARY_YES,       0.0f,   BZY_STATE_PREBOOT,     0.0f,   ENZYME_RENEW   },
};
#define N_DEFAULT_TRANSITIONS ((int)(sizeof(DEFAULT_TRANSITIONS)/sizeof(DEFAULT_TRANSITIONS[0])))

/* =========================================================================
 * MPDA INIT
 * ========================================================================= */
void mpda_init(MPDA *m)
{
    if (!m) return;
    memset(m, 0, sizeof(MPDA));
    m->current_state       = BZY_STATE_PREBOOT;
    m->stack.top           = -1;
    m->theta               = 0.0f;
    m->q0                  = BZY_STATE_PREBOOT;
    m->stack_empty_accept  = true;
    m->discriminant_floor  = 0.0;

    /* Copy default transitions */
    int n = N_DEFAULT_TRANSITIONS < BZY_MAX_TRANSITIONS
            ? N_DEFAULT_TRANSITIONS : BZY_MAX_TRANSITIONS;
    memcpy(m->transitions, DEFAULT_TRANSITIONS, sizeof(BZY_Transition) * n);
    m->n_transitions = n;
}

/* =========================================================================
 * FIRE A MAGNETIC TRANSITION
 * Finds the best matching transition for (current_state, input, θ)
 * θ tolerance: within same 120° band counts as matching
 * ========================================================================= */
bool mpda_transition(MPDA *m, TrinaryState input)
{
    if (!m) return false;

    TemporalFrame current_frame = mpda_theta_to_frame(m->theta);

    for (int i = 0; i < m->n_transitions; i++) {
        const BZY_Transition *t = &m->transitions[i];

        if (t->from_state != m->current_state) continue;
        if (t->input_symbol != input)           continue;

        /* Check θ frame matches */
        TemporalFrame t_frame = mpda_theta_to_frame(t->theta_in);
        if (t_frame != current_frame) continue;

        /* Transition fires */
        m->current_state = t->to_state;
        m->theta         = t->theta_out;

        /* Push MAYBE states onto stack; pop YES resolutions */
        if (input == TRINARY_MAYBE) {
            mpda_push(m, MASLOW_TIER1_PHYSIOLOGICAL, TRINARY_MAYBE,
                      current_frame, m->theta);
        } else if (input == TRINARY_YES && !stack_empty(&m->stack)) {
            BZY_StackEntry top;
            if (mpda_peek(m, &top) && top.result == TRINARY_MAYBE) {
                mpda_pop(m, NULL);  /* resolve the pending need */
            }
        }

        return true;
    }

    /* No matching transition: implicit NO → violated */
    printf("[MPDA] No transition for state=%d input=%d θ=%.1f\n",
           m->current_state, input, m->theta);
    m->current_state = BZY_STATE_VIOLATED;
    return false;
}

/* =========================================================================
 * RUN FULL INPUT STRING THROUGH MPDA
 * ========================================================================= */
BZY_State mpda_run(MPDA *m, const TrinaryState *input_str, size_t len)
{
    if (!m || !input_str) return BZY_STATE_VIOLATED;

    for (size_t i = 0; i < len; i++) {
        mpda_transition(m, input_str[i]);
        if (m->current_state == BZY_STATE_VIOLATED ||
            m->current_state == BZY_STATE_ESCALATED)
            break;
    }

    return m->current_state;
}

/* =========================================================================
 * ACCEPTANCE CHECK
 * 1. Final state = ACCEPTED
 * 2. Stack empty (no unresolved needs)
 * 3. Discriminant Δ ≥ floor
 * ========================================================================= */
bool mpda_accepts(const MPDA *m, double discriminant)
{
    if (!m) return false;
    if (m->current_state != BZY_STATE_ACCEPTED) return false;
    if (m->stack_empty_accept && !stack_empty(&m->stack)) return false;
    if (discriminant < m->discriminant_floor) return false;
    return true;
}

/* =========================================================================
 * REVERSE STACK TRAVERSAL (LTCodec reversibility)
 * Re-reads past calibration records without popping
 * ========================================================================= */
bool mpda_reverse_read(MPDA *m, size_t steps, TrinaryState *out_buf)
{
    if (!m || !out_buf) return false;
    if (stack_empty(&m->stack)) return false;

    int read_from = m->stack.top;
    for (size_t i = 0; i < steps && read_from >= 0; i++, read_from--) {
        out_buf[i] = m->stack.entries[read_from].result;
    }
    return true;
}

/* =========================================================================
 * DEBUG PRINT
 * ========================================================================= */
void mpda_print_state(const MPDA *m)
{
    if (!m) return;
    static const char *state_names[] = {
        "PREBOOT", "CALIBRATING", "PENDING", "VIOLATED", "ESCALATED", "ACCEPTED"
    };
    printf("[MPDA] state=%s  θ=%.1f°  stack_depth=%d\n",
           state_names[m->current_state],
           m->theta,
           m->stack.top + 1);
}
