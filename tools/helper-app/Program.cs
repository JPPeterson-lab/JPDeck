using System;
using System.Threading;
using System.Windows.Forms;

namespace JPDeckHelper
{
    internal static class Program
    {
        [STAThread]
        private static void Main(string[] args)
        {
            // Nur eine Instanz gleichzeitig
            bool created;
            using (var mutex = new Mutex(true, "JPDeckHelper.SingleInstance", out created))
            {
                if (!created)
                {
                    MessageBox.Show("Der JPDeck-Helfer läuft bereits (Symbol im Infobereich der Taskleiste).",
                        "JPDeck-Helfer", MessageBoxButtons.OK, MessageBoxIcon.Information);
                    return;
                }
                Settings.MigrateFromOldName();
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                bool hidden = Array.IndexOf(args, "--tray") >= 0;
                Application.Run(new MainForm(hidden));
            }
        }
    }
}
