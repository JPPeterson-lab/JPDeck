using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Reflection;
using System.Windows.Forms;

namespace JPDeckHelper
{
    /// <summary>Statusfenster plus Symbol im Infobereich. Das Fenster zu schliessen versteckt es nur; "Beenden" beendet den Helfer.</summary>
    internal sealed class MainForm : Form
    {
        private const string EditorUrl = "https://jppeterson-lab.github.io/JPDeck/editor.html";

        private readonly HelperCore core = new HelperCore();
        private readonly NotifyIcon tray = new NotifyIcon();
        private readonly Timer uiTimer = new Timer { Interval = 500 };
        private readonly Label lblStatus = new Label();
        private readonly Label lblPort = new Label();
        private readonly Label lblFg = new Label();
        private readonly ListBox logBox = new ListBox();
        private readonly CheckBox chkAutostart = new CheckBox();
        private readonly CheckBox chkExit = new CheckBox();
        private readonly Icon appIcon;
        private readonly bool startHidden;
        private bool reallyExit;
        private bool loading = true;
        private int seenLost;

        public MainForm(bool startHidden)
        {
            this.startHidden = startHidden;
            appIcon = LoadIcon();

            Text = "JPDeck-Helfer";
            Icon = appIcon;
            AutoScaleMode = AutoScaleMode.Dpi;
            ClientSize = new Size(460, 400);
            FormBorderStyle = FormBorderStyle.FixedDialog;
            MaximizeBox = false;
            StartPosition = FormStartPosition.CenterScreen;
            Font = SystemFonts.MessageBoxFont;

            lblStatus.SetBounds(16, 14, 428, 30);
            lblStatus.Font = new Font(Font.FontFamily, 13f, FontStyle.Bold);
            lblPort.SetBounds(16, 48, 428, 20);
            lblFg.SetBounds(16, 70, 428, 20);

            var lblLog = new Label { Text = "Letzte Ereignisse", Left = 16, Top = 100, Width = 300 };
            logBox.SetBounds(16, 120, 428, 130);
            logBox.IntegralHeight = false;
            logBox.HorizontalScrollbar = true;

            chkAutostart.Text = "Mit Windows starten (im Hintergrund)";
            chkAutostart.SetBounds(16, 262, 428, 22);
            chkExit.Text = "Beenden, wenn das JPDeck getrennt wird";
            chkExit.SetBounds(16, 286, 428, 22);

            var hint = new Label
            {
                Text = "Beide Optionen sind standardmäßig aus: Der Helfer läuft nur, wenn Sie ihn starten.",
                ForeColor = SystemColors.GrayText
            }.Init(16, 312, 428, 34);

            var btnEditor = new Button { Text = "Editor öffnen" };
            btnEditor.SetBounds(16, 354, 130, 30);
            var btnHide = new Button { Text = "Verstecken" };
            btnHide.SetBounds(228, 354, 100, 30);
            var btnQuit = new Button { Text = "Beenden" };
            btnQuit.SetBounds(338, 354, 106, 30);

            Controls.AddRange(new Control[] { lblStatus, lblPort, lblFg, lblLog, logBox, chkAutostart, chkExit, hint, btnEditor, btnHide, btnQuit });

            // Symbol im Infobereich
            var menu = new ContextMenuStrip();
            menu.Items.Add("Fenster zeigen", null, (s, e) => ShowWindow());
            menu.Items.Add("Editor öffnen", null, (s, e) => OpenEditor());
            menu.Items.Add(new ToolStripSeparator());
            menu.Items.Add("Beenden", null, (s, e) => ExitApp());
            tray.ContextMenuStrip = menu;
            tray.Icon = appIcon;
            tray.Text = "JPDeck-Helfer";
            tray.DoubleClick += (s, e) => ShowWindow();

            btnEditor.Click += (s, e) => OpenEditor();
            btnHide.Click += (s, e) => Hide();
            btnQuit.Click += (s, e) => ExitApp();
            chkAutostart.CheckedChanged += (s, e) => { if (!loading) SaveOptions(); };
            chkExit.CheckedChanged += (s, e) => { if (!loading) SaveOptions(); };
            uiTimer.Tick += (s, e) => RefreshUi();

            try { chkAutostart.Checked = Settings.Autostart; chkExit.Checked = Settings.ExitOnDisconnect; } catch { }
            loading = false;
        }

        private static Icon LoadIcon()
        {
            try
            {
                using (Stream s = Assembly.GetExecutingAssembly().GetManifestResourceStream("jpdeck.ico"))
                    if (s != null) return new Icon(s);
            }
            catch { }
            return SystemIcons.Application;
        }

        private bool started;

        protected override void OnHandleCreated(EventArgs e)
        {
            base.OnHandleCreated(e);
            if (started) return;
            started = true;   // Symbol und Arbeits-Thread starten, auch wenn das Fenster nie sichtbar wird (--tray)
            tray.Visible = true;
            core.Start();
            uiTimer.Start();
            RefreshUi();
        }

        private bool firstShow = true;

        protected override void SetVisibleCore(bool value)
        {
            // Beim Start mit --tray bleibt das Fenster zunaechst unsichtbar
            if (startHidden && firstShow)
            {
                firstShow = false;
                if (!IsHandleCreated) CreateHandle();
                value = false;
            }
            base.SetVisibleCore(value);
        }

        protected override void OnFormClosing(FormClosingEventArgs e)
        {
            if (!reallyExit && e.CloseReason == CloseReason.UserClosing)
            {
                e.Cancel = true;
                Hide();
                return;
            }
            base.OnFormClosing(e);
        }

        private void ShowWindow()
        {
            Show();
            if (WindowState == FormWindowState.Minimized) WindowState = FormWindowState.Normal;
            Activate();
        }

        private void ExitApp()
        {
            reallyExit = true;
            uiTimer.Stop();
            core.Stop();
            tray.Visible = false;
            tray.Dispose();
            Application.Exit();
        }

        private void OpenEditor()
        {
            try { Process.Start(new ProcessStartInfo(EditorUrl) { UseShellExecute = true }); } catch { }
        }

        private void SaveOptions()
        {
            try
            {
                Settings.Autostart = chkAutostart.Checked;
                Settings.ExitOnDisconnect = chkExit.Checked;
            }
            catch (Exception ex)
            {
                MessageBox.Show("Einstellung konnte nicht gespeichert werden:\n" + ex.Message, "JPDeck-Helfer", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
        }

        private void RefreshUi()
        {
            string port = core.PortName;
            if (core.EditorHasPort) lblStatus.Text = "Editor hat den Anschluss";
            else if (port.Length > 0) lblStatus.Text = "JPDeck verbunden";
            else lblStatus.Text = "JPDeck nicht gefunden";
            lblStatus.ForeColor = port.Length > 0 ? Color.FromArgb(0, 128, 60) : (core.EditorHasPort ? SystemColors.ControlText : Color.FromArgb(190, 40, 40));
            lblPort.Text = port.Length > 0 ? "Anschluss: " + port : "Anschluss: –  (USB-Kabel am nativen Port einstecken)";
            lblFg.Text = "Vordergrund: " + (core.Foreground.Length > 0 ? core.Foreground : "–");
            tray.Text = port.Length > 0 ? "JPDeck-Helfer – verbunden (" + port + ")" : "JPDeck-Helfer – nicht verbunden";

            string line;
            bool added = false;
            while (core.TryGetLog(out line))
            {
                logBox.Items.Add(DateTime.Now.ToString("HH:mm:ss") + "  " + line);
                if (logBox.Items.Count > 200) logBox.Items.RemoveAt(0);
                added = true;
            }
            if (added) logBox.TopIndex = Math.Max(0, logBox.Items.Count - 1);

            // Option "Beenden, wenn das JPDeck getrennt wird"
            if (core.DeviceLostCount != seenLost)
            {
                seenLost = core.DeviceLostCount;
                if (chkExit.Checked) ExitApp();
            }
        }
    }

    internal static class LabelExt
    {
        public static Label Init(this Label l, int x, int y, int w, int h) { l.SetBounds(x, y, w, h); return l; }
    }
}
