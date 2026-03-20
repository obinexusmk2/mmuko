// NSIGII_HeartfullFirmware.cs
// NSIGII Heartfull Firmware — C# Compositor Layer
// OBINexus Computing | Nnamdi Michael Okpala
// Version: 0.1-DRAFT | 20 March 2026
//
// This compositor is a LINKABLE THEN EXECUTABLE (LTE) component.
// It is NOT loaded until the assembly boot sector (boot.asm) issues MEMBRANE_PASS.
// P/Invoke bridges to the C firmware library: libnsigii_firmware.so/.dll
//
// LTF Pipeline:
//   boot.asm (NASM) → PASS → libnsigii_firmware.so → NSIGII_HeartfullFirmware.cs
//   → .NET runtime → KanbanCompositor → UI (Track A / Track B)

using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.Threading;

namespace OBINexus.MMUKO.NSIGII
{
    // =========================================================================
    // TRINARY STATE — mirrors C enum TrinaryState
    // =========================================================================
    public enum TrinaryState : int
    {
        Yes      =  1,   // needs met, contract honoured
        No       =  0,   // needs violated, contract breached
        Maybe    = -1,   // needs uncertain, response delayed
        MaybeNot = -2    // defer — do NOT handle (system absorbs)
    }

    // =========================================================================
    // MEMBRANE OUTCOME — mirrors C enum MembraneOutcome
    // =========================================================================
    public enum MembraneOutcome : int
    {
        Pass  = 0,
        Hold  = 1,
        Alert = 2
    }

    // =========================================================================
    // ENZYME OPERATION — mirrors C enum EnzymeOp
    // =========================================================================
    public enum EnzymeOp : int
    {
        Create  = 0,
        Destroy = 1,
        Build   = 2,
        Break   = 3,
        Renew   = 4,
        Repair  = 5
    }

    // =========================================================================
    // KANBAN TASK — three-track system
    // =========================================================================
    public enum KanbanTrack { FoundationA, AspirationB, AdversarialW }
    public enum KanbanColumn { Backlog, InProgress, Done, Blocked }

    public class KanbanTask
    {
        public string         Id          { get; init; } = Guid.NewGuid().ToString("N")[..8];
        public string         Title       { get; set; }  = string.Empty;
        public KanbanTrack    Track       { get; set; }
        public KanbanColumn   Column      { get; set; }  = KanbanColumn.Backlog;
        public TrinaryState   State       { get; set; }  = TrinaryState.Maybe;
        public EnzymeOp?      PendingOp   { get; set; }  = null;
        public DateTimeOffset Created     { get; init; } = DateTimeOffset.UtcNow;
    }

    // =========================================================================
    // P/INVOKE BRIDGE — native C firmware library
    // Library name resolves platform-specifically at runtime
    // =========================================================================
    internal static class FirmwareNative
    {
        private const string LIB = "nsigii_firmware";

        [DllImport(LIB, CallingConvention = CallingConvention.Cdecl)]
        public static extern void membrane_init(IntPtr membrane);

        [DllImport(LIB, CallingConvention = CallingConvention.Cdecl)]
        public static extern int membrane_calibrate(IntPtr membrane, IntPtr needs);

        [DllImport(LIB, CallingConvention = CallingConvention.Cdecl)]
        public static extern int trinary_compose(int a, int b);

        [DllImport(LIB, CallingConvention = CallingConvention.Cdecl)]
        public static extern int enzyme_apply(int op, int current);

        [DllImport(LIB, CallingConvention = CallingConvention.Cdecl)]
        public static extern double tripartite_discriminant(IntPtr tri);

        [DllImport(LIB, CallingConvention = CallingConvention.Cdecl)]
        public static extern int tripartite_check(int u, int v, int w);

        [DllImport(LIB, CallingConvention = CallingConvention.Cdecl)]
        public static extern void mpda_init(IntPtr mpda);

        [DllImport(LIB, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool mpda_accepts(IntPtr mpda, double discriminant);
    }

    // =========================================================================
    // HEARTFULL FIRMWARE — main compositor class
    // =========================================================================
    public sealed class HeartfullFirmware : IDisposable
    {
        private static readonly object _bootLock = new();
        private static bool            _bootPassed = false;

        private readonly List<KanbanTask> _tasks = new();
        private TrinaryState _tier1 = TrinaryState.Maybe;
        private TrinaryState _tier2 = TrinaryState.Maybe;
        private TrinaryState _wActor = TrinaryState.Maybe;
        private MembraneOutcome _outcome = MembraneOutcome.Hold;
        private double _discriminant = 0.0;
        private bool _disposed;

        // Six-phase NSIGII boot phase tracker
        private int _currentPhase = 0;
        private static readonly string[] PhaseNames =
        {
            "N — Need-state initialisation",
            "S — Safety scan",
            "I — Identity calibration",
            "G — Governance layer check",
            "I — Internal probe P_I",
            "I — Integrity verification"
        };

        // =====================================================================
        // BOOT GATE — cannot construct compositor until assembly issues PASS
        // In production: reads from shared memory / UEFI variable written by boot.asm
        // =====================================================================
        public static HeartfullFirmware? Create(bool bootPassedFromAssembly)
        {
            lock (_bootLock)
            {
                if (!bootPassedFromAssembly)
                {
                    Console.Error.WriteLine("[COMPOSITOR] Boot gate BLOCKED — membrane has not issued PASS.");
                    Console.Error.WriteLine("             Run boot.asm via QEMU first.");
                    return null;
                }
                _bootPassed = true;
                return new HeartfullFirmware();
            }
        }

        private HeartfullFirmware() { }

        // =====================================================================
        // SIX-PHASE NSIGII RUN (software-level, mirroring C firmware phases)
        // =====================================================================
        public MembraneOutcome RunNSIGII(
            TrinaryState tier1,
            TrinaryState tier2,
            TrinaryState wActor = TrinaryState.Maybe)
        {
            _tier1  = tier1;
            _tier2  = tier2;
            _wActor = wActor;

            Console.WriteLine("\n=== NSIGII Heartfull Firmware — Compositor Boot ===");

            for (_currentPhase = 0; _currentPhase < 6; _currentPhase++)
            {
                Console.WriteLine($"[{_currentPhase + 1}] {PhaseNames[_currentPhase]}");
                bool ok = RunPhase(_currentPhase);
                if (!ok)
                {
                    Console.WriteLine($"    ↳ Phase {_currentPhase + 1} HOLD — issuing HOLD outcome");
                    _outcome = MembraneOutcome.Hold;
                    return _outcome;
                }
                Console.WriteLine($"    ↳ OK");
            }

            // Final membrane gate
            _outcome = EvaluateMembrane();
            Console.WriteLine($"\n[MEMBRANE] Outcome = {_outcome}");
            return _outcome;
        }

        private bool RunPhase(int phase) => phase switch
        {
            0 => PhaseN_NeedInit(),
            1 => PhaseS_SafetyScan(),
            2 => PhaseI_IdentityCalibration(),
            3 => PhaseG_GovernanceCheck(),
            4 => PhaseI2_InternalProbe(),
            5 => PhaseI3_IntegrityVerification(),
            _ => false
        };

        private bool PhaseN_NeedInit()
        {
            // Need-state initialisation: accept MAYBE (pending is OK)
            return _tier1 != TrinaryState.No;
        }

        private bool PhaseS_SafetyScan()
        {
            return _tier2 != TrinaryState.No;
        }

        private bool PhaseI_IdentityCalibration()
        {
            // Identity: apply RENEW enzyme to α (want pointer)
            _tier1 = ApplyEnzyme(EnzymeOp.Renew, _tier1);
            return true;
        }

        private bool PhaseG_GovernanceCheck()
        {
            // Governance: apply BUILD enzyme to β (need pointer)
            _tier2 = ApplyEnzyme(EnzymeOp.Build, _tier2);
            return true;
        }

        private bool PhaseI2_InternalProbe()
        {
            // Compose α⊗β⊗γ via RIFT trinary logic
            var result = TrinaryCompose(
                            TrinaryCompose(_tier1, _tier2),
                            TrinaryState.Maybe);  // γ = SHOULD starts as MAYBE
            return result != TrinaryState.No;
        }

        private bool PhaseI3_IntegrityVerification()
        {
            // Discriminant: Δ = b² - 4ac where b = U+V+W
            double b = (double)_tier1 + (double)_tier2 + (double)_wActor;
            _discriminant = (b * b) - 4.0;  // a=1, c=1
            Console.WriteLine($"    Δ = {b}² - 4 = {_discriminant:F4}");
            return _discriminant >= 0.0;
        }

        private MembraneOutcome EvaluateMembrane()
        {
            // T1 violated → ALERT
            if (_tier1 == TrinaryState.No || _discriminant < 0.0)
                return MembraneOutcome.Alert;

            // T1 still pending → HOLD
            if (_tier1 == TrinaryState.Maybe)
                return MembraneOutcome.Hold;

            return MembraneOutcome.Pass;
        }

        // =====================================================================
        // RIFT TRINARY COMPOSITION (pure C# — mirrors trinary_compose.c)
        // =====================================================================
        public static TrinaryState TrinaryCompose(TrinaryState a, TrinaryState b)
        {
            if (a == TrinaryState.MaybeNot || b == TrinaryState.MaybeNot)
                return TrinaryState.MaybeNot;
            if (a == TrinaryState.No || b == TrinaryState.No)
                return TrinaryState.No;
            if (a == TrinaryState.Yes && b == TrinaryState.Yes)
                return TrinaryState.Yes;
            if (a == TrinaryState.Maybe && b == TrinaryState.Maybe)
                return TrinaryState.Yes;   // double negation resolves
            return TrinaryState.Maybe;
        }

        // =====================================================================
        // ENZYME APPLICATION
        // =====================================================================
        public static TrinaryState ApplyEnzyme(EnzymeOp op, TrinaryState current)
            => op switch
        {
            EnzymeOp.Create  => current == TrinaryState.Maybe ? TrinaryState.Yes  : current,
            EnzymeOp.Destroy => current == TrinaryState.Maybe ? TrinaryState.No   : current,
            EnzymeOp.Build   => current == TrinaryState.Maybe ? TrinaryState.Yes  :
                                current == TrinaryState.No    ? TrinaryState.Maybe : current,
            EnzymeOp.Break   => current == TrinaryState.Yes   ? TrinaryState.Maybe :
                                current == TrinaryState.Maybe  ? TrinaryState.No   : current,
            EnzymeOp.Renew   => current == TrinaryState.Yes   ? TrinaryState.Yes  : TrinaryState.Maybe,
            EnzymeOp.Repair  => current is TrinaryState.MaybeNot or TrinaryState.No
                                    ? TrinaryState.Maybe : current,
            _                => current
        };

        // =====================================================================
        // KANBAN THREE-TRACK INTERFACE
        // Track A (Foundation): T1+T2 — always active
        // Track B (Aspiration): T3–T5 — locked until PASS
        // Track W (Adversarial): W-actor monitoring — always active
        // =====================================================================
        public KanbanTask AddTask(string title, KanbanTrack track, TrinaryState state = TrinaryState.Maybe)
        {
            if (track == KanbanTrack.AspirationB && _outcome != MembraneOutcome.Pass)
                throw new InvalidOperationException(
                    "Track B (Aspiration) is LOCKED — membrane has not issued PASS for T1/T2.");

            var task = new KanbanTask { Title = title, Track = track, State = state };
            _tasks.Add(task);
            return task;
        }

        public IReadOnlyList<KanbanTask> GetTasks(KanbanTrack track) =>
            _tasks.FindAll(t => t.Track == track).AsReadOnly();

        // =====================================================================
        // BYZANTINE MAYBE STATE HANDLING
        // MAYBE  → enzymatic degradation pathway (create/destroy thread action)
        // MAYBE_NOT → deferred entirely (system absorbs, not operator)
        // =====================================================================
        public (TrinaryState result, EnzymeOp? appliedOp)
            ProcessMaybeState(TrinaryState state, EnzymeOp preferredOp)
        {
            if (state == TrinaryState.MaybeNot)
            {
                // MAYBE_NOT = -2: system defers — do not handle for operator
                Console.WriteLine("[MAYBE_NOT] Deferred to system — operator NOT burdened.");
                return (TrinaryState.MaybeNot, null);
            }

            if (state == TrinaryState.Maybe)
            {
                // MAYBE = -1: apply enzymatic action
                var result = ApplyEnzyme(preferredOp, state);
                Console.WriteLine($"[MAYBE] Enzyme {preferredOp} applied: {state} → {result}");
                return (result, preferredOp);
            }

            return (state, null);
        }

        // =====================================================================
        // DRIFT THEOREM — compute radial drift between two trinary scan vectors
        // Dr = ||V_t|| - ||V_{t-1}||
        // =====================================================================
        public static double DriftRadial(
            (TrinaryState alpha, TrinaryState beta, TrinaryState gamma) vPrev,
            (TrinaryState alpha, TrinaryState beta, TrinaryState gamma) vCurr)
        {
            static double Mag(int a, int b, int c) =>
                Math.Sqrt((double)(a*a + b*b + c*c));

            double magPrev = Mag((int)vPrev.alpha, (int)vPrev.beta, (int)vPrev.gamma);
            double magCurr = Mag((int)vCurr.alpha, (int)vCurr.beta, (int)vCurr.gamma);

            return magCurr - magPrev;  // positive = diverging, negative = converging
        }

        // Properties
        public MembraneOutcome Outcome      => _outcome;
        public double          Discriminant => _discriminant;
        public bool            TrackBOpen   => _outcome == MembraneOutcome.Pass;

        public void Dispose()
        {
            if (!_disposed) { _disposed = true; }
        }
    }
}
