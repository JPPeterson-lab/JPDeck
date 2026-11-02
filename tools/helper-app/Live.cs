using System;
using System.Runtime.InteropServices;
using FILETIME = System.Runtime.InteropServices.ComTypes.FILETIME;

namespace JPDeckHelper
{
    /// <summary>
    /// Messwerte, die das JPDeck auf Kacheln anzeigt: Lautstaerke und Stumm-Schalter der Wiedergabe, Stumm-Schalter des Mikrofons,
    /// CPU- und Arbeitsspeicher-Last. Fehlende Werte (z. B. kein Mikrofon) werden einfach weggelassen.
    /// </summary>
    internal static class Live
    {
        // ── CPU und RAM ──
        [DllImport("kernel32.dll")] private static extern bool GetSystemTimes(out FILETIME idle, out FILETIME kernel, out FILETIME user);
        [StructLayout(LayoutKind.Sequential)]
        private struct MEMORYSTATUSEX
        {
            public uint dwLength, dwMemoryLoad;
            public ulong ullTotalPhys, ullAvailPhys, ullTotalPageFile, ullAvailPageFile, ullTotalVirtual, ullAvailVirtual, ullAvailExtendedVirtual;
        }
        [DllImport("kernel32.dll")] private static extern bool GlobalMemoryStatusEx(ref MEMORYSTATUSEX m);

        private static ulong prevIdle, prevTotal;

        private static ulong ToU(FILETIME f) { return ((ulong)(uint)f.dwHighDateTime << 32) | (uint)f.dwLowDateTime; }

        private static int Cpu()
        {
            FILETIME i, k, u;
            if (!GetSystemTimes(out i, out k, out u)) return -1;
            ulong idle = ToU(i), total = ToU(k) + ToU(u);            // Kernelzeit enthaelt die Leerlaufzeit
            ulong dIdle = idle - prevIdle, dTotal = total - prevTotal;
            prevIdle = idle; prevTotal = total;
            if (dTotal == 0) return -1;
            int pct = (int)Math.Round(100.0 * (1.0 - (double)dIdle / dTotal));
            return Math.Max(0, Math.Min(100, pct));
        }

        private static int Ram()
        {
            var m = new MEMORYSTATUSEX { dwLength = (uint)Marshal.SizeOf(typeof(MEMORYSTATUSEX)) };
            return GlobalMemoryStatusEx(ref m) ? (int)m.dwMemoryLoad : -1;
        }

        // ── Core Audio (Lautstaerke, Stumm) ──
        [ComImport, Guid("BCDE0395-E52F-467C-8E3D-C4579291692E")] private class MMDeviceEnumeratorCom { }

        [ComImport, Guid("A95664D2-9614-4F35-A746-DE8DB63617E6"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        private interface IMMDeviceEnumerator
        {
            int EnumAudioEndpoints(int dataFlow, int stateMask, out IntPtr devices);
            int GetDefaultAudioEndpoint(int dataFlow, int role, out IMMDevice endpoint);
        }

        [ComImport, Guid("D666063F-1587-4E43-81F1-B948E807363F"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        private interface IMMDevice
        {
            int Activate(ref Guid iid, int clsCtx, IntPtr activationParams, [MarshalAs(UnmanagedType.IUnknown)] out object iface);
        }

        [ComImport, Guid("5CDF2C82-841E-4546-9722-0CF74078229A"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        private interface IAudioEndpointVolume
        {
            int RegisterControlChangeNotify(IntPtr notify);
            int UnregisterControlChangeNotify(IntPtr notify);
            int GetChannelCount(out uint count);
            int SetMasterVolumeLevel(float level, ref Guid context);
            int SetMasterVolumeLevelScalar(float level, ref Guid context);
            int GetMasterVolumeLevel(out float level);
            int GetMasterVolumeLevelScalar(out float level);
            int SetChannelVolumeLevel(uint channel, float level, ref Guid context);
            int SetChannelVolumeLevelScalar(uint channel, float level, ref Guid context);
            int GetChannelVolumeLevel(uint channel, out float level);
            int GetChannelVolumeLevelScalar(uint channel, out float level);
            int SetMute([MarshalAs(UnmanagedType.Bool)] bool mute, ref Guid context);
            int GetMute([MarshalAs(UnmanagedType.Bool)] out bool mute);
        }

        private static readonly Guid IID_IAudioEndpointVolume = new Guid("5CDF2C82-841E-4546-9722-0CF74078229A");

        /// <summary>flow: 0 = Wiedergabe, 1 = Aufnahme (Mikrofon); das Standardgeraet fuer "Konsole".</summary>
        private static bool Audio(int flow, out int volume, out bool mute)
        {
            volume = -1; mute = false;
            IMMDeviceEnumerator en = null; IMMDevice dev = null; object o = null;
            try
            {
                en = (IMMDeviceEnumerator)new MMDeviceEnumeratorCom();
                if (en.GetDefaultAudioEndpoint(flow, 0, out dev) != 0 || dev == null) return false;
                Guid iid = IID_IAudioEndpointVolume;
                if (dev.Activate(ref iid, 23, IntPtr.Zero, out o) != 0 || o == null) return false;   // 23 = CLSCTX_ALL
                var ev = (IAudioEndpointVolume)o;
                float v;
                if (ev.GetMasterVolumeLevelScalar(out v) != 0) return false;
                volume = (int)Math.Round(v * 100);
                ev.GetMute(out mute);
                return true;
            }
            catch { return false; }
            finally
            {
                if (o != null) Marshal.ReleaseComObject(o);
                if (dev != null) Marshal.ReleaseComObject(dev);
                if (en != null) Marshal.ReleaseComObject(en);
            }
        }

        /// <summary>Eine Zeile fuer das JPDeck: "LIVE vol=45 mute=0 mic=1 cpu=23 ram=61" (mic=1: Mikrofon ist stumm).</summary>
        public static string Line()
        {
            var sb = new System.Text.StringBuilder("LIVE");
            int vol; bool mute;
            if (Audio(0, out vol, out mute)) sb.Append(" vol=").Append(vol).Append(" mute=").Append(mute ? 1 : 0);
            int mv; bool mm;
            if (Audio(1, out mv, out mm)) sb.Append(" mic=").Append(mm ? 1 : 0);
            int cpu = Cpu(), ram = Ram();
            if (cpu >= 0) sb.Append(" cpu=").Append(cpu);
            if (ram >= 0) sb.Append(" ram=").Append(ram);
            return sb.ToString();
        }
    }
}
