// Main.cs
// MMUKO-OS Compositor Entry Point
// OBINexus Computing | Nnamdi Michael Okpala
// Version: 0.1-DRAFT | 20 March 2026
//
// LTF (Linkable Then Executable) entry point.
// This file is the C# compositor that loads AFTER boot.asm issues MEMBRANE_PASS.
//
// Pipeline:
//   nasm -f bin boot.asm -o boot.bin
//   → QEMU loads boot.bin → NSIGII 6-phase calibration
//   → PASS → OS kernel → C firmware library → dotnet run (this file)
//
// Usage:
//   dotnet run -- --boot-passed true --tier1 maybe --tier2 maybe
//   dotnet run -- --simulate-pass   (for development testing)

using System;
using System.Collections.Generic;
using OBINexus.MMUKO.NSIGII;

namespace OBINexus.MMUKO
{
    internal static class Program
    {
        static int Main(string[] args)
        {
            Console.OutputEncoding = System.Text.Encoding.UTF8;

            Console.ForegroundColor = ConsoleColor.Cyan;
            Console.WriteLine("╔══════════════════════════════════════════════════════════╗");
            Console.WriteLine("║  MMUKO-OS  │  NSIGII Firmware Compositor  │  LTE v0.1  ║");
            Console.WriteLine("║  OBINexus Computing  │  Nnamdi Michael Okpala           ║");
            Console.WriteLine("╚══════════════════════════════════════════════════════════╝");
            Console.ResetColor();

            // ─── Parse arguments ────────────────────────────────────────────
            var parsed = ParseArgs(args);

            bool   simulatePass = parsed.GetValueOrDefault("simulate-pass", "false") == "true";
            bool   bootPassed   = simulatePass ||
                                  parsed.GetValueOrDefault("boot-passed", "false") == "true";

            TrinaryState tier1  = ParseTrinary(parsed.GetValueOrDefault("tier1",  "maybe"));
            TrinaryState tier2  = ParseTrinary(parsed.GetValueOrDefault("tier2",  "maybe"));
            TrinaryState wActor = ParseTrinary(parsed.GetValueOrDefault("w-actor","maybe"));

            if (simulatePass)
            {
                Console.ForegroundColor = ConsoleColor.Yellow;
                Console.WriteLine("\n[DEV MODE] --simulate-pass: bypassing hardware boot gate.");
                Console.ResetColor();
            }

            // ─── Boot gate ──────────────────────────────────────────────────
            Console.WriteLine($"\n[COMPOSITOR] Boot-passed = {bootPassed}");
            Console.WriteLine($"[COMPOSITOR] T1={tier1}  T2={tier2}  W={wActor}");

            using var firmware = HeartfullFirmware.Create(bootPassed);

            if (firmware == null)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.Error.WriteLine("\n[COMPOSITOR] BLOCKED — boot gate not passed.");
                Console.Error.WriteLine("  Run: qemu-system-x86_64 -drive format=raw,file=img/mmuko-os.img");
                Console.Error.WriteLine("  Then re-launch compositor with: --boot-passed true");
                Console.ResetColor();
                return 1;
            }

            // ─── Run NSIGII six-phase calibration ───────────────────────────
            var outcome = firmware.RunNSIGII(tier1, tier2, wActor);

            Console.WriteLine($"\n[COMPOSITOR] Membrane outcome: {outcome}");

            // ─── Demonstrate trinary logic ───────────────────────────────────
            DemoTrinaryLogic();

            // ─── Demonstrate enzyme degradation ─────────────────────────────
            DemoEnzymeDegradation();

            // ─── Demonstrate Byzantine discriminant ─────────────────────────
            DemoByzantineDiscriminant(tier1, tier2, wActor);

            // ─── Load Kanban tasks ───────────────────────────────────────────
            if (outcome == MembraneOutcome.Pass)
            {
                PopulateKanban(firmware);
            }
            else
            {
                Console.ForegroundColor = ConsoleColor.Yellow;
                Console.WriteLine($"\n[KANBAN] Track B LOCKED — outcome is {outcome}.");
                Console.WriteLine("         Maslow T1/T2 must resolve before aspiration tasks load.");
                Console.ResetColor();
            }

            // ─── Render HeartFelt UI ─────────────────────────────────────────
            if (outcome == MembraneOutcome.Pass)
            {
                try
                {
                    var ui = new HeartFeltFirmware(firmware);
                    ui.RenderBoard();
                }
                catch (Exception ex)
                {
                    Console.Error.WriteLine($"[UI] HeartFelt render error: {ex.Message}");
                }
            }

            // ─── Drift theorem demo ──────────────────────────────────────────
            DemoDriftTheorem();

            Console.WriteLine("\n[COMPOSITOR] Execution complete.");
            return outcome == MembraneOutcome.Pass ? 0 : 1;
        }

        // =====================================================================
        // TRINARY LOGIC DEMONSTRATION
        // =====================================================================
        static void DemoTrinaryLogic()
        {
            Console.ForegroundColor = ConsoleColor.Magenta;
            Console.WriteLine("\n─── RIFT Trinary Composition ───────────────────────────────");
            Console.ResetColor();

            var cases = new[]
            {
                (TrinaryState.Yes,      TrinaryState.Yes,      "YES ⊗ YES"),
                (TrinaryState.No,       TrinaryState.Yes,      "NO  ⊗ YES"),
                (TrinaryState.Maybe,    TrinaryState.Maybe,    "MAYBE ⊗ MAYBE (double neg → YES)"),
                (TrinaryState.Maybe,    TrinaryState.Yes,      "MAYBE ⊗ YES   (uncertainty persists)"),
                (TrinaryState.MaybeNot, TrinaryState.Yes,      "MAYBE_NOT ⊗ YES (defer wins)"),
            };

            foreach (var (a, b, label) in cases)
            {
                var result = HeartfullFirmware.TrinaryCompose(a, b);
                Console.WriteLine($"  {label,-40} = {result}");
            }
        }

        // =====================================================================
        // ENZYME DEGRADATION CHAIN DEMONSTRATION
        // MAYBE → CREATE/DESTROY a state
        // MAYBE → BUILD/BREAK → RENEW/REPAIR
        // =====================================================================
        static void DemoEnzymeDegradation()
        {
            Console.ForegroundColor = ConsoleColor.Magenta;
            Console.WriteLine("\n─── Enzyme MAYBE-Degradation Pathway ──────────────────────");
            Console.ResetColor();

            TrinaryState s = TrinaryState.Maybe;
            Console.WriteLine($"  Initial: {s}");

            var chain = new[] {
                EnzymeOp.Create, EnzymeOp.Build, EnzymeOp.Renew,
                EnzymeOp.Break,  EnzymeOp.Repair
            };
            foreach (var op in chain)
            {
                var next = HeartfullFirmware.ApplyEnzyme(op, s);
                Console.WriteLine($"  {op,-10} : {s} → {next}");
                s = next;
            }
        }

        // =====================================================================
        // BYZANTINE DISCRIMINANT DEMONSTRATION
        // =====================================================================
        static void DemoByzantineDiscriminant(TrinaryState u, TrinaryState v, TrinaryState w)
        {
            Console.ForegroundColor = ConsoleColor.Magenta;
            Console.WriteLine("\n─── Byzantine Discriminant  G = {U, V, W} ──────────────────");
            Console.ResetColor();

            var examples = new[]
            {
                (TrinaryState.Yes,   TrinaryState.Yes,   TrinaryState.Yes,   "All YES (stable)"),
                (TrinaryState.Maybe, TrinaryState.Maybe, TrinaryState.Maybe, "All MAYBE (check)"),
                (TrinaryState.No,    TrinaryState.No,    TrinaryState.No,    "All NO   (fault)"),
                (u, v, w, $"Runtime: U={u} V={v} W={w}"),
            };

            foreach (var (eu, ev, ew, label) in examples)
            {
                double b     = (double)eu + (double)ev + (double)ew;
                double delta = b * b - 4.0;
                string region = delta > 0 ? "STABLE" : delta == 0 ? "CRITICAL" : "FAULT";
                Console.WriteLine($"  {label,-40} Δ={delta,6:F1}  {region}");
            }
        }

        // =====================================================================
        // KANBAN POPULATION
        // =====================================================================
        static void PopulateKanban(HeartfullFirmware fw)
        {
            // Track A — Foundation tasks
            var t1 = fw.AddTask("Physiological needs verified",
                                KanbanTrack.FoundationA, TrinaryState.Yes);
            t1.Column = KanbanColumn.Done;

            var t2 = fw.AddTask("Safety scan completed",
                                KanbanTrack.FoundationA, TrinaryState.Yes);
            t2.Column = KanbanColumn.Done;

            var t3 = fw.AddTask("NSIGII 6-phase boot",
                                KanbanTrack.FoundationA, TrinaryState.Maybe);
            t3.Column = KanbanColumn.InProgress;

            // Track B — Aspiration tasks (only available after PASS)
            var b1 = fw.AddTask("Build OBINexus platform",
                                KanbanTrack.AspirationB, TrinaryState.Maybe);
            b1.Column = KanbanColumn.Backlog;

            var b2 = fw.AddTask("Deploy CORN governance module",
                                KanbanTrack.AspirationB, TrinaryState.MaybeNot);
            b2.Column = KanbanColumn.Backlog;

            // Track W — Adversarial monitoring
            var w1 = fw.AddTask("W-actor discriminant monitor",
                                KanbanTrack.AdversarialW, TrinaryState.Maybe);
            w1.Column = KanbanColumn.InProgress;
        }

        // =====================================================================
        // DRIFT THEOREM DEMONSTRATION
        // =====================================================================
        static void DemoDriftTheorem()
        {
            Console.ForegroundColor = ConsoleColor.Magenta;
            Console.WriteLine("\n─── Drift Theorem (Tripolar Radial Drift) ──────────────────");
            Console.ResetColor();

            var vPrev = (alpha: TrinaryState.Maybe, beta: TrinaryState.Maybe, gamma: TrinaryState.Maybe);
            var vCurr = (alpha: TrinaryState.Yes,   beta: TrinaryState.Maybe, gamma: TrinaryState.Maybe);

            double dr = HeartfullFirmware.DriftRadial(vPrev, vCurr);
            Console.WriteLine($"  V_prev = ({(int)vPrev.alpha}, {(int)vPrev.beta}, {(int)vPrev.gamma})");
            Console.WriteLine($"  V_curr = ({(int)vCurr.alpha}, {(int)vCurr.beta}, {(int)vCurr.gamma})");
            Console.WriteLine($"  D_r    = {dr:F4}  {(dr > 0 ? "(diverging — needs resolving)" : "(converging — approaching resolution)")}");
        }

        // =====================================================================
        // ARGUMENT PARSING
        // =====================================================================
        static Dictionary<string, string> ParseArgs(string[] args)
        {
            var dict = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            for (int i = 0; i < args.Length; i++)
            {
                string arg = args[i].TrimStart('-');
                if (i + 1 < args.Length && !args[i + 1].StartsWith('-'))
                {
                    dict[arg] = args[++i];
                }
                else
                {
                    dict[arg] = "true";
                }
            }
            return dict;
        }

        static TrinaryState ParseTrinary(string s) => s.ToLower() switch {
            "yes" or "1"    => TrinaryState.Yes,
            "no"  or "0"    => TrinaryState.No,
            "maybenot"
            or "-2"         => TrinaryState.MaybeNot,
            _               => TrinaryState.Maybe
        };
    }
}
