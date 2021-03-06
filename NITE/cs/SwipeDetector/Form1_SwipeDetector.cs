﻿using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Windows.Forms;
using OpenNI;
using NITE;

namespace SwipeDetector
{
  // アプリケーション固有の処理を記述
  partial class Form1
  {
    // 設定ファイルのパス(環境に合わせて変更してください)
    private const string CONFIG_XML_PATH = @"../../../../../Data/SamplesConfig.xml";

    private Context context;
    private ImageGenerator image;

    private SessionManager sessionManager;
    private NITE.SwipeDetector swipeDetector;

    enum SessionState
    {
      NotInSession,
      DetectSession,
      InSession,
    }

    private SessionState sessionState = SessionState.NotInSession;
    private NITE.Direction direction = NITE.Direction.Illegal;

    // 描画用
    private Brush brush = new SolidBrush(Color.Black);
    private Font font = new Font("Arial", 30);
    private PointF point = new Point(0, 0);

    // 初期化
    private void xnInitialize()
    {
      // コンテキストの初期化
      ScriptNode scriptNode;
      context = Context.CreateFromXmlFile(CONFIG_XML_PATH, out scriptNode);

      // イメージジェネレータの作成
      image = context.FindExistingNode(NodeType.Image) as ImageGenerator;
      if (image == null) {
        throw new Exception(context.GlobalErrorState);
      }

      // NITEのためのセッションマネージャを作成
      sessionManager = new SessionManager(context, "Wave,Click", "RaiseHand");

      // セッションの開始と終了を通知するコールバックを登録する
      sessionManager.SessionStart += new EventHandler<PositionEventArgs>(sessionManager_SessionStart);
      sessionManager.SessionEnd += new EventHandler(sessionManager_SessionEnd);
      sessionManager.SessionFocusProgress += new EventHandler<SessionProgressEventArgs>(sessionManager_SessionFocusProgress);

      // Wave(左右運動の検出器)
      swipeDetector = new NITE.SwipeDetector();
      swipeDetector.GeneralSwipe += new EventHandler<DirectionVelocityAngleEventArgs>(swipeDetector_GeneralSwipe);
      swipeDetector.SwipeUp += new EventHandler<VelocityAngleEventArgs>(swipeDetector_SwipeUp);
      swipeDetector.SwipeDown += new EventHandler<VelocityAngleEventArgs>(swipeDetector_SwipeDown);
      swipeDetector.SwipeRight += new EventHandler<VelocityAngleEventArgs>(swipeDetector_SwipeRight);
      swipeDetector.SwipeLeft += new EventHandler<VelocityAngleEventArgs>(swipeDetector_SwipeLeft);

      // リスナーに追加する
      sessionManager.AddListener(swipeDetector);

      // ジェネレータの動作を開始する
      context.StartGeneratingAll();
    }

    // 描画
    private unsafe void xnDraw()
    {
      // コンテキストとセッションマネージャーの更新
      context.WaitAndUpdateAll();
      sessionManager.Update(context);

      // 画像データを取得する
      ImageMetaData imageMD = image.GetMetaData();

      // カメラ画像の作成
      lock (this) {
        // 書き込み用のビットマップデータを作成
        Rectangle rect = new Rectangle(0, 0, bitmap.Width, bitmap.Height);
        BitmapData data = bitmap.LockBits(rect, ImageLockMode.WriteOnly,
                        System.Drawing.Imaging.PixelFormat.Format24bppRgb);

        // 生データへのポインタを取得
        byte* dst = (byte*)data.Scan0.ToPointer();
        byte* src = (byte*)image.ImageMapPtr.ToPointer();

        for (int i = 0; i < imageMD.DataSize; i += 3, src += 3, dst += 3) {
          dst[0] = src[2];
          dst[1] = src[1];
          dst[2] = src[0];
        }

        bitmap.UnlockBits(data);


        // 現在の状態を表示する
        Graphics g = Graphics.FromImage(bitmap);
        string message = sessionState.ToString() + "\r\n" +
                         direction.ToString();
        g.DrawString(message, font, brush, new Point(0, 0));
      }
    }

    // キーイベント
    private void xnKeyDown(Keys key)
    {
    }

    // 左方向への動きを通知する
    void swipeDetector_SwipeLeft(object sender, VelocityAngleEventArgs e)
    {
      brush = new SolidBrush(Color.Red);
    }

    // 右方向への動きを通知する
    void swipeDetector_SwipeRight(object sender, VelocityAngleEventArgs e)
    {
      brush = new SolidBrush(Color.Blue);
    }

    // 下方向への動きを通知する
    void swipeDetector_SwipeDown(object sender, VelocityAngleEventArgs e)
    {
      brush = new SolidBrush(Color.Green);
    }

    // 上方向への動きを通知する
    void swipeDetector_SwipeUp(object sender, VelocityAngleEventArgs e)
    {
      brush = new SolidBrush(Color.Yellow);
    }

    // スワイプの検出を通知する
    void swipeDetector_GeneralSwipe(object sender, DirectionVelocityAngleEventArgs e)
    {
      direction = e.Direction;
    }

    // セッションの開始を通知する
    void sessionManager_SessionStart(object sender, PositionEventArgs e)
    {
      sessionState = SessionState.InSession;
    }

    // セッションの終了を通知する
    void sessionManager_SessionEnd(object sender, EventArgs e)
    {
      sessionState = SessionState.NotInSession;
    }

    // セッションフォーカスの検出を通知する
    void sessionManager_SessionFocusProgress(object sender, SessionProgressEventArgs e)
    {
      sessionState = SessionState.DetectSession;
    }
  }
}
