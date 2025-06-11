mode 128 40
fs0:
bcfg driver add 0 sfpoc_signed.efi "SFPoC"
rm EFI/Boot/bootx64.efi
mv EFI/Boot/bootx64.efi.org EFI/Boot/bootx64.efi
rm EFI/Microsoft/Boot/bootmgfw.efi
mv EFI/Microsoft/Boot/bootmgfw.efi.org EFI/Microsoft/Boot/bootmgfw.efi
rm startup.nsh
mv startup_2.nsh startup.nsh
stall 1000
reset

