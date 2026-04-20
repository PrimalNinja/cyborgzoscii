# Securing Your PC Running Windows 10+

*Last Updated: January 2026*  
*Version DRAFT 1.0*

---

## Overview

Note: This document refers to **unsecured data** as data you want to be secure.

This document explains how to configure your PC to ensure that **zero traces of unsecured data** remain on any PC drive that could be stolen. This is for situations where your unsecured original files are highly sensitive and you need absolute certainty that they leave no trace on your computer.  

This document does NOT cover internet or network security.

**Important:** This is an advanced configuration. Most users should follow the simpler approach in the **"Securing Your Files Guide"** document. Only pursue this setup if you have specific requirements for maximum security against PC theft or seizure.

---

## The Simplest and Most Secure Solution: Portable Windows

**RECOMMENDED APPROACH:**

The easiest and most reliable way to ensure a stolen PC does not contain unwanted traces of secured data is to **run Windows from portable removable media** (such as a USB drive or external SSD).

### Why Portable Windows is Best

- **100% guaranteed isolation** - Your PC's internal hard drives are not used for windows caching and other related leaks
- **Simplest setup** - No complex Windows configuration needed
- **Most secure** - When you shut down, unplug the portable Windows drive, and your PC has zero traces
- **Easy destruction** - If needed, physically destroy just the portable drive
- **Easiest to use** - Works like normal Windows, but completely separate from PC drives

### How to Set Up Portable Windows

1. Get a fast USB 3.0+ drive or external SSD (at least 32GB, preferably 64GB+)
2. Use Windows To Go (Windows Enterprise) or third-party tools like Rufus or WinToUSB
3. Install Windows on the portable drive
4. Boot from the portable drive when processing sensitive files
5. When done, shut down and unplug the drive - your PC has zero traces

### Using Portable Windows

- Boot your PC from the portable Windows drive
- Store unsecured originals on a separate USB drive or the portable Windows drive itself
- Store ROM files on separate removable media with multiple backups
- Process files and save secured output anywhere (even back to PC's internal drives - it's protected!)
- Shut down and unplug the portable Windows drive
- Your PC's internal drives contain zero traces of unsecured data

**This is the recommended approach for maximum security.** The rest of this document describes alternative configurations if you cannot use portable Windows.

---

## Alternative: Advanced PC Configuration

**Scenario:** You want to use your regular Windows installation but ensure no traces of unsecured data remain on PC drives if stolen.

**Security Goal:** Ensure that if your PC is stolen, forensic analysis will find ZERO traces of unsecured data on any drive. Encoded data can be anywhere - it doesn't matter since it's information-theoretically secure.

**The Challenge:** Even after "deleting" files, Windows leaves traces everywhere:
- Page file (virtual memory)
- Hibernation file  
- System restore points
- Shadow copies
- Search indexes
- Temporary caches
- Journal files
- Thumbnail caches
- Recent files lists

**WARNING This configuration hopes to eliminate all these traces of unsecured data as much as possible. Note however Windows changes frequently with updates, and new leaks / behavioural changes may occur without your knowledge - do NOT consider this document complete - you need to your system knowledge up to date**

---

## Required Windows Configuration

Complete ALL steps below before using this application. Most require administrator privileges.

### Step 1: Disable Virtual Memory (Page File)

Virtual memory can write unsecured data from RAM to disk, compromising security.

**Instructions:**
1. Press `Win + Pause/Break` or right-click "This PC" → Properties
2. Click "Advanced system settings" (left sidebar)
3. Under "Performance", click "Settings..."
4. Go to "Advanced" tab
5. Under "Virtual memory", click "Change..."
6. **Uncheck** "Automatically manage paging file size for all drives"
7. Select "No paging file"
8. Click "Set", then "OK"
9. **Restart your computer** (required)

**Verification:**
- After restart, return to Virtual Memory settings
- Confirm "No paging file" is selected for all drives

---

### Step 2: Disable Hibernation

Hibernation saves RAM contents (including your unsecured data) to disk.

**Instructions:**
1. Open Command Prompt or PowerShell **as Administrator**
2. Run: `powercfg /hibernate off`
3. Press Enter

**Verification:**
- Check that `C:\hiberfil.sys` no longer exists
- Or run: `powercfg /a` and verify "Hibernate has not been enabled" appears

---

### Step 3: Disable System Restore and Shadow Copies

These features create snapshots that may contain your unsecured files.

**Disable System Restore:**
1. Press `Win + Pause/Break` or right-click "This PC" → Properties
2. Click "System protection" (left sidebar)
3. Select each drive that could potentially contain traces
4. Click "Configure"
5. Select "Disable system protection"
6. Click "Apply", then "OK"
7. When prompted to delete restore points, click "Yes"

**Delete Existing Shadow Copies:**
1. Open Command Prompt **as Administrator**
2. For EACH drive, run:
   ```
   vssadmin Delete Shadows /For=C: /All /Quiet
   ```
   (Replace `C:` with your drive letter)

---

### Step 4: Disable Windows Search Indexing

Search indexing may cache fragments of your file contents.

**Instructions:**
1. Press `Win + R`, type `services.msc`, press Enter
2. Find "Windows Search" in the list
3. Right-click → Properties
4. Set "Startup type" to "Disabled"
5. Click "Stop" if the service is running
6. Click "Apply", then "OK"

**Alternative - Disable for specific drives:**
1. Open "Indexing Options" from Control Panel
2. Click "Modify"
3. Uncheck all drives
4. Click "OK"

---

### Step 5: Disable Crash Dumps

Crash dumps may contain memory contents including unsecured data.

**Instructions:**
1. Press `Win + Pause/Break` or right-click "This PC" → Properties
2. Click "Advanced system settings"
3. Under "Startup and Recovery", click "Settings..."
4. Under "Write debugging information", select "(none)"
5. Click "OK"

---

### Step 6: Disable Prefetch and Superfetch

These features cache file access patterns and fragments.

**Instructions:**
1. Open Command Prompt **as Administrator**
2. Run these commands:
   ```
   sc stop "SysMain"
   sc config "SysMain" start=disabled
   reg add "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Memory Management\PrefetchParameters" /v EnablePrefetcher /t REG_DWORD /d 0 /f
   reg add "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Memory Management\PrefetchParameters" /v EnableSuperfetch /t REG_DWORD /d 0 /f
   ```
3. Restart your computer

---

### Step 7: Clear Thumbnail Cache and Disable Thumbnail Generation

Even if you only open an unsecured file from a USB stick, Windows File Explorer often automatically generates thumbnails stored on the local C: drive.

**Clear Existing Thumbnails:**
1. Open Command Prompt **as Administrator**
2. Run:
   ```
   del /f /s /q /a %LocalAppData%\Microsoft\Windows\Explorer\thumbcache_*.db
   ```

**Disable Future Thumbnail Generation:**
1. Press `Win + R`, type `gpedit.msc`, press Enter (Group Policy Editor)
2. Navigate to: User Configuration → Administrative Templates → Windows Components → File Explorer
3. Find "Turn off the caching of thumbnails in hidden thumbs.db files"
4. Set to "Enabled"
5. Also find "Turn off the display of thumbnails and only display icons"
6. Set to "Enabled"

**Alternative (via Registry):**
1. Open Command Prompt **as Administrator**
2. Run:
   ```
   reg add "HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced" /v DisableThumbnailCache /t REG_DWORD /d 1 /f
   reg add "HKCU\Software\Microsoft\Windows\CurrentVersion\Policies\Explorer" /v DisableThumbnails /t REG_DWORD /d 1 /f
   ```

---

### Step 8: Disable Recent Files and Quick Access

The Windows Jump Lists and "Recent Items" store filenames and paths of files on the local disk.

**Instructions:**
1. Open File Explorer
2. Click "..." (three dots) → Options
3. In the "General" tab:
   - Uncheck "Show recently used files in Quick access"
   - Uncheck "Show frequently used folders in Quick access"
4. Click "Clear" next to "Clear File Explorer history"
5. Click "OK"

**Additionally, disable via Privacy Settings:**
1. Press `Win + I` for Settings
2. Go to Privacy → Activity history
3. Uncheck "Store my activity history on this device"
4. Click "Clear" under "Clear activity history"

---

## Storage Setup

### Unsecured Zone (Input Files - MUST NOT TOUCH PC)
**Use removable media only:**
- USB flash drive containing original files
- Separate USB flash drive containing the ROM files
- External SSD with originals
- **Never on PC's internal drives** - not even temporarily

### Secured Zone (Output Files - CAN GO ANYWHERE)
**Store wherever convenient:**
- Your PC's C:\ drive
- Any internal PC drive
- External storage
- Cloud storage
- **Doesn't matter** - encoded data is information-theoretically secure

**The point:** Your PC can be stolen with all its drives, and the attacker will find ZERO traces of unsecured data.

---

## Using the Application

### Best Practices

1. **Before processing:**
   - Keep unsecured originals on removable media ONLY
   - Keep unsecured ROM files on removable media ONLY with multiple backups
   - Never copy them to your PC
   - Verify all Windows configurations are complete

2. **During processing:**
   - Read directly from removable media
   - Write secured output to your PC (any drive - doesn't matter)
   - Windows configurations prevent any unsecured data leaking to PC drives

3. **After processing:**
   - Secured files on your PC are safe - no special handling needed

---

## Verification Checklist

Before using this application, verify ALL items are complete:

**Windows Configuration:**
- [ ] Are you using Portable Windows? (Recommended - if yes, skip remaining checklist)
- [ ] Virtual memory (page file) is disabled
- [ ] Hibernation is disabled  
- [ ] System Restore is disabled
- [ ] Shadow Copies are deleted
- [ ] Windows Search indexing is disabled
- [ ] Crash dumps are disabled
- [ ] Prefetch/Superfetch are disabled
- [ ] Thumbnail cache cleared and generation disabled
- [ ] Recent files and Quick Access disabled
- [ ] Computer has been restarted after configuration changes

**Storage Setup:**
- [ ] Unsecured originals are on removable media ONLY (USB, external drive)
- [ ] Unsecured ROM files are on removable media ONLY (USB, external drive) with multiple backups
- [ ] Unsecured originals have NEVER been copied to PC's internal drives

**Understanding:**
- [ ] I understand secured output can go anywhere on my PC (it's protected)
- [ ] I understand the ONLY risk is traces of unsecured data
- [ ] I understand portable Windows is the easiest and most secure option

---

## Additional Security Measures (Optional)

### RAM Disk for Temporary Processing
If you need temporary storage during processing:
- Install RAM disk software (e.g., ImDisk, OSFMount)
- Create a RAM disk for temporary files
- Configure application to use RAM disk for temp storage
- RAM disk contents are lost when computer powers off (no traces)

### Disable Cloud Sync
Ensure cloud services don't silently copy your files:
- OneDrive: Settings → Account → Unlink this PC
- Dropbox: Pause or quit the application
- Google Drive: Pause or quit the application

### Disable Antivirus Scanning (Advanced)
Some antivirus software caches scanned files:
- Add your removable media to exclusion list
- Or temporarily disable real-time protection during processing
- Re-enable after secure deletion

### Additional Privacy Measures

**Disable Cortana/Search Suggestions:**
- Settings → Privacy → Search permissions
- Disable all search history and suggestions

**Disable Diagnostic Data:**
- Settings → Privacy → Diagnostics & feedback
- Set to "Basic" or use third-party tools to disable entirely

**Clear Temp Folders Regularly:**
```
del /f /s /q %temp%\*
del /f /s /q C:\Windows\Temp\*
```

---

## Understanding the Limitations

### What This DOES Protect Against:
✓ PC stolen with all drives - ZERO traces of unsecured data found  
✓ Forensic analysis of PC - No unsecured data discoverable  
✓ Memory dumps to disk - Prevented by disabling page file/hibernation  
✓ Windows caching/indexing unsecured data - Prevented by disabling these features  

### What This CANNOT Fully Protect Against:
✗ **Active malware during processing** - Can capture data from RAM while you're processing  
✗ **Screen capture** - If someone is watching or recording your screen  
✗ **Keyloggers** - If capturing your inputs  
✗ **Hardware keyloggers or screen recorders** - Physical devices monitoring your activity

### The Guarantee:
After processing:
- Your PC contains ONLY secured data (which is safe even if stolen)
- ZERO traces of unsecured data remain on PC drives (if properly configured)
- Forensic analysis of your stolen PC will find nothing useful

### For Maximum Security:
- **Use Portable Windows** (recommended - simplest and most secure)
- Use this in a physically secure environment
- Ensure PC is clean (no malware) before processing
- Keep ROM file backups in physically separate locations

---

## Troubleshooting

### "The application detected insecure configuration"
- Review the verification checklist above
- Restart your computer after making changes
- Run the application as Administrator to perform checks

### "Cannot disable page file"
- You must have Administrator privileges
- Some system configurations require page file (minimum size)
- Consider using a RAM disk for sensitive operations instead
- **Or use Portable Windows** (recommended)

### "Performance is slow after disabling page file"
- Add more physical RAM to your computer
- Close unnecessary applications before processing
- This is normal when RAM is limited
- **Or use Portable Windows** with adequate RAM

### "Cannot access Group Policy Editor (gpedit.msc)"
- You may have Windows Home edition (doesn't include Group Policy Editor)
- Use the Registry alternative methods provided in the steps above
- **Or use Portable Windows with Professional/Enterprise edition**

### "Files are on the same drive warning"
- Verify you're using different **physical drives**, not just partitions
- Check drive letters in Disk Management (diskmgmt.msc)
- Use removable media for unsecured files
- **Portable Windows eliminates this issue entirely**

---

## Common Questions

**Is this really necessary?**  
For most users, NO. The simple approach in "Securing Your Files Guide" is sufficient. This advanced configuration is only for highly sensitive data where you need absolute certainty that zero traces remain on your PC if stolen.

**What's the easiest way to achieve maximum security?**  
**Use Portable Windows.** It's simpler than all the configuration steps above and provides 100% guaranteed isolation. This is the recommended approach.

**Why doesn't Portable Windows need all these configuration steps?**  
Because Portable Windows runs entirely from removable media. Your PC's Windows leaky nature does not touch your hard drives, so there are no traces to worry about. When you unplug the portable drive, your PC has zero traces.

**How does this provide perfect protection?**  
By keeping unsecured originals ONLY on removable media that never touches your PC's drives, and by disabling all Windows features that might cache data to disk during processing. 

**Can I skip some Windows configuration steps?**  
No (unless using Portable Windows). Each step prevents a specific way Windows might write unsecured data from RAM to disk during processing (page file, hibernation, caching, thumbnails, etc.). Skipping steps creates risk.

**What if I make a mistake?**  
If you accidentally copy unsecured data to your PC before processing, those traces may remain. You'd need to secure-erase affected drives or use Portable Windows going forward.

**Does this protect against malware?**  
No. If your PC has active malware while processing, it could capture unsecured data from RAM or steal ROM files. This approach assumes your PC is clean during processing - it protects against physical theft afterward, not active compromise during use.

**Can I go back to normal Windows settings later?**  
Yes, you can re-enable all the features you disabled. However, any traces created while features were enabled may persist. For maximum security, use a clean Windows installation or Portable Windows.

**What about SSDs with wear-leveling?**  
Modern SSDs automatically move data around for longevity, which can leave traces even after deletion. This is another reason Portable Windows is recommended - it provides guaranteed isolation regardless of storage technology.

---

## Quick Reference Card

**Portable Windows (Recommended):**
1. **Install Windows on USB/external SSD** - Use Windows To Go, Rufus, or WinToUSB
2. **Boot from portable drive** - When processing sensitive files
3. **Keep originals on separate removable media** - Never on PC drives
4. **Process and save secured output anywhere** - Even PC drives are fine (it's protected)
5. **Shut down and unplug** - Zero traces on PC

**Advanced PC Configuration (Alternative):**
1. **Unsecured data NEVER touches PC** - Keep on removable media only
2. **Configure Windows FIRST** - All 8+ configuration steps required
3. **Process directly from removable media** - Don't copy to PC first
5. **Secured output anywhere on PC is fine** - It's protected, can be stolen without risk
