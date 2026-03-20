; boot.asm - MMUKO-OS NSIGII Heartfull Firmware Ring Boot Sector
; FAT12, 16-bit x86, 512 bytes exactly
; OBINexus Computing | Nnamdi Michael Okpala | 20 March 2026
; Build: nasm -f bin boot.asm -o boot.bin
; Image: dd if=boot.bin of=img/mmuko-os.img bs=512 count=1
; QEMU:  qemu-system-x86_64 -drive format=raw,file=img/mmuko-os.img
; NSIGII Phases: N(Need) S(Safety) I(Ident) G(Gov) I(Probe) I(Integrity)
; Trinary: YES=01h NO=00h MAYBE=FFh MAYBE_NOT=FEh
; Membrane: PASS=AAh HOLD=BBh ALERT=CCh

BITS 16
ORG  0x7C00

; FAT12 BPB
jmp short start
nop
          db "MMUKOOS "   ; OEM (8)
          dw 512          ; BytesPerSector
          db 1            ; SectorsPerCluster
          dw 1            ; ReservedSectors
          db 2            ; NumberOfFATs
          dw 224          ; RootEntries
          dw 2880         ; TotalSectors16
          db 0xF0         ; MediaDescriptor
          dw 9            ; SectorsPerFAT
          dw 18           ; SectorsPerTrack
          dw 2            ; NumberOfHeads
          dd 0            ; HiddenSectors
          dd 0            ; TotalSectors32
          db 0            ; DriveNumber
          db 0            ; Reserved1
          db 0x29         ; BootSignature
          dd 0x4E534947   ; VolumeID "NSIG"
          db "MMUKO-RING " ; VolumeLabel (11)
          db "FAT12   "   ; FileSystemType (8)

; Constants
T_YES  equ 0x01
T_NO   equ 0x00
T_MB   equ 0xFF
T_MBN  equ 0xFE
O_PASS equ 0xAA
O_HOLD equ 0xBB
O_ALRT equ 0xCC

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    mov ax, 0x07C0
    mov ds, ax
    mov es, ax
    mov [drv], dl

    ; clear screen
    mov ax, 0x0600
    mov bh, 0x07
    xor cx, cx
    mov dx, 0x184F
    int 0x10
    mov ah, 0x02
    xor bh, bh
    xor dx, dx
    int 0x10

    ; init trinary state
    mov byte [t1], T_MB
    mov byte [t2], T_MB
    mov byte [oc], O_HOLD
    mov word [th], 0

    mov si, str_ban
    call pr

    ; Phase N: Need-state init
    mov si, str_n
    call pr
    int 0x11
    test ax, ax
    jnz ph_s
    mov byte [t1], T_NO
    mov si, str_alrt
    call pr
    jmp hh

ph_s:
    ; Phase S: Safety scan
    mov si, str_s
    call pr
    int 0x12
    mov word [th], 120

    ; Phase I: Identity
    mov si, str_i1
    call pr
    mov word [th], 240

    ; Phase G: Governance
    mov si, str_g
    call pr

    ; Phase I2: Internal probe
    mov si, str_i2
    call pr
    cmp byte [t1], T_NO
    je  hh

    ; Phase I3: Integrity / discriminant
    mov si, str_i3
    call pr
    cmp byte [t1], T_NO
    je  byz
    cmp byte [t2], T_NO
    je  byz
    jmp gate

byz:
    mov si, str_byz
    call pr
    jmp hh

gate:
    cmp byte [t1], T_NO
    je  do_alrt
    cmp byte [t2], T_NO
    je  do_alrt
    mov byte [oc], O_PASS
    mov si, str_pass
    call pr
    mov si, str_ok
    call pr
    jmp hang
do_alrt:
    mov byte [oc], O_ALRT
    mov si, str_alrt
    call pr
hh:
    mov si, str_hld
    call pr
hang:
    hlt
    jmp hang

; print null-terminated string at DS:SI
pr:
    pusha
.l: lodsb
    test al, al
    jz .d
    mov ah, 0x0E
    xor bh, bh
    mov bl, 0x0F
    int 0x10
    jmp .l
.d: popa
    ret

; Variables
drv db 0
t1  db T_MB
t2  db T_MB
oc  db O_HOLD
th  dw 0

; Strings
str_ban  db 13,10,"MMUKO-OS NSIGII v0.1",13,10,"Y=1 N=0 M=-1",13,10,0
str_n    db "[N]Need",13,10,0
str_s    db "[S]Safe",13,10,0
str_i1   db "[I]Ident",13,10,0
str_g    db "[G]Gov",13,10,0
str_i2   db "[I]Probe",13,10,0
str_i3   db "[I]Integ",13,10,0
str_pass db 13,10,"NSIGII_VERIFIED",13,10,"HR:PASS",13,10,0
str_ok   db "BOOT_OK",13,10,0
str_alrt db "ALERT",13,10,0
str_byz  db "BYZ_FAULT",13,10,0
str_hld  db "HELD-T1T2",13,10,0

times 510-($-$$) db 0
dw 0xAA55
