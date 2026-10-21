using System;
using System.Threading;
using System.Windows.Forms;

namespace DeckPadHelper
{
    internal static class Program
    {
        [STAThread]
        private static void Main(string[] args)
        {
            // Nur eine Instanz gleichzeitig
            bool created;
            using (var mutex = new Mutex(true, "DeckPadHelper.SingleInstance", out created))
            {
                if (!created)
                {
                    MessageBox.Show("Der DeckPad-Helfer läuft bereits (Symbol im Infobereich der Taskleiste).",
                        "DeckPad-Helfer", MessageBoxButtons.OK, MessageBoxIcon.Information);
                    return;
                }
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                bool hidden = Array.IndexOf(args, "--tray") >= 0;
                Application.Run(new MainForm(hidden));
            }
        }
    }
}
