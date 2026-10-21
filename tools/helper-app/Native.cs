using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

namespace DeckPadHelper
{
    /// <summary>Windows-Funktionen: Vordergrundfenster ermitteln und Tasten senden.</summary>
    internal static class Native
    {
        [DllImport("user32.dll")] private static extern IntPtr GetForegroundWindow();
        [DllImport("user32.dll")] private static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
        [DllImport("user32.dll", CharSet = CharSet.Unicode)] private static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
        [DllImport("user32.dll", SetLastError = true)] private static extern uint SendInput(uint n, INPUT[] inputs, int size);

        [StructLayout(LayoutKind.Sequential)]
        private struct MOUSEINPUT { public int dx, dy; public uint mouseData, dwFlags, time; public IntPtr dwExtraInfo; }
        [StructLayout(LayoutKind.Sequential)]
        private struct KEYBDINPUT { public ushort wVk, wScan; public uint dwFlags, time; public IntPtr dwExtraInfo; }
        [StructLayout(LayoutKind.Explicit)]
        private struct INPUTUNION { [FieldOffset(0)] public MOUSEINPUT mi; [FieldOffset(0)] public KEYBDINPUT ki; }
        [StructLayout(LayoutKind.Sequential)]
        private struct INPUT { public uint type; public INPUTUNION u; }

        private const uint KEYEVENTF_EXTENDEDKEY = 1, KEYEVENTF_KEYUP = 2, KEYEVENTF_UNICODE = 4;

        /// <summary>Programmname (z. B. "chrome.exe") und Fenstertitel des Fensters im Vordergrund.</summary>
        public static bool GetForeground(out string exe, out string title, out int pid)
        {
            exe = ""; title = ""; pid = 0;
            IntPtr h = GetForegroundWindow();
            if (h == IntPtr.Zero) return false;
            uint p; GetWindowThreadProcessId(h, out p);
            pid = (int)p;
            var sb = new StringBuilder(256);
            GetWindowText(h, sb, sb.Capacity);
            title = sb.ToString();
            try { exe = Process.GetProcessById((int)p).ProcessName + ".exe"; }
            catch { return false; }       // Zugriff verweigert oder Prozess weg
            return true;
        }

        private static void Send(ushort vk, ushort scan, uint flags)
        {
            var a = new INPUT[1];
            a[0].type = 1;                // INPUT_KEYBOARD
            a[0].u.ki.wVk = vk; a[0].u.ki.wScan = scan; a[0].u.ki.dwFlags = flags;
            SendInput(1, a, Marshal.SizeOf(typeof(INPUT)));
        }

        // Bild/Pos1/Ende/Pfeile/Einfg/Entf brauchen das "erweiterte Taste"-Flag
        private static bool Ext(ushort vk) { return (vk >= 0x21 && vk <= 0x28) || vk == 0x2D || vk == 0x2E; }

        /// <summary>Tasten gleichzeitig druecken und wieder loslassen.</summary>
        public static void Chord(ushort[] vks)
        {
            foreach (ushort v in vks) Send(v, 0, Ext(v) ? KEYEVENTF_EXTENDEDKEY : 0u);
            Thread.Sleep(30);
            for (int i = vks.Length - 1; i >= 0; i--) Send(vks[i], 0, (Ext(vks[i]) ? KEYEVENTF_EXTENDEDKEY : 0u) | KEYEVENTF_KEYUP);
        }

        /// <summary>Text tippen (Unicode, unabhaengig vom Tastaturlayout).</summary>
        public static void TypeText(string s)
        {
            foreach (char c in s) { Send(0, c, KEYEVENTF_UNICODE); Send(0, c, KEYEVENTF_UNICODE | KEYEVENTF_KEYUP); }
        }
    }
}
