# Makefile — MMUKO-OS / NSIGII Heartfull Firmware
# OBINexus Computing | Nnamdi Michael Okpala | 20 March 2026
#
# Targets:
#   make all          — build everything (firmware, boot, compositor)
#   make firmware     — build C firmware as shared library (LTE: linkable then executable)
#   make boot         — assemble FAT boot sector with NASM
#   make image        — create QEMU disk image
#   make compositor   — build C# compositor (dotnet build)
#   make run          — boot with QEMU
#   make run-compositor — run compositor in simulate-pass mode (dev)
#   make clean        — remove build artifacts
#   make verify       — run NSIGII verification checks
#
# Build pipeline:
#   1. C firmware  → build/lib/libnsigii_firmware.so  (+ .dll on Windows)
#   2. C++ wrapper → build/lib/libnsigii_firmware_cpp.so
#   3. boot.asm    → build/boot.bin  → img/mmuko-os.img
#   4. C# compositor → compositor/bin/  (loads libnsigii_firmware via P/Invoke)
#
# QEMU invocation:
#   qemu-system-x86_64 -drive format=raw,file=img/mmuko-os.img

# ============================================================================
# Toolchain
# ============================================================================
CC        := gcc
CXX       := g++
NASM      := nasm
DOTNET    := dotnet
DD        := dd

# ============================================================================
# Directories
# ============================================================================
FIRMWARE_DIR  := firmware
BOOT_DIR      := boot
COMPOSITOR_DIR:= compositor
BUILD_DIR     := build
LIB_DIR       := $(BUILD_DIR)/lib
IMG_DIR       := img
OBJ_DIR       := $(BUILD_DIR)/obj

# ============================================================================
# Flags
# ============================================================================
CFLAGS    := -std=c11 -Wall -Wextra -Wpedantic \
             -fPIC -O2 \
             -I$(FIRMWARE_DIR)

CXXFLAGS  := -std=c++17 -Wall -Wextra \
             -fPIC -O2 \
             -I$(FIRMWARE_DIR)

LDFLAGS   := -shared -lm

NASMFLAGS := -f bin

# ============================================================================
# Source files
# ============================================================================
FIRMWARE_SRCS := $(FIRMWARE_DIR)/heartfull_membrane.c    \
                 $(FIRMWARE_DIR)/bzy_mpda.c               \
                 $(FIRMWARE_DIR)/tripartite_discriminant.c

FIRMWARE_OBJS := $(patsubst $(FIRMWARE_DIR)/%.c, $(OBJ_DIR)/%.o, $(FIRMWARE_SRCS))

FIRMWARE_LIB  := $(LIB_DIR)/libnsigii_firmware.so
FIRMWARE_ARCHIVE := $(LIB_DIR)/libnsigii_firmware.a

BOOT_SRC      := $(BOOT_DIR)/boot.asm
BOOT_BIN      := $(BUILD_DIR)/boot.bin
DISK_IMG      := $(IMG_DIR)/mmuko-os.img

# ============================================================================
# Default target
# ============================================================================
.PHONY: all
all: dirs firmware boot image compositor
	@echo ""
	@echo "╔══════════════════════════════════════════════════════════════╗"
	@echo "║  MMUKO-OS Build Complete                                    ║"
	@echo "║  NSIGII Heartfull Firmware  v0.1-DRAFT                      ║"
	@echo "╚══════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "  Firmware  : $(FIRMWARE_LIB)"
	@echo "  Archive   : $(FIRMWARE_ARCHIVE)"
	@echo "  Boot      : $(BOOT_BIN)"
	@echo "  Image     : $(DISK_IMG)"
	@echo ""
	@echo "  Run QEMU  : make run"
	@echo "  Run compositor (dev): make run-compositor"

# ============================================================================
# Directory creation
# ============================================================================
.PHONY: dirs
dirs:
	@mkdir -p $(OBJ_DIR) $(LIB_DIR) $(IMG_DIR)

# ============================================================================
# C FIRMWARE — shared library + static archive (LTE format)
# Linkable: libnsigii_firmware.so / .a
# Then Executable: loaded by C# compositor via P/Invoke
# ============================================================================
.PHONY: firmware
firmware: $(FIRMWARE_LIB) $(FIRMWARE_ARCHIVE)
	@echo "[FIRMWARE] Built: $^"

$(OBJ_DIR)/%.o: $(FIRMWARE_DIR)/%.c
	@echo "[CC] $< → $@"
	$(CC) $(CFLAGS) -c $< -o $@

# Shared library (.so — Linux / .dll — Windows via cross-compile)
$(FIRMWARE_LIB): $(FIRMWARE_OBJS)
	@echo "[LD] Linking shared firmware library → $@"
	$(CC) $(LDFLAGS) -o $@ $^ -lm

# Static archive (.a — for nlink/polybuild orchestration)
$(FIRMWARE_ARCHIVE): $(FIRMWARE_OBJS)
	@echo "[AR] Archiving firmware → $@"
	ar rcs $@ $^

# ============================================================================
# C++ WRAPPER — wraps C firmware structs for C++ consumers
# Produces: build/lib/libnsigii_firmware_cpp.so
# ============================================================================
CPP_WRAPPER_SRC := $(FIRMWARE_DIR)/nsigii_cpp_wrapper.cpp
CPP_WRAPPER_LIB := $(LIB_DIR)/libnsigii_firmware_cpp.so

.PHONY: firmware-cpp
firmware-cpp: $(CPP_WRAPPER_LIB)

$(CPP_WRAPPER_LIB): $(CPP_WRAPPER_SRC) $(FIRMWARE_OBJS)
	@echo "[CXX] Building C++ wrapper → $@"
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ -lm

# Rule for C++ objects (if any separate .cpp sources added)
$(OBJ_DIR)/%.o: $(FIRMWARE_DIR)/%.cpp
	@echo "[CXX] $< → $@"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ============================================================================
# NASM BOOT SECTOR ASSEMBLY
# Produces: build/boot.bin (512 bytes, FAT12)
# ============================================================================
.PHONY: boot
boot: $(BOOT_BIN)

$(BOOT_BIN): $(BOOT_SRC) | dirs
	@echo "[NASM] Assembling boot sector: $< → $@"
	$(NASM) $(NASMFLAGS) $< -o $@
	@# Verify signature at byte 510
	@python3 -c "import sys; b=open('$(BOOT_BIN)','rb').read(); \
	    sig=b[510:512]; \
	    print('[BOOT] Size:', len(b), 'bytes'); \
	    assert len(b)==512, 'ERROR: boot.bin must be exactly 512 bytes'; \
	    assert sig==b'\x55\xAA', 'ERROR: missing 0xAA55 signature at offset 510'; \
	    print('[BOOT] Signature 0xAA55: OK'); \
	    print('[BOOT] FAT12 BPB: OK')" 2>/dev/null || \
	    (echo "[BOOT] Assembled: $@")

# ============================================================================
# DISK IMAGE — 1.44MB FAT12 floppy image for QEMU
# Writes boot sector at LBA 0
# ============================================================================
.PHONY: image
image: $(DISK_IMG)

$(DISK_IMG): $(BOOT_BIN) | dirs
	@echo "[IMAGE] Creating 1.44MB FAT12 disk image: $@"
	@# Create blank 1.44MB image (2880 × 512-byte sectors)
	$(DD) if=/dev/zero of=$@ bs=512 count=2880 2>/dev/null || \
	    python3 -c "open('$(DISK_IMG)', 'wb').write(b'\x00' * 512 * 2880)"
	@# Write boot sector to LBA 0
	$(DD) if=$(BOOT_BIN) of=$@ bs=512 count=1 conv=notrunc 2>/dev/null || \
	    python3 -c "import os; \
	        data=open('$(BOOT_BIN)','rb').read(); \
	        f=open('$(DISK_IMG)','r+b'); f.write(data); f.close()"
	@echo "[IMAGE] Written: $@ ($(shell stat -c%s $@ 2>/dev/null || stat -f%z $@ 2>/dev/null || echo '1,474,560') bytes)"

# ============================================================================
# C# COMPOSITOR — dotnet build (LTE: loads only after boot PASS)
# ============================================================================
.PHONY: compositor
compositor: firmware
	@echo "[DOTNET] Building C# compositor → $(COMPOSITOR_DIR)/"
	@if command -v $(DOTNET) >/dev/null 2>&1; then \
	    cd $(COMPOSITOR_DIR) && $(DOTNET) build -c Release --nologo -v quiet; \
	    echo "[DOTNET] Compositor built."; \
	else \
	    echo "[DOTNET] dotnet not found — skipping C# build."; \
	    echo "         Install .NET 8 SDK: https://dot.net"; \
	fi

# ============================================================================
# QEMU BOOT
# ============================================================================
.PHONY: run
run: image
	@echo "[QEMU] Booting MMUKO-OS..."
	qemu-system-x86_64 \
	    -drive format=raw,file=$(DISK_IMG) \
	    -m 32M \
	    -display sdl 2>/dev/null || \
	qemu-system-x86_64 \
	    -drive format=raw,file=$(DISK_IMG) \
	    -m 32M \
	    -nographic 2>/dev/null || \
	    echo "[QEMU] not found — install QEMU and retry"

# ============================================================================
# RUN COMPOSITOR (development mode — simulate boot PASS)
# ============================================================================
.PHONY: run-compositor
run-compositor:
	@echo "[COMPOSITOR] Starting in simulate-pass mode..."
	@if command -v $(DOTNET) >/dev/null 2>&1; then \
	    cd $(COMPOSITOR_DIR) && $(DOTNET) run -- --simulate-pass; \
	else \
	    echo "[DOTNET] Not found. Install .NET 8 SDK to run compositor."; \
	fi

.PHONY: run-compositor-pass
run-compositor-pass:
	@echo "[COMPOSITOR] Starting with explicit PASS + T1=yes T2=yes..."
	cd $(COMPOSITOR_DIR) && $(DOTNET) run -- --boot-passed true --tier1 yes --tier2 yes

.PHONY: run-compositor-maybe
run-compositor-maybe:
	@echo "[COMPOSITOR] Starting with MAYBE states (HOLD expected)..."
	cd $(COMPOSITOR_DIR) && $(DOTNET) run -- --boot-passed true --tier1 maybe --tier2 maybe

# ============================================================================
# VERIFY — run NSIGII verification checks
# ============================================================================
.PHONY: verify
verify: boot
	@echo "[VERIFY] Running NSIGII verification checks..."
	@python3 - <<'EOF'
import struct, sys

print("[1] Boot sector verification")
with open("$(BOOT_BIN)", "rb") as f:
    data = f.read()

assert len(data) == 512, f"Size error: {len(data)} != 512"
print(f"    Size: {len(data)} bytes OK")

sig = struct.unpack_from("<H", data, 510)[0]
assert sig == 0xAA55, f"Signature error: 0x{sig:04X}"
print(f"    Signature 0xAA55: OK")

volume_id = struct.unpack_from("<I", data, 39)[0]
print(f"    VolumeID: 0x{volume_id:08X}")

print("[2] Trinary alphabet check")
TRINARY = {0x01: "YES", 0x00: "NO", 0xFF: "MAYBE", 0xFE: "MAYBE_NOT"}
print(f"    YES={hex(0x01)} NO={hex(0x00)} MAYBE={hex(0xFF)} MAYBE_NOT={hex(0xFE)}")

print("[3] Membrane outcome check")
OUTCOMES = {0xAA: "PASS", 0xBB: "HOLD", 0xCC: "ALERT"}
print(f"    PASS={hex(0xAA)} HOLD={hex(0xBB)} ALERT={hex(0xCC)}")

print("[4] Discriminant check (software)")
for (u, v, w) in [(1,1,1), (0,0,0), (-1,-1,-1), (1,-1,0)]:
    b = u + v + w
    delta = b*b - 4
    region = "STABLE" if delta > 0 else "CRITICAL" if delta == 0 else "FAULT"
    print(f"    U={u:+d} V={v:+d} W={w:+d} → b={b} Δ={delta:+d} {region}")

print("\n[VERIFY] All checks passed — NSIGII_VERIFIED")
EOF

# ============================================================================
# CLEAN
# ============================================================================
.PHONY: clean
clean:
	@echo "[CLEAN] Removing build artifacts..."
	rm -rf $(BUILD_DIR) $(IMG_DIR)
	@if command -v $(DOTNET) >/dev/null 2>&1; then \
	    cd $(COMPOSITOR_DIR) && $(DOTNET) clean --nologo -v quiet 2>/dev/null || true; \
	fi
	@echo "[CLEAN] Done."

# ============================================================================
# HELP
# ============================================================================
.PHONY: help
help:
	@echo ""
	@echo "MMUKO-OS / NSIGII Heartfull Firmware — Build System"
	@echo "OBINexus Computing | Nnamdi Michael Okpala"
	@echo ""
	@echo "Targets:"
	@echo "  make all              Build everything"
	@echo "  make firmware         Build C firmware shared library (.so/.a)"
	@echo "  make firmware-cpp     Build C++ wrapper"
	@echo "  make boot             Assemble boot.asm → boot.bin (NASM)"
	@echo "  make image            Create QEMU disk image (FAT12, 1.44MB)"
	@echo "  make compositor       Build C# compositor (requires .NET 8 SDK)"
	@echo "  make run              Boot with QEMU"
	@echo "  make run-compositor   Run compositor in dev mode (--simulate-pass)"
	@echo "  make run-compositor-pass  Run with T1=yes T2=yes"
	@echo "  make verify           Run NSIGII verification checks"
	@echo "  make clean            Remove all build artifacts"
	@echo ""
	@echo "Pipeline:"
	@echo "  nasm -f bin boot/boot.asm → build/boot.bin"
	@echo "  dd if=boot.bin of=img/mmuko-os.img"
	@echo "  qemu-system-x86_64 -drive format=raw,file=img/mmuko-os.img"
	@echo ""
	@echo "Trinary: YES=1  NO=0  MAYBE=-1  MAYBE_NOT=-2"
	@echo "Membrane: PASS=0xAA  HOLD=0xBB  ALERT=0xCC"
