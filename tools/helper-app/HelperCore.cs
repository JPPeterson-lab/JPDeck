using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO.Ports;
using System.Management;
using System.Text.RegularExpressions;
using System.Threading;

namespace DeckPadHelper
{
    /// <summary>
    /// Die eigentliche Arbeit, in einem eigenen Thread:
    ///  - findet das DeckPad (USB-Geraet 303A:4DC0, Rueckfall 303A:1001) und oeffnet seinen COM-Port,
    ///  - meldet das Programm im Vordergrund ("ACTIVE chrome.exe") fuer den automatischen Profilwechsel,
    ///  - fuehrt Aktionen aus, die das DeckPad schickt ("DO media|keys|text|run ..."),
    ///  - gibt den Port frei, solange ein Fenster "DeckPad Editor" im Vordergrund ist (ein COM-Port gehoert nur einem Programm).
    /// </summary>
    internal sealed class HelperCore
    {
        // Zustand fuer die Oberflaeche (nur lesen)
        public volatile string PortName = "";
        public volatile string Foreground = "";
        public volatile bool EditorHasPort;
        public volatile int DeviceLostCount;

        private readonly ConcurrentQueue<string> log = new ConcurrentQueue<string>();
        private Thread thread;
        private volatile bool stop;
        private SerialPort port;
        private readonly int ownPid = Process.GetCurrentProcess().Id;

        public void Start()
        {
            thread = new Thread(Run) { IsBackground = true, Name = "DeckPad-Helfer" };
            thread.Start();
        }

        public void Stop()
        {
            stop = true;
            if (thread != null) thread.Join(1500);
            ClosePort();
        }

        public bool TryGetLog(out string line) { return log.TryDequeue(out line); }
        private void Log(string text) { log.Enqueue(text); }

        // ── Hauptschleife ────────────────────────────────────────────────────────
        private void Run()
        {
            var fgTimer = Stopwatch.StartNew();
            var scanTimer = Stopwatch.StartNew();
            var sendTimer = Stopwatch.StartNew();
            string curExe = "", lastExe = "";
            bool editorOpen = false;
            string buf = "";
            bool everConnected = false;

            while (!stop)
            {
                try
                {
                    // Vordergrundfenster pruefen (seltener als die Leitung)
                    if (fgTimer.ElapsedMilliseconds >= 600)
                    {
                        fgTimer.Restart();
                        string exe, title; int pid;
                        if (Native.GetForeground(out exe, out title, out pid) && pid != ownPid)
                        {
                            curExe = exe;
                            Foreground = exe;
                        }
                        editorOpen = title != null && title.IndexOf("DeckPad Editor", StringComparison.OrdinalIgnoreCase) >= 0 && pid != ownPid;
                        if (editorOpen && port != null)
                        {
                            ClosePort();
                            Log("Editor im Vordergrund: COM-Port freigegeben");
                        }
                    }
                    EditorHasPort = editorOpen;

                    if (!editorOpen)
                    {
                        if (port == null && scanTimer.ElapsedMilliseconds >= 3000)
                        {
                            scanTimer.Restart();
                            string name = FindPort();
                            if (name != null) OpenPort(name, ref buf, ref lastExe, sendTimer);
                            if (port != null) everConnected = true;
                        }
                        if (port != null)
                        {
                            // Programm im Vordergrund melden: bei Wechsel sofort, sonst alle 3 s als Lebenszeichen
                            if (curExe.Length > 0 && (curExe != lastExe || sendTimer.ElapsedMilliseconds >= 3000))
                            {
                                port.WriteLine("ACTIVE " + curExe);
                                lastExe = curExe;
                                sendTimer.Restart();
                            }
                            // Eingehende Zeilen vom Geraet: Aktionen ausfuehren, Antworten verwerfen
                            if (port.BytesToRead > 0)
                            {
                                buf += port.ReadExisting();
                                int i;
                                while ((i = buf.IndexOf('\n')) >= 0)
                                {
                                    string line = buf.Substring(0, i).TrimEnd('\r');
                                    buf = buf.Substring(i + 1);
                                    if (line.StartsWith("DO ", StringComparison.Ordinal)) HandleAction(line.Substring(3));
                                }
                                if (buf.Length > 4096) buf = "";
                            }
                        }
                    }
                }
                catch (Exception ex)
                {
                    // Kabel ab, Port von einem anderen Programm belegt ...: spaeter erneut versuchen
                    bool wasOpen = port != null;
                    ClosePort();
                    if (wasOpen)
                    {
                        Log("Verbindung verloren: " + ex.Message);
                        if (everConnected) DeviceLostCount++;
                    }
                    Thread.Sleep(1500);
                }
                Thread.Sleep(40);
            }
        }

        // ── Anschluss ────────────────────────────────────────────────────────────
        private void OpenPort(string name, ref string buf, ref string lastExe, Stopwatch sendTimer)
        {
            var sp = new SerialPort(name, 115200) { DtrEnable = true, RtsEnable = true, NewLine = "\n", WriteTimeout = 1000 };
            sp.Open();
            port = sp;
            PortName = name;
            buf = ""; lastExe = "";
            sendTimer.Restart();
            Log("Verbunden mit dem DeckPad an " + name);
        }

        private void ClosePort()
        {
            var sp = port;
            port = null;
            PortName = "";
            if (sp == null) return;
            try { sp.Close(); sp.Dispose(); } catch { }
        }

        /// <summary>COM-Port des DeckPad: eigene Kennung 303A:4DC0, 303A:1001 als Rueckfall (aeltere Firmware).</summary>
        private static string FindPort()
        {
            try
            {
                string fallback = null;
                using (var s = new ManagementObjectSearcher("SELECT Name, PNPDeviceID FROM Win32_PnPEntity WHERE Name LIKE '%(COM%)'"))
                {
                    foreach (ManagementBaseObject o in s.Get())
                    {
                        string id = (o["PNPDeviceID"] as string) ?? "";
                        string nm = (o["Name"] as string) ?? "";
                        Match m = Regex.Match(nm, @"\((COM\d+)\)");
                        if (!m.Success) continue;
                        if (id.StartsWith(@"USB\VID_303A&PID_4DC0", StringComparison.OrdinalIgnoreCase)) return m.Groups[1].Value;
                        if (id.StartsWith(@"USB\VID_303A&PID_1001", StringComparison.OrdinalIgnoreCase)) fallback = m.Groups[1].Value;
                    }
                }
                return fallback;
            }
            catch { return null; }
        }

        // ── Aktionen ─────────────────────────────────────────────────────────────
        private static readonly Dictionary<string, ushort> Media = new Dictionary<string, ushort>
        {
            { "vol_up", 0xAF }, { "vol_down", 0xAE }, { "mute", 0xAD }, { "play_pause", 0xB3 }, { "next", 0xB0 },
            { "prev", 0xB1 }, { "stop", 0xB2 }, { "calculator", 0xB7 }, { "browser", 0xAC }, { "mail", 0xB4 },
        };

        private static readonly Dictionary<string, ushort> Named = new Dictionary<string, ushort>
        {
            { "ctrl", 0x11 }, { "shift", 0x10 }, { "alt", 0x12 }, { "win", 0x5B }, { "gui", 0x5B }, { "rctrl", 0xA3 }, { "rshift", 0xA1 }, { "rwin", 0x5C },
            { "enter", 0x0D }, { "esc", 0x1B }, { "tab", 0x09 }, { "space", 0x20 }, { "backspace", 0x08 }, { "delete", 0x2E }, { "insert", 0x2D },
            { "home", 0x24 }, { "end", 0x23 }, { "pageup", 0x21 }, { "pagedown", 0x22 }, { "up", 0x26 }, { "down", 0x28 }, { "left", 0x25 }, { "right", 0x27 },
            { "printscreen", 0x2C }, { "scrolllock", 0x91 }, { "pause", 0x13 }, { "capslock", 0x14 },
        };

        private static ushort[] ParseKeys(string spec)
        {
            var list = new List<ushort>();
            foreach (string raw in spec.ToLowerInvariant().Split('+'))
            {
                string p = raw.Trim();
                ushort v;
                Match f = Regex.Match(p, @"^f(\d{1,2})$");
                if (p == "altgr") { list.Add(0x11); list.Add(0x12); }
                else if (Named.TryGetValue(p, out v)) list.Add(v);
                else if (p.Length == 1 && ((p[0] >= 'a' && p[0] <= 'z') || (p[0] >= '0' && p[0] <= '9'))) list.Add((ushort)char.ToUpperInvariant(p[0]));
                else if (f.Success && int.Parse(f.Groups[1].Value) >= 1 && int.Parse(f.Groups[1].Value) <= 24) list.Add((ushort)(0x70 + int.Parse(f.Groups[1].Value) - 1));
                else return null;
            }
            return list.Count > 0 ? list.ToArray() : null;
        }

        private void HandleAction(string rest)
        {
            int sp = rest.IndexOf(' ');
            string type = sp < 0 ? rest : rest.Substring(0, sp);
            string spec = sp < 0 ? "" : rest.Substring(sp + 1);
            try
            {
                switch (type)
                {
                    case "media":
                        ushort vk;
                        if (Media.TryGetValue(spec, out vk)) Native.Chord(new[] { vk });
                        break;
                    case "keys":
                        ushort[] keys = ParseKeys(spec);
                        if (keys != null) Native.Chord(keys); else { Log("Unbekannte Taste: " + spec); return; }
                        break;
                    case "text":
                        Native.TypeText(spec);
                        break;
                    case "run":
                        Process.Start(new ProcessStartInfo(spec) { UseShellExecute = true });
                        break;
                    default:
                        return;
                }
                Log("Aktion: " + type + " " + spec);
            }
            catch (Exception ex)
            {
                Log("Aktion fehlgeschlagen (" + type + " " + spec + "): " + ex.Message);
            }
        }
    }
}
