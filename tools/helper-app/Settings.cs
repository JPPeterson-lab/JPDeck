using System;
using System.Windows.Forms;
using Microsoft.Win32;

namespace JPDeckHelper
{
    /// <summary>Einstellungen in der Registry des Benutzers (keine Administratorrechte noetig). Beide Schalter sind standardmaessig aus.</summary>
    internal static class Settings
    {
        private const string AppKey = @"Software\JPDeck\Helper";
        private const string RunKey = @"Software\Microsoft\Windows\CurrentVersion\Run";
        private const string RunName = "JPDeck-Helper";

        /// <summary>
        /// Bis v0.6.1 hiess das Programm "DeckPad-Helper". Einstellungen und Autostart-Eintrag werden einmal uebernommen,
        /// der alte Autostart-Eintrag (zeigt auf die alte exe) und die alten Schluessel werden entfernt.
        /// </summary>
        public static void MigrateFromOldName()
        {
            try
            {
                const string oldApp = @"Software\DeckPad\Helper", oldRun = "DeckPad-Helper";
                bool oldAutostart;
                using (var k = Registry.CurrentUser.OpenSubKey(RunKey)) oldAutostart = k != null && k.GetValue(oldRun) != null;
                using (var k = Registry.CurrentUser.OpenSubKey(oldApp))
                    if (k != null && Registry.CurrentUser.OpenSubKey(AppKey) == null)
                        ExitOnDisconnect = Convert.ToInt32(k.GetValue("ExitOnDisconnect", 0)) != 0;
                if (oldAutostart)
                {
                    using (var k = Registry.CurrentUser.OpenSubKey(RunKey, true)) k.DeleteValue(oldRun, false);
                    Autostart = true;
                }
                Registry.CurrentUser.DeleteSubKeyTree(@"Software\DeckPad", false);
            }
            catch { }
        }

        /// <summary>Programm beenden, sobald das JPDeck getrennt wird.</summary>
        public static bool ExitOnDisconnect
        {
            get
            {
                using (var k = Registry.CurrentUser.OpenSubKey(AppKey))
                    return k != null && Convert.ToInt32(k.GetValue("ExitOnDisconnect", 0)) != 0;
            }
            set
            {
                using (var k = Registry.CurrentUser.CreateSubKey(AppKey))
                    k.SetValue("ExitOnDisconnect", value ? 1 : 0, RegistryValueKind.DWord);
            }
        }

        /// <summary>Beim Anmelden an Windows im Hintergrund starten (Eintrag unter "Autostart").</summary>
        public static bool Autostart
        {
            get
            {
                using (var k = Registry.CurrentUser.OpenSubKey(RunKey))
                    return k != null && k.GetValue(RunName) != null;
            }
            set
            {
                using (var k = Registry.CurrentUser.OpenSubKey(RunKey, true))
                {
                    if (k == null) return;
                    if (value) k.SetValue(RunName, "\"" + Application.ExecutablePath + "\" --tray");
                    else if (k.GetValue(RunName) != null) k.DeleteValue(RunName, false);
                }
            }
        }
    }
}
