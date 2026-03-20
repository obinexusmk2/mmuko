// NSIGII_HeartFeltFirmware.cs
// HeartFelt — UI Rendering Layer for Kanban Compositor
// OBINexus Computing | Nnamdi Michael Okpala
// Version: 0.1-DRAFT | 20 March 2026
//
// HeartFelt is the "felt experience" surface of the Heartfull firmware.
// It renders the Maslow–Kanban three-track interface to the console/terminal.
//
// This class is ONLY instantiated after HeartfullFirmware.Outcome == PASS.
// The UI is written only in C# — loaded, never run until booted.
// Assembly must load via C and C# interpolation language (P/Invoke extension).

using System;
using System.Collections.Generic;
using System.Text;
using OBINexus.MMUKO.NSIGII;

namespace OBINexus.MMUKO.NSIGII
{
    // =========================================================================
    // HEARTFELT — UI compositor for the Kanban three-track interface
    // =========================================================================
    public sealed class HeartFeltFirmware
    {
        private readonly HeartfullFirmware _firmware;

        // Track B lock enforced at construction
        public HeartFeltFirmware(HeartfullFirmware firmware)
        {
            _firmware = firmware ?? throw new ArgumentNullException(nameof(firmware));

            if (firmware.Outcome == MembraneOutcome.Hold ||
                firmware.Outcome == MembraneOutcome.Alert)
            {
                throw new InvalidOperationException(
                    $"[HeartFelt] Cannot render UI — membrane is {firmware.Outcome}.\n" +
                    "            Maslow T1/T2 must be resolved before the interface loads.");
            }
        }

        // =====================================================================
        // RENDER KANBAN BOARD — three-track display
        // =====================================================================
        public void RenderBoard()
        {
            Console.Clear();
            RenderHeader();
            RenderMaslowStatus();
            RenderTrack(KanbanTrack.FoundationA, "Track A — Foundation (Maslow T1+T2)");
            RenderTrack(KanbanTrack.AspirationB, "Track B — Aspiration  (Maslow T3–T5)");
            RenderTrack(KanbanTrack.AdversarialW,"Track W — Adversarial (G={U,V,W} monitor)");
            RenderDiscriminant();
            RenderFooter();
        }

        private void RenderHeader()
        {
            Console.ForegroundColor = ConsoleColor.Cyan;
            Console.WriteLine("╔══════════════════════════════════════════════════════════════╗");
            Console.WriteLine("║   MMUKO-OS  │  NSIGII HeartFelt Compositor  │  v0.1-DRAFT   ║");
            Console.WriteLine("║   OBINexus Computing  │  Nnamdi Michael Okpala              ║");
            Console.WriteLine("╚══════════════════════════════════════════════════════════════╝");
            Console.ResetColor();
        }

        private void RenderMaslowStatus()
        {
            Console.WriteLine();
            Console.ForegroundColor = ConsoleColor.Green;
            Console.WriteLine("  ┌─ MEMBRANE STATUS ─────────────────────────────────────────┐");
            Console.WriteLine($"  │  Outcome   : {_firmware.Outcome,-10} {OutcomeIcon(_firmware.Outcome)}");
            Console.WriteLine($"  │  Δ (disc.) : {_firmware.Discriminant:F4}");
            Console.WriteLine($"  │  Track B   : {(_firmware.TrackBOpen ? "UNLOCKED ✓" : "LOCKED ✗")}");
            Console.WriteLine("  └────────────────────────────────────────────────────────────┘");
            Console.ResetColor();
        }

        private void RenderTrack(KanbanTrack track, string label)
        {
            bool locked = (track == KanbanTrack.AspirationB && !_firmware.TrackBOpen);
            var tasks = _firmware.GetTasks(track);

            Console.WriteLine();
            Console.ForegroundColor = TrackColor(track);
            Console.WriteLine($"  ╔═ {label} {(locked ? "[LOCKED]" : "")}");
            Console.ResetColor();

            if (locked)
            {
                Console.ForegroundColor = ConsoleColor.DarkGray;
                Console.WriteLine("  ║  ⛔ Locked — resolve Maslow T1/T2 to unlock");
                Console.ResetColor();
                Console.WriteLine("  ╚══════════════════════════════════════════════════");
                return;
            }

            // Columns
            var byColumn = new Dictionary<KanbanColumn, List<KanbanTask>>();
            foreach (KanbanColumn col in Enum.GetValues<KanbanColumn>())
                byColumn[col] = new();
            foreach (var t in tasks)
                byColumn[t.Column].Add(t);

            foreach (KanbanColumn col in Enum.GetValues<KanbanColumn>())
            {
                Console.Write($"  ║  [{col}] ");
                if (byColumn[col].Count == 0)
                {
                    Console.ForegroundColor = ConsoleColor.DarkGray;
                    Console.Write("(empty)");
                    Console.ResetColor();
                }
                else
                {
                    foreach (var t in byColumn[col])
                    {
                        Console.ForegroundColor = TrinaryColor(t.State);
                        Console.Write($"  ● {t.Title} [{TrinarySymbol(t.State)}]  ");
                        Console.ResetColor();
                    }
                }
                Console.WriteLine();
            }
            Console.WriteLine("  ╚══════════════════════════════════════════════════");
        }

        private void RenderDiscriminant()
        {
            Console.WriteLine();
            Console.ForegroundColor = ConsoleColor.Yellow;
            Console.WriteLine("  ┌─ BYZANTINE DISCRIMINANT  G = {U, V, W} ──────────────────┐");

            double delta = _firmware.Discriminant;
            string region = delta > 0 ? "STABLE (Δ>0) — W benign or absent"
                          : delta == 0 ? "CRITICAL (Δ=0) — W present, pressure"
                          :              "FAULT (Δ<0) — W disrupting U–V relationship";

            Console.WriteLine($"  │  Δ = {delta:F4}");
            Console.WriteLine($"  │  Region: {region}");
            Console.WriteLine("  │");
            Console.WriteLine("  │  Trinary consensus map:");
            Console.WriteLine("  │    YES  ( 1) → all three actors agree");
            Console.WriteLine("  │    NO   ( 0) → V or W actively failing");
            Console.WriteLine("  │    MAYBE(-1) → W introducing noise / V decoherent");
            Console.WriteLine("  │    MAYBE_NOT(-2) → deferred to system");
            Console.WriteLine("  └────────────────────────────────────────────────────────────┘");
            Console.ResetColor();
        }

        private void RenderFooter()
        {
            Console.WriteLine();
            Console.ForegroundColor = ConsoleColor.DarkCyan;
            Console.WriteLine("  OBINexus Computing — Neurodivergent-First Constitutional Infrastructure");
            Console.WriteLine("  Igbo Ontological Framework: Uche/Obi/Eze · OHA/IWU/IJI");
            Console.ResetColor();
        }

        // Helpers
        private static string OutcomeIcon(MembraneOutcome o) => o switch {
            MembraneOutcome.Pass  => "✅",
            MembraneOutcome.Hold  => "⏸️",
            MembraneOutcome.Alert => "🚨",
            _                    => "?"
        };

        private static ConsoleColor TrackColor(KanbanTrack t) => t switch {
            KanbanTrack.FoundationA  => ConsoleColor.Green,
            KanbanTrack.AspirationB  => ConsoleColor.Blue,
            KanbanTrack.AdversarialW => ConsoleColor.Red,
            _                        => ConsoleColor.White
        };

        private static ConsoleColor TrinaryColor(TrinaryState s) => s switch {
            TrinaryState.Yes      => ConsoleColor.Green,
            TrinaryState.No       => ConsoleColor.Red,
            TrinaryState.Maybe    => ConsoleColor.Yellow,
            TrinaryState.MaybeNot => ConsoleColor.DarkGray,
            _                     => ConsoleColor.White
        };

        private static string TrinarySymbol(TrinaryState s) => s switch {
            TrinaryState.Yes      => "YES=1",
            TrinaryState.No       => "NO=0",
            TrinaryState.Maybe    => "MAYBE=-1",
            TrinaryState.MaybeNot => "DEFER=-2",
            _                     => "?"
        };
    }
}
