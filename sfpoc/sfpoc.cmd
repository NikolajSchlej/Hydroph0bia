sfcd set
mountvol U: /s
mkdir U:\\EFI\\Insyde
copy isflash.bin U:\\EFI\\Insyde\\
move U:\\EFI\Boot\\bootx64.efi U:\\EFI\\Boot\\bootx64.efi.org
move U:\\EFI\Microsoft\\Boot\\bootmgfw.efi U:\\EFI\Microsoft\\Boot\\bootmgfw.efi.org
copy isflash.bin U:\\EFI\Boot\\bootx64.efi
copy isflash.bin U:\\EFI\Microsoft\\Boot\\bootmgfw.efi
copy bios.bin U:\\
copy fpt_signed.efi U:\\
copy sfpoc_signed.efi U:\\
copy startup_2.nsh U:\\
copy startup_1.nsh U:\\startup.nsh
dir U:
pause
mountvol U: /d
shutdown /r /t 0
