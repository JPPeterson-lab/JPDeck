using System;
using System.Windows.Forms;
using Microsoft.Win32;

namespace DeckPadHelper
{
    /// <summary>Einstellungen in der Registry des Benutzers (keine Administratorrechte noetig). Beide Schalter sind standardmaessig aus.</summary>
    internal static class Settings
    {
        private const string AppKey = @"Software\DeckPad\Helper";
        private const string RunKey = @"Software\Microsoft\Windows\CurrentVersion\Run";
        private const string RunName = "DeckPad-Helper";

        /// <summary>Programm beenden, sobald das DeckPad getrennt wird.</summary>
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
