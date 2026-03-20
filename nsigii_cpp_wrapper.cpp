/**
 * nsigii_cpp_wrapper.cpp
 * C++ Wrapper — NSIGII Heartfull Firmware
 * OBINexus Computing | Nnamdi Michael Okpala | 20 March 2026
 *
 * Wraps the C firmware API in idiomatic C++ classes.
 * Enables C++ consumers (game engines, Qt UIs, etc.) to use the firmware
 * without raw pointer management.
 *
 * LTF pipeline:
 *   libnsigii_firmware.a → libnsigii_firmware_cpp.so → dotnet (P/Invoke)
 */

extern "C" {
#include "heartfull_firmware.h"
#include "bzy_mpda.h"
#include "tripartite_discriminant.h"
}

#include <stdexcept>
#include <string>
#include <vector>
#include <cmath>

namespace OBINexus::NSIGII {

// =============================================================================
// TRINARY ARITHMETIC — C++ operator overloads for trinary composition
// =============================================================================

class Trinary {
public:
    int value;  // 1, 0, -1, -2

    explicit Trinary(TrinaryState s) : value(static_cast<int>(s)) {}
    explicit Trinary(int v) : value(v) {}

    // RIFT composition: mirrors trinary_compose()
    Trinary operator*(const Trinary& rhs) const {
        if (value == -2 || rhs.value == -2) return Trinary(-2);  // MAYBE_NOT absorbs
        if (value ==  0 || rhs.value ==  0) return Trinary(0);   // NO absorbs
        if (value ==  1 && rhs.value ==  1) return Trinary(1);   // YES*YES = YES
        if (value == -1 && rhs.value == -1) return Trinary(1);   // MAYBE*MAYBE = YES (double neg)
        return Trinary(-1);                                        // otherwise MAYBE
    }

    Trinary apply_enzyme(EnzymeOp op) const {
        return Trinary(enzyme_apply(op, static_cast<TrinaryState>(value)));
    }

    std::string name() const {
        switch (value) {
            case  1: return "YES";
            case  0: return "NO";
            case -1: return "MAYBE";
            case -2: return "MAYBE_NOT";
            default: return "UNKNOWN";
        }
    }

    TrinaryState state() const { return static_cast<TrinaryState>(value); }
    bool is_yes()      const { return value ==  1; }
    bool is_no()       const { return value ==  0; }
    bool is_maybe()    const { return value == -1; }
    bool is_deferred() const { return value == -2; }
};

// =============================================================================
// PERSPECTIVE MEMBRANE — C++ RAII wrapper
// =============================================================================

class MembraneCalibrator {
    PerspectiveMembrane m_membrane{};
    bool                m_calibrated = false;

public:
    MembraneCalibrator() {
        membrane_init(&m_membrane);
    }

    MembraneOutcome calibrate(TrinaryState t1, TrinaryState t2) {
        MaslowNeedsState needs{};
        needs.tier[MASLOW_TIER1_PHYSIOLOGICAL] = t1;
        needs.tier[MASLOW_TIER2_SAFETY]        = t2;

        MembraneOutcome outcome = membrane_calibrate(&m_membrane, &needs);
        m_calibrated = true;
        return outcome;
    }

    MembraneOutcome outcome()     const { return m_membrane.outcome; }
    double          discriminant() const { return m_membrane.discriminant; }
    bool            calibrated()   const { return m_calibrated; }
    bool            track_b_open() const { return kanban_track_b_unlocked(&m_membrane); }

    const PerspectiveMembrane& raw() const { return m_membrane; }
};

// =============================================================================
// BYZANTINE DISCRIMINANT — C++ wrapper with operator<< support
// =============================================================================

struct DiscriminantResult {
    double             delta;
    DiscriminantRegion region;
    double             root_pos;
    double             root_neg;
    bool               has_real_roots;

    std::string region_name() const {
        switch (region) {
            case DISC_STABLE:   return "STABLE (delta>0)";
            case DISC_CRITICAL: return "CRITICAL (delta=0)";
            case DISC_FAULT:    return "FAULT (delta<0)";
            default:            return "UNKNOWN";
        }
    }
};

class ByzantineChecker {
public:
    static DiscriminantResult check(TrinaryState u, TrinaryState v, TrinaryState w) {
        TripartiteState tri = tripartite_build(u, v, w);
        double delta = tripartite_discriminant(&tri);

        DiscriminantResult res;
        res.delta         = delta;
        res.region        = tripartite_classify(delta);
        res.has_real_roots = tripartite_roots(&tri, &res.root_pos, &res.root_neg);
        if (!res.has_real_roots) {
            res.root_pos = 0.0;
            res.root_neg = 0.0;
        }
        return res;
    }

    // Check all eight combinations of YES/NO/MAYBE for G={U,V,W}
    static std::vector<DiscriminantResult> sweep() {
        std::vector<DiscriminantResult> results;
        TrinaryState states[] = { TRINARY_YES, TRINARY_NO, TRINARY_MAYBE };
        for (auto u : states)
            for (auto v : states)
                for (auto w : states)
                    results.push_back(check(u, v, w));
        return results;
    }
};

// =============================================================================
// MPDA WRAPPER — C++ RAII wrapper for the pushdown automaton
// =============================================================================

class MPDARunner {
    MPDA m_mpda{};

public:
    MPDARunner() {
        mpda_init(&m_mpda);
    }

    // Run a vector of trinary inputs through the automaton
    BZY_State run(const std::vector<TrinaryState>& inputs) {
        return mpda_run(&m_mpda,
                        inputs.data(),
                        inputs.size());
    }

    bool accepts(double discriminant) const {
        return mpda_accepts(&m_mpda, discriminant);
    }

    BZY_State state()  const { return m_mpda.current_state; }
    float     theta()  const { return m_mpda.theta; }
    int       depth()  const { return m_mpda.stack.top + 1; }

    void push(MaslowTier tier, TrinaryState result,
              TemporalFrame frame, float theta_val) {
        mpda_push(&m_mpda, tier, result, frame, theta_val);
    }

    // Read past calibration history (LTCodec reversibility)
    std::vector<TrinaryState> reverse_read(size_t steps) {
        std::vector<TrinaryState> buf(steps, TRINARY_MAYBE);
        mpda_reverse_read(&m_mpda, steps, buf.data());
        return buf;
    }

    void print() const { mpda_print_state(&m_mpda); }
};

// =============================================================================
// DRIFT THEOREM — C++ vector operations
// Drift theorem: D(t) = dV(t)/dt  where V(t) = P(t) - C(t)
// =============================================================================

struct TripolarVector {
    double alpha;  // WANT
    double beta;   // NEED
    double gamma;  // SHOULD

    double magnitude() const {
        return std::sqrt(alpha*alpha + beta*beta + gamma*gamma);
    }

    TripolarVector operator-(const TripolarVector& rhs) const {
        return { alpha - rhs.alpha, beta - rhs.beta, gamma - rhs.gamma };
    }

    double dot(const TripolarVector& rhs) const {
        return alpha*rhs.alpha + beta*rhs.beta + gamma*rhs.gamma;
    }

    // Angular drift: theta = acos(V1·V2 / |V1||V2|)
    double angular_drift(const TripolarVector& other) const {
        double denom = magnitude() * other.magnitude();
        if (denom < 1e-9) return 0.0;
        double cosine = dot(other) / denom;
        cosine = std::max(-1.0, std::min(1.0, cosine));
        return std::acos(cosine);
    }

    // Weighted observation: W(t) = (2/3)*P(t) + (1/3)*P(t-1)
    static TripolarVector weighted(const TripolarVector& curr,
                                   const TripolarVector& prev,
                                   double alpha_w = 2.0/3.0) {
        double b = 1.0 - alpha_w;
        return {
            alpha_w * curr.alpha + b * prev.alpha,
            alpha_w * curr.beta  + b * prev.beta,
            alpha_w * curr.gamma + b * prev.gamma
        };
    }
};

class DriftMonitor {
    TripolarVector m_prev{0,0,0};
    bool           m_has_prev = false;

public:
    struct DriftReport {
        double radial;    // Dr > 0 = diverging, < 0 = converging
        double angular;   // omega = d(theta)/dt
        enum { APPROACH, SEPARATION, ANGULAR_DRIFT, STABLE } state;
    };

    DriftReport update(const TripolarVector& curr) {
        DriftReport report{};
        if (!m_has_prev) {
            m_prev = curr;
            m_has_prev = true;
            return report;
        }

        report.radial  = curr.magnitude() - m_prev.magnitude();
        report.angular = m_prev.angular_drift(curr);

        if (std::abs(report.radial) < 0.01 && std::abs(report.angular) < 0.01)
            report.state = DriftReport::STABLE;
        else if (report.radial < 0)
            report.state = DriftReport::APPROACH;
        else if (report.angular > 0.01)
            report.state = DriftReport::ANGULAR_DRIFT;
        else
            report.state = DriftReport::SEPARATION;

        m_prev = curr;
        return report;
    }
};

}  // namespace OBINexus::NSIGII


// =============================================================================
// C-LINKAGE EXPORTS — for P/Invoke from C# compositor
// =============================================================================
extern "C" {

// Additional C++ convenience functions exposed for .NET
double nsigii_cpp_drift_radial(int a1, int b1, int g1,
                                int a2, int b2, int g2) {
    using namespace OBINexus::NSIGII;
    TripolarVector v1{(double)a1, (double)b1, (double)g1};
    TripolarVector v2{(double)a2, (double)b2, (double)g2};
    return v2.magnitude() - v1.magnitude();
}

double nsigii_cpp_discriminant(int u, int v, int w) {
    TripartiteState tri = tripartite_build(
        static_cast<TrinaryState>(u),
        static_cast<TrinaryState>(v),
        static_cast<TrinaryState>(w));
    return tripartite_discriminant(&tri);
}

int nsigii_cpp_compose(int a, int b) {
    using T = OBINexus::NSIGII::Trinary;
    return (T(a) * T(b)).value;
}

}  // extern "C"
