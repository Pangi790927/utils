# Types a message into another process's console via WriteConsoleInput -- no window focus needed.
#
# It attaches to the target console (AttachConsole), opens CONIN$, and writes the message as one
# bracketed paste (ESC[200~ text ESC[201~) followed by Enter, so an interactive `claude` running
# in that console receives it the way a Ctrl+V delivers it. Newlines are flattened to spaces, so
# only the final Enter submits. Every step is logged to -LogFile (default inject.log beside this
# script) for diagnosis. 09-10-2026
#
#   powershell -ExecutionPolicy Bypass -File inject.ps1 -Target <pid> -TextFile <file>
#
# 08-10-2026-10:40
param([int]$Target, [string]$TextFile, [string]$LogFile = (Join-Path $PSScriptRoot 'inject.log'),
      [switch]$NoPaste,   # -NoPaste: type plainly, e.g. a /command or a shell command
      [switch]$Trust,     # -Trust: answer Claude Code's "trust this folder?" dialog with Yes, if shown
      [switch]$Dump)      # -Dump: write the console's visible text to the log, press nothing
$ErrorActionPreference = 'Stop'
function Log($m) { Add-Content -LiteralPath $LogFile -Value ("{0} {1}" -f (Get-Date -Format HH:mm:ss.fff), $m) }

try {
    Log "=== inject target=$Target textfile=$TextFile trust=$Trust dump=$Dump ==="
    Add-Type -Language CSharp -TypeDefinition @"
using System;
using System.Text;
using System.Runtime.InteropServices;
public static class CI {
    [DllImport("kernel32.dll")] public static extern bool FreeConsole();
    [DllImport("kernel32.dll", SetLastError=true)] public static extern bool AttachConsole(uint pid);
    [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Unicode)]
    public static extern IntPtr CreateFileW(string name, uint access, uint share, IntPtr sec,
        uint disposition, uint flags, IntPtr template);
    [DllImport("kernel32.dll", SetLastError=true)]
    public static extern bool WriteConsoleInputW(IntPtr h, INPUT_RECORD[] buf, uint len,
        out uint written);
    [DllImport("kernel32.dll", SetLastError=true)]
    public static extern bool GetConsoleScreenBufferInfo(IntPtr h, out CSBI info);
    [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Unicode)]
    public static extern bool ReadConsoleOutputCharacterW(IntPtr h, StringBuilder buf, uint len,
        COORD at, out uint read);
    [StructLayout(LayoutKind.Sequential)] public struct COORD { public short X; public short Y; }
    [StructLayout(LayoutKind.Sequential)] public struct RECT {
        public short Left; public short Top; public short Right; public short Bottom;
    }
    [StructLayout(LayoutKind.Sequential)] public struct CSBI {
        public COORD Size; public COORD Cursor; public ushort Attributes; public RECT Window;
        public COORD MaxWindow;
    }
    /* The visible lines of the console screen buffer h, top to bottom. 10-10-2026 */
    public static string[] Visible(IntPtr h) {
        CSBI i;
        if (!GetConsoleScreenBufferInfo(h, out i)) return new string[0];
        int w = i.Window.Right - i.Window.Left + 1;
        var rows = new string[i.Window.Bottom - i.Window.Top + 1];
        for (int y = i.Window.Top; y <= i.Window.Bottom; y++) {
            var sb = new StringBuilder(w); uint read;
            COORD at; at.X = i.Window.Left; at.Y = (short)y;
            ReadConsoleOutputCharacterW(h, sb, (uint)w, at, out read);
            rows[y - i.Window.Top] = sb.ToString().TrimEnd();
        }
        return rows;
    }
    [StructLayout(LayoutKind.Explicit)] public struct INPUT_RECORD {
        [FieldOffset(0)] public ushort EventType;
        [FieldOffset(4)] public KEY_EVENT_RECORD KeyEvent;
    }
    [StructLayout(LayoutKind.Sequential)] public struct KEY_EVENT_RECORD {
        public int bKeyDown;
        public ushort wRepeatCount;
        public ushort wVirtualKeyCode;
        public ushort wVirtualScanCode;
        public char UnicodeChar;
        public uint dwControlKeyState;
    }
}
"@
    Log "Add-Type ok"

    # -Dump and -Trust read the screen first. -Trust presses keys only when Claude Code's trust
    # dialog is on screen, and moves the cursor onto the "Yes" option whichever way round the
    # options are ordered (one version puts "No, exit" first and preselected). 10-10-2026
    if ($Trust -or $Dump) {
        [CI]::FreeConsole() | Out-Null
        if (-not [CI]::AttachConsole([uint32]$Target)) { throw "AttachConsole failed" }
        $out = [CI]::CreateFileW("CONOUT$", [uint32]3221225472, 3, [IntPtr]::Zero, 3, 0, [IntPtr]::Zero)
        $in = [CI]::CreateFileW("CONIN$", [uint32]1073741824, 3, [IntPtr]::Zero, 3, 0, [IntPtr]::Zero)
        $rows = [CI]::Visible($out)
        if ($Dump) {
            foreach ($r in $rows) { Log ("| " + $r) }
            [CI]::FreeConsole() | Out-Null
            exit 0
        }
        $mark = [char]0x276F
        $yes = -1; $no = -1; $cur = -1
        for ($i = 0; $i -lt $rows.Length; $i++) {
            $t = $rows[$i]
            if ($t -match 'Yes, (I trust this folder|proceed)') { $yes = $i }
            if ($t -match 'No, exit') { $no = $i }
            if (($t -match 'Yes, (I trust|proceed)|No, exit') -and $t.Contains([string]$mark)) { $cur = $i }
        }
        if ($yes -lt 0 -or $no -lt 0) { Log "no trust dialog on screen"; [CI]::FreeConsole() | Out-Null; exit 0 }
        if ($cur -lt 0) { Log "trust dialog without a visible cursor"; [CI]::FreeConsole() | Out-Null; exit 3 }
        function Send-Key([uint16]$vk, [char]$ch) {
            $a = New-Object 'CI+INPUT_RECORD[]' 2
            foreach ($d in 1, 0) {
                $r = New-Object CI+INPUT_RECORD; $r.EventType = 1
                $k = New-Object CI+KEY_EVENT_RECORD
                $k.bKeyDown = $d; $k.wRepeatCount = 1; $k.wVirtualKeyCode = $vk; $k.UnicodeChar = $ch
                $r.KeyEvent = $k; $a[1 - $d] = $r
            }
            $w = 0; [CI]::WriteConsoleInputW($in, $a, 2, [ref]$w) | Out-Null
            Start-Sleep -Milliseconds 300
        }
        $steps = $yes - $cur
        Log ("trust dialog: cursor on row {0}, Yes on row {1}: {2} step(s)" -f $cur, $yes, $steps)
        for ($s = 0; $s -lt [Math]::Abs($steps); $s++) {
            if ($steps -gt 0) { Send-Key 0x28 ([char]0) } else { Send-Key 0x26 ([char]0) }
        }
        Send-Key 0x0D ([char]13)
        Start-Sleep -Milliseconds 1500
        $after = ([CI]::Visible($out) | Where-Object { $_ -match 'No, exit' }).Count
        [CI]::FreeConsole() | Out-Null
        if ($after -gt 0) { Log "trust dialog still on screen"; exit 3 }
        Log "trust dialog answered Yes"
        exit 0
    }

    $text = ([IO.File]::ReadAllText($TextFile)) -replace "[\r\n]+", " "
    Log ("text length (flattened) = {0}" -f $text.Length)
    $list = New-Object System.Collections.Generic.List[object]
    function Add-Key([bool]$down, [char]$ch, [uint16]$vk) {
        $r = New-Object CI+INPUT_RECORD
        $r.EventType = 1
        $k = New-Object CI+KEY_EVENT_RECORD
        $k.bKeyDown = [int]$down
        $k.wRepeatCount = 1
        $k.wVirtualKeyCode = $vk
        $k.wVirtualScanCode = 0
        $k.UnicodeChar = $ch
        $k.dwControlKeyState = 0
        $r.KeyEvent = $k
        $list.Add($r)
    }
    # The text goes in as ONE paste, the way a real Ctrl+V delivers it: wrapped in the bracketed-
    # paste markers ESC[200~ ... ESC[201~, so the app takes it whole, whatever its length, instead
    # of as thousands of separate keystrokes. 09-10-2026
    function Add-Seq([string]$s) {
        foreach ($c in $s.ToCharArray()) {
            $vk = if ([int]$c -eq 27) { 0x1B } else { 0 }
            Add-Key $true $c $vk; Add-Key $false $c $vk
        }
    }
    $esc = [string][char]27
    if ($text.Length -gt 0 -and -not $NoPaste) { Add-Seq ($esc + '[200~') }
    foreach ($c in $text.ToCharArray()) { Add-Key $true $c 0; Add-Key $false $c 0 }
    if ($text.Length -gt 0 -and -not $NoPaste) { Add-Seq ($esc + '[201~') }
    $textArr = New-Object 'CI+INPUT_RECORD[]' ($list.Count)
    for ($i = 0; $i -lt $list.Count; $i++) { $textArr[$i] = [CI+INPUT_RECORD]$list[$i] }
    $list.Clear()
    Add-Key $true ([char]13) 0x0D                       # Enter sent SEPARATELY, so the text burst
    Add-Key $false ([char]13) 0x0D                      # is pasted and then this submits it
    $enterArr = New-Object 'CI+INPUT_RECORD[]' ($list.Count)
    for ($i = 0; $i -lt $list.Count; $i++) { $enterArr[$i] = [CI+INPUT_RECORD]$list[$i] }
    Log ("built {0} text + {1} enter records" -f $textArr.Length, $enterArr.Length)

    [CI]::FreeConsole() | Out-Null
    $attached = [CI]::AttachConsole([uint32]$Target)
    Log ("AttachConsole -> {0} (lastErr {1})" -f $attached, [Runtime.InteropServices.Marshal]::GetLastWin32Error())
    if (-not $attached) { throw "AttachConsole failed" }

    $GENERIC_WRITE = 0x40000000; $SHARE_RW = 3; $OPEN_EXISTING = 3
    $h = [CI]::CreateFileW("CONIN$", $GENERIC_WRITE, $SHARE_RW, [IntPtr]::Zero, $OPEN_EXISTING, 0, [IntPtr]::Zero)
    Log ("CreateFile CONIN$ -> handle {0}" -f $h)
    $w1 = 0
    $ok1 = $true
    if ($textArr.Length -gt 0) {
        $ok1 = [CI]::WriteConsoleInputW($h, $textArr, [uint32]$textArr.Length, [ref]$w1)
        Log ("WriteConsoleInput text -> {0} written={1}" -f $ok1, $w1)
    }
    Start-Sleep -Milliseconds (900 + [Math]::Min(3000, $textArr.Length / 6))  # bigger paste, longer wait
    $w2 = 0
    $ok2 = [CI]::WriteConsoleInputW($h, $enterArr, [uint32]$enterArr.Length, [ref]$w2)
    Log ("WriteConsoleInput enter -> {0} written={1}" -f $ok2, $w2)
    [CI]::FreeConsole() | Out-Null
    if (-not ($ok1 -and $ok2)) { throw "WriteConsoleInput failed" }
    Log "OK"
    exit 0
}
catch {
    Log ("ERROR: " + $_.Exception.Message)
    Log ($_.ScriptStackTrace)
    exit 1
}
