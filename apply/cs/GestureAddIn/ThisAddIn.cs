using System;
using PowerPoint = Microsoft.Office.Interop.PowerPoint;
using System.Windows.Forms;
using System.Threading;

namespace GestureAddIn
{
    public partial class ThisAddIn
    {
        // パワーポイントのスライドを操作するクラス
        PowerPoint.SlideShowWindow SlideShow = null;

        // 設定ファイルのパス(環境に合わせて変更してください)
        private const string CONFIG_XML_PATH = @"../../../../../Data/SamplesConfig.xml";

        // OpenNIクラス群
        xn.Context context = null;
        xnv.SessionManager sessionManager = null;
        xnv.WaveDetector waveDetector = null;
        xnv.PushDetector pushDetector = null;

        Thread UpdateThread = null;

        // アドインの開始時に呼ばれる
        private void ThisAddIn_Startup(object sender, System.EventArgs e)
        {
            try {
                this.Application.SlideShowBegin += new Microsoft.Office.Interop.PowerPoint.EApplication_SlideShowBeginEventHandler(Application_SlideShowBegin);
                this.Application.SlideShowEnd += new Microsoft.Office.Interop.PowerPoint.EApplication_SlideShowEndEventHandler(Application_SlideShowEnd);

                // OpenNIの初期設定
                context = new xn.Context(CONFIG_XML_PATH);
                sessionManager = new xnv.SessionManager(context, "Click", "RaiseHand");

                waveDetector = new xnv.WaveDetector();
                pushDetector = new xnv.PushDetector();

                waveDetector.Wave += new xnv.WaveDetector.WaveHandler(waveDetector_Wave);
                pushDetector.Push += new xnv.PushDetector.PushHandler(pushDetector_Push);

                sessionManager.AddListener(waveDetector);
                sessionManager.AddListener(pushDetector);

                // データ更新のためのスレッドを生成
                UpdateThread = new Thread(() =>
                {
                    while (true) {
                        context.WaitAndUpdateAll();
                        sessionManager.Update(context);
                        Thread.Sleep(1);
                    }
                });
                UpdateThread.Start();
            }
            catch (Exception ex) {
                MessageBox.Show("アドインの初期化に失敗しました\n" + ex.Message);
            }
        }

        // アドインの終了時に呼ばれる
        private void ThisAddIn_Shutdown(object sender, System.EventArgs e)
        {
            try {
                if (UpdateThread != null) {
                    UpdateThread.Abort();
                }
            }
            catch (Exception ex) {
                MessageBox.Show(ex.Message);
            }
        }

        #region VSTO generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InternalStartup()
        {
            this.Startup += new System.EventHandler(ThisAddIn_Startup);
            this.Shutdown += new System.EventHandler(ThisAddIn_Shutdown);
        }

        #endregion

        // スライドショーの開始
        void Application_SlideShowBegin(Microsoft.Office.Interop.PowerPoint.SlideShowWindow Wn)
        {
            SlideShow = Wn;
        }

        // スライドショーの終了
        void Application_SlideShowEnd(Microsoft.Office.Interop.PowerPoint.Presentation Pres)
        {
            SlideShow = null;
        }

        // 前後運動を検出
        void pushDetector_Push(float velocity, float angle)
        {
            // スライドショーが開始されていれば、次のスライドに進む
            if (SlideShow != null) {
                SlideShow.View.Next();
            }
        }

        // 左右運動を検出
        void waveDetector_Wave()
        {
            // スライドショーが開始されていれば、前のスライドに戻る
            if (SlideShow != null) {
                SlideShow.View.Previous();
            }
        }
    }
}
