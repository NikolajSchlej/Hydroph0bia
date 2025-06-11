mode 128 40
fs0:
setvar SecureFlashInfo -guid 382af2bb-ffff-abcd-aaee-cce099338877 -nv -bs -rt =
fpt_signed.efi -bios -f bios.bin
rm startup.nsh
rm bios.bin
rm fpt_signed.efi
rm sfpoc_signed.efi
rm EFI\Insyde\isflash.bin
rm EFI\Insyde\




